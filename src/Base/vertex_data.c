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
#include "camera.h"

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

uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, State *state) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(state->vk_core.physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return 0;
}

void alloc_buffer(VkBuffer *buffer, VkDeviceMemory *buffer_memory, VkMemoryPropertyFlags properties, State *state){
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(state->vk_core.device, *(buffer), &mem_requirements);


    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties, state),
    };

    EXPECT(vkAllocateMemory(state->vk_core.device, &alloc_info, state->vk_core.allocator, buffer_memory), "Failed to allocate for vertex_buffer_memory")
    vkBindBufferMemory(state->vk_core.device, *(buffer), *(buffer_memory), 0);
}

void create_buffer(VkDeviceSize size,
       VkBufferUsageFlags usage,
       VkMemoryPropertyFlags properties,
       VkBuffer* buffer, 
       VkDeviceMemory* buffer_memory,
       State *state) {

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    EXPECT(vkCreateBuffer(state->vk_core.device, &buffer_info, state->vk_core.allocator, buffer), "failed to create vertex buffer!")

    alloc_buffer(buffer, buffer_memory, properties, state);
}

void copy_buffer(VkDeviceSize size, VkBuffer src_buffer, VkBuffer dst_buffer, State *state) {
    VkCommandBuffer command_buffer = begin_single_time_commands(state);

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);
    
    end_single_time_commands(command_buffer, state);
}

void create_vertex_buffer(BufferData *buffer_data, Models *models, State *state) {
    size_t vertex_count = models[0].geometry->vertex_count;
    VkDeviceSize buffer_size = sizeof(models[0].geometry->vertices[0]) * vertex_count;

    create_buffer(buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &buffer_data->staging_buffer, &buffer_data->staging_buffer_memory, state);
        
    void* data;
    vkMapMemory(state->vk_core.device, buffer_data->staging_buffer_memory, 0, buffer_size, 0, &data);
       memcpy(data, models[0].geometry->vertices, (size_t)buffer_size);
    vkUnmapMemory(state->vk_core.device, buffer_data->staging_buffer_memory);

    create_buffer(buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &buffer_data->vertex_buffer, &buffer_data->vertex_buffer_memory, state);

    copy_buffer(buffer_size, buffer_data->staging_buffer, buffer_data->vertex_buffer, state);

    vkDestroyBuffer(state->vk_core.device, buffer_data->staging_buffer, state->vk_core.allocator);
    vkFreeMemory(state->vk_core.device, buffer_data->staging_buffer_memory, state->vk_core.allocator);
}

void destroy_vertex_buffer(BufferData *buffer_data, State *state){
    vkDestroyBuffer(state->vk_core.device, buffer_data->vertex_buffer, state->vk_core.allocator);
    vkFreeMemory(state->vk_core.device, buffer_data->vertex_buffer_memory, state->vk_core.allocator);
}

void create_index_buffer(BufferData *buffer_data, Models *models, State *state) {
    size_t indices_count = models[0].geometry->index_count;
    VkDeviceSize buffer_size = sizeof(models[0].geometry->indices[0]) * indices_count;

    create_buffer(buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &buffer_data->staging_buffer, &buffer_data->staging_buffer_memory, state);
        
    void* data;
    vkMapMemory(state->vk_core.device, buffer_data->staging_buffer_memory, 0, buffer_size, 0, &data);
       memcpy(data, models[0].geometry->indices, (size_t)buffer_size);
    vkUnmapMemory(state->vk_core.device, buffer_data->staging_buffer_memory);

    create_buffer(buffer_size, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    &buffer_data->index_buffer, &buffer_data->index_buffer_memory, state);

    copy_buffer(buffer_size, buffer_data->staging_buffer, buffer_data->index_buffer, state);

    vkDestroyBuffer(state->vk_core.device, buffer_data->staging_buffer, state->vk_core.allocator);
    vkFreeMemory(state->vk_core.device, buffer_data->staging_buffer_memory, state->vk_core.allocator);
}

void destroy_index_buffer(BufferData *buffer_data, State *state){
    vkDestroyBuffer(state->vk_core.device, buffer_data->index_buffer, state->vk_core.allocator);
    vkFreeMemory(state->vk_core.device, buffer_data->index_buffer_memory, state->vk_core.allocator);
}

void create_uniform_buffers(State *state, BufferData *buffer_data) {
    VkDeviceSize buffer_size = sizeof(UBO);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(buffer_size, 
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                     &buffer_data->uniform_buffers[i], 
                     &buffer_data->uniform_buffers_memory[i], state);

        vkMapMemory(state->vk_core.device, buffer_data->uniform_buffers_memory[i], 0, buffer_size, 0, 
                   &buffer_data->uniform_buffers_mapped[i]);
    }
}
void destroy_uniform_buffers(BufferData *buffer_data, State *state){
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(state->vk_core.device, buffer_data->uniform_buffers[i], state->vk_core.allocator);
        vkFreeMemory(state->vk_core.device, buffer_data->uniform_buffers_memory[i], state->vk_core.allocator);
    }
}

void update_uniform_buffer(uint32_t current_image, State *state, BufferData *buffer_data) {
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

    // Scale down the model (adjust this value as needed)
    vec3 scale = {0.01f, 0.01f, 0.01f}; // Scale to 10% of original size
    glm_scale(model, scale);
    
    vec3 axis = {0.0f, 1.0f, 0.0f};
    float angle = time * glm_rad(90.0f); // Rotate 90 degrees per second
    glm_rotate(model, angle, axis);
    glm_mat4_copy(model, ubo.model);
    
    mat4 view;
    camera_get_view_matrix(&state->input_state.camera, view);  // Assuming you add camera to your State struct
    glm_mat4_copy(view, ubo.view);   

    // Create projection matrix
    mat4 proj;
    glm_perspective(glm_rad(45.0f), 
                   state->swp_ch.extent.width / (float)state->swp_ch.extent.height,
                   0.1f, 10.0f, proj);
    proj[1][1] *= -1; 
    glm_mat4_copy(proj, ubo.proj);
    
    // Copy to mapped uniform buffer
    memcpy(buffer_data->uniform_buffers_mapped[current_image], &ubo, sizeof(ubo));
}

VkFormat find_supported_format(VkFormat *candidates,
                               size_t candidate_count,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features, State *state) {
    for (size_t i = 0; i < candidate_count; i++) {
        VkFormat format = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(state->vk_core.physical_device, format, &props);

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

VkFormat find_depth_format(State *state) {
    VkFormat candidates[] = {
       VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT 
    };
    return find_supported_format(candidates, sizeof(candidates)/sizeof(candidates[0]), 
                                 VK_IMAGE_TILING_OPTIMAL,
                                 VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, state);
}

bool has_stencil_component(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


void create_depth_resources(State *state) {
    VkFormat depth_format = find_depth_format(state);

    create_image(state->swp_ch.extent.width, state->swp_ch.extent.height, depth_format,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state->renderer.depth_image, &state->renderer.depth_image_memory, state);

    state->renderer.depth_image_view = create_image_view(state->renderer.depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, state);
    transition_image_layout(state->renderer.depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, state);
}

