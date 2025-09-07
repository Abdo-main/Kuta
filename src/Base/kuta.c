#include <stddef.h>
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
#include "textures.h"
#include "models.h"


void kuta_init(Models *models, State *state, Config *config, BufferData *buffer_data){
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

    create_swapchain(state); 
    create_renderer(buffer_data, models, state);
}
float lastFrame = 0.0f;
void kuta_loop(Models *models, State *state, Config *config, BufferData *buffer_data){
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;  

    glfwPollEvents();

    process_input(state, deltaTime);

    uint32_t frame = state->renderer.current_frame;
    vkWaitForFences(state->vk_core.device, 1, &state->renderer.in_flight_fence[frame], VK_TRUE, UINT64_MAX);
    vkResetFences(state->vk_core.device, 1, &state->renderer.in_flight_fence[frame]);

    acquire_next_swapchain_image(state);
    record_command_buffer(buffer_data, config, models, state);
    submit_command_buffer(buffer_data, state);
    present_swapchain_image(models, state);

    state->renderer.current_frame = (state->renderer.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;    
}

void kuta_deinit(Models *models, State *state, BufferData *buffer_data){
    destroy_renderer(state);
    cleanup_swapchain(state);  
    for (size_t i = 0; i < 2; i++) {
        vkDestroySampler(state->vk_core.device, models->texture[i].texture_sampler, state->vk_core.allocator);
        vkDestroyImageView(state->vk_core.device, models->texture[i].texture_image_view, state->vk_core.allocator);
        vkDestroyImage(state->vk_core.device, models->texture[i].texture_image, state->vk_core.allocator);
        vkFreeMemory(state->vk_core.device, models->texture[i].texture_image_memory, state->vk_core.allocator);
    }
    destroy_uniform_buffers(buffer_data, state);
    destroy_descriptor_sets(state);
    destroy_descriptor_set_layout(state);
    for (size_t i = 0; i < 2; i++) {
        destroy_index_buffers(models, state, i);
        destroy_vertex_buffers(models, state, i);
    }    
    if (state->vk_core.device != VK_NULL_HANDLE)
        vkDestroyDevice(state->vk_core.device, state->vk_core.allocator);

    if (state->vk_core.surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(state->vk_core.instance, state->vk_core.surface, state->vk_core.allocator);

    if (state->window_data.window)
        glfwDestroyWindow(state->window_data.window);

    if (state->vk_core.instance != VK_NULL_HANDLE)
        vkDestroyInstance(state->vk_core.instance, state->vk_core.allocator);
}


