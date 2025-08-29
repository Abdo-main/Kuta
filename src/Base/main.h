#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>


typedef struct {
    VkPipeline graphics_pipeline;
    VkExtent2D image_extent;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkCommandPool command_pool;
    VkCommandBuffer* command_buffers;
    VkSemaphore* acquired_image_semaphore;
    VkSemaphore* finished_render_semaphore;
    VkFence* in_flight_fence;
    VkFramebuffer* frame_buffers;
} Renderer;

typedef struct State {
    const char *window_title;
    const char *application_name;
    const char *engine_name;


    int window_width, window_height;

    bool window_fullscreen;

    GLFWwindow *window;
    GLFWmonitor *windowMonitor;

    VkFormat image_format;
    Renderer renderer;

    //Vulkan
    uint32_t queue_family;
    uint32_t api_version;
    uint32_t swap_chain_image_count;
    uint32_t current_frame;


    VkAllocationCallbacks *allocator;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue queue;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memmory;
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memmory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memmory;


    VkClearValue background_color;
    VkSwapchainKHR swap_chain;
    VkImage *swap_chain_images;
    VkImageView *swap_chain_image_views;
    uint32_t acquired_image_index;

} State;
