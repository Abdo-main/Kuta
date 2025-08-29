#include <stdbool.h>
#include "utils.h"
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "utils.h"
#include "vertex_data.h"

void create_image(State *state, uint32_t width, uint32_t height,
                  VkFormat format, VkImageTiling tiling,
                  VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties
                  ) {

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = (uint32_t)width,
        .extent.height = (uint32_t) height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = format,
        .tiling = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = VK_SAMPLE_COUNT_1_BIT,
    };

    EXPECT(vkCreateImage(state->device, &image_info, state->allocator, &state->texture_image), "Failed to create image")

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(state->device, state->texture_image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(state, mem_requirements.memoryTypeBits, properties),
    };

    EXPECT(vkAllocateMemory(state->device, &alloc_info, state->allocator, &state->texture_image_memmory), "Failed to allocate memmory for image")
    vkBindImageMemory(state->device, state->texture_image, state->texture_image_memmory, 0);
}

void create_texture_image(State *state) {
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memmory;

    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load("textures/twitch.jpg", &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
    VkDeviceSize image_size = tex_width * tex_height * 4;

    EXPECT(!pixels, "Failed to load texture image!")

    create_buffer(state, image_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer, &staging_buffer_memmory);

    void* data;
    vkMapMemory(state->device, staging_buffer_memmory, 0, image_size, 0, &data);
        memcpy(data, pixels, (size_t)image_size);
    vkUnmapMemory(state->device, staging_buffer_memmory);

    stbi_image_free(pixels);

    create_image(state, tex_width, tex_height, 
                 VK_FORMAT_R8G8B8A8_SRGB,
                 VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void transition_image_layout(VkImage image, VkFormat format, State *state, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBuffer command_buffer = begin_single_time_commands(state);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .srcAccessMask = 0, // TODO
        .dstAccessMask = 0, // TODO
    };

    vkCmdPipelineBarrier(
        command_buffer,
        0 /* TODO */, 0 /* TODO */,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
    

    end_single_time_commands(state, command_buffer);
}



