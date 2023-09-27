#pragma once

#include <string>
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "vulkan/vulkan.h"
#include "Walnut/Application.h"

#include "Utils.h"

namespace ImageLibrary {
	class Image
	{
	public:
		Image(std::string filePath) noexcept(false) : m_filePath(filePath) { ReadRawData(); };
		~Image() noexcept { Release(); };

		uint32_t GetWidth() const noexcept { return m_width; }
		uint32_t GetHeight() const noexcept { return m_height; }
		VkDescriptorSet GetDescriptorSet() const noexcept { return m_descriptorSet; }

	protected:
		// Vulkan functions for child classes
		void GenerateDescriptorSet();
		void SetData();

		// Function that must be implemented by child class to read and process image
		virtual void ReadFile() = 0;

	private:
		// Internal function to read raw file data when initialised
		void ReadRawData();

		// Convert pixel data to a vulkan useable format
		std::vector<uint8_t> PixelDataToBuffer();

		// Internal Vulkan functions
		VkFormat GetVulkanisedImageFormat();
		VkImageCreateInfo AddAlphaChannel();
		uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits);
		void Release();

	protected:
		// File information
		std::string m_filePath;
		std::vector<uint8_t> m_rawData;

		// Image information
		std::vector<std::vector<Utils::Pixel>> m_pixelData;
		uint32_t m_width = 0, m_height = 0;
		Utils::PixelFormat m_pixelFormat = Utils::INVALID;

		// Vulkan information
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