#pragma once
#include "vulkan_core.h"

void render_system_draw(World *world, VkCommandBuffer cmd_buffer);

void lighting_system_gather(World *world, LightingUBO *lighting_ubo);

CameraComponent *get_active_camera(World *world);
