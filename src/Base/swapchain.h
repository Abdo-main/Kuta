#pragma once

#include <vulkan/vulkan.h>
#include "main.h"

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max);
void recreate_swapchain(VkCore *vk_core, SwapchainData *swp_ch, TextureData *texture_data, WindowData *window_data, Renderer *renderer);
void acquire_next_swapchain_image(VkCore *vk_core, SwapchainData *swp_ch, TextureData *texture_data, WindowData *window_data, Renderer *renderer);
void present_swapchain_image(VkCore *vk_core, SwapchainData *swp_ch, TextureData *texture_data, WindowData *window_data, Renderer *renderer);
void create_swapchain(VkCore *vk_core, SwapchainData *swp_ch, TextureData *texture_data, WindowData *window_data);
void cleanup_swapchain(VkCore *vk_core, SwapchainData *swp_ch, TextureData *texture_data);

