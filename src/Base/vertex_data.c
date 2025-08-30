#include <cglm/cglm.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <cglm/cglm.h>        
#include <cglm/affine.h>     
#include <cglm/mat4.h>       
#include <cglm/vec3.h>        
#include <time.h> 
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "main.h"
#include "vertex_data.h"
#include "textures.h"
#include "utils.h"
#include "descriptors.h"
#include "utils.h"

#define MAX_FRAMES_IN_FLIGHT 2


VkVertexInputBindingDescription get_binding_description() {
    VkVertexInputBindingDescription binding_description = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    return binding_description;
}

AttributeDescriptions get_attribute_descriptions(void) {
    AttributeDescriptions descs = {0}; // zero initialize entire struct
    
    descs.items[0].binding = 0;
    descs.items[0].location = 0;
    descs.items[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs.items[0].offset = offsetof(Vertex, pos);

    descs.items[1].binding = 0;
    descs.items[1].location = 1;
    descs.items[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    descs.items[1].offset = offsetof(Vertex, color);

    descs.items[2].binding = 0;
    descs.items[2].location = 2;
    descs.items[2].format = VK_FORMAT_R32G32_SFLOAT;
    descs.items[2].offset = offsetof(Vertex, tex_coord);

    descs.count = 3; // Set the count if you included it
    
    return descs;
}

uint32_t find_memory_type(State *state, uint32_t type_filter, VkMemoryPropertyFlags properties, VkCore *vk_core) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_core->physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return 0;
}

void alloc_buffer(State *state, VkBuffer *buffer, VkDeviceMemory *buffer_memory, VkMemoryPropertyFlags properties, VkCore *vk_core){
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(vk_core->device, *(buffer), &mem_requirements);


    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(state, mem_requirements.memoryTypeBits, properties, vk_core),
    };

    EXPECT(vkAllocateMemory(vk_core->device, &alloc_info, vk_core->allocator, buffer_memory), "Failed to allocate for vertex_buffer_memory")
    vkBindBufferMemory(vk_core->device, *(buffer), *(buffer_memory), 0);
}

void create_buffer(State *state, VkDeviceSize size,
       VkBufferUsageFlags usage,
       VkMemoryPropertyFlags properties,
       VkBuffer* buffer, 
       VkDeviceMemory* buffer_memory,
       VkCore *vk_core) {

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    EXPECT(vkCreateBuffer(vk_core->device, &buffer_info, vk_core->allocator, buffer), "failed to create vertex buffer!")

    alloc_buffer(state, buffer, buffer_memory, properties, vk_core);
}

void copy_buffer(State *state, VkDeviceSize size, VkBuffer src_buffer, VkBuffer dst_buffer, VkCore *vk_core) {
    VkCommandBuffer command_buffer = begin_single_time_commands(state, vk_core);

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
    
    end_single_time_commands(state, command_buffer, vk_core);
}

void create_vertex_buffer(State *state, VkCore *vk_core, BufferData *buffer_data) {
    size_t vertex_count = state->vertex_count;
    VkDeviceSize buffer_size = sizeof(state->vertices[0]) * vertex_count;

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &buffer_data->staging_buffer, &buffer_data->staging_buffer_memory, vk_core);
        
    void* data;
    vkMapMemory(vk_core->device, buffer_data->staging_buffer_memory, 0, buffer_size, 0, &data);
       memcpy(data, state->vertices, (size_t)buffer_size);
    vkUnmapMemory(vk_core->device, buffer_data->staging_buffer_memory);

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &buffer_data->vertex_buffer, &buffer_data->vertex_buffer_memory, vk_core);

    copy_buffer(state, buffer_size, buffer_data->staging_buffer, buffer_data->vertex_buffer, vk_core);

    vkDestroyBuffer(vk_core->device, buffer_data->staging_buffer, vk_core->allocator);
    vkFreeMemory(vk_core->device, buffer_data->staging_buffer_memory, vk_core->allocator);
}

void destroy_vertex_buffer(State *state, VkCore *vk_core, BufferData *buffer_data){
    vkDestroyBuffer(vk_core->device, buffer_data->vertex_buffer, vk_core->allocator);
    vkFreeMemory(vk_core->device, buffer_data->vertex_buffer_memory, vk_core->allocator);
}

