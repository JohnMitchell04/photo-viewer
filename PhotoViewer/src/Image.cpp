#include "backends/imgui_impl_vulkan.h"

#include "Image.h"

namespace ImageLibrary {
	/*
		All code relating to Vulkan in this file was taken from Walnut created by Yan Chernovik
		Accessible here: https://github.com/StudioCherno/Walnut
	*/
	void Image::GenerateDescriptorSet() {
		// Get necessary information
		VkDevice device = Walnut::Application::GetDevice();
		VkResult err;
		VkFormat imageFormat = GetImageFormat();

		// Create the Image
		{
			// Create image information
			VkImageCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			info.imageType = VK_IMAGE_TYPE_2D;
			info.format = imageFormat;
			info.extent.width = m_width;
			info.extent.height = m_height;
			info.extent.depth = 1;
			info.mipLevels = 1;
			info.arrayLayers = 1;
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.tiling = VK_IMAGE_TILING_OPTIMAL;
			info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			// Create image
			err = vkCreateImage(device, &info, nullptr, &m_image);
			check_vk_result(err);

			// Get memory requirements of the image
			VkMemoryRequirements req;
			vkGetImageMemoryRequirements(device, m_image, &req);

			// Create allocation information
			VkMemoryAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize = req.size;
			alloc_info.memoryTypeIndex = GetVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);

			// Allocate memory for image
			err = vkAllocateMemory(device, &alloc_info, nullptr, &m_memory);
			check_vk_result(err);

			// Bind image to allocated memory
			err = vkBindImageMemory(device, m_image, m_memory, 0);
			check_vk_result(err);
		}

		// Create image view
		{
			// Create image view information
			VkImageViewCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.image = m_image;
			info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			info.format = imageFormat;
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			info.subresourceRange.levelCount = 1;
			info.subresourceRange.layerCount = 1;

			// Create image view
			err = vkCreateImageView(device, &info, nullptr, &m_imageView);
			check_vk_result(err);
		}

		// Create sampler:
		{
			// Create sampler information
			VkSamplerCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			info.magFilter = VK_FILTER_LINEAR;
			info.minFilter = VK_FILTER_LINEAR;
			info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			info.minLod = -1000;
			info.maxLod = 1000;
			info.maxAnisotropy = 1.0f;

			// Create sampler
			VkResult err = vkCreateSampler(device, &info, nullptr, &m_sampler);
			check_vk_result(err);
		}

		// Create the descriptor set:
		m_descriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(m_sampler, m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void Image::SetData() {
		// Get necessary information
		VkDevice device = Walnut::Application::GetDevice();
		size_t upload_size = m_width * m_height * m_nBytesPerPixel;
		VkResult err;

		if (!m_stagingBuffer)
		{
			// Create the Upload Buffer
			{
				// Create upload buffer information
				VkBufferCreateInfo buffer_info = {};
				buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffer_info.size = upload_size;
				buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				// Create upload buffer
				err = vkCreateBuffer(device, &buffer_info, nullptr, &m_stagingBuffer);
				check_vk_result(err);

				// Get memory requirements for upload buffer
				VkMemoryRequirements req;
				vkGetBufferMemoryRequirements(device, m_stagingBuffer, &req);
				m_alignedSize = req.size;

				// Create allocation information
				VkMemoryAllocateInfo alloc_info = {};
				alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				alloc_info.allocationSize = req.size;
				alloc_info.memoryTypeIndex = GetVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);

				// Allocate memory for staging buffer
				err = vkAllocateMemory(device, &alloc_info, nullptr, &m_stagingBufferMemory);
				check_vk_result(err);

				// Bind memory for staging buffer
				err = vkBindBufferMemory(device, m_stagingBuffer, m_stagingBufferMemory, 0);
				check_vk_result(err);
			}

		}

		// Upload to Buffer
		{
			// Map device staging buffer memory so it is application addressable
			char* map = NULL;
			err = vkMapMemory(device, m_stagingBufferMemory, 0, m_alignedSize, 0, (void**)(&map));
			check_vk_result(err);

			// Copy image data into map
			memcpy(map, m_imageData.data(), upload_size);

			// Create mapped memory information
			VkMappedMemoryRange range[1] = {};
			range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = m_stagingBufferMemory;
			range[0].size = m_alignedSize;

			// Flush devide memory
			err = vkFlushMappedMemoryRanges(device, 1, range);
			check_vk_result(err);

			// We no longer need access to memory so unmap it
			vkUnmapMemory(device, m_stagingBufferMemory);
		}


		// Copy to Image
		{
			// Get necessary information
			VkCommandBuffer command_buffer = Walnut::Application::GetCommandBuffer(true);

			// Create copy barrier information
			VkImageMemoryBarrier copy_barrier = {};
			copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copy_barrier.image = m_image;
			copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy_barrier.subresourceRange.levelCount = 1;
			copy_barrier.subresourceRange.layerCount = 1;

			// Create copy barrier
			vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &copy_barrier);

			// Create information about the copy to be performed
			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = m_width;
			region.imageExtent.height = m_height;
			region.imageExtent.depth = 1;

			// Copy buffer to image
			vkCmdCopyBufferToImage(command_buffer, m_stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			// Create barrier information
			VkImageMemoryBarrier use_barrier = {};
			use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			use_barrier.image = m_image;
			use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			use_barrier.subresourceRange.levelCount = 1;
			use_barrier.subresourceRange.layerCount = 1;

			// Create barrier
			vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &use_barrier);

			// Flush command buffer
			Walnut::Application::FlushCommandBuffer(command_buffer);
		}
	}

	VkFormat Image::GetImageFormat() {
		switch (m_pixelFormat) {
		case Utils::RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;
			break;
		case Utils::RGBA16:
			return VK_FORMAT_R16G16B16A16_UNORM;
			break;
		}
	}

	uint32_t Image::GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
	{
		VkPhysicalDeviceMemoryProperties prop;
		vkGetPhysicalDeviceMemoryProperties(Walnut::Application::GetPhysicalDevice(), &prop);
		for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
		{
			if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
			{
				return i;
			}
		}

		return 0xffffffff;
	}

	void Image::Release()
	{
		Walnut::Application::SubmitResourceFree([
			sampler = m_sampler, imageView = m_imageView, image = m_image, memory = m_memory, 
			stagingBuffer = m_stagingBuffer, stagingBufferMemory = m_stagingBufferMemory
		](){
			VkDevice device = Walnut::Application::GetDevice();
			vkDestroySampler(device, sampler, nullptr);
			vkDestroyImageView(device, imageView, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, memory, nullptr);
			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingBufferMemory, nullptr);
		});

		m_sampler = nullptr;
		m_imageView = nullptr;
		m_image = nullptr;
		m_memory = nullptr;
		m_stagingBuffer = nullptr;
		m_stagingBufferMemory = nullptr;
	}
}