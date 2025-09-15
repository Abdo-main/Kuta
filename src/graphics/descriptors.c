#include <cglm/cglm.h>
#include <cglm/types.h>
#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "descriptors.h"
#include "utils.h"

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

  VkDescriptorSetLayoutBinding bindings[2] = {ubo_layout_binding,
                                              sampler_layout_binding};

  VkDescriptorSetLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = sizeof(bindings) / sizeof(bindings[0]),
      .pBindings = bindings,
  };

  EXPECT(vkCreateDescriptorSetLayout(state->vk_core.device, &layout_info,
                                     state->vk_core.allocator,
                                     &state->renderer.descriptor_set_layout),
         "Failed to create descriptor set layout!")
}

void destroy_descriptor_set_layout(State *state) {
  vkDestroyDescriptorSetLayout(state->vk_core.device,
                               state->renderer.descriptor_set_layout,
                               state->vk_core.allocator);
}

void create_descriptor_pool(State *state, ResourceManager *rm) {
  uint32_t total_sets = MAX_FRAMES_IN_FLIGHT * rm->geometry_count;

  VkDescriptorPoolSize pool_sizes[2] = {0};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = total_sets;
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = total_sets;

  VkDescriptorPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]),
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .pPoolSizes = pool_sizes,
      .maxSets = total_sets,
  };
  EXPECT(vkCreateDescriptorPool(state->vk_core.device, &pool_info,
                                state->vk_core.allocator,
                                &state->renderer.descriptor_pool),
         "Failed to create descriptor pool!")
}

void create_descriptor_sets(BufferData *buffer_data, ResourceManager *rm,
                            State *state) {

  size_t total_sets = MAX_FRAMES_IN_FLIGHT * rm->geometry_count;

  VkDescriptorSetLayout *layouts =
      malloc(sizeof(VkDescriptorSetLayout) * total_sets);
  for (size_t i = 0; i < total_sets; i++) {
    layouts[i] = state->renderer.descriptor_set_layout;
  }

  VkDescriptorSetAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = state->renderer.descriptor_pool,
      .descriptorSetCount = total_sets,
      .pSetLayouts = layouts,
  };

  state->renderer.descriptor_sets =
      malloc(total_sets * sizeof(VkDescriptorSet));
  state->renderer.descriptor_set_count = total_sets;

  EXPECT(vkAllocateDescriptorSets(state->vk_core.device, &alloc_info,
                                  state->renderer.descriptor_sets),
         "Failed to allocate descriptor sets");

  for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
    for (size_t model = 0; model < rm->geometry_count; model++) {
      size_t set_index = frame * rm->geometry_count + model;

      VkDescriptorBufferInfo buffer_info = {
          .buffer = buffer_data->uniform_buffers[frame],
          .offset = 0,
          .range = sizeof(UBO),
      };

      VkDescriptorImageInfo image_info = {
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .imageView = rm->textures[model].texture_image_view,
          .sampler = rm->textures[model].texture_sampler,
      };

      VkWriteDescriptorSet descriptor_writes[2] = {
          {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .dstSet = state->renderer.descriptor_sets[set_index],
              .dstBinding = 0,
              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              .descriptorCount = 1,
              .pBufferInfo = &buffer_info,
          },
          {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .dstSet = state->renderer.descriptor_sets[set_index],
              .dstBinding = 1,
              .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              .descriptorCount = 1,
              .pImageInfo = &image_info,
          }};

      vkUpdateDescriptorSets(state->vk_core.device, 2, descriptor_writes, 0,
                             NULL);
    }
  }

  free(layouts);
}

void destroy_descriptor_sets(State *state) {
  vkDestroyDescriptorPool(state->vk_core.device,
                          state->renderer.descriptor_pool,
                          state->vk_core.allocator);
}
