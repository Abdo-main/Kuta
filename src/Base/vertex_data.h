#pragma once
#include "main.h"
#include <cglm/types.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  VkVertexInputAttributeDescription items[3];
  size_t count;
} AttributeDescriptions;

VkVertexInputBindingDescription get_binding_description();

AttributeDescriptions get_attribute_descriptions(void);

uint32_t find_memory_type(uint32_t type_filter,
                          VkMemoryPropertyFlags properties, State *state);

void create_vertex_buffer(BufferData *buffer_data, Models *models, State *state,
                          size_t index);

void create_index_buffer(BufferData *buffer_data, Models *models, State *state,
                         size_t index);

void create_uniform_buffers(State *state, BufferData *buffer_data);

bool has_stencil_component(VkFormat format);

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer *buffer,
                   VkDeviceMemory *buffer_memory, State *state);

void create_depth_resources(State *state);

VkFormat find_depth_format(State *state);

void update_uniform_buffer(uint32_t current_image, State *state,
                           BufferData *buffer_data);

void destroy_vertex_buffers(Models *models, State *state, size_t index);

void destroy_index_buffers(Models *models, State *state, size_t index);

void destroy_uniform_buffers(BufferData *buffer_data, State *state);
