#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
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

		void ReadRawData();
		void ParseSignature();
		void ParseChunks();
		void CheckCRC(uint32_t length);
		void ParseIHDR();
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
		bool m_indexedAlpha = false;
	};
}