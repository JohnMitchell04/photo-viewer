#include "../vendor/zlib/zlib.h"

#include "PNG.h"
#include "Utils.h"

namespace ImageLibrary {
	void PNG::InitCRC() {
		// Taken directly from the specification and cleaned slightly
		for (uint32_t n = 0; n < 256; n++) {
			uint32_t c = (uint32_t)n;
			for (uint32_t k = 0; k < 8; k++) {
				if (c & 1) {
					c = 0xedb88320L ^ (c >> 1);
				}
				else {
					c = c >> 1;
				}
			}
			m_crcTable[n] = c;
		}
	}

	void PNG::ReadFile() {
		// Read raw data from file
		ReadRawData();

		// Get and check the PNG signature
		ParseSignature();

		// Pase PNG chunks from the data
		ParseChunks();

		// Decompress the IDAT image data
		std::vector<uint8_t> filteredData = DecompressData();

		// Unfilter the IDAT image data
		std::vector<uint8_t> interlacedData = UnfilterData(filteredData);

		// Deinterlace IDAT image data if interlacing was used
		std::vector<uint8_t> pixelData = DeinterlaceData(interlacedData);

		// Parse pixels
		ParsePixels(interlacedData);
	}

	void PNG::ReadRawData() {
		// Create input stream in binary mode
		std::ifstream file;
		file.open(m_filePath, std::ios_base::binary);
		file.unsetf(std::ios_base::skipws);

		if (file) {
			// Get and reserve file size
			uintmax_t size = std::filesystem::file_size(m_filePath);
			m_rawData.reserve(size);

			// Read file into vector
			std::copy(std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>(), std::back_inserter(m_rawData));
		}
	}

	void PNG::ParseSignature() {
		// Check signature is correct
		// XOR and consume the first 8 values
		uint8_t check = 0;
		for (int i = 0; i < 8; i++) {
			uint8_t temp = m_rawData[0];
			check ^= temp;
			m_rawData.erase(m_rawData.begin());
		}

		// Check header is valid
		if (check != Utils::PNG_SIGNATURE) {
			throw new std::runtime_error("Error: PNG signature is invalid");
		}
	}

