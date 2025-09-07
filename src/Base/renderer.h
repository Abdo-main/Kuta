#pragma once

#include "main.h"
void record_command_buffer(BufferData *buffer_data, Config *config, Models *models, State *state);

void submit_command_buffer(BufferData *buffer_data, State *state);

void create_renderer(BufferData *buffer_data,  Models *models, State *state);

void create_frame_buffers(State *state);

void destroy_frame_buffers(State *state);

void destroy_renderer(State *state);

