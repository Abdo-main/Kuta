#pragma once

#include "main.h"

void record_command_buffer(State *state);
void submit_command_buffer(State *state, VkCore *vk_core);
void create_renderer(State *state, VkCore *vk_core);
void create_frame_buffers(State *state, VkCore *vk_core);

void destroy_frame_buffers(State *state, VkCore *vk_core);
void destroy_renderer(State *state, VkCore *vk_core);
