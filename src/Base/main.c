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



void init(State *state, VkCore *vk_core) {
    setup_error_handling();
    log_info();

    create_window(state);

    init_vk(vk_core, state);

    create_swapchain(state, vk_core);
    create_renderer(state, vk_core);
}

void loop(State *state, VkCore *vk_core) {
    while (!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();
        
        uint32_t frame = state->current_frame;
        Renderer* renderer = &state->renderer;

        vkWaitForFences(vk_core->device, 1, &renderer->in_flight_fence[frame], VK_TRUE, UINT64_MAX);
        vkResetFences(vk_core->device, 1, &renderer->in_flight_fence[frame]);

        acquire_next_swapchain_image(state, vk_core);
        record_command_buffer(state);
        submit_command_buffer(state, vk_core);
        present_swapchain_image(state, vk_core);

        state->current_frame = (state->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;    
    }
}

void cleanup(State *state, VkCore *vk_core) {
    destroy_renderer(state, vk_core);
    cleanup_swapchain(state, vk_core);  

    vkDestroySampler(vk_core->device, state->texture_sampler, vk_core->allocator);
    vkDestroyImageView(vk_core->device, state->texture_image_view, vk_core->allocator);
    vkDestroyImage(vk_core->device, state->texture_image, vk_core->allocator);
    vkFreeMemory(vk_core->device, state->texture_image_memmory, vk_core->allocator);
    
    destroy_uniform_buffers(state, vk_core);
    destroy_descriptor_sets(state, vk_core);
    destroy_descriptor_set_layout(state, vk_core);

    destroy_index_buffer(state, vk_core);
    destroy_vertex_buffer(state, vk_core);

    if (vk_core->device != VK_NULL_HANDLE)
        vkDestroyDevice(vk_core->device, vk_core->allocator);

    if (vk_core->surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(vk_core->instance, vk_core->surface, vk_core->allocator);

    if (state->window)
        glfwDestroyWindow(state->window);

    if (vk_core->instance != VK_NULL_HANDLE)
        vkDestroyInstance(vk_core->instance, vk_core->allocator);
}

int main(void) {
    State state = {
        .application_name = "Kodo",
        .engine_name = "Kuta",
        .window_title = "Hello, Kuta!",
        .window_width = 800,
        .window_height = 600,
        .window_fullscreen = false,
        .background_color = (VkClearColorValue){{0.3f, 0.1f, 0.0f, 1.0f}}
    };

    VkCore vk_core = {
        .api_version = VK_API_VERSION_1_4,
    };

    init(&state, &vk_core);
    loop(&state, &vk_core);
    cleanup(&state, &vk_core);

    return EXIT_SUCCESS;
}
