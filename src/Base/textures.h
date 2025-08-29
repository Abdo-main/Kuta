#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

void create_texture_image(State *state, char* filename);
void create_texture_image_view(State *state);
void transition_image_layout(VkImage image, VkFormat format, State *state, VkImageLayout old_layout, VkImageLayout new_layout);
void copy_buffer_to_image(State *state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
void create_texture_sampler(State *state);
