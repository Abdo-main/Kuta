#include <cglm/cglm.h>
#include <string.h>
#include <cglm/types.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "main.h"
#include "vertex_data.h"
#include "utils.h"

Vertex vertices[3] = {
    { { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
    { { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }
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

void alloc_vertex_buffer(State *state){
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(state->device, state->vertex_buffer, &mem_requirements);


    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(state, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };

    EXPECT(vkAllocateMemory(state->device, &alloc_info, state->allocator, &state->vertex_buffer_memory), "Failed to allocate for vertex_buffer_memory")
    vkBindBufferMemory(state->device, state->vertex_buffer, state->vertex_buffer_memory, 0);
}

void create_vertex_buffer(State *state) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(vertices),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    EXPECT(vkCreateBuffer(state->device, &buffer_info, state->allocator, &state->vertex_buffer), "failed to create vertex buffer!")

    alloc_vertex_buffer(state);
    void* data;
    
    vkMapMemory(state->device, state->vertex_buffer_memory, 0, buffer_info.size, 0, &data);
       memcpy(data, vertices, buffer_info.size);
    vkUnmapMemory(state->device, state->vertex_buffer_memory);
}

void destroy_vertex_buffer(State *state){
    vkDestroyBuffer(state->device, state->vertex_buffer, state->allocator);
    vkFreeMemory(state->device, state->vertex_buffer_memory, state->allocator);
}
