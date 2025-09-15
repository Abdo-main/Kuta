#pragma once

#include "internal_types.h"
#include <cglm/cglm.h>

typedef struct {
  alignas(16) mat4 view;
  alignas(16) mat4 proj;
} UBO;

void create_descriptor_set_layout(State *state);
void destroy_descriptor_set_layout(State *state);
void create_descriptor_pool(State *state, ResourceManager *rm);

void create_descriptor_sets(BufferData *buffer_data, ResourceManager *rm,
                            State *state);
void destroy_descriptor_sets(State *state);
