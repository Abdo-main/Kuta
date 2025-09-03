#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>

#include "main.h"
#include "camera.h"
#include "window.h"
#include "swapchain.h"
#include "vulkan_core.h"
#include "renderer.h"
#include "utils.h"
#include "vertex_data.h"
#include "descriptors.h"



void init(Config *config, BufferData *buffer_data, TextureData *texture_data, State *state, GeometryData *geometry_data) {
    setup_error_handling();
    log_info();

    create_window(&state->window_data);
    // Set the state as window user pointer so callbacks can access it
    glfwSetWindowUserPointer(state->window_data.window, state);

    // Initialize input state
    state->input_state.firstMouse = true;
    state->input_state.lastX = state->window_data.width / 2.0f;
    state->input_state.lastY = state->window_data.height / 2.0f;

    // Set callbacks
    glfwSetKeyCallback(state->window_data.window, key_callback);
    glfwSetCursorPosCallback(state->window_data.window, mouse_callback);
    glfwSetInputMode(state->window_data.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    camera_init(&state->input_state.camera);    init_vk(config, state);

    create_swapchain(texture_data, state);

    create_renderer(buffer_data, texture_data, geometry_data, state);
}

void loop(BufferData *buffer_data, TextureData *texture_data, State *state, GeometryData *geometry_data, Config *config) {

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(state->window_data.window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;  

        glfwPollEvents();

        process_input(state, deltaTime);

        uint32_t frame = state->renderer.current_frame;
        vkWaitForFences(state->vk_core.device, 1, &state->renderer.in_flight_fence[frame], VK_TRUE, UINT64_MAX);
        vkResetFences(state->vk_core.device, 1, &state->renderer.in_flight_fence[frame]);

        acquire_next_swapchain_image(texture_data, state);
        record_command_buffer(buffer_data, config, geometry_data, state);
        submit_command_buffer(buffer_data, state);
        present_swapchain_image(texture_data, state);

        state->renderer.current_frame = (state->renderer.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;    
    }
}

void cleanup(BufferData *buffer_data, TextureData *texture_data, State *state) {
    destroy_renderer(state);
    cleanup_swapchain(texture_data, state);  

    vkDestroySampler(state->vk_core.device, texture_data->texture_sampler, state->vk_core.allocator);
    vkDestroyImageView(state->vk_core.device, texture_data->texture_image_view, state->vk_core.allocator);
    vkDestroyImage(state->vk_core.device, texture_data->texture_image, state->vk_core.allocator);
    vkFreeMemory(state->vk_core.device, texture_data->texture_image_memory, state->vk_core.allocator);
    
    destroy_uniform_buffers(buffer_data, state);
    destroy_descriptor_sets(state);
    destroy_descriptor_set_layout(state);

    destroy_index_buffer(buffer_data, state);
    destroy_vertex_buffer(buffer_data, state);

    if (state->vk_core.device != VK_NULL_HANDLE)
        vkDestroyDevice(state->vk_core.device, state->vk_core.allocator);

    if (state->vk_core.surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(state->vk_core.instance, state->vk_core.surface, state->vk_core.allocator);

    if (state->window_data.window)
        glfwDestroyWindow(state->window_data.window);

    if (state->vk_core.instance != VK_NULL_HANDLE)
        vkDestroyInstance(state->vk_core.instance, state->vk_core.allocator);
}

int main(void) {
    Config config = {
        .application_name = "Kuta",
        .engine_name = "Kuta",
        .background_color = (VkClearColorValue) {1.0f, 1.0f, 1.0f},
        .window_title = "Hello World",
    };
    State state = {
        .window_data = {
            .title = "Kuta",
            .fullscreen = false,
            .width = 1020,
            .height = 720,
        },
        .vk_core = {
            .api_version = VK_API_VERSION_1_4,
        }
    };

    BufferData buffer_data = {};
    TextureData texture_data = {};
    GeometryData geometry_data = {};

    init(&config , &buffer_data, &texture_data, &state, &geometry_data);
    loop(&buffer_data, &texture_data, &state, &geometry_data, &config);
    cleanup(&buffer_data, &texture_data, &state);

    return EXIT_SUCCESS;
}