void create_index_buffer(State *state, VkCore *vk_core, BufferData *buffer_data) {
    size_t indices_count = state->index_count;
    VkDeviceSize buffer_size = sizeof(state->indices[0]) * indices_count;

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &buffer_data->staging_buffer, &buffer_data->staging_buffer_memory, vk_core);
        
    void* data;
    vkMapMemory(vk_core->device, buffer_data->staging_buffer_memory, 0, buffer_size, 0, &data);
       memcpy(data, state->indices, (size_t)buffer_size);
    vkUnmapMemory(vk_core->device, buffer_data->staging_buffer_memory);

    create_buffer(state, buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &buffer_data->index_buffer, &buffer_data->index_buffer_memory, vk_core);

    copy_buffer(state, buffer_size, buffer_data->staging_buffer, buffer_data->index_buffer, vk_core);

    vkDestroyBuffer(vk_core->device, buffer_data->staging_buffer, vk_core->allocator);
    vkFreeMemory(vk_core->device, buffer_data->staging_buffer_memory, vk_core->allocator);
}

void destroy_index_buffer(State *state, VkCore *vk_core, BufferData *buffer_data){
    vkDestroyBuffer(vk_core->device, buffer_data->index_buffer, vk_core->allocator);
    vkFreeMemory(vk_core->device, buffer_data->index_buffer_memory, vk_core->allocator);
}

void create_uniform_buffers(State *state, VkCore *vk_core, BufferData *buffer_data) {
    VkDeviceSize buffer_size = sizeof(UBO); // Assuming you have a ubo struct

    // In C, we use arrays instead of vectors - make sure your State struct has these arrays:
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(state, buffer_size, 
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                     &buffer_data->uniform_buffers[i], 
                     &buffer_data->uniform_buffers_memory[i], vk_core);

        vkMapMemory(vk_core->device, buffer_data->uniform_buffers_memory[i], 0, buffer_size, 0, 
                   &buffer_data->uniform_buffers_mapped[i]);
    }
}
void destroy_uniform_buffers(State *state, VkCore *vk_core, BufferData *buffer_data){
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(vk_core->device, buffer_data->uniform_buffers[i], vk_core->allocator);
        vkFreeMemory(vk_core->device, buffer_data->uniform_buffers_memory[i], vk_core->allocator);
    }
}

void update_uniform_buffer(uint32_t current_image, SwapchainData *swp_ch, BufferData *buffer_data) {
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

  // Rotate 90 degrees around X-axis
    vec3 ax = {1.0f, 0.0f, 0.0f}; // Rotate around X-axis
    float ang = glm_rad(90.0f);   // -90 degrees to make it stand up
    glm_rotate(model, ang, ax);

    // Scale down the model (adjust this value as needed)
    vec3 scale = {0.01f, 0.01f, 0.01f}; // Scale to 10% of original size
    glm_scale(model, scale);
    
    vec3 axis = {0.0f, 1.0f, 0.0f};
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
                   swp_ch->extent.width / (float)swp_ch->extent.height,
                   0.1f, 10.0f, proj);
    proj[1][1] *= -1; // Flip Y axis for Vulkan (GLM doesn't do this automatically in cglm)
    glm_mat4_copy(proj, ubo.proj);
    
    // Copy to mapped uniform buffer
    memcpy(buffer_data->uniform_buffers_mapped[current_image], &ubo, sizeof(ubo));
}

VkFormat find_supported_format(State *state,
                               VkFormat *candidates,
                               size_t candidate_count,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features, VkCore *vk_core) {
    for (size_t i = 0; i < candidate_count; i++) {
        VkFormat format = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(vk_core->physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat find_depth_format(State *state, VkCore *vk_core) {
    VkFormat candidates[] = {
       VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT 
    };
    return find_supported_format(state, candidates, sizeof(candidates)/sizeof(candidates[0]), 
                                 VK_IMAGE_TILING_OPTIMAL,
                                 VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, vk_core);
}

bool has_stencil_component(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


void create_depth_resources(State *state, VkCore *vk_core, SwapchainData *swp_ch) {
    VkFormat depth_format = find_depth_format(state, vk_core);

    create_image(state, swp_ch->extent.width, swp_ch->extent.height, depth_format,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state->depth_image, &state->depth_image_memmory, vk_core);

    state->depth_image_view = create_image_view(state, state->depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, vk_core);
    transition_image_layout(state->depth_image, depth_format, state, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, vk_core);
}

