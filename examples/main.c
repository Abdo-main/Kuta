#include "cglm/types.h"
#include "kuta.h"
#include "types.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

int main(void) {
  Settings settings = {
      .api_version = VK_API_VERSION_1_3,
      .application_name = "Kudo",
      .engine_name = "Kuta",
      .background_color = {{0.0f, 0.0f, 0.0f}},
      .window_width = 800,
      .window_height = 600,
      .window_title = "Hello, Kuta ECS with Camera & Lighting!!!",
  };

  kuta_init(&settings);
  renderer_init();

  uint32_t player_mesh = load_geometry("./models/twitch.glb");
  uint32_t enemy_mesh = load_geometry("./models/aatrox.glb");
  uint32_t player_texture = load_texture("./textures/pasted__twitch.png");
  uint32_t enemy_texture = load_texture("./textures/Body-Aatrox.png");

  renderer_deinit();

  World world;
  world_init(&world);

  // Create Camera Entity
  Entity camera_entity = create_entity(&world);
  CameraComponent camera = {.position = {0.0f, 2.0f, 5.0f},
                            .worldUp = {0.0f, 1.0f, 0.0f},
                            .yaw = -90.0f,
                            .pitch = 0.0f,
                            .fov = 45.0f,
                            .nearPlane = 0.1f,
                            .farPlane = 100.0f,
                            .active = true,
                            .dirty = true};
  camera_system_update_vectors(&camera);
  add_component(&world, camera_entity, COMPONENT_CAMERA, &camera);

  set_active_camera(&camera_entity);

  // Create Light Entity (Point Light)
  Entity light_entity = create_entity(&world);
  TransformComponent light_transform = {.position = {0.0f, 2.0f, 2.0f},
                                        .rotation = {0.0f, 0.0f, 0.0f},
                                        .scale = {1.0f, 1.0f, 1.0f},
                                        .dirty = true};
  add_component(&world, light_entity, COMPONENT_TRANSFORM, &light_transform);

  LightComponent light = {.type = LIGHT_TYPE_POINT,
                          .color = {1.0f, 1.0f, 1.0f},
                          .intensity = 1.0f,
                          .radius = 10.0f,
                          .enabled = true};
  add_component(&world, light_entity, COMPONENT_LIGHT, &light);

  // Create another light (moving)
  Entity light2_entity = create_entity(&world);
  TransformComponent light2_transform = {.position = {-3.0f, 3.0f, 0.0f},
                                         .rotation = {0.0f, 0.0f, 0.0f},
                                         .scale = {1.0f, 1.0f, 1.0f},
                                         .dirty = true};
  add_component(&world, light2_entity, COMPONENT_TRANSFORM, &light2_transform);

  LightComponent light2 = {.type = LIGHT_TYPE_POINT,
                           .color = {1.0f, 0.5f, 0.2f}, // Orange light
                           .intensity = 0.8f,
                           .radius = 8.0f,
                           .enabled = true};
  add_component(&world, light2_entity, COMPONENT_LIGHT, &light2);

  // Create Player
  Entity player = create_entity(&world);
  TransformComponent transform1 = {.position = {0.0f, 0.0f, 0.0f},
                                   .rotation = {0.0f, 0.0f, 0.0f},
                                   .scale = {0.01f, 0.01f, 0.01f},
                                   .dirty = true};
  add_component(&world, player, COMPONENT_TRANSFORM, &transform1);

  MeshRendererComponent renderer1 = {
      .model_id = player_mesh,
      .texture_id = player_texture,
  };
  add_component(&world, player, COMPONENT_MESH_RENDERER, &renderer1);

  VisibilityComponent visibility1 = {.visible = true, .alpha = 1.0f};
  add_component(&world, player, COMPONENT_VISIBILITY, &visibility1);

  // Create Enemy
  Entity enemy = create_entity(&world);
  TransformComponent transform2 = {.position = {2.0f, 0.0f, 0.0f},
                                   .rotation = {0.0f, 0.0f, 0.0f},
                                   .scale = {0.01f, 0.01f, 0.01f},
                                   .dirty = true};
  add_component(&world, enemy, COMPONENT_TRANSFORM, &transform2);

  MeshRendererComponent renderer2 = {
      .model_id = enemy_mesh,
      .texture_id = enemy_texture,
  };
  add_component(&world, enemy, COMPONENT_MESH_RENDERER, &renderer2);

  VisibilityComponent visibility2 = {.visible = true, .alpha = 1.0f};
  add_component(&world, enemy, COMPONENT_VISIBILITY, &visibility2);

  while (running()) {
    begin_frame(&world);

    float time = glfwGetTime();
    vec3 light_pos = {
        cos(time) * 5.0f, // X position (circular motion)
        3.0f,             // Y position (constant height)
        sin(time) * 5.0f  // Z position (circular motion)
    };
    set_entity_position(&world, light_entity, light_pos);

    // Move and rotate entities
    vec3 rotation = {0.0f, time, 0.0f};
    set_entity_rotation(&world, player, rotation);
    set_entity_rotation(&world, enemy, rotation);

    end_frame(&world);
  }

  world_cleanup(&world);
  kuta_deinit();
  return EXIT_SUCCESS;
}
