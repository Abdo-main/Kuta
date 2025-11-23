#pragma once
#include "internal_types.h"
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

void create_vertex_buffer(State *state, BufferData *buffer_data,
                          Vertex *vertices, size_t vertex_count,
                          VkBuffer *vertex_buffer,
                          VkDeviceMemory *vertex_memory);

void create_index_buffer(State *state, BufferData *buffer_data,
                         uint32_t *indices, size_t indices_count,
                         VkBuffer *index_buffer, VkDeviceMemory *index_memory);

void create_uniform_buffers(State *state, BufferData *buffer_data);

void create_lighting_buffers(State *state);

void destroy_lighting_buffers(State *state);

bool has_stencil_component(VkFormat format);

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer *buffer,
                   VkDeviceMemory *buffer_memory, State *state);

void create_depth_resources(State *state, uint32_t mip_levels);

VkFormat find_depth_format(State *state);

void destroy_uniform_buffers(BufferData *buffer_data, State *state);

void update_camera_uniform_buffer(World *world, BufferData *buffer_data,
                                  State *state, uint32_t current_image);

void update_lighting_uniform_buffer(World *world, State *state,
                                    uint32_t current_image);
