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
    VkImageView* image_views; VkFormat image_format;
    VkExtent2D extent;
    uint32_t image_count;
    uint32_t acquired_image_index;
} SwapchainData;

typedef struct {
    // Geometry buffers
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    
    // Per-frame uniform buffers
    VkBuffer uniform_buffers[MAX_FRAMES_IN_FLIGHT];
    VkDeviceMemory uniform_buffers_memory[MAX_FRAMES_IN_FLIGHT];
    void* uniform_buffers_mapped[MAX_FRAMES_IN_FLIGHT];
    
    // Staging for uploads
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
} BufferData;

typedef struct {
    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkSampler texture_sampler;
    
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;
} TextureData;

typedef struct {
    GLFWwindow* window;
    GLFWmonitor *monitor;
    const char* title;
    int width, height;
    bool fullscreen;
} WindowData;

typedef struct {
    Vertex* vertices;
    uint32_t* indices;
    size_t vertex_count;
    size_t index_count;
} GeometryData;

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
    uint32_t current_frame;
    
} Renderer;

typedef struct {
    const char *window_title;
    const char *application_name;
    const char *engine_name;
    VkClearColorValue background_color;
} Config;

typedef struct {
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;
    
    float yaw;
    float pitch;
    
    float movementSpeed;
    float mouseSensitivity;
    
    mat4 viewMatrix;
} Camera;

typedef struct {
    bool keys[GLFW_KEY_LAST];
    bool firstMouse;
    float lastX, lastY;
    Camera camera;
} InputState;

typedef struct {
    Renderer renderer;
    SwapchainData swp_ch;
    VkCore vk_core;
    WindowData window_data;
    InputState input_state;
} State;

