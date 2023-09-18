#pragma once

#include <string>
#include <vector>
#include <iterator>

#include "vulkan/vulkan.h"
#include "Walnut/Application.h"

#include "Utils.h"

namespace ImageLibrary {
	class Image
	{
	public:
		Image(std::string filePath) noexcept(false) : m_filePath(filePath) {};
		~Image() noexcept { Release(); };

		uint32_t GetWidth() const noexcept { return m_width; }
		uint32_t GetHeight() const noexcept { return m_height; }
		VkDescriptorSet GetDescriptorSet() const noexcept { return m_descriptorSet; }

	protected:
		void GenerateDescriptorSet();
		void SetData();

		virtual void ReadFile() = 0;

	private:
		VkFormat GetImageFormat();
		uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits);
		void Release();

	protected:
		std::string m_filePath;
		std::vector<uint8_t> m_rawData;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		Utils::PixelFormat m_pixelFormat = Utils::INVALID;
		std::vector<uint8_t> m_imageData;
		uint8_t m_nBytesPerPixel = 0;

		// Vulkan stuff
		VkImage m_image = nullptr;
		VkImageView m_imageView = nullptr;
		VkDeviceMemory m_memory = nullptr;
		VkSampler m_sampler = nullptr;

		VkBuffer m_stagingBuffer = nullptr;
		VkDeviceMemory m_stagingBufferMemory = nullptr;
		size_t m_alignedSize = 0;

		VkDescriptorSet m_descriptorSet = nullptr;
	};
}