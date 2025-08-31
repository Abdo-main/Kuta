#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdbool.h>

#include "utils.h"
#include "main.h"

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
    if(value < min) {
        return min;
    } else if(value > max) {
        return max;
    }
    return value;
}

VkCommandBuffer begin_single_time_commands(State *state) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->renderer.command_pool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(state->vk_core.device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(command_buffer, &begin_info);
    return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer, State *state) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };

    vkQueueSubmit(state->vk_core.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->vk_core.graphics_queue);

    vkFreeCommandBuffers(state->vk_core.device, state->renderer.command_pool, 1, &command_buffer);
}

VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_Flags, State *state) {
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = aspect_Flags,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };

    VkImageView image_view;
    EXPECT(vkCreateImageView(state->vk_core.device, &view_info, state->vk_core.allocator, &image_view), "Failed to create texture image view")

    return image_view;
}

const uint32_t* read_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }

    // Go to end to get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if (file_size <= 0 || file_size % 4 != 0) {
        fprintf(stderr, "Invalid SPIR-V file size: %ld\n", file_size);
        fclose(file);
        return NULL;
    }

    // Allocate memory for the file content
    uint32_t* buffer = malloc(file_size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for shader\n");
        fclose(file);
        return NULL;
    }

    // Read file into buffer
    size_t read_size = fread(buffer, 1, file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        fprintf(stderr, "Failed to read entire file: %s\n", filename);
        free(buffer);
        return NULL;
    }

    *size = (size_t)file_size;
    return buffer;
}

void glfw_error_callback(int error_code, const char *description) {
    EXPECT(error_code, "GLFW: %s", description)
}

void exit_callback() {
    glfwTerminate();
}

void setup_error_handling() {
    glfwSetErrorCallback(glfw_error_callback);
    atexit(exit_callback);
}

void log_info() {
    uint32_t instance_api_version;
    EXPECT(vkEnumerateInstanceVersion(&instance_api_version), "Failed to enumerate vulkan instance version")
    uint32_t api_version_variant = VK_API_VERSION_VARIANT(instance_api_version);
    uint32_t api_version_major = VK_API_VERSION_MAJOR(instance_api_version);
    uint32_t api_version_minor = VK_API_VERSION_MINOR(instance_api_version);
    uint32_t api_version_patch = VK_API_VERSION_PATCH(instance_api_version);

    printf("Vulkan Api %i.%i.%i.%i\n", api_version_variant, api_version_major, api_version_minor, api_version_patch);
    printf("Glfw %s\n", glfwGetVersionString());
}
