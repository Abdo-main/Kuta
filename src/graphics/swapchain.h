#pragma once

#include "internal_types.h"
#include <vulkan/vulkan.h>

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max);

void recreate_swapchain(State *state);

void acquire_next_swapchain_image(State *state);

void present_swapchain_image(State *state);

void create_swapchain(State *state);

void cleanup_swapchain(State *state);
