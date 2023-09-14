#pragma once

#include <vector>

#include "Utils.h"

namespace ImageLibrary {
	class PixelData
	{
	public:
		PixelData() {};
		PixelData(Utils::PixelFormat format, std::vector<std::vector<uint8_t>>& data) : m_pixelFormat(format), m_data(data) {};

		void SetPixelData(std::vector<std::vector<uint8_t>>& data) { m_data = data; }
		void SetFormat(Utils::PixelFormat format) { m_pixelFormat = format; }

		//Utils::Pixel GetPixel(uint32_t x, uint32_t y);

	private:
		Utils::PixelFormat m_pixelFormat = Utils::INVALID;
		std::vector<std::vector<uint8_t>> m_data;
	};
}