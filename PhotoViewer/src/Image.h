#pragma once

#include <string>
#include <vector>

#include "Utils.h"

namespace ImageLibrary {
	class Image
	{
	public:
		Image(std::string filePath) noexcept(false) : m_filePath(filePath) {};

		std::vector<uint8_t> GetPixelData() const noexcept { return m_pixelData; };

	private:
		virtual void ReadFile() noexcept(false) = 0;

	protected:
		std::string m_filePath;
		std::vector<uint8_t> m_rawData;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		std::vector<uint8_t> m_pixelData;
	};
}