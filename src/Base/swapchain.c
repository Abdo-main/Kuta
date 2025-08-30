#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#include <stdlib.h>

#include "swapchain.h"
#include "main.h"
#include "renderer.h"
#include "utils.h"
#include "vertex_data.h"


void acquire_next_swapchain_image(State *state, VkCore *vk_core, SwapchainData *swp_ch) {
    Renderer* renderer = &state->renderer;
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        vk_core->device,
        swp_ch->swapchain, UINT64_MAX, renderer->acquired_image_semaphore[state->current_frame],
        VK_NULL_HANDLE,
        &image_index
    );
    swp_ch->acquired_image_index = image_index;

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain(state, vk_core, swp_ch);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        EXPECT(2, "failed to acquire swap chain image!");
    }
}

void present_swapchain_image(State *state, VkCore *vk_core, SwapchainData *swp_ch) {
    uint32_t image_index = swp_ch->acquired_image_index;
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &swp_ch->swapchain,
        .pImageIndices = &swp_ch->acquired_image_index,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &state->renderer.finished_render_semaphore[image_index],
    };
    
    VkResult result = vkQueuePresentKHR(vk_core->graphics_queue, &present_info);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swapchain(state, vk_core, swp_ch);
    } else if (result != VK_SUCCESS) {
        EXPECT(2, "Couldn't present swapchain image %i", swp_ch->acquired_image_index);
    }
}

void cleanup_swapchain(State *state, VkCore *vk_core, SwapchainData *swp_ch) {
    vkDestroyImageView(vk_core->device, state->depth_image_view, vk_core->allocator);
    vkDestroyImage(vk_core->device, state->depth_image, vk_core->allocator);
    vkFreeMemory(vk_core->device, state->depth_image_memmory, vk_core->allocator);
    if (swp_ch->image_views) {
        for (uint32_t i = 0; i < swp_ch->image_count; i++) {
            vkDestroyImageView(vk_core->device, swp_ch->image_views[i], vk_core->allocator);
        }
        free(swp_ch->image_views);
        free(swp_ch->images);
        swp_ch->image_views = NULL;
        swp_ch->images = NULL;
    }
    
    if (swp_ch->swapchain) {
        vkDestroySwapchainKHR(vk_core->device, swp_ch->swapchain, vk_core->allocator);
        swp_ch->swapchain = VK_NULL_HANDLE;
    }
}


VkSurfaceCapabilitiesKHR get_capabilities(VkPhysicalDevice *physical_device, VkSurfaceKHR *surface){
    VkSurfaceCapabilitiesKHR capabilities;
    EXPECT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*(physical_device), *(surface), &capabilities), "Failed to get surface capabilities")
    return capabilities;
}

VkSurfaceFormatKHR get_formats(VkPhysicalDevice *physical_device, VkSurfaceKHR *surface){
    // Get the number of available surface formats
    uint32_t format_count;
    EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(*(physical_device), *(surface), &format_count, NULL), "Failed to get the number of surface formats")

    // Allocate memory for the formats and query them
    VkSurfaceFormatKHR *formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
    EXPECT(!formats, "Failed to allocate memmory for formats") 
    EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(*(physical_device), *(surface), &format_count, formats), "Failed to get surface formats")

    // Select the best surface format 
    uint32_t format_index = 0;
    for (uint32_t i = 0; i < format_count; i++) {
        VkSurfaceFormatKHR format = formats[i];
        if(format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8_SRGB) {
            format_index = i;
            break;
        }
    }
    VkSurfaceFormatKHR format = formats[format_index];
    return format;
}

VkPresentModeKHR select_present_mode(VkPhysicalDevice *physical_device, VkSurfaceKHR *surface){
    // Default to FIFO present mode
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t present_mode_count;

    // Query the number of present modes
    EXPECT(vkGetPhysicalDeviceSurfacePresentModesKHR(*(physical_device), *(surface), &present_mode_count, NULL), "Failed to get present mode count")

    // Allocate and get supported present modes
    VkPresentModeKHR *present_modes = malloc(present_mode_count * sizeof(VkPresentModeKHR));
    EXPECT(!present_modes,"Failed to allocate memmory for present modes")
    EXPECT(vkGetPhysicalDeviceSurfacePresentModesKHR(*(physical_device), *(surface), &present_mode_count, present_modes), "Failed to get present modes");

    // Try to find MAILBOX present mode for lower latency if available
    uint32_t present_mode_index = UINT32_MAX;
    for (uint32_t i = 0; i < present_mode_count; i++) {
        VkPresentModeKHR present = present_modes[i];
        if(present == VK_PRESENT_MODE_MAILBOX_KHR){
            present_mode_index = i;
            break;
        }
    }
    // Use MAILBOX if available, otherwise use FIFO
    if (present_mode_index != UINT32_MAX) {
        present_mode = present_modes[present_mode_index];
    }

    return present_mode;
}

