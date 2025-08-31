#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max);
void recreate_swapchain(TextureData *texture_data, State *state);
void acquire_next_swapchain_image(TextureData *texture_data, State *state);
void present_swapchain_image(TextureData *texture_data, State *state);
void create_swapchain(TextureData *texture_data, State *state);
void cleanup_swapchain(TextureData *texture_data, State *state);