	void PNG::ParseChunks() {
		std::vector<Utils::PNG::Chunk> encounteredChunks;
		bool encounteredIDAT = false;
		int chunksIndex = 0;

		do {
			// Consume chunk length remembering it is big endian
			uint32_t length;
			Utils::ExtractBigEndian(length, m_rawData.data());
			m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

			// Check length is within standard
			if (length > (INT_MAX - 1)) { throw new std::runtime_error("Error: Invalid chunk length"); }

			// Check CRC matches data
			CheckCRC(length);

			// Copy and consume chunk specifier
			std::string chunkSpecifier(m_rawData.begin(), m_rawData.begin() + 4);
			m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

			Utils::PNG::ChunkIdentifier chunkSpecifierE = Utils::PNG::StringToFormat(chunkSpecifier);

			// If chunk is unknown skip the chunk
			if (chunkSpecifierE == Utils::PNG::UNKOWN) {
				m_rawData.erase(m_rawData.begin(), m_rawData.begin() + length + 4);
				continue;
			}

			// If chunk is invalid exit
			if (chunkSpecifierE == Utils::PNG::INVALID) { throw new std::runtime_error("Error: Encountered chunk is invalid"); }

			// Ensure the correct chunk is encountered first
			if (chunksIndex == 0 && chunkSpecifier != "IHDR") { throw new std::runtime_error("Error: Chunk order is invalid"); }

			// Check IDAT chunks are consecutive
			if (encounteredIDAT && chunkSpecifierE == Utils::PNG::IDAT) {
				if (encounteredChunks.back().identifier != Utils::PNG::IDAT) { throw new std::runtime_error("Error: Chunk order is invalid"); }
			}

			// TODO: Deal with PLTE chunk
			switch (chunkSpecifierE) {
			case Utils::PNG::IHDR:
				ParseIHDR();
				break;
			case Utils::PNG::IDAT:
				// Consume IDAT data into compressed data
				encounteredIDAT = true;
				std::copy(m_rawData.begin(), m_rawData.begin() + length, std::back_inserter(m_compressedData));
				m_rawData.erase(m_rawData.begin(), m_rawData.begin() + length + 4);
				break;
			case Utils::PNG::IEND:
				// Consume CRC
				m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);
				// Ensure IEND is last data
				if (m_rawData.size() > 0) { throw new std::runtime_error("Error: Data is present after IEND chunk"); }
				// Shrink vector capacity to 0
				m_rawData.shrink_to_fit();
				break;
			}

			// Add encountered chunk to list
			encounteredChunks.push_back(Utils::PNG::Chunk{ .identifier = chunkSpecifierE, .position = chunksIndex });
			chunksIndex++;
		} while (encounteredChunks.back().identifier != Utils::PNG::IEND);
	}

	void PNG::CheckCRC(uint32_t length) {
		// Taken directly from specification and cleaned slightly

		// Get the CRC in the chunk remembering it is big endian
		uint32_t chunkCRC;
		Utils::ExtractBigEndian(chunkCRC, m_rawData.data() + 4 + length);

		// Calculate the expected CRC
		uint32_t c = 0xffffffffL;

		for (uint32_t n = 0; n < length + 4; n++) {
			c = m_crcTable[(c ^ m_rawData[n]) & 0xff] ^ (c >> 8);
		}

		uint32_t calculatedCRC = c ^ 0xffffffffL;

		// Compare calculated and stored CRC
		if (chunkCRC != calculatedCRC) {
			throw new std::runtime_error("Error: Chunk CRC mismatch");
		}
	}

	void PNG::ParseIHDR() {
		// Consume width remembering it is big endian
		Utils::ExtractBigEndian(m_width, m_rawData.data());
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

		// Consume height remembering it is big endian
		Utils::ExtractBigEndian(m_height, m_rawData.data());
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

		// Perform checks on dimensions
		if (m_width > Utils::PNG_SPEC_MAX_DIMENSION || m_height > Utils::PNG_SPEC_MAX_DIMENSION || m_width < 0 || m_height < 0) {
			throw new std::runtime_error("Error: Image dimensions invalid");
		}

		if (m_width > Utils::PNG_APP_MAX_DIMENSION || m_height > Utils::PNG_APP_MAX_DIMENSION) { throw new std::runtime_error("Error: Application cannot display image"); }

		// Get more image info
		m_bitDepth = m_rawData[0];
		m_colourType = m_rawData[1];
		m_compressionMethod = m_rawData[2];
		m_filterMethod = m_rawData[3];
		m_interlaceMethod = m_rawData[4];

		// Consume additional information and CRC
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 9);

		// Check image info and set number of bytes per pixel
		switch (m_colourType) {
		// Greyscale
		case 0:
			if (m_bitDepth != 1 && m_bitDepth != 2 && m_bitDepth != 4 && m_bitDepth != 8 && m_bitDepth != 16) {
				throw new std::runtime_error("Error: Invalid colour type and bit depth combination");
			}
			m_bytesPerPixel = std::ceil(m_bitDepth / 8);
			break;
		// True colour
		case 2:
			if (m_bitDepth != 8 && m_bitDepth != 16) {
				throw new std::runtime_error("Error: Invalid colour type and bit depth combination");
			}
			m_bytesPerPixel = 3 * (m_bitDepth / 8);
			break;
		// Indexed Colour
		case 3:
			if (m_bitDepth != 1 && m_bitDepth != 2 && m_bitDepth != 4 && m_bitDepth != 8) {
				throw new std::runtime_error("Error: Invalid colour type and bit depth combination");
			}
			m_bytesPerPixel = 1;
			break;
		// Greyscale alpha
		case 4:
			if (m_bitDepth != 8 && m_bitDepth != 16) {
				throw new std::runtime_error("Error: Invalid colour type and bit depth combination");
			}
			m_bytesPerPixel = 2 * (m_bitDepth / 8);
			break;
		// True colour alpha
		case 6:
			if (m_bitDepth != 8 && m_bitDepth != 16) {
				throw new std::runtime_error("Error: Invalid colour type and bit depth combination");
			}
			m_bytesPerPixel = 4 * (m_bitDepth / 8);
			break;
		// Invalid colour type
		default:
			throw new std::runtime_error("Error: Invalid colour type");
		}

		// Do not process images with private compression methods
		if (m_compressionMethod != 0) { throw new std::runtime_error("Error: Incompatible compression method"); }

		// Do not process images with private filter methods
		if (m_filterMethod != 0) { throw new std::runtime_error("Error: Incompatible filter method"); }

		// Do not process images with private interlace methods
		if (m_interlaceMethod != 0 && m_interlaceMethod != 1) { throw new std::runtime_error("Error: Incompatible interlace method"); }
	}

	std::vector<uint8_t> PNG::DecompressData() {
		std::vector<uint8_t> output;

		// Decompress IDAT data
		int err;
		z_stream infStream{};
		infStream.zalloc = Z_NULL;
		infStream.zfree = Z_NULL;
		infStream.opaque = Z_NULL;
		infStream.next_in = m_compressedData.data();
		infStream.avail_in = m_compressedData.size();

		// Initialise the infaltion
		err = inflateInit(&infStream);
		if (err != Z_OK) {
			throw new std::runtime_error("Error: Decompression of data failed");
		}

		// Run inflate until all data has been decompressed
		do {
			// Arbitrary size to process
			std::vector<uint8_t> tempOutput;
			tempOutput.resize(65536);
			infStream.next_out = tempOutput.data();
			infStream.avail_out = tempOutput.size();

			// Perform decompression
			err = inflate(&infStream, Z_SYNC_FLUSH);
			if (!(err == Z_OK || err == Z_STREAM_END)) {
				inflateEnd(&infStream);
				throw new std::runtime_error("Error: Decompression of data failed");
			}

			// Copy decompressed data chunk
			std::copy(tempOutput.begin(), tempOutput.begin() + (tempOutput.size() - infStream.avail_out), std::back_inserter(output));
		} while (err != Z_STREAM_END);

		inflateEnd(&infStream);

		m_compressedData.clear();
		return output;
	}

	std::vector<uint8_t> PNG::UnfilterData(std::vector<uint8_t>& input) {
		std::vector<uint8_t> output;

		// Define paeth functor for use
		auto paeth = [](uint8_t aByte, uint8_t bByte, uint8_t cByte) -> uint8_t {
			// Perform paeth
			uint8_t result;
			int p = aByte + bByte - cByte;
			int paethA = std::abs(p - aByte);
			int paethB = std::abs(p - bByte);
			int paethC = std::abs(p - cByte);

			if (paethA <= paethB && paethA <= paethC) { result = aByte; }
			else if (paethB <= paethC) { result = bByte; }
			else { result = cByte; }
			return result;
		};

		// Iterate through each scanline and unfliter appropriately
		for (uint32_t y = 0; y < m_height; y++) {
			// Get scanline filter type
			uint8_t filterType = input[0];

			// Consume byte
			input.erase(input.begin());

			// TODO: Check performance impact of this for larger images
			for (uint32_t x = 0; x < m_width * m_bytesPerPixel; x++) {
				uint8_t currentByte;

				bool firstPixel = (std::floor(x / m_bytesPerPixel) == 0 ? true : false);
				bool firstScanline = (y == 0 ? true : false);

				uint8_t aByte = (firstPixel ? 0 : output[output.size() - m_bytesPerPixel]);
				uint8_t bByte = (firstScanline ? 0 : output[output.size() - m_bytesPerPixel * m_width]);
				uint8_t cByte = (firstPixel || firstScanline ? 0 : output[output.size() - m_bytesPerPixel * (m_width + 1)]);

				switch (filterType) {
					// None
				case 0:
					currentByte = input[0];
					break;
					// Sub
				case 1:
					// Can't get preceding byte if first pixel
					currentByte = input[0] + aByte;
					break;
					// Up
				case 2:
					// Can't get up byte if first scanline
					currentByte = input[0] + bByte;
					break;
					// Average
				case 3:
					currentByte = input[0] + std::floor((aByte + bByte) / 2);
					break;
					// Paeth
				case 4:
					currentByte = input[0] + paeth(aByte, bByte, cByte);
					break;
					// Invalid filter type type
				default:
					throw new std::runtime_error("Error: Invalid filter type");
					break;
				}

				// Add reconstructed byte to output
				output.push_back(currentByte);

				// Consume byte
				input.erase(input.begin());
			}
		}

		// Shrink input to 0
		input.shrink_to_fit();

		return output;
	}

	std::vector<uint8_t> PNG::DeinterlaceData(std::vector<uint8_t>& input) {
		// If no interlacing has been done, simply return data
		if (m_interlaceMethod == 0) { return input; }

		// TODO: Implement Adam7 deinterlacing
		return input;
	}

	void PNG::ParsePixels(std::vector<uint8_t>& input) {
		// TODO: Decide how to deal with pixels
		// Thinking of having raw pixel data for vulkan operations etc. and a pixel struct for greater abstraction

		// Select pixel format
		switch (m_colourType) {
			// Greyscale
		case 0:
			m_pixelFormat = Utils::RGB8;
			break;
			// True colour
		case 2:
			m_pixelFormat = (m_bitDepth == 8 ? Utils::RGB8 : Utils::RGB16);
			break;
			// Indexed colour
		case 3:
			// Check for alpha channel
			m_pixelFormat = (m_indexedAlpha ? Utils::RGBA8 : Utils::RGB8);
			break;
			// Greyscale with alpha
		case 4:
			[[fallthrough]];
			// True colour with alpha
		case 6:
			m_pixelFormat = (m_bitDepth == 8 ? Utils::RGBA8 : Utils::RGBA16);
			break;
		}

		// If true colour type data can be copied as is accounting for endianess
		if (m_colourType == 2 || m_colourType == 6) {
			// If using bit depth equal to one byte or on big endian system, copy as is
			if (m_bitDepth == 8 || std::endian::native == std::endian::big) {
				std::copy(input.begin(), input.end(), std::back_inserter(m_imageData));
				input.clear();
			}
			// Otherwise perform endianess conversion
			else {
				for (uint32_t i = 0; i < m_width * m_height * 4; i++) {
					// Extract value
					uint16_t output;
					Utils::ExtractBigEndian(output, input.data());

					// Flip endianess
					uint16_t flipped;
					Utils::FlipEndian(flipped, output);

					// Copy value to image data
					m_imageData.push_back(flipped >> 8);
					m_imageData.push_back(flipped);

					input.erase(input.begin(), input.begin() + 2);
				}

				int test = 0;
			}
		}
		// If indexed colour substitute pallete values in
		else if (m_colourType == 3) {
			// TODO: Implement
		}
		// If greyscale with alpha copy value for each colour component and add alpha
		else if (m_colourType == 4) {
			for (uint32_t i = 0; i < m_width * m_height; i++) {
				for (int j = 0; j < 3; j++) {
					std::copy(input.begin(), input.begin() + (m_bitDepth / 8), std::back_inserter(m_imageData));
				}

				// Copy alpha value
				std::copy(input.begin() + (m_bitDepth / 8), input.begin() + 2 * (m_bitDepth / 8), std::back_inserter(m_imageData));

				// Consume pixel data
				input.erase(input.begin(), input.begin() + 2 * (m_bitDepth / 8));
			}
		}

		// Shrink input to 0
		input.shrink_to_fit();
	}
}