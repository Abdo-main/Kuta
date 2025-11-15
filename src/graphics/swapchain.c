#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "buffer_data.h"
#include "internal_types.h"
#include "kuta_internal.h"
#include "renderer.h"
#include "swapchain.h"
#include "types.h"
#include "utils.h"

void acquire_next_swapchain_image(State *state) {
  uint32_t image_index;
  VkResult result = vkAcquireNextImageKHR(
      state->vk_core.device, state->swp_ch.swapchain, UINT64_MAX,
      state->renderer.acquired_image_semaphore[state->renderer.current_frame],
      VK_NULL_HANDLE, &image_index);
  state->swp_ch.acquired_image_index = image_index;

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swapchain(state, &state->world);
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    EXPECT(2, "failed to acquire swap chain image!");
  }
}

void present_swapchain_image(State *state) {
  uint32_t image_index = state->swp_ch.acquired_image_index;
  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .swapchainCount = 1,
      .pSwapchains = &state->swp_ch.swapchain,
      .pImageIndices = &state->swp_ch.acquired_image_index,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &state->renderer.finished_render_semaphore[image_index],
  };

  VkResult result =
      vkQueuePresentKHR(state->vk_core.graphics_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreate_swapchain(state, &state->world);
  } else if (result != VK_SUCCESS) {
    EXPECT(2, "Couldn't present swapchain image %i",
           state->swp_ch.acquired_image_index);
  }
}

VkSurfaceCapabilitiesKHR get_capabilities(VkPhysicalDevice *physical_device,
                                          VkSurfaceKHR *surface) {
  VkSurfaceCapabilitiesKHR capabilities;
  EXPECT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*(physical_device),
                                                   *(surface), &capabilities),
         "Failed to get surface capabilities")
  return capabilities;
}

VkSurfaceFormatKHR get_formats(VkPhysicalDevice *physical_device,
                               VkSurfaceKHR *surface) {
  // Get the number of available surface formats
  uint32_t format_count;
  EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(*(physical_device), *(surface),
                                              &format_count, NULL),
         "Failed to get the number of surface formats")

  // Allocate memory for the formats and query them
  VkSurfaceFormatKHR *formats =
      malloc(format_count * sizeof(VkSurfaceFormatKHR));
  EXPECT(!formats, "Failed to allocate memmory for formats")
  EXPECT(vkGetPhysicalDeviceSurfaceFormatsKHR(*(physical_device), *(surface),
                                              &format_count, formats),
         "Failed to get surface formats")

  // Select the best surface format
  uint32_t format_index = 0;
  for (uint32_t i = 0; i < format_count; i++) {
    VkSurfaceFormatKHR format = formats[i];
    if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
        format.format == VK_FORMAT_B8G8R8_SRGB) {
      format_index = i;
      break;
    }
  }
  VkSurfaceFormatKHR format = formats[format_index];
  return format;
}

