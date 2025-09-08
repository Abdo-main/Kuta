#include <stddef.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>

#include "kuta.h"
#include "main.h"

#define MODELS_COUNT 2
#define TEXTURE_COUNT 2

int main(void) {
    Settings settings = {
        .api_version = VK_API_VERSION_1_4,
        .application_name = "Kudo",
        .engine_name = "Kuta",
        .background_color = (VkClearColorValue){1.0f, 1.5f, 1.0f},
        .window_width = 800,
        .window_height = 600,
        .window_title = "Hello, Kuta Library!!!",
    };
 
    const char* models_files[MODELS_COUNT] = {
        "./models/twitch.glb",
        "./models/t1_yone.glb",
    };

    const char* textures_files[TEXTURE_COUNT] = {
        "./textures/pasted__twitch.png",
        "./textures/Body.png",
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
