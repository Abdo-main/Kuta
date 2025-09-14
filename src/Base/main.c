#include "main.h"
#include "kuta.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stddef.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define MODELS_COUNT 2
#define TEXTURE_COUNT 2

int main(void) {
  Settings settings = {
      .api_version = VK_API_VERSION_1_3,
      .application_name = "Kudo",
      .engine_name = "Kuta",
      .background_color = (VkClearColorValue){1.0f, 1.0f, 1.0f, 1.0f},
      .window_width = 800,
      .window_height = 600,
      .window_title = "Hello, Kuta ECS Library!!!",
      .models_count = 2,
  };

  const char *models_files[MODELS_COUNT] = {
      "./models/aatrox.glb",
      "./models/twitch.glb",
  };

  const char *textures_files[TEXTURE_COUNT] = {
      "./textures/Body-Aatrox.png",
      "./textures/pasted__twitch.png",
  };

  // Initialize Kuta
  kuta_init(&settings);
  renderer_init();
  load_glbs(models_files);
  load_texture(textures_files);
  renderer_deinit();

  // *** INITIALIZE ECS WORLD ***
  World world;
  world_init(&world);

  // *** CREATE ENTITIES ***
  // Create first entity
  Entity player = create_entity(&world);

  TransformComponent transform1 = {
      .position = {0.0f, 0.0f, 0.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {0.01f, 0.01f, 0.01f}, // Scale down the model
      .dirty = true};
  add_component(&world, player, COMPONENT_TRANSFORM, &transform1);

  MeshRendererComponent renderer1 = {
      .model_id = 0,   // First (and only) loaded model
      .texture_id = 0, // First (and only) loaded texture
  };
  add_component(&world, player, COMPONENT_MESH_RENDERER, &renderer1);

  VisibilityComponent visibility1 = {.visible = true, .alpha = 1.0f};
  add_component(&world, player, COMPONENT_VISIBILITY, &visibility1);

  // Create second entity (same model, different position)
  Entity enemy = create_entity(&world);

  TransformComponent transform2 = {
      .position = {2.0f, 0.0f, 0.0f}, // Different position
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {0.01f, 0.01f, 0.01f},
      .dirty = true};
  add_component(&world, enemy, COMPONENT_TRANSFORM, &transform2);

  MeshRendererComponent renderer2 = {
      .model_id = 1,   // Same model as player
      .texture_id = 1, // Same texture as player
  };
  add_component(&world, enemy, COMPONENT_MESH_RENDERER, &renderer2);

  VisibilityComponent visibility2 = {.visible = true, .alpha = 1.0f};
  add_component(&world, enemy, COMPONENT_VISIBILITY, &visibility2);

  // *** MAIN GAME LOOP ***
  while (running()) {
    begin_frame();

    // *** ANIMATE ENTITIES ***
    float time = glfwGetTime();

    // Rotate the player
    vec3 rotation = {0.0f, time, 0.0f};
    set_entity_rotation(&world, player, rotation);

    // Move the enemy in a circle
    vec3 enemy_pos = {cos(time) * 3.0f, 0.0f, sin(time) * 3.0f};
    set_entity_position(&world, enemy, enemy_pos);

    // *** RENDER FRAME ***
    end_frame(&world);
  }

  // *** CLEANUP ***
  world_cleanup(&world);
  kuta_deinit();
  return EXIT_SUCCESS;
}
