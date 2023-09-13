#pragma once

#include "Image.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <ctype.h>
#include <tuple>
#include <array>

namespace ImageLibrary {
	class PNG : Image
	{
	public:
		PNG(std::string filePath) : Image(filePath) { InitCRC(); ReadFile(); };

	private:
		void InitCRC();
		void ReadFile();

		void ReadRawData();
		void ParseSignature();
		void ParseChunks();
		void CheckCRC(uint32_t length);
		void ParseIHDR();
		void DecompressData();

	private:
		std::array<uint32_t, 256> m_crcTable;
		uint8_t m_bitDepth;
		uint8_t m_colourType;
		uint8_t m_compressionMethod;
		uint8_t m_filterMethod;
		uint8_t m_interlaceMethod;
		std::vector<uint8_t> m_compressedData;
		std::vector<uint8_t> m_filteredData;
	};
}