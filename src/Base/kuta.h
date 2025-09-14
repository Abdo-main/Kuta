#pragma once

#include "main.h"

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

void world_init(World *world);

void world_cleanup(World *world);

Entity create_entity(World *world);

bool entity_exists(World *world, Entity entity);

void add_component(World *world, Entity entity, ComponentType type,
                   void *component);

void *get_component(World *world, Entity entity, ComponentType type);

void set_entity_position(World *world, Entity entity, vec3 position);

void set_entity_rotation(World *world, Entity entity, vec3 rotation);

void set_entity_scale(World *world, Entity entity, vec3 scale);

void move_entity(World *world, Entity entity, vec3 delta);

void render_system_draw(World *world, VkCommandBuffer cmd_buffer);

bool kuta_init(Settings *settings);

void renderer_init(void);

uint32_t load_geometry(const char *filepath);

void free_geometry_buffers(ResourceManager *rm, State *state, uint32_t id);

void load_texture(const char *textures_files[]);

void renderer_deinit(void);

void begin_frame(void);

void end_frame(World *world);

bool running();

void kuta_deinit(void);
