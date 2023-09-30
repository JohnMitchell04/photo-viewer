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
		ParsePixels(pixelData);
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
			Utils::ExtractBigEndianBytes(length, m_rawData.data(), 4);
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
			if (chunksIndex == 0 && chunkSpecifierE != Utils::PNG::IHDR) { throw new std::runtime_error("Error: Chunk order is invalid - IHDR is not first"); }

			// Check IDAT chunks are consecutive
			if (encounteredIDAT && chunkSpecifierE == Utils::PNG::IDAT) {
				if (encounteredChunks.back().identifier != Utils::PNG::IDAT) { throw new std::runtime_error("Error: Chunk order is invalid - IDAT are not consecutive"); }
			}

			// TODO: Clean
			switch (chunkSpecifierE) {
			case Utils::PNG::IHDR:
				CheckChunkOccurence(encounteredChunks, Utils::PNG::IHDR, 0);
				ParseIHDR();
				break;
			case Utils::PNG::PLTE:
				if (m_colourType == 0 || m_colourType == 4) { throw new std::runtime_error("Error: PLTE chunk must not appear for his colour type"); }
				CheckChunkOccurence(encounteredChunks, Utils::PNG::PLTE, 0);
				ParsePLTE(length);
				break;
			case Utils::PNG::IDAT:
				if (m_colourType == 3 && !CheckChunkOccurence(encounteredChunks, Utils::PNG::PLTE, 1)) { throw new std::runtime_error("Error: Chunk order is invalid - PLTE required before IDAT"); }
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

	bool PNG::CheckChunkOccurence(const std::vector<Utils::PNG::Chunk>& encounteredChunks, Utils::PNG::ChunkIdentifier chunk, int number) {
		int count = std::count_if(encounteredChunks.begin(), encounteredChunks.end(), [chunk](Utils::PNG::Chunk val) { return val.identifier == chunk; });
		return (count == number ? true : false);
	}

	void PNG::CheckCRC(uint32_t length) {
		// Taken directly from specification and cleaned slightly

		// Get the CRC in the chunk remembering it is big endian
		uint32_t chunkCRC;
		Utils::ExtractBigEndianBytes(chunkCRC, m_rawData.data() + 4 + length, 4);

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
		Utils::ExtractBigEndianBytes(m_width, m_rawData.data(), 4);
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

		// Consume height remembering it is big endian
		Utils::ExtractBigEndianBytes(m_height, m_rawData.data(), 4);
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

		// Initialise pixel data
		m_pixelData.resize(m_height, std::vector<Utils::Pixel>());

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
			m_bytesPerPixel = (m_bitDepth == 16 ? 2 : 1);
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

	void PNG::ParsePLTE(uint32_t length) {
		// Do some checking that this chunk is valid and should be present
		if (m_colourType == 3 && length % 3 != 0) { throw new std::runtime_error("Error: PLTE chunk is invalid"); }
		else if (length % 3 != 0) { return; }

		if ((length / 3) > std::pow(2, m_bitDepth)) { throw new std::runtime_error("Error: PLTE chunk is invalid"); }

		// Copy pixel data
		for (int i = 0; i < (length / 3); i++) {
			Utils::Pixel pixel;

			pixel.R = m_rawData[i * 3];
			pixel.G = m_rawData[(i * 3) + 1];
			pixel.B = m_rawData[(i * 3) + 2];

			m_PLTEData.push_back(pixel);
		}

		// Erase rest of chunk
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + length + 4);
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
		infStream.avail_in = (uInt)m_compressedData.size();

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
			infStream.avail_out = (uInt)tempOutput.size();

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
		std::vector<uint8_t> unfilteredData;

		if (m_interlaceMethod == 0) {
			// Iterate through each scanline and unfliter appropriately
			for (uint32_t y = 0; y < m_height; y++) {
				// Get scanline filter type
				uint8_t filterType = input[0];

				// Consume byte
				input.erase(input.begin());

				// Extract scanline
				uint32_t lineWidth = (m_bytesPerPixel == 1 ? (uint32_t)std::ceil(m_width * m_bitDepth / 8.0) : m_bytesPerPixel * m_width);
				std::vector<uint8_t> scanline(input.begin(), input.begin() + lineWidth);

				// Unfilter scanline
				unfilteredData = UnfilterScanline(scanline, unfilteredData, filterType);
				std::copy(unfilteredData.begin(), unfilteredData.end(), std::back_inserter(output));

				// Consme data
				input.erase(input.begin(), input.begin() + lineWidth);
			}
		}
		else {
			std::array<std::array<int, 4>, 7> passes = {{
				{0, 0, 8, 8},
				{4, 0, 8, 8},
				{0, 4, 4, 8},
				{2, 0, 4, 4},
				{0, 2, 2, 4},
				{1, 0, 2, 2},
				{0, 1, 1, 2}
			}};

			for (int i = 0; i < 7; i++) {
				std::array<int, 4> pass = passes[i];
				int xStart = pass[0];
				int yStart = pass[1];
				int xStep = pass[2];
				int yStep = pass[3];

				if (xStart >= m_width) { continue; }

				unfilteredData.clear();

				int pixelsPerRow = (int)std::ceil((m_width - xStart) / (double)xStep);
				int rowSize = std::ceil(pixelsPerRow * m_bitDepth / 8.0);

				for (int y = yStart; y < m_height; y += yStep) {
					// Extract filter type
					int filterType = input[0];
					input.erase(input.begin());

					// Extract scanline
					std::vector<uint8_t> scanline;
					std::copy(input.begin(), input.begin() + rowSize, std::back_inserter(scanline));
					input.erase(input.begin(), input.begin() + rowSize);

					// Unfilter scanline
					unfilteredData = UnfilterScanline(scanline, unfilteredData, filterType);
					std::copy(unfilteredData.begin(), unfilteredData.end(), std::back_inserter(output));
				}
			}
		}

		// Shrink input to 0
		input.shrink_to_fit();

		return output;
	}

	std::vector<uint8_t> PNG::UnfilterScanline(std::vector<uint8_t>& scanline, std::vector<uint8_t> previousLine, uint8_t filterType) {
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

		// TODO: Check performance impact of this for larger images
		uint32_t lineWidth = scanline.size();
		for (uint32_t x = 0; x < lineWidth; x++) {
			uint8_t currentByte;

			bool firstPixel = (std::floor(x / m_bytesPerPixel) == 0 ? true : false);
			bool firstScanline = (previousLine.size() == 0 ? true : false);

			uint8_t aByte = (firstPixel ? 0 : output[output.size() - m_bytesPerPixel]);
			uint8_t bByte = (firstScanline ? 0 : previousLine[x]);
			uint8_t cByte = (firstPixel || firstScanline ? 0 : previousLine[x - m_bytesPerPixel]);

			switch (filterType) {
				// None
			case 0:
				currentByte = scanline[0];
				break;
				// Sub
			case 1:
				// Can't get preceding byte if first pixel
				currentByte = scanline[0] + aByte;
				break;
				// Up
			case 2:
				// Can't get up byte if first scanline
				currentByte = scanline[0] + bByte;
				break;
				// Average
			case 3:
				currentByte = scanline[0] + (uint8_t)std::floor((aByte + bByte) / 2);
				break;
				// Paeth
			case 4:
				currentByte = scanline[0] + paeth(aByte, bByte, cByte);
				break;
				// Invalid filter type type
			default:
				throw new std::runtime_error("Error: Invalid filter type");
				break;
			}

			// Add reconstructed byte to output
			output.push_back(currentByte);

			// Consume byte
			scanline.erase(scanline.begin());
		}

		return output;
	}

	// TODO: Need to extract pixels before this for sub byte data
	std::vector<uint8_t> PNG::DeinterlaceData(std::vector<uint8_t>& input) {
		// If no interlacing has been done, simply return data
		if (m_interlaceMethod == 0) { return input; }

		std::vector<uint8_t> output(m_width * m_height * m_bytesPerPixel);

		std::array<std::array<int, 4>, 7> passes = {{
			{0, 0, 8, 8},
			{4, 0, 8, 8},
			{0, 4, 4, 8},
			{2, 0, 4, 4},
			{0, 2, 2, 4},
			{1, 0, 2, 2},
			{0, 1, 1, 2}
		}};

		for (int i = 0; i < 7; i++) {
			std::array<int, 4> pass = passes[i];
			int xStart = pass[0];
			int yStart = pass[1];
			int xStep = pass[2];
			int yStep = pass[3];

			int pixelsPerRow = std::ceil((m_width - xStart) / (double)xStep);

			for (int y = yStart; y < m_height; y += yStep) {
				int rowOffset = y * (m_width * m_bytesPerPixel) + xStart * m_bytesPerPixel;
				int skip = m_bytesPerPixel * xStep;

				for (int j = 0; j < pixelsPerRow; j++) {
					for (int k = 0; k < m_bytesPerPixel; k++) {
						output[rowOffset + j * skip] = input[0];
						input.erase(input.begin());
					}
				}
			}
		}

		return output;
	}

	void PNG::ParsePixels(std::vector<uint8_t>& input) {
		// Select pixel format
		switch (m_colourType) {
			// Greyscale
		case 0:
			m_pixelFormat = (m_bitDepth <= 8 ? Utils::RGB8 : Utils::RGB16);
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

		// TODO: Multi byte colour data doesn't appears to have visual issues
		// Get number of bytes per channel so data can be copied correctly
		int channelSize = Utils::GetChannelByteSize(m_pixelFormat);
		for (uint32_t i = 0; i < m_width * m_height; i++) {
			Utils::Pixel pixel;
			switch (m_colourType) {
				// Truecolour
			case 2:
				[[fallthrough]];
				// Truecolour with alpha
			case 6:
				// Create pixel and copy and consume data in
				Utils::ExtractBigEndianBytes(pixel.R, input.data(), channelSize);
				input.erase(input.begin(), input.begin() + channelSize);
				Utils::ExtractBigEndianBytes(pixel.G, input.data(), channelSize);
				input.erase(input.begin(), input.begin() + channelSize);
				Utils::ExtractBigEndianBytes(pixel.B, input.data(), channelSize);
				input.erase(input.begin(), input.begin() + channelSize);

				// If alpha channel is present copy and consume data
				if (Utils::HasAlphaChannel(m_pixelFormat)) {
					Utils::ExtractBigEndianBytes(pixel.A, input.data(), channelSize);
					input.erase(input.begin(), input.begin() + channelSize);
				}
				break;
				// Indexed colour
			case 3: {
				// Construct mask
				uint8_t mask = 0;
				for (int i = 0; i < m_bitDepth; i++) {
					mask |= 128 >> i;
				}

				// Extract index
				uint8_t index = (mask & input[0]) >> (8 - m_bitDepth);

				// Select pixel
				pixel = m_PLTEData[index];

				// Shift value for next extraction
				input[0] <<= m_bitDepth;

				// Erase value if needed
				if ((i * m_bitDepth) % 8 == 0 && i != 0) {
					input.erase(input.begin());
				}
				break;
			}
				// Greyscale
			case 0:
				if (m_bitDepth < 8) {
					int maxVal = (int)std::pow(2, m_bitDepth) - 1;

					// Construct mask
					uint8_t mask = 0;
					for (int i = 0; i < m_bitDepth; i++) {
						mask |= 128 >> i;
					}

					// Extract value
					uint8_t value = (mask & input[0]) >> (8 - m_bitDepth);

					// Normalise value
					value = (uint8_t)std::round(((double)value / maxVal) * UINT8_MAX);

					// Assign components
					pixel.R = value;
					pixel.G = value;
					pixel.B = value;

					// Shift value for next extraction
					input[0] <<= m_bitDepth;

					// Erase value if needed
					if ((i * m_bitDepth) % 8 == 0 && i != 0) {
						input.erase(input.begin());
					}
					break;
				}
				else { [[fallthrough]]; }
				// Greyscale with alpha
			case 4:
				// Create pixel and copy and consume data in
				Utils::ExtractBigEndianBytes(pixel.R, input.data(), channelSize);
				input.erase(input.begin(), input.begin() + channelSize);
				pixel.G = pixel.R;
				pixel.B = pixel.R;

				// If alpha channel is present copy and consume data
				if (Utils::HasAlphaChannel(m_pixelFormat)) {
					Utils::ExtractBigEndianBytes(pixel.A, input.data(), channelSize);
					input.erase(input.begin(), input.begin() + channelSize);
				}
				break;
			}

			// Add pixel to data
			m_pixelData[std::floor(i / m_width)].push_back(pixel);
		}

		input.clear();

		// Shrink input to 0
		input.shrink_to_fit();
	}
}