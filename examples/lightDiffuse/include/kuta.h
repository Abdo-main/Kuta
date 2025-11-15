#pragma once

#include "types.h"

void world_init(World *world);

void world_cleanup(World *world);

Entity create_entity(World *world);

void add_component(World *world, Entity entity, ComponentType type,
                   void *component);

void set_entity_position(World *world, Entity entity, vec3 position);

float get_time();

void set_entity_rotation(World *world, Entity entity, vec3 rotation);

void set_entity_scale(World *world, Entity entity, vec3 scale);

void move_entity(World *world, Entity entity, vec3 delta);

void camera_system_update_vectors(CameraComponent *camera);

void set_active_camera(Entity *camera_entity);

bool kuta_init(Settings *settings);

void renderer_init(void);

uint32_t load_geometry(const char *filepath);

uint32_t load_texture(const char *texture_file);

void renderer_deinit(void);

void begin_frame(World *world);
void end_frame(World *world);

bool running();

void kuta_deinit(void);
