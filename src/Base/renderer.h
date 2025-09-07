#pragma once

#include "main.h"
void record_command_buffer(BufferData *buffer_data, Config *config, Models *models, State *state);

void submit_command_buffer(BufferData *buffer_data, State *state);

void create_renderer(BufferData *buffer_data,  Models *models, State *state);

void create_frame_buffers(State *state);

void destroy_frame_buffers(State *state);

void destroy_renderer(State *state);

void alloc_model_data(Models *models);

void create_graphics_pipeline(State *state);

void create_render_pass(State *state);

void create_command_pool(State *state);

void allocate_command_buffer(State *state);

void create_sync_objects(State *state);
