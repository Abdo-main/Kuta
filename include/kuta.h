#pragma once

#include "internal_types.h"
#include "types.h"

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

uint32_t load_texture(const char *texture_file);

void renderer_deinit(void);

void begin_frame(void);

void end_frame(World *world);

bool running();

void kuta_deinit(void);