VkPresentModeKHR select_present_mode(VkPhysicalDevice *physical_device,
                                     VkSurfaceKHR *surface) {
  // Default to FIFO present mode
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  uint32_t present_mode_count;

  // Query the number of present modes
  EXPECT(vkGetPhysicalDeviceSurfacePresentModesKHR(
             *(physical_device), *(surface), &present_mode_count, NULL),
         "Failed to get present mode count")

  // Allocate and get supported present modes
  VkPresentModeKHR *present_modes =
      malloc(present_mode_count * sizeof(VkPresentModeKHR));
  EXPECT(!present_modes, "Failed to allocate memmory for present modes")
  EXPECT(
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          *(physical_device), *(surface), &present_mode_count, present_modes),
      "Failed to get present modes");

  // Try to find MAILBOX present mode for lower latency if available
  uint32_t present_mode_index = UINT32_MAX;
  for (uint32_t i = 0; i < present_mode_count; i++) {
    VkPresentModeKHR present = present_modes[i];
    if (present == VK_PRESENT_MODE_MAILBOX_KHR) {
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

VkExtent2D choose_extent(GLFWwindow *window,
                         VkSurfaceCapabilitiesKHR capabilities) {
  // Choose the swapchain extent (resolution)
  VkExtent2D extent;
  if (capabilities.currentExtent.width != UINT32_MAX) {
    // Surface has a fixed size (e.g. windowed system)
    extent = capabilities.currentExtent;
  } else {
    // Surface size can vary, so clamp to allowed extents
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    extent.width = clamp(width, capabilities.minImageExtent.width,
                         capabilities.maxImageExtent.width);
    extent.height = clamp(height, capabilities.minImageExtent.height,
                          capabilities.maxImageExtent.height);
  }
  return extent;
}

void get_swapchain_images_and_create_image_views(VkSurfaceFormatKHR format,
                                                 State *state) {
  // Query the number of images in the new swapchain
  EXPECT(vkGetSwapchainImagesKHR(state->vk_core.device, state->swp_ch.swapchain,
                                 &state->swp_ch.image_count, NULL),
         "Failed to get swap chain images count")
  // Allocate based on the number of images in the new swapchain
  state->swp_ch.images = malloc(state->swp_ch.image_count * sizeof(VkImage));
  EXPECT(!state->swp_ch.images,
         "Failed to allocate memmory for swap chain images")
  EXPECT(vkGetSwapchainImagesKHR(state->vk_core.device, state->swp_ch.swapchain,
                                 &state->swp_ch.image_count,
                                 state->swp_ch.images),
         "Failed to get swap chain images")

  // Allocate array for image views
  state->swp_ch.image_views =
      malloc(state->swp_ch.image_count * sizeof(VkImageView));
  EXPECT(!state->swp_ch.image_views,
         "Failed to allocate memmory for swap chain image views")

  // Create an image view for each swapchain image
  for (uint32_t i = 0; i < state->swp_ch.image_count; i++) {
    EXPECT(vkCreateImageView(
               state->vk_core.device,
               &(VkImageViewCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                   .format = format.format,
                   .image = state->swp_ch.images[i],
                   .components = (VkComponentMapping){},
                   .subresourceRange =
                       (VkImageSubresourceRange){
                           .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .layerCount = 1,
                           .levelCount = 1,
                       },
                   .viewType = VK_IMAGE_VIEW_TYPE_2D,
               },
               state->vk_core.allocator, &state->swp_ch.image_views[i]),
           "Failed to create Image View %i", i)
  }
}

void create_swapchain(State *state) {
  // Query the surface capabilities for the current physical device and surface
  VkSurfaceCapabilitiesKHR capabilities = get_capabilities(
      &state->vk_core.physical_device, &state->vk_core.surface);

  // Get format
  VkSurfaceFormatKHR format =
      get_formats(&state->vk_core.physical_device, &state->vk_core.surface);
  // select_present_mode
  VkPresentModeKHR present_mode = select_present_mode(
      &state->vk_core.physical_device, &state->vk_core.surface);

  state->swp_ch.image_format = format.format;
  // choose_extent
  state->swp_ch.extent = choose_extent(state->window_data.window, capabilities);

  cleanup_swapchain(state);

  // Create the swapchain with the chosen parameters
  EXPECT(vkCreateSwapchainKHR(
             state->vk_core.device,
             &(VkSwapchainCreateInfoKHR){
                 .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                 .surface = state->vk_core.surface,
                 .queueFamilyIndexCount = 1,
                 .pQueueFamilyIndices = &state->vk_core.graphics_queue_family,
                 .clipped = true,
                 .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                 .imageArrayLayers = 1,
                 .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                 .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                 .oldSwapchain = state->swp_ch.swapchain,
                 .preTransform = capabilities.currentTransform,
                 .imageExtent = state->swp_ch.extent,
                 .imageFormat = format.format,
                 .imageColorSpace = format.colorSpace,
                 .presentMode = present_mode,
                 .minImageCount = clamp(3, capabilities.minImageCount,
                                        capabilities.maxImageCount
                                            ? capabilities.maxImageCount
                                            : UINT32_MAX),
             },
             state->vk_core.allocator, &state->swp_ch.swapchain),
         "Failed to create swap chain")

  get_swapchain_images_and_create_image_views(format, state);
}

void recreate_swapchain(State *state, World *world) {
  int width = 0, height = 0;
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(state->window_data.window, &width, &height);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(state->vk_core.device);

  destroy_frame_buffers(state);
  cleanup_swapchain(state);

  create_swapchain(state);
  create_depth_resources(state);
  create_frame_buffers(state);
  camera_dirty(world);
}

void cleanup_swapchain(State *state) {
  if (state->renderer.depth_image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(state->vk_core.device, state->renderer.depth_image_view,
                       state->vk_core.allocator);
    state->renderer.depth_image_view = VK_NULL_HANDLE;
  }
  if (state->renderer.depth_image != VK_NULL_HANDLE) {
    vkDestroyImage(state->vk_core.device, state->renderer.depth_image,
                   state->vk_core.allocator);
    state->renderer.depth_image = VK_NULL_HANDLE;
  }
  if (state->renderer.depth_image_memory != VK_NULL_HANDLE) {
    vkFreeMemory(state->vk_core.device, state->renderer.depth_image_memory,
                 state->vk_core.allocator);
    state->renderer.depth_image_memory = VK_NULL_HANDLE;
  }
  if (state->swp_ch.image_views) {
    for (uint32_t i = 0; i < state->swp_ch.image_count; i++) {
      vkDestroyImageView(state->vk_core.device, state->swp_ch.image_views[i],
                         state->vk_core.allocator);
    }
    free(state->swp_ch.image_views);
    free(state->swp_ch.images);
    state->swp_ch.image_views = NULL;
    state->swp_ch.images = NULL;
  }

  if (state->swp_ch.swapchain) {
    vkDestroySwapchainKHR(state->vk_core.device, state->swp_ch.swapchain,
                          state->vk_core.allocator);
    state->swp_ch.swapchain = VK_NULL_HANDLE;
  }
}
