#pragma once
#include <cglm/types.h>
#include <stdbool.h>
#include <stdint.h>
#include "main.h"

typedef struct {
    vec2 pos;
    vec3 color;
}Vertex;

typedef struct {
    VkVertexInputAttributeDescription items[2];
} AttributeDescriptions;

extern Vertex vertices[4];
extern uint16_t indices[6];

VkVertexInputBindingDescription get_binding_description();

AttributeDescriptions get_attribute_descriptions(void);

void create_vertex_buffer(State *state);
void create_index_buffer(State *state);
void create_uniform_buffers(State *state);

void update_uniform_buffer(State* state, uint32_t currentImage);


void destroy_vertex_buffer(State *state);
void destroy_index_buffer(State *state);
void destroy_uniform_buffers(State *state);

