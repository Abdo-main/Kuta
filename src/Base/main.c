#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "kuta.h"
#include "main.h"

#define MODELS_COUNT 2
#define TEXTURE_COUNT 2

int main(void) {
  Settings settings = {
      .api_version = VK_API_VERSION_1_3,
      .application_name = "Kudo",
      .engine_name = "Kuta",
      .background_color = (VkClearColorValue){1.0f, 1.5f, 1.0f},
      .window_width = 800,
      .window_height = 600,
      .window_title = "Hello, Kuta Library!!!",
      .models_count = 1,
  };

  const char *models_files[MODELS_COUNT] = {
      "./models/aatrox.glb",
  };

  const char *textures_files[TEXTURE_COUNT] = {
      "./textures/Body-Aatrox.png",
  };

  kuta_init(&settings);

  renderer_init();

  load_texture(textures_files);
  load_glbs(models_files);

  renderer_deinit();

  while (running()) {
    begin_frame();
    end_frame();
  }

  kuta_deinit();

  return EXIT_SUCCESS;
}
