#include <cglm/cglm.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <cglm/types.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "main.h"
#include "vertex_data.h"
#include "utils.h"

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
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->renderer.command_pool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    EXPECT(vkAllocateCommandBuffers(state->device, &alloc_info, &command_buffer), "failed to allocate command buffer for copying buffers")

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    EXPECT(vkBeginCommandBuffer(command_buffer, &begin_info), "couldn't begin command buffer for copying buffers")

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    vkQueueSubmit(state->queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->queue);

    vkFreeCommandBuffers(state->device, state->renderer.command_pool, 1, &command_buffer);
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