VkExtent2D choose_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities) {
    // Choose the swapchain extent (resolution)
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        // Surface has a fixed size (e.g. windowed system)
        extent = capabilities.currentExtent;
    } else {
        // Surface size can vary, so clamp to allowed extents
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        extent.width = clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    return extent;
}

void get_swapchain_images_and_create_image_views(State *state, VkSurfaceFormatKHR format, VkCore *vk_core, SwapchainData *swp_ch) {
    // Query the number of images in the new swapchain
    EXPECT(vkGetSwapchainImagesKHR(vk_core->device, swp_ch->swapchain, &swp_ch->image_count, NULL), "Failed to get swap chain images count")
    // Allocate based on the number of images in the new swapchain
    swp_ch->images = malloc(swp_ch->image_count * sizeof(VkImage));
    EXPECT(!swp_ch->images, "Failed to allocate memmory for swap chain images")
    EXPECT(vkGetSwapchainImagesKHR(vk_core->device, swp_ch->swapchain, &swp_ch->image_count, swp_ch->images), "Failed to get swap chain images")

    // Allocate array for image views
    swp_ch->image_views = malloc(swp_ch->image_count * sizeof(VkImageView));
    EXPECT(!swp_ch->image_views, "Failed to allocate memmory for swap chain image views")


    // Create an image view for each swapchain image
    for (uint32_t i = 0; i < swp_ch->image_count; i++) {
        EXPECT(vkCreateImageView(vk_core->device, &(VkImageViewCreateInfo){
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format = format.format,
            .image = swp_ch->images[i],
            .components = (VkComponentMapping) {},
            .subresourceRange = (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1,
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
        }, vk_core->allocator, &swp_ch->image_views[i]), "Failed to create Image View %i", i)
    }
}

void create_swapchain(State *state, VkCore *vk_core, SwapchainData *swp_ch) {
    // Query the surface capabilities for the current physical device and surface
    VkSurfaceCapabilitiesKHR capabilities = get_capabilities(&vk_core->physical_device, &vk_core->surface);
    
    //Get format
    VkSurfaceFormatKHR format = get_formats(&vk_core->physical_device, &vk_core->surface);
    // select_present_mode
    VkPresentModeKHR present_mode = select_present_mode(&vk_core->physical_device, &vk_core->surface);

    swp_ch->image_format = format.format;
    // choose_extent
    swp_ch->extent = choose_extent(state->window, capabilities);

    cleanup_swapchain(state, vk_core, swp_ch);

    // Create the swapchain with the chosen parameters
    EXPECT(vkCreateSwapchainKHR(vk_core->device, &(VkSwapchainCreateInfoKHR){
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_core->surface,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &vk_core->graphics_queue_family,
        .clipped = true,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .oldSwapchain = swp_ch->swapchain,
        .preTransform = capabilities.currentTransform,
        .imageExtent = swp_ch->extent,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .presentMode = present_mode,
        .minImageCount = clamp(3, capabilities.minImageCount, capabilities.maxImageCount ? capabilities.maxImageCount : UINT32_MAX),
    }, vk_core->allocator, &swp_ch->swapchain), "Failed to create swap chain")

   get_swapchain_images_and_create_image_views(state, format, vk_core, swp_ch);
}

void recreate_swapchain(State *state, VkCore *vk_core, SwapchainData *swp_ch) {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(state->window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vk_core->device);

    destroy_frame_buffers(state, vk_core, swp_ch);
    cleanup_swapchain(state, vk_core, swp_ch);

    create_swapchain(state, vk_core, swp_ch);
    create_depth_resources(state, vk_core, swp_ch);
    create_frame_buffers(state, vk_core, swp_ch);
}
