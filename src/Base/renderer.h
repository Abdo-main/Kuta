#pragma once

#include "main.h"

void record_command_buffer(State *state, SwapchainData *swp_ch);

void submit_command_buffer(State *state, VkCore *vk_core, SwapchainData *swp_ch);

void create_renderer(State *state, VkCore *vk_core, SwapchainData *swp_ch);
void create_frame_buffers(State *state, VkCore *vk_core, SwapchainData *swp_ch);

void destroy_frame_buffers(State *state, VkCore *vk_core, SwapchainData *swp_ch);

void destroy_renderer(State *state, VkCore *vk_core, SwapchainData *swp_ch);

