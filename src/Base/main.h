#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>
#include "cglm/cglm.h"
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct {
    vec3 pos;
    vec3 color;
    vec2 tex_coord;
}Vertex;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphics_queue;
    uint32_t graphics_queue_family;
    uint32_t api_version;
    VkAllocationCallbacks* allocator;
} VkCore;

typedef struct {
    VkSwapchainKHR swapchain;
    VkImage* images;
    VkImageView* image_views;
    VkFormat image_format;
    VkExtent2D extent;
    uint32_t image_count;
    uint32_t acquired_image_index;
} SwapchainData;


typedef struct {
    VkPipeline graphics_pipeline;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkCommandPool command_pool;
    VkCommandBuffer* command_buffers;
    VkSemaphore* acquired_image_semaphore;
    VkSemaphore* finished_render_semaphore;
    VkFence* in_flight_fence;
    VkFramebuffer* frame_buffers;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout;
    VkDescriptorSet* descriptor_sets;
    uint32_t descriptor_set_count;
    
} Renderer;

typedef struct State {
    const char *window_title;
    const char *application_name;
    const char *engine_name;


    int window_width, window_height;

    bool window_fullscreen;

    GLFWwindow *window;
    GLFWmonitor *windowMonitor;

    Renderer renderer;


    VkImage texture_image;
    VkDeviceMemory texture_image_memmory;
    VkImageView texture_image_view;
    VkSampler texture_sampler;

    VkImage depth_image;
    VkDeviceMemory depth_image_memmory;
    VkImageView depth_image_view;

    //Vulkan
    uint32_t current_frame;



    Vertex* vertices;
    uint32_t* indices;
    size_t vertex_count;
    size_t index_count;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memmory;
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memmory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memmory;

    VkBuffer uniform_buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory uniform_buffers_memmory[MAX_FRAMES_IN_FLIGHT];
    void* uniform_buffers_mapped[MAX_FRAMES_IN_FLIGHT];

    VkClearColorValue background_color;

} State;
