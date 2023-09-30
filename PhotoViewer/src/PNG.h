#pragma once

#include <iterator>
#include <ctype.h>
#include <array>

#include "Image.h"

namespace ImageLibrary {
	class PNG : public Image
	{
	public:
		PNG(std::string filePath) : Image(filePath) { InitCRC(); ReadFile(); GenerateDescriptorSet(); SetData(); };

	private:
		void InitCRC();
		void ReadFile();

		void ParseSignature();
		void ParseChunks();
		bool CheckChunkOccurence(const std::vector<Utils::PNG::Chunk>& encounteredChunks, Utils::PNG::ChunkIdentifier chunk, int number);
		void CheckCRC(uint32_t length);
		void ParseIHDR();
		void ParsePLTE(uint32_t length);
		std::vector<uint8_t> DecompressData();
		std::vector<uint8_t> UnfilterData(std::vector<uint8_t>& input);
		std::vector<uint8_t> DeinterlaceData(std::vector<uint8_t>& input);
		void ParsePixels(std::vector<uint8_t>& input);

	private:
		std::array<uint32_t, 256> m_crcTable;
		uint8_t m_bitDepth;
		uint8_t m_colourType;
		uint8_t m_compressionMethod;
		uint8_t m_filterMethod;
		uint8_t m_interlaceMethod;
		std::vector<uint8_t> m_compressedData;
		std::vector<Utils::Pixel> m_PLTEData;
		bool m_indexedAlpha = false;
		int m_bytesPerPixel;
	};
}