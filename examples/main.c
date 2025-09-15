#include "kuta.h"
#include "types.h"
#include <cglm/cglm.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

int main(void) {
  Settings settings = {
      .api_version = VK_API_VERSION_1_3,
      .application_name = "Kudo",
      .engine_name = "Kuta",
      .background_color = {{1.5f, 1.9f, 1.6f}},
      .window_width = 800,
      .window_height = 600,
      .window_title = "Hello, Kuta ECS Library!!!",
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
    begin_frame();

    float time = glfwGetTime();

    vec3 rotation = {0.0f, time, 0.0f};
    set_entity_rotation(&world, player, rotation);
    set_entity_rotation(&world, enemy, rotation);

    end_frame(&world);
  }

  world_cleanup(&world);
  kuta_deinit();
  return EXIT_SUCCESS;
}
