#pragma once
#include <cglm/types.h>
#include <stdbool.h>
#include <stdint.h>
#include "main.h"

typedef struct {
    VkVertexInputAttributeDescription items[3];
    size_t count;
} AttributeDescriptions;


VkVertexInputBindingDescription get_binding_description();

AttributeDescriptions get_attribute_descriptions(void);

uint32_t find_memory_type(State *state, uint32_t type_filter, VkMemoryPropertyFlags properties, VkCore *vk_core);

void create_vertex_buffer(State *state, VkCore *vk_core);

void create_index_buffer(State *state, VkCore *vk_core);

void create_uniform_buffers(State *state, VkCore *vk_core);


bool has_stencil_component(VkFormat format);
void create_buffer(State *state, VkDeviceSize size,
       VkBufferUsageFlags usage,
       VkMemoryPropertyFlags properties,
       VkBuffer* buffer, 
       VkDeviceMemory* buffer_memory,
       VkCore *vk_core);

void create_depth_resources(State *state, VkCore *vk_core);


VkFormat find_depth_format(State *state, VkCore *vk_core);


void update_uniform_buffer(State* state, uint32_t currentImage);


void destroy_vertex_buffer(State *state, VkCore *vk_core);
void destroy_index_buffer(State *state, VkCore *vk_core);
void destroy_uniform_buffers(State *state, VkCore *vk_core);


