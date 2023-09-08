#include "PNG.h"

namespace ImageLibrary {
	void PNG::ReadFile() {
		ReadRawData();
		ParseSignature();
		ParseChunks();
	}

	void PNG::ReadRawData() {
		// Create input stream in binary mode throwing exceptions
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
			throw new std::runtime_error("PNG signature is invalid");
		}
	}

	void PNG::ParseChunks() {
		std::vector<Utils::PNGChunk> encounteredChunks;
		bool reachedIEND = false;
		int chunksIndex = 0;

		while (!reachedIEND) {
			// Get chunk length remembering it is big endian
			uint32_t length = 0;
			for (int i = 0; i < 4; i++) {
				length |= ((uint32_t)m_rawData[i] << (8 * (3 - i)));
			}

			// Consume chunk length
			m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

			// Copy and consume chunk specifier
			std::string chunkSpecifier(m_rawData.begin(), m_rawData.begin() + 4);
			m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

			// Check chunk has valid specifier
			for (int i = 0; i < 4; i++) {
				if (!isalpha(chunkSpecifier[i])) { throw new std::runtime_error("A chunk is invalid"); }
			}

			// If chunk is private ignore
			if (!isupper(chunkSpecifier[1])) { continue; }

			// If chunk isn't critical ignore for now
			// TODO: Deal with ancilliary chunks
			if (!isupper(chunkSpecifier[0])) { continue; }

			// Check 3rd bit is valid
			if (!isupper(chunkSpecifier[2])) { throw new std::runtime_error("A chunk is invalid"); }

			// Ensure the correct chunk is encountered first
			if (chunksIndex == 0 && chunkSpecifier != "IHDR") { throw new std::runtime_error("Chunk order is invalid"); }

			// Convert string specifier to enum
			std::unordered_map<std::string, Utils::PNGChunkIdentifier> table{ {"IHDR", Utils::IHDR}, {"PLTE", Utils::PLTE} };
			auto it = table.find(chunkSpecifier);
			Utils::PNGChunkIdentifier chunkSpecifierE = Utils::INVALID;

			if (it != table.end()) {
				chunkSpecifierE = it->second;
			}

			switch (chunkSpecifierE) {
			case Utils::IHDR:
				ParseIHDR();
				break;
			}

			chunksIndex++;
		}
	}

	void PNG::ParseIHDR() {
		// Get width remembering it is big endian
		for (int i = 0; i < 4; i++) {
			m_width |= ((uint32_t)m_rawData[i] << (8 * (3 - i)));
		}

		// Consume width
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

		// Get height remembering it is big endian
		uint32_t length = 0;
		for (int i = 0; i < 4; i++) {
			m_height |= ((uint32_t)m_rawData[i] << (8 * (3 - i)));
		}

		// Consume height
		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 4);

		// Perform checks on dimensions
		if (m_width > Utils::PNG_SPEC_MAX_DIMENSION || m_height > Utils::PNG_SPEC_MAX_DIMENSION || m_width < 0 || m_height < 0) {
			throw new std::runtime_error("Image dimensions invalid");
		}

		if (m_width > Utils::PNG_APP_MAX_DIMENSION || m_height > Utils::PNG_APP_MAX_DIMENSION) { throw new std::runtime_error("Application cannot display image"); }

		// Get more image info
		m_bitDepth = m_rawData[0];
		m_colourType = m_rawData[1];
		m_compressionMethod = m_rawData[2];
		m_filterMethod = m_rawData[3];
		m_interlaceMethod = m_rawData[4];

		m_rawData.erase(m_rawData.begin(), m_rawData.begin() + 5);

		// Check image info
		switch (m_colourType) {
		// Greyscale
		case 0:
			if (m_bitDepth != 1 && m_bitDepth != 2 && m_bitDepth != 4 && m_bitDepth != 8 && m_bitDepth != 16) {
				throw new std::runtime_error("Invalid colour type and bit depth combination");
			}
			break;
		// Indexed Colour
		case 3:
			if (m_bitDepth != 1 && m_bitDepth != 2 && m_bitDepth != 4 && m_bitDepth != 8) {
				throw new std::runtime_error("Invalid colour type and bit depth combination");
			}
			break;
		// True colour
		case 2:
		// Greyscale alpha
		case 4:
		// True colour alpha
		case 6:
			if (m_bitDepth != 8 && m_bitDepth != 16) {
				throw new std::runtime_error("Invalid colour type and bit depth combination");
			}
			break;
		// Invalid colour type
		default:
			throw new std::runtime_error("Invalid colour type");
		}

		// Do not process images with private compression methods
		if (m_compressionMethod != 0) { throw new std::runtime_error("Incompatible compression method"); }

		// Do not process images with private filter methods
		if (m_filterMethod != 0) { throw new std::runtime_error("Incompatible filter method"); }

		// Do not process images with private interlace methods
		if (m_interlaceMethod != 0 && m_interlaceMethod != 1) { throw new std::runtime_error("Incompatible interlace method"); }
	}
}