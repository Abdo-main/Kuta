#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

void create_texture_image(char* filename, Models *models, State *state);

void create_texture_image_view(State *state, Models *models);

void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, State *state);

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, State *state);

void create_texture_sampler(State *state, Models *models);

void create_image(uint32_t width, uint32_t height,
                  VkFormat format, VkImageTiling tiling,
                  VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkImage* image,
                  VkDeviceMemory* image_memory,
                  State *state
                  );
