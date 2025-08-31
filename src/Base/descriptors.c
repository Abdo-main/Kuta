#include <cglm/cglm.h>
#include <cglm/types.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "descriptors.h"
#include "main.h"
#include "utils.h"


#define MAX_FRAMES_IN_FLIGHT 2

void create_descriptor_set_layout(State *state) {
    VkDescriptorSetLayoutBinding sampler_layout_binding = {
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = NULL,
    };
    VkDescriptorSetLayoutBinding ubo_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutBinding bindings[2] = {ubo_layout_binding, sampler_layout_binding};

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = sizeof(bindings)/sizeof(bindings[0]),
        .pBindings = bindings,
    };

    EXPECT(vkCreateDescriptorSetLayout(state->vk_core.device, &layout_info, state->vk_core.allocator, &state->renderer.descriptor_set_layout), "Failed to create descriptor set layout!")

}

void destroy_descriptor_set_layout(State *state){
    vkDestroyDescriptorSetLayout(state->vk_core.device, state->renderer.descriptor_set_layout, state->vk_core.allocator);
}

void create_descriptor_pool(State *state) {
    VkDescriptorPoolSize pool_sizes[2] = {0};

    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = sizeof(pool_sizes)/sizeof(pool_sizes[0]),
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .pPoolSizes = pool_sizes,
        .maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT,
    };

    EXPECT(vkCreateDescriptorPool(state->vk_core.device, &pool_info, state->vk_core.allocator, &state->renderer.descriptor_pool), "Failed to create descriptor pool!")
}

void create_descriptor_sets(BufferData *buffer_data, TextureData *texture_data, State *state) {
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

    EXPECT(vkAllocateDescriptorSets(state->vk_core.device, &alloc_info, state->renderer.descriptor_sets), "Failed to allocate for descriptor sets")

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo buffer_info = {
            .buffer = buffer_data->uniform_buffers[i],
            .offset = 0,
            .range = sizeof(UBO),
        };
        VkDescriptorImageInfo image_info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = texture_data->texture_image_view,
            .sampler = texture_data->texture_sampler,
        };
        VkWriteDescriptorSet descriptor_writes[2] = {0};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = state->renderer.descriptor_sets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &buffer_info;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = state->renderer.descriptor_sets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pImageInfo = &image_info;
        vkUpdateDescriptorSets(state->vk_core.device, sizeof(descriptor_writes)/sizeof(descriptor_writes[0]), descriptor_writes, 0, NULL);
    }
    
}

void destroy_descriptor_sets(State *state) {
    vkDestroyDescriptorPool(state->vk_core.device, state->renderer.descriptor_pool, state->vk_core.allocator);
}
