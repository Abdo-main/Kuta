#pragma once
#include <cglm/types.h>
#include <stdbool.h>
#include "main.h"

typedef struct {
    vec2 pos;
    vec3 color;
}Vertex;

typedef struct {
    VkVertexInputAttributeDescription items[2];
} AttributeDescriptions;

extern Vertex vertices[3];
VkVertexInputBindingDescription get_binding_description();

AttributeDescriptions get_attribute_descriptions(void);

void create_vertex_buffer(State *state);

void destroy_vertex_buffer(State *state);
