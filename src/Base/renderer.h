#pragma once

#include "main.h"

void record_command_buffer(SwapchainData *swp_ch, BufferData *buffer_data, Config *config, Renderer *renderer, GeometryData *geometry_data);



void submit_command_buffer(VkCore *vk_core, SwapchainData *swp_ch, BufferData *buffer_data, Renderer *renderer);

void create_renderer(VkCore *vk_core, SwapchainData *swp_ch, BufferData *buffer_data, TextureData *texture_data, Renderer *renderer, GeometryData *geometry_data);


void create_frame_buffers(VkCore *vk_core, SwapchainData *swp_ch, TextureData *texture_data, Renderer *renderer);

void destroy_frame_buffers(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer);


void destroy_renderer(VkCore *vk_core, SwapchainData *swp_ch, Renderer *renderer);

