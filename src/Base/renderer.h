#pragma once

#include "main.h"

void record_command_buffer(State *state);
void submit_command_buffer(State *state);
void create_renderer(State *state);
void recreate_swapchain(State *state);

void destroy_frame_buffers(State *state);
void destroy_renderer(State *state);
