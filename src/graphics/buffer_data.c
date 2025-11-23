#include <cglm/cglm.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "buffer_data.h"
#include "descriptors.h"
#include "internal_types.h"
#include "kuta_internal.h"
#include "texture_data.h"
#include "utils.h"

VkVertexInputBindingDescription get_binding_description() {
  VkVertexInputBindingDescription binding_description = {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  return binding_description;
}
AttributeDescriptions get_attribute_descriptions(void) {
  AttributeDescriptions descs = {0};

  // Position (location 0)
  descs.items[0].binding = 0;
  descs.items[0].location = 0;
  descs.items[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  descs.items[0].offset = offsetof(Vertex, pos);

  // Normal (location 1) - ADD THIS!
  descs.items[1].binding = 0;
  descs.items[1].location = 1;
  descs.items[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  descs.items[1].offset = offsetof(Vertex, normal);

  // TexCoord (location 2) - Changed from location 1!
  descs.items[2].binding = 0;
  descs.items[2].location = 2;
  descs.items[2].format = VK_FORMAT_R32G32_SFLOAT;
  descs.items[2].offset = offsetof(Vertex, tex_coord);

  descs.count = 3; // Changed from 2 to 3

  return descs;
}

uint32_t find_memory_type(uint32_t type_filter,
                          VkMemoryPropertyFlags properties, State *state) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(state->vk_core.physical_device,
                                      &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }

  return 0;
}

void alloc_buffer(VkBuffer *buffer, VkDeviceMemory *buffer_memory,
                  VkMemoryPropertyFlags properties, State *state) {
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(state->vk_core.device, *(buffer),
                                &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex =
          find_memory_type(mem_requirements.memoryTypeBits, properties, state),
  };

  EXPECT(vkAllocateMemory(state->vk_core.device, &alloc_info,
                          state->vk_core.allocator, buffer_memory),
         "Failed to allocate for vertex_buffer_memory")
  vkBindBufferMemory(state->vk_core.device, *(buffer), *(buffer_memory), 0);
}

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkBuffer *buffer,
                   VkDeviceMemory *buffer_memory, State *state) {

  VkBufferCreateInfo buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  EXPECT(vkCreateBuffer(state->vk_core.device, &buffer_info,
                        state->vk_core.allocator, buffer),
         "failed to create vertex buffer!")

  alloc_buffer(buffer, buffer_memory, properties, state);
}

void copy_buffer(VkDeviceSize size, VkBuffer src_buffer, VkBuffer dst_buffer,
                 State *state) {
  VkCommandBuffer command_buffer = begin_single_time_commands(state);

  VkBufferCopy copy_region = {.srcOffset = 0, .dstOffset = 0, .size = size};
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  end_single_time_commands(command_buffer, state);
}

void create_vertex_buffer(State *state, BufferData *buffer_data,
                          Vertex *vertices, size_t vertex_count,
                          VkBuffer *vertex_buffer,
                          VkDeviceMemory *vertex_memory) {
  VkDeviceSize buffer_size = sizeof(Vertex) * vertex_count;

  create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &buffer_data->staging_buffer,
                &buffer_data->staging_buffer_memory, state);

  void *data;
  vkMapMemory(state->vk_core.device, buffer_data->staging_buffer_memory, 0,
              buffer_size, 0, &data);
  memcpy(data, vertices, buffer_size);
  vkUnmapMemory(state->vk_core.device, buffer_data->staging_buffer_memory);

  create_buffer(
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_memory, state);

  copy_buffer(buffer_size, buffer_data->staging_buffer, *vertex_buffer, state);

  vkDestroyBuffer(state->vk_core.device, buffer_data->staging_buffer,
                  state->vk_core.allocator);
  vkFreeMemory(state->vk_core.device, buffer_data->staging_buffer_memory,
               state->vk_core.allocator);
}

void create_index_buffer(State *state, BufferData *buffer_data,
                         uint32_t *indices, size_t indices_count,
                         VkBuffer *index_buffer, VkDeviceMemory *index_memory) {
  VkDeviceSize buffer_size = sizeof(indices[0]) * indices_count;

  create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &buffer_data->staging_buffer,
                &buffer_data->staging_buffer_memory, state);

  void *data;
  vkMapMemory(state->vk_core.device, buffer_data->staging_buffer_memory, 0,
              buffer_size, 0, &data);
  memcpy(data, indices, (size_t)buffer_size);
  vkUnmapMemory(state->vk_core.device, buffer_data->staging_buffer_memory);

  create_buffer(
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_memory, state);

  copy_buffer(buffer_size, buffer_data->staging_buffer, *index_buffer, state);

  vkDestroyBuffer(state->vk_core.device, buffer_data->staging_buffer,
                  state->vk_core.allocator);
  vkFreeMemory(state->vk_core.device, buffer_data->staging_buffer_memory,
               state->vk_core.allocator);
}

