#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

void create_texture_image(State *state, char* filename, VkCore *vk_core, TextureData *texture_data);
void create_texture_image_view(State *state, VkCore *vk_core, TextureData *texture_data);

void transition_image_layout(VkImage image, VkFormat format, State *state, VkImageLayout old_layout, VkImageLayout new_layout, VkCore *vk_core);
void copy_buffer_to_image(State *state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkCore *vk_core);
void create_texture_sampler(State *state, VkCore *vk_core, TextureData *texture_data);

void create_image(State *state, uint32_t width, uint32_t height,
                  VkFormat format, VkImageTiling tiling,
                  VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkImage* image,
                  VkDeviceMemory* image_memory,
                  VkCore *vk_core
                  );
