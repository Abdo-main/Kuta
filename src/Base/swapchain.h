#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max);

void recreate_swapchain(State *state);

void acquire_next_swapchain_image(State *state);

void present_swapchain_image(Models *models, State *state);

void create_swapchain(State *state);

void cleanup_swapchain(State *state);

