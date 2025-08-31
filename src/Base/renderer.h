#pragma once

#include "main.h"

void record_command_buffer(BufferData *buffer_data, Config *config, GeometryData *geometry_data, State *state);

void submit_command_buffer(BufferData *buffer_data, State *state);

void create_renderer(BufferData *buffer_data, TextureData *texture_data, GeometryData *geometry_data, State *state);


void create_frame_buffers(State *state, TextureData *texture_data);
void destroy_frame_buffers(State *state);

void destroy_renderer(State *state);

