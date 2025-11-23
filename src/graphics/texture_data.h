#pragma once

#include "internal_types.h"

#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

typedef struct {
  VkImage texture_image;
  VkDeviceMemory *texture_image_memory;
  uint32_t mipLevels;
} Texture_image__memory;

Texture_image__memory
create_texture_image(const char *filename, State *state,
                     VkDeviceMemory *texture_image_memory);

VkImageView create_texture_image_view(State *state, VkImage texture_image,
                                      uint32_t mipLevels);

void transition_image_layout(VkImage image, VkFormat format,
                             VkImageLayout old_layout, VkImageLayout new_layout,
                             uint32_t mipLevels, State *state);

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width,
                          uint32_t height, State *state);

VkSampler create_texture_sampler(State *state);

void create_image(uint32_t width, uint32_t height, VkFormat format,
                  VkImageTiling tiling, VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkImage *image,
                  VkDeviceMemory *image_memory, uint32_t mipLevels,
                  VkSampleCountFlagBits num_samples, State *state);

VkSampleCountFlagBits get_max_usable_sample_count(State *state);

void create_color_resources(State *state);
