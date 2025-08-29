#include <cglm/cglm.h>
#include <cglm/types.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "descriptors.h"
#include "main.h"
#include "utils.h"


#define MAX_FRAMES_IN_FLIGHT 2

void create_descriptor_set_layout(State *state) {
    VkDescriptorSetLayoutBinding ubo_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &ubo_layout_binding,
    };

    EXPECT(vkCreateDescriptorSetLayout(state->device, &layout_info, state->allocator, &state->renderer.descriptor_set_layout), "Failed to create descriptor set layout!")

}

void destroy_descriptor_set_layout(State *state){
    vkDestroyDescriptorSetLayout(state->device, state->renderer.descriptor_set_layout, state->allocator);
}

void create_descriptor_pool(State *state) {
    VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
        .maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT,
    };

    EXPECT(vkCreateDescriptorPool(state->device, &pool_info, state->allocator, &state->renderer.descriptor_pool), "Failed to create descriptor pool!")
}

void create_descriptor_sets(State *state) {
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {};
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        layouts[i] = state->renderer.descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = state->renderer.descriptor_pool,
        .descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts,
    };
    state->renderer.descriptor_sets = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkDescriptorSet));
    state->renderer.descriptor_set_count = MAX_FRAMES_IN_FLIGHT;

    EXPECT(vkAllocateDescriptorSets(state->device, &alloc_info, state->renderer.descriptor_sets), "Failed to allocate for descriptor sets")

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo buffer_info = {
            .buffer = state->uniform_buffers[i],
            .offset = 0,
            .range = sizeof(UBO),
        };
        VkWriteDescriptorSet descriptor_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->renderer.descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &buffer_info,
        };
        vkUpdateDescriptorSets(state->device, 1, &descriptor_write, 0, NULL);
    }
    
}

void destroy_descriptor_sets(State *state) {
    vkDestroyDescriptorPool(state->device, state->renderer.descriptor_pool, state->allocator);
}
