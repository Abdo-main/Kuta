#include "cglm/util.h"
#include "utils.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "buffer_data.h"
#include "stb/stb_image.h"
#include "texture_data.h"
#include "utils.h"

void create_image(uint32_t width, uint32_t height, VkFormat format,
                  VkImageTiling tiling, VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkImage *image,
                  VkDeviceMemory *image_memory, uint32_t mipLevels,
                  VkSampleCountFlagBits num_samples, State *state) {

  VkImageCreateInfo image_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent.width = (uint32_t)width,
      .extent.height = (uint32_t)height,
      .extent.depth = 1,
      .mipLevels = mipLevels,
      .arrayLayers = 1,
      .format = format,
      .tiling = tiling,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .samples = num_samples,
  };

  EXPECT(vkCreateImage(state->vk_core.device, &image_info,
                       state->vk_core.allocator, image),
         "Failed to create image")

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(state->vk_core.device, *image,
                               &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex =
          find_memory_type(mem_requirements.memoryTypeBits, properties, state),
  };

  EXPECT(vkAllocateMemory(state->vk_core.device, &alloc_info,
                          state->vk_core.allocator, image_memory),
         "Failed to allocate memmory for image")
  vkBindImageMemory(state->vk_core.device, *image, *image_memory, 0);
}

void generate_mipmaps(VkImage image, VkFormat image_format, int32_t tex_width,
                      int32_t tex_height, uint32_t mipLevels, State *state) {

  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(state->vk_core.physical_device,
                                      image_format, &format_properties);
  if (!(format_properties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    perror("texture image format does not support linear blitting!");
  }

  VkCommandBuffer command_buffer = begin_single_time_commands(state);

  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .image = image,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
      .subresourceRange.levelCount = 1,
  };

  int32_t mip_width = tex_width;
  int32_t mip_height = tex_height;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &barrier);
    VkImageBlit blit = {
        .srcOffsets[0] = {0, 0, 0},
        .srcOffsets[1] = {mip_width, mip_height, 1},
        .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .srcSubresource.mipLevel = i - 1,
        .srcSubresource.baseArrayLayer = 0,
        .srcSubresource.layerCount = 1,
        .dstOffsets[0] = {0, 0, 0},
        .dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1,
                          mip_height > 1 ? mip_height / 2 : 1, 1},
        .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .dstSubresource.mipLevel = i,
        .dstSubresource.baseArrayLayer = 0,
        .dstSubresource.layerCount = 1,
    };
    vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0,
                         NULL, 1, &barrier);
    if (mip_width > 1)
      mip_width /= 2;
    if (mip_height > 1)
      mip_height /= 2;
  }
  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0,
                       NULL, 1, &barrier);
  end_single_time_commands(command_buffer, state);
}

Texture_image__memory
create_texture_image(const char *filename, State *state,
                     VkDeviceMemory *texture_image_memory) {
  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memmory;
  VkImage texture_image; // Declared but not initialized yet

  int tex_width = 0, tex_height = 0, tex_channels = 0;
  stbi_uc *pixels = stbi_load(filename, &tex_width, &tex_height, &tex_channels,
                              STBI_rgb_alpha);

  uint32_t mipLevels = (uint32_t)floor(log2(glm_max(tex_width, tex_height)));

  EXPECT(!pixels, "Failed to load texture image!")
  VkDeviceSize image_size = tex_width * tex_height * 4;

  create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &staging_buffer, &staging_buffer_memmory, state);

  void *data;
  vkMapMemory(state->vk_core.device, staging_buffer_memmory, 0, image_size, 0,
              &data);
  memcpy(data, pixels, (size_t)image_size);
  vkUnmapMemory(state->vk_core.device, staging_buffer_memmory);

  stbi_image_free(pixels);

  create_image(tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texture_image,
               texture_image_memory, mipLevels, VK_SAMPLE_COUNT_1_BIT, state);

  Texture_image__memory tx = {
      .texture_image = texture_image,
      .texture_image_memory = texture_image_memory,
      .mipLevels = mipLevels,
  };

  transition_image_layout(
      texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, state);

  copy_buffer_to_image(staging_buffer, texture_image, (uint32_t)tex_width,
                       (uint32_t)tex_height, state);

  generate_mipmaps(texture_image, VK_FORMAT_R8G8B8A8_SRGB, tex_width,
                   tex_height, mipLevels, state);

  vkDestroyBuffer(state->vk_core.device, staging_buffer,
                  state->vk_core.allocator);
  vkFreeMemory(state->vk_core.device, staging_buffer_memmory,
               state->vk_core.allocator);

  return tx;
}

void transition_image_layout(VkImage image, VkFormat format,
                             VkImageLayout old_layout, VkImageLayout new_layout,
                             uint32_t mipLevels, State *state) {
  VkCommandBuffer command_buffer = begin_single_time_commands(state);

  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = mipLevels,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
      .srcAccessMask = 0, // TODO
      .dstAccessMask = 0, // TODO
  };

  // Set proper aspectMask depending on the layout / format
  if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (has_stencil_component(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  VkPipelineStageFlags source_stage;
  VkPipelineStageFlags destination_stage;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    printf("unsupported layout transition!");
    return;
  }

  vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0,
                       NULL, 0, NULL, 1, &barrier);

  end_single_time_commands(command_buffer, state);
}

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width,
                          uint32_t height, State *state) {
  VkCommandBuffer command_buffer = begin_single_time_commands(state);

  VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .imageSubresource.mipLevel = 0,
      .imageSubresource.baseArrayLayer = 0,
      .imageSubresource.layerCount = 1,
      .imageOffset = {0, 0, 0},
      .imageExtent = {width, height, 1},
  };

  vkCmdCopyBufferToImage(command_buffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  end_single_time_commands(command_buffer, state);
}

VkImageView create_texture_image_view(State *state, VkImage texture_image,
                                      uint32_t mipLevels) {
  VkImageView texture_image_view =
      create_image_view(texture_image, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, state);

  return texture_image_view;
}

VkSampler create_texture_sampler(State *state) {
  VkPhysicalDeviceProperties properties = {};
  vkGetPhysicalDeviceProperties(state->vk_core.physical_device, &properties);

  VkSamplerCreateInfo sampler_info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .mipLodBias = 0.0f,
      .minLod = 0.0f,
      .maxLod = VK_LOD_CLAMP_NONE,
  };
  VkSampler texture_sampler;
  EXPECT(vkCreateSampler(state->vk_core.device, &sampler_info,
                         state->vk_core.allocator, &texture_sampler),
         "Failed to create texture sampler")
  return texture_sampler;
}

VkSampleCountFlagBits get_max_usable_sample_count(State *state) {
  VkPhysicalDeviceProperties physical_device_properties;
  vkGetPhysicalDeviceProperties(state->vk_core.physical_device,
                                &physical_device_properties);

  VkSampleCountFlags counts =
      physical_device_properties.limits.framebufferColorSampleCounts &
      physical_device_properties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }

  return VK_SAMPLE_COUNT_1_BIT;
}

void create_color_resources(State *state) {
  VkFormat color_format = state->swp_ch.image_format;
  create_image(state->swp_ch.extent.width, state->swp_ch.extent.height,
               color_format, VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               &state->texture_data.color_image,
               &state->texture_data.color_image_memory, 1,
               state->renderer.msaa_samples, state);

  state->texture_data.color_image_view =
      create_image_view(state->texture_data.color_image, color_format,
                        VK_IMAGE_ASPECT_COLOR_BIT, 1, state);
}
