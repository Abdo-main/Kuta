#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#include <stdlib.h>

#include "main.h"
#include "window.h"
#include "swapchain.h"
#include "vulkan_core.h"
#include "renderer.h"
#include "utils.h"
#include "vertex_data.h"
#include "descriptors.h"



void init(State *state) {
    setup_error_handling();
    log_info();

    create_window(state);
    create_instance(state);

    select_physical_device(state);
    create_surface(state);
    select_queue_family(state);
    create_device(state);
    get_queue(state);
    create_swapchain(state);
    create_renderer(state);
}

void loop(State *state) {
    while (!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();
        
        uint32_t frame = state->current_frame;
        Renderer* renderer = &state->renderer;

        vkWaitForFences(state->device, 1, &renderer->in_flight_fence[frame], VK_TRUE, UINT64_MAX);
        vkResetFences(state->device, 1, &renderer->in_flight_fence[frame]);

        acquire_next_swapchain_image(state);
        record_command_buffer(state);
        submit_command_buffer(state);
        present_swapchain_image(state);

        state->current_frame = (state->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;    
    }
}

void cleanup(State *state) {
    destroy_renderer(state);
    cleanup_swapchain(state);  
    
    destroy_uniform_buffers(state);
    destroy_descriptor_sets(state);
    destroy_descriptor_set_layout(state);

    destroy_index_buffer(state);
    destroy_vertex_buffer(state);

    if (state->device != VK_NULL_HANDLE)
        vkDestroyDevice(state->device, state->allocator);

    if (state->surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(state->instance, state->surface, state->allocator);

    if (state->window)
        glfwDestroyWindow(state->window);

    if (state->instance != VK_NULL_HANDLE)
        vkDestroyInstance(state->instance, state->allocator);
}

int main(void) {
    State state = {
        .application_name = "Kodo",
        .engine_name = "Kuta",
        .window_title = "Hello, Kuta!",
        .window_width = 800,
        .window_height = 600,
        .window_fullscreen = false,
        .api_version = VK_API_VERSION_1_4,
    };

    init(&state);
    loop(&state);
    cleanup(&state);

    return EXIT_SUCCESS;
}
