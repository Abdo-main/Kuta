#include <stdbool.h>
#include "utils.h"
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "utils.h"
#include "vertex_data.h"
#include "textures.h"

void create_image(State *state, uint32_t width, uint32_t height,
                  VkFormat format, VkImageTiling tiling,
                  VkImageUsageFlags usage,
                  VkMemoryPropertyFlags properties,
                  VkImage* image,
                  VkDeviceMemory* image_memory
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

    EXPECT(vkCreateImage(state->device, &image_info, state->allocator, image), "Failed to create image")

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(state->device, *image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = find_memory_type(state, mem_requirements.memoryTypeBits, properties),
    };

    EXPECT(vkAllocateMemory(state->device, &alloc_info, state->allocator, image_memory), "Failed to allocate memmory for image")
    vkBindImageMemory(state->device, *image, *image_memory, 0);
}

void create_texture_image(State *state, char* filename) {
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memmory;

    int tex_width, tex_height, tex_channels;
    stbi_uc* pixels = stbi_load(filename, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
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
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state->texture_image,
                 &state->texture_image_memmory);

    transition_image_layout(state->texture_image, VK_FORMAT_R8G8B8A8_SRGB, state, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(state, staging_buffer, state->texture_image, (uint32_t)tex_width, (uint32_t)tex_height);
    transition_image_layout(state->texture_image, VK_FORMAT_R8G8B8A8_SRGB, state, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    // CLEANUP
    vkDestroyBuffer(state->device, staging_buffer, state->allocator);
    vkFreeMemory(state->device, staging_buffer_memmory, state->allocator);

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
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .srcAccessMask = 0, // TODO
        .dstAccessMask = 0, // TODO
    };
    
    // Set proper aspectMask depending on the layout / format
    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
       printf("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        command_buffer,
        source_stage, destination_stage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
    

    end_single_time_commands(state, command_buffer);
}

void copy_buffer_to_image(State *state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer command_buffer = begin_single_time_commands(state);
    
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            width,
            height,
            1
        },
    };

    vkCmdCopyBufferToImage(
        command_buffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    end_single_time_commands(state, command_buffer);
}

void create_texture_image_view(State *state) {
    state->texture_image_view = create_image_view(state, state->texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void create_texture_sampler(State *state) {
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(state->physical_device, &properties);

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };
    EXPECT(vkCreateSampler(state->device, &sampler_info, state->allocator, &state->texture_sampler), "Failed to create texture sampler")
}


