#include <cglm/cglm.h>
#include <cglm/types.h>
#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "descriptors.h"
#include "utils.h"

void create_descriptor_set_layout(State *state) {
  // Binding 0: Camera UBO (Vertex Shader)
  VkDescriptorSetLayoutBinding ubo_layout_binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .pImmutableSamplers = NULL,
  };

  // Binding 1: Texture Sampler (Fragment Shader)
  VkDescriptorSetLayoutBinding sampler_layout_binding = {
      .binding = 1,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = NULL,
  };

  // Binding 2: Lighting UBO (Fragment Shader)
  VkDescriptorSetLayoutBinding lighting_layout_binding = {
      .binding = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      .pImmutableSamplers = NULL,
  };

  VkDescriptorSetLayoutBinding bindings[3] = {
      ubo_layout_binding, sampler_layout_binding, lighting_layout_binding};

  VkDescriptorSetLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 3,
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

  VkDescriptorPoolSize pool_sizes[3] = {0}; // Changed from 2 to 3

  // Camera UBOs
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = total_sets;

  // Texture Samplers
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = total_sets;

  // Lighting UBOs - NEW!
  pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[2].descriptorCount = total_sets;

  VkDescriptorPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = 3, // Changed from 2 to 3
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

      // Camera UBO info (binding 0)
      VkDescriptorBufferInfo buffer_info = {
          .buffer = buffer_data->uniform_buffers[frame],
          .offset = 0,
          .range = sizeof(UBO),
      };

      // Texture info (binding 1)
      VkDescriptorImageInfo image_info = {
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .imageView = rm->textures[model].texture_image_view,
          .sampler = rm->textures[model].texture_sampler,
      };

      // Lighting UBO info (binding 2) - NEW!
      VkDescriptorBufferInfo lighting_buffer_info = {
          .buffer = state->renderer.lighting_buffers[frame],
          .offset = 0,
          .range = sizeof(LightingUBO),
      };

      VkWriteDescriptorSet descriptor_writes[3] = {
          // Changed from 2 to 3
          // Binding 0: Camera UBO
          {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .dstSet = state->renderer.descriptor_sets[set_index],
              .dstBinding = 0,
              .dstArrayElement = 0,
              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              .descriptorCount = 1,
              .pBufferInfo = &buffer_info,
          },
          // Binding 1: Texture Sampler
          {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .dstSet = state->renderer.descriptor_sets[set_index],
              .dstBinding = 1,
              .dstArrayElement = 0,
              .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              .descriptorCount = 1,
              .pImageInfo = &image_info,
          },
          // Binding 2: Lighting UBO - NEW!
          {
              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
              .dstSet = state->renderer.descriptor_sets[set_index],
              .dstBinding = 2,
              .dstArrayElement = 0,
              .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              .descriptorCount = 1,
              .pBufferInfo = &lighting_buffer_info,
          }};

      vkUpdateDescriptorSets(state->vk_core.device, 3, descriptor_writes, 0,
                             NULL); // Changed from 2 to 3
    }
  }

  free(layouts);
}

void destroy_descriptor_sets(State *state) {
  vkDestroyDescriptorPool(state->vk_core.device,
                          state->renderer.descriptor_pool,
                          state->vk_core.allocator);
}
