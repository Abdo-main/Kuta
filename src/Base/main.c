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



void init(Config *config, VkCore *vk_core, SwapchainData *swp_ch, BufferData *buffer_data, TextureData *texture_data, WindowData *window_data, Renderer *renderer, GeometryData *geometry_data) {
    setup_error_handling();
    log_info();

    create_window(window_data);

    init_vk(vk_core, config, window_data);

    create_swapchain(vk_core, swp_ch, texture_data, window_data);

    create_renderer(vk_core, swp_ch, buffer_data, texture_data, renderer, geometry_data);
}

void loop(VkCore *vk_core,SwapchainData *swp_ch, BufferData *buffer_data, TextureData *texture_data, WindowData *window_data, Renderer *renderer, GeometryData *geometry_data, Config *config) {
    while (!glfwWindowShouldClose(window_data->window)) {
        glfwPollEvents();
        
        uint32_t frame = renderer->current_frame;

        vkWaitForFences(vk_core->device, 1, &renderer->in_flight_fence[frame], VK_TRUE, UINT64_MAX);
        vkResetFences(vk_core->device, 1, &renderer->in_flight_fence[frame]);

        acquire_next_swapchain_image(vk_core, swp_ch, texture_data, window_data, renderer);
        record_command_buffer(swp_ch, buffer_data, config, renderer, geometry_data);
        submit_command_buffer(vk_core, swp_ch, buffer_data, renderer);
        present_swapchain_image(vk_core, swp_ch, texture_data, window_data, renderer);

        renderer->current_frame = (renderer->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;    
    }
}

void cleanup(VkCore *vk_core, SwapchainData *swp_ch, BufferData *buffer_data, TextureData *texture_data, WindowData *window_data, Renderer *renderer) {
    destroy_renderer(vk_core, swp_ch, renderer);
    cleanup_swapchain(vk_core, swp_ch, texture_data);  

    vkDestroySampler(vk_core->device, texture_data->texture_sampler, vk_core->allocator);
    vkDestroyImageView(vk_core->device, texture_data->texture_image_view, vk_core->allocator);
    vkDestroyImage(vk_core->device, texture_data->texture_image, vk_core->allocator);
    vkFreeMemory(vk_core->device, texture_data->texture_image_memory, vk_core->allocator);
    
    destroy_uniform_buffers(vk_core, buffer_data);
    destroy_descriptor_sets(vk_core, renderer);
    destroy_descriptor_set_layout(vk_core, renderer);

    destroy_index_buffer(vk_core, buffer_data);
    destroy_vertex_buffer(vk_core, buffer_data);

    if (vk_core->device != VK_NULL_HANDLE)
        vkDestroyDevice(vk_core->device, vk_core->allocator);

    if (vk_core->surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(vk_core->instance, vk_core->surface, vk_core->allocator);

    if (window_data->window)
        glfwDestroyWindow(window_data->window);

    if (vk_core->instance != VK_NULL_HANDLE)
        vkDestroyInstance(vk_core->instance, vk_core->allocator);
}

int main(void) {
    Config config = {
        .application_name = "Kuta",
        .engine_name = "Kuta",
        .background_color = (VkClearColorValue) {1.0f, 1.0f, 1.0f},
        .window_title = "Hello World",
    };
    VkCore vk_core = {
        .api_version = VK_API_VERSION_1_4,
    };
    SwapchainData swp_ch = {};
    BufferData buffer_data = {};
    TextureData texture_data = {};
    WindowData window_data = {
        .title = "Kuta",
        .fullscreen = false,
        .width = 1020,
        .height = 720,
    };
    Renderer renderer = {};
    GeometryData geometry_data = {};

    init(&config ,&vk_core, &swp_ch, &buffer_data, &texture_data, &window_data, &renderer, &geometry_data);
    loop(&vk_core, &swp_ch, &buffer_data, &texture_data, &window_data, &renderer, &geometry_data, &config);
    cleanup(&vk_core, &swp_ch, &buffer_data, &texture_data, &window_data, &renderer);

    return EXIT_SUCCESS;
}
