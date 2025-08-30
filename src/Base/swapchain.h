#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max);

void recreate_swapchain(State *state, VkCore *vk_core, SwapchainData *swp_ch);
void acquire_next_swapchain_image(State *state, VkCore *vk_core, SwapchainData *swp_ch);
void present_swapchain_image(State *state, VkCore *vk_core, SwapchainData *swp_ch);
void create_swapchain(State *state, VkCore *vk_core, SwapchainData *swp_ch);
void cleanup_swapchain(State *state, VkCore *vk_core, SwapchainData *swp_ch);

