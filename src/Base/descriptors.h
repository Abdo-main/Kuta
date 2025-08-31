#pragma 

#include <cglm/cglm.h>
#include "main.h"

typedef struct {
    alignas(16)mat4 model;
    alignas(16)mat4 view;
    alignas(16)mat4 proj;
} UBO;

void create_descriptor_set_layout(VkCore *vk_core, Renderer *renderer);
void destroy_descriptor_set_layout(VkCore *vk_core, Renderer *renderer);
void create_descriptor_pool(VkCore *vk_core, Renderer *renderer);
void create_descriptor_sets(VkCore *vk_core, BufferData *buffer_data, TextureData *texture_data, Renderer *renderer);
void destroy_descriptor_sets(VkCore *vk_core, Renderer *renderer);
