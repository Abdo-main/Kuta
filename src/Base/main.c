#include <stddef.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>

#include "main.h"
#include "kuta.h"

#define MODELS_COUNT 2
#define TEXTURE_COUNT 2

int main(void) {
    const char* models_files[MODELS_COUNT] = {
        "./models/twitch.glb",
        "./models/t1_yone.glb",
    };

    const char* texture_files[TEXTURE_COUNT] = {
        "./textures/pasted__twitch.png",
        "./textures/Body.png",
    }; 

    Models models = {
        .model_count = MODELS_COUNT,
        .model_files = models_files,
        .texture_files = texture_files,
    };

    State state = {
        .window_data = {
            .fullscreen = false,
            .width = 800,
            .height = 600,
            .title = "Hello Library",
        },
        .vk_core = {
            .api_version = VK_API_VERSION_1_4,
        }
    };

    Config config = {
        .application_name = "Kudo",
        .engine_name = "Kuta",
        .background_color = (VkClearColorValue) {1.0f, 1.0f, 1.0f},
        .window_title = "Hello, World!!!",
    };

    BufferData buffer_data = {};

    kuta_init(&models, &state, &config, &buffer_data);

    while (!glfwWindowShouldClose(state.window_data.window)) {
        kuta_loop(&models, &state, &config, &buffer_data);
    }

    kuta_deinit(&models, &state, &buffer_data);
 
    return EXIT_SUCCESS;
}
