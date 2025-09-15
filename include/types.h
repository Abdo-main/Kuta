#pragma once
#include <cglm/cglm.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

#define MAX_ENTITIES 1024
#define MAX_COMPONENT_TYPES 32
#define COMPONENT_SIGNATURE(type) (1ULL << (type))

typedef uint32_t Entity;

typedef enum {
  COMPONENT_TRANSFORM = 0,
  COMPONENT_MESH_RENDERER,
  COMPONENT_CAMERA,
  COMPONENT_VISIBILITY,
  COMPONENT_COUNT
} ComponentType;

typedef struct {
  vec3 position;
  vec3 rotation;
  vec3 scale;
  bool dirty;
  mat4 matrix;
} TransformComponent;

typedef struct {
  uint32_t model_id;
  uint32_t texture_id;
} MeshRendererComponent;

typedef struct {
  bool visible;
  float alpha;
} VisibilityComponent;

typedef uint64_t ComponentSignature;

typedef struct {
  Entity entities[MAX_ENTITIES];
  ComponentSignature signatures[MAX_ENTITIES];
  void *component_pools[COMPONENT_COUNT];
  size_t component_sizes[COMPONENT_COUNT];
  uint32_t entity_count;
  uint32_t next_entity_id;
} World;

typedef struct {
  const char *window_title;
  const char *application_name;
  const char *engine_name;

  uint32_t window_width, window_height;
  uint32_t api_version;
  VkClearColorValue background_color;
} Settings;