void create_uniform_buffers(State *state, BufferData *buffer_data) {
  VkDeviceSize buffer_size = sizeof(UBO);

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &buffer_data->uniform_buffers[i],
                  &buffer_data->uniform_buffers_memory[i], state);

    vkMapMemory(state->vk_core.device, buffer_data->uniform_buffers_memory[i],
                0, buffer_size, 0, &buffer_data->uniform_buffers_mapped[i]);
  }
}
void destroy_uniform_buffers(BufferData *buffer_data, State *state) {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(state->vk_core.device, buffer_data->uniform_buffers[i],
                    state->vk_core.allocator);
    vkFreeMemory(state->vk_core.device, buffer_data->uniform_buffers_memory[i],
                 state->vk_core.allocator);
  }
}

void create_lighting_buffers(State *state) {
  VkDeviceSize buffer_size = sizeof(LightingUBO);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &state->renderer.lighting_buffers[i],
                  &state->renderer.lighting_memory[i], state);
  }
}

void destroy_lighting_buffers(State *state) {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (state->renderer.lighting_buffers[i] != VK_NULL_HANDLE) {
      vkDestroyBuffer(state->vk_core.device,
                      state->renderer.lighting_buffers[i],
                      state->vk_core.allocator);
    }
    if (state->renderer.lighting_memory[i] != VK_NULL_HANDLE) {
      vkFreeMemory(state->vk_core.device, state->renderer.lighting_memory[i],
                   state->vk_core.allocator);
    }
  }
}

void update_camera_uniform_buffer(World *world, BufferData *buffer_data,
                                  State *state, uint32_t current_image) {
  CameraComponent *camera = get_active_camera(world);
  if (!camera)
    return;

  typedef struct {
    mat4 view;
    mat4 proj;
  } CameraUBO;

  CameraUBO ubo;
  glm_mat4_copy(camera->view, ubo.view);
  glm_mat4_copy(camera->projection, ubo.proj);

  void *data;
  vkMapMemory(state->vk_core.device,
              buffer_data->uniform_buffers_memory[current_image], 0,
              sizeof(CameraUBO), 0, &data);
  memcpy(data, &ubo, sizeof(CameraUBO));
  vkUnmapMemory(state->vk_core.device,
                buffer_data->uniform_buffers_memory[current_image]);
}

void update_lighting_uniform_buffer(World *world, State *state,
                                    uint32_t current_image) {
  LightingUBO lighting_ubo;
  lighting_system_gather(world, &lighting_ubo);

  void *data;
  vkMapMemory(state->vk_core.device,
              state->renderer.lighting_memory[current_image], 0,
              sizeof(LightingUBO), 0, &data);
  memcpy(data, &lighting_ubo, sizeof(LightingUBO));
  vkUnmapMemory(state->vk_core.device,
                state->renderer.lighting_memory[current_image]);
}

VkFormat find_supported_format(VkFormat *candidates, size_t candidate_count,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features, State *state) {
  for (size_t i = 0; i < candidate_count; i++) {
    VkFormat format = candidates[i];
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(state->vk_core.physical_device, format,
                                        &props);

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
  VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                           VK_FORMAT_D24_UNORM_S8_UINT};
  return find_supported_format(
      candidates, sizeof(candidates) / sizeof(candidates[0]),
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
      state);
}

bool has_stencil_component(VkFormat format) {
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
         format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void create_depth_resources(State *state, uint32_t mip_levels) {
  VkFormat depth_format = find_depth_format(state);

  create_image(
      state->swp_ch.extent.width, state->swp_ch.extent.height, depth_format,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &state->renderer.depth_image,
      &state->renderer.depth_image_memory, mip_levels,
      state->renderer.msaa_samples, state);

  state->renderer.depth_image_view =
      create_image_view(state->renderer.depth_image, depth_format,
                        VK_IMAGE_ASPECT_DEPTH_BIT, mip_levels, state);
  transition_image_layout(
      state->renderer.depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, mip_levels, state);
}
