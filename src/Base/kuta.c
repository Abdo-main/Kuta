#include <stddef.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>

#include "main.h"
#include "camera.h"
#include "textures.h"
#include "window.h"
#include "swapchain.h"
#include "vulkan_core.h"
#include "renderer.h"
#include "utils.h"
#include "vertex_data.h"
#include "descriptors.h"
#include "models.h"

static KutaContext* kuta_context = NULL;

bool kuta_init(Settings *settings){
       if (kuta_context != NULL) {
        return false;
    }
    kuta_context = calloc(1, sizeof(KutaContext)); // Use calloc instead of malloc
    if (!kuta_context) return false; 

    setup_error_handling();
    log_info();

    kuta_context->state.window_data.width = settings->window_width;
    kuta_context->state.window_data.height = settings->window_height;
    kuta_context->state.window_data.title = settings->window_title;
    kuta_context->state.vk_core.api_version = settings->api_version;
    kuta_context->settings.background_color = settings->background_color;
    kuta_context->models.model_count = settings->models_count;
    
    create_window(&kuta_context->state.window_data);
    // Set the state as window user pointer so callbacks can access it
    glfwSetWindowUserPointer(kuta_context->state.window_data.window, &kuta_context->state);

    // Initialize input state
    kuta_context->state.input_state.firstMouse = true;
    kuta_context->state.input_state.lastX = kuta_context->state.window_data.width / 2.0f;
    kuta_context->state.input_state.lastY = kuta_context->state.window_data.height / 2.0f;

    // Set callbacks
    glfwSetKeyCallback(kuta_context->state.window_data.window, key_callback);
    glfwSetCursorPosCallback(kuta_context->state.window_data.window, mouse_callback);
    glfwSetInputMode(kuta_context->state.window_data.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    camera_init(&kuta_context->state.input_state.camera);    
    init_vk(&kuta_context->settings, &kuta_context->state);

    create_swapchain(&kuta_context->state); 

    return true;
}

void renderer_init(void){
    alloc_model_data(&kuta_context->models);
    create_render_pass(&kuta_context->state);
    create_descriptor_set_layout(&kuta_context->state);
    create_graphics_pipeline(&kuta_context->state);
    create_command_pool(&kuta_context->state);
    create_depth_resources(&kuta_context->state);
    create_frame_buffers(&kuta_context->state);
}

void renderer_deinit(void){
    create_descriptor_pool(&kuta_context->state, &kuta_context->models);
    create_uniform_buffers(&kuta_context->state, &kuta_context->buffer_data);
    create_descriptor_sets(&kuta_context->buffer_data, &kuta_context->models, &kuta_context->state);
    allocate_command_buffer(&kuta_context->state);
    create_sync_objects(&kuta_context->state);
}

void load_glbs(const char* models_files[]){
   load_models(models_files, &kuta_context->models, kuta_context->models.model_count, &kuta_context->buffer_data, &kuta_context->state); 
}

void load_texture(const char* textures_files[]){
    load_textures(&kuta_context->models, &kuta_context->state, textures_files);
}

float lastFrame = 0.0f;
void begin_frame(void){
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;  

    glfwPollEvents();

    process_input(&kuta_context->state, deltaTime);

    uint32_t frame = kuta_context->state.renderer.current_frame;
    vkWaitForFences(kuta_context->state.vk_core.device, 1, &kuta_context->state.renderer.in_flight_fence[frame], VK_TRUE, UINT64_MAX);
    vkResetFences(kuta_context->state.vk_core.device, 1, &kuta_context->state.renderer.in_flight_fence[frame]);

    acquire_next_swapchain_image(&kuta_context->state);
   
}

void end_frame(void){
    record_command_buffer(&kuta_context->buffer_data, &kuta_context->settings, &kuta_context->models, &kuta_context->state);
    submit_command_buffer(&kuta_context->buffer_data, &kuta_context->state);
    present_swapchain_image(&kuta_context->models, &kuta_context->state);

    kuta_context->state.renderer.current_frame = (kuta_context->state.renderer.current_frame + 1) % MAX_FRAMES_IN_FLIGHT; 
}

void kuta_deinit(void) {
    if (!kuta_context) return;
    
    vkDeviceWaitIdle(kuta_context->state.vk_core.device); // Wait before cleanup
    
    destroy_renderer(&kuta_context->state);
    cleanup_swapchain(&kuta_context->state);
    for (size_t i = 0; i < kuta_context->models.model_count; i++) {
        if (kuta_context->models.texture[i].texture_sampler != VK_NULL_HANDLE){
            vkDestroySampler(kuta_context->state.vk_core.device, kuta_context->models.texture[i].texture_sampler, kuta_context->state.vk_core.allocator);
        }
        if (kuta_context->models.texture[i].texture_image_view != VK_NULL_HANDLE){
            vkDestroyImageView(kuta_context->state.vk_core.device, kuta_context->models.texture[i].texture_image_view, kuta_context->state.vk_core.allocator);
        }
        if (kuta_context->models.texture[i].texture_image != VK_NULL_HANDLE){
            vkDestroyImage(kuta_context->state.vk_core.device, kuta_context->models.texture[i].texture_image, kuta_context->state.vk_core.allocator);
        }
        if (kuta_context->models.texture[i].texture_image_memory != VK_NULL_HANDLE){
            vkFreeMemory(kuta_context->state.vk_core.device, kuta_context->models.texture[i].texture_image_memory, kuta_context->state.vk_core.allocator);
        }
    }
    destroy_uniform_buffers(&kuta_context->buffer_data, &kuta_context->state);
    destroy_descriptor_sets(&kuta_context->state);
    destroy_descriptor_set_layout(&kuta_context->state);
    for (size_t i = 0; i < kuta_context->models.model_count; i++) {
        destroy_index_buffers(&kuta_context->models, &kuta_context->state, i);
        destroy_vertex_buffers(&kuta_context->models, &kuta_context->state, i);
    }    
    if (kuta_context->state.vk_core.device != VK_NULL_HANDLE)
        vkDestroyDevice(kuta_context->state.vk_core.device, kuta_context->state.vk_core.allocator);

    if (kuta_context->state.vk_core.surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(kuta_context->state.vk_core.instance, kuta_context->state.vk_core.surface, kuta_context->state.vk_core.allocator);

    if (kuta_context->state.window_data.window)
        glfwDestroyWindow(kuta_context->state.window_data.window);

    if (kuta_context->state.vk_core.instance != VK_NULL_HANDLE)
        vkDestroyInstance(kuta_context->state.vk_core.instance, kuta_context->state.vk_core.allocator);
}

bool running(){
    return !glfwWindowShouldClose(kuta_context->state.window_data.window);
}
