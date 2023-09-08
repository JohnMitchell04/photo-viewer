#pragma once

#include "Image.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <ctype.h>
#include <tuple>
#include <unordered_map>

namespace ImageLibrary {
	class PNG : Image
	{
	public:
		PNG(std::string filePath) : Image(filePath) { ReadFile(); };

	private:
		void ReadFile();

		void ReadRawData();
		void ParseSignature();
		void ParseChunks();
		void ParseIHDR();

	private:
		uint8_t m_bitDepth;
		uint8_t m_colourType;
		uint8_t m_compressionMethod;
		uint8_t m_filterMethod;
		uint8_t m_interlaceMethod;
	};
}