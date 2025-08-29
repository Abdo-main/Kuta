#include <cglm/cglm.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <cglm/cglm.h>        
#include <cglm/affine.h>     
#include <cglm/mat4.h>       
#include <cglm/vec3.h>        
#include <time.h> 
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "main.h"
#include "vertex_data.h"
#include "utils.h"
#include "descriptors.h"
#include "utils.h"

#define MAX_FRAMES_IN_FLIGHT 2

Vertex vertices[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
};

uint16_t indices[6] = {
    0, 1, 2, 2, 3, 0
};

VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription binding_description = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    return binding_description;
}

AttributeDescriptions get_attribute_descriptions(void) {
    AttributeDescriptions descs = {0}; // zero initialize
    
    descs.items[0].binding = 0;
    descs.items[0].location = 0;
    descs.items[0].format = VK_FORMAT_R32G32_SFLOAT;
    descs.items[0].offset = offsetof(Vertex, pos);

    descs.items[1].binding = 0;
    descs.items[1].location = 1;
    descs.items[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs.items[1].offset = offsetof(Vertex, color);

    return descs;
}

uint32_t find_memory_type(State *state, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(state->physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return 0;
}

void alloc_buffer(State *state, VkBuffer *buffer, VkDeviceMemory *buffer_memory, VkMemoryPropertyFlags properties){
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(state->device, *(buffer), &mem_requirements);


    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(state, mem_requirements.memoryTypeBits, properties),
    };

    EXPECT(vkAllocateMemory(state->device, &alloc_info, state->allocator, buffer_memory), "Failed to allocate for vertex_buffer_memory")
    vkBindBufferMemory(state->device, *(buffer), *(buffer_memory), 0);
}

void create_buffer(State *state, VkDeviceSize size,
       VkBufferUsageFlags usage,
       VkMemoryPropertyFlags properties,
       VkBuffer* buffer, 
       VkDeviceMemory* buffer_memory) {

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    EXPECT(vkCreateBuffer(state->device, &buffer_info, state->allocator, buffer), "failed to create vertex buffer!")

    alloc_buffer(state, buffer, buffer_memory, properties);
}

void copy_buffer(State *state, VkDeviceSize size, VkBuffer src_buffer, VkBuffer dst_buffer) {
    VkCommandBuffer command_buffer = begin_single_time_commands(state);

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
    
    end_single_time_commands(state, command_buffer);
}

void create_vertex_buffer(State *state) {
    size_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertex_count;

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &state->staging_buffer, &state->staging_buffer_memmory);
        
    void* data;
    vkMapMemory(state->device, state->staging_buffer_memmory, 0, buffer_size, 0, &data);
       memcpy(data, vertices, (size_t)buffer_size);
    vkUnmapMemory(state->device, state->staging_buffer_memmory);

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &state->vertex_buffer, &state->vertex_buffer_memmory);

    copy_buffer(state, buffer_size, state->staging_buffer, state->vertex_buffer);

    vkDestroyBuffer(state->device, state->staging_buffer, state->allocator);
    vkFreeMemory(state->device, state->staging_buffer_memmory, state->allocator);
}

void destroy_vertex_buffer(State *state){
    vkDestroyBuffer(state->device, state->vertex_buffer, state->allocator);
    vkFreeMemory(state->device, state->vertex_buffer_memmory, state->allocator);
}

void create_index_buffer(State *state) {
    size_t indices_count = sizeof(indices) / sizeof(indices[0]);
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices_count;

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &state->staging_buffer, &state->staging_buffer_memmory);
        
    void* data;
    vkMapMemory(state->device, state->staging_buffer_memmory, 0, buffer_size, 0, &data);
       memcpy(data, indices, (size_t)buffer_size);
    vkUnmapMemory(state->device, state->staging_buffer_memmory);

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &state->index_buffer, &state->index_buffer_memmory);

    copy_buffer(state, buffer_size, state->staging_buffer, state->index_buffer);

    vkDestroyBuffer(state->device, state->staging_buffer, state->allocator);
    vkFreeMemory(state->device, state->staging_buffer_memmory, state->allocator);
}

void destroy_index_buffer(State *state){
    vkDestroyBuffer(state->device, state->index_buffer, state->allocator);
    vkFreeMemory(state->device, state->index_buffer_memmory, state->allocator);
}

void create_uniform_buffers(State *state) {
    VkDeviceSize buffer_size = sizeof(UBO); // Assuming you have a ubo struct

    // In C, we use arrays instead of vectors - make sure your State struct has these arrays:
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(state, buffer_size, 
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                     &state->uniform_buffers[i], 
                     &state->uniform_buffers_memmory[i]);

        vkMapMemory(state->device, state->uniform_buffers_memmory[i], 0, buffer_size, 0, 
                   &state->uniform_buffers_mapped[i]);
    }
}
void destroy_uniform_buffers(State *state){
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(state->device, state->uniform_buffers[i], state->allocator);
        vkFreeMemory(state->device, state->uniform_buffers_memmory[i], state->allocator);
    }
}

void update_uniform_buffer(State* state, uint32_t current_image) {
    static int initialized = 0;
    static struct timespec start_time;
    
    // Initialize start_time on first call
    if (!initialized) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        initialized = 1;
    }
    
    // Get current time
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    
    // Calculate elapsed time in seconds
    float time = (current_time.tv_sec - start_time.tv_sec) + 
                ((float)(current_time.tv_nsec - start_time.tv_nsec)) / 1000000000.0f;
    
    // Now use 'time' for your uniform buffer updates
    // For example:
    UBO ubo = {0};
    
    // Create model matrix with rotation over time
    mat4 model;
    glm_mat4_identity(model);
    
    vec3 axis = {0.0f, 0.0f, 1.0f};
    float angle = time * glm_rad(90.0f); // Rotate 90 degrees per second
    glm_rotate(model, angle, axis);
    glm_mat4_copy(model, ubo.model);
    
    // Create view matrix (lookAt)
    mat4 view;
    vec3 eye = {2.0f, 2.0f, 2.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 0.0f, 1.0f};
    glm_lookat(eye, center, up, view);
    glm_mat4_copy(view, ubo.view);
    
    // Create projection matrix
    mat4 proj;
    glm_perspective(glm_rad(45.0f), 
                   state->renderer.image_extent.width / (float)state->renderer.image_extent.height,
                   0.1f, 10.0f, proj);
    proj[1][1] *= -1; // Flip Y axis for Vulkan (GLM doesn't do this automatically in cglm)
    glm_mat4_copy(proj, ubo.proj);
    
    // Copy to mapped uniform buffer
    memcpy(state->uniform_buffers_mapped[current_image], &ubo, sizeof(ubo));
}
