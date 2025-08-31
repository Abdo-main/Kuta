#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

void create_texture_image(char* filename, VkCore *vk_core, TextureData *texture_data, Renderer *renderer);
void create_texture_image_view(VkCore *vk_core, TextureData *texture_data);
void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkCore *vk_core, Renderer *renderer);
void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCore *vk_core, Renderer *renderer);

void create_texture_sampler(VkCore *vk_core, TextureData *texture_data);

void create_image(uint32_t width, uint32_t height,
                  VkFormat format, VkImageTiling tiling,
                  VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkImage* image,
                  VkDeviceMemory* image_memory,
                  VkCore *vk_core
                  );
