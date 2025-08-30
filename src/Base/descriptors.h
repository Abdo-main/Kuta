#pragma 

#include <cglm/cglm.h>
#include "main.h"

typedef struct {
    alignas(16)mat4 model;
    alignas(16)mat4 view;
    alignas(16)mat4 proj;
} UBO;

void create_descriptor_set_layout(State *state, VkCore *vk_core);


void destroy_descriptor_set_layout(State *state, VkCore *vk_core);


void destroy_descriptor_sets(State *state, VkCore *vk_core);

void create_descriptor_pool(State *state, VkCore *vk_core);
void create_descriptor_sets(State *state, VkCore *vk_core, BufferData *buffer_data, TextureData *texture_data);

