#pragma

#include "main.h"
#include <cglm/cglm.h>

typedef struct {
  alignas(16) mat4 view;
  alignas(16) mat4 proj;
} UBO;

void create_descriptor_set_layout(State *state);
void destroy_descriptor_set_layout(State *state);
void create_descriptor_pool(State *state, Models *models);
void create_descriptor_sets(BufferData *buffer_data, Models *models,
                            State *state);
void destroy_descriptor_sets(State *state);
