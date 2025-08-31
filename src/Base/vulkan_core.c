#include "vulkan_core.h"
#include <cglm/cglm.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "utils.h"

void create_instance(State *state, Config *config) {
    uint32_t required_extensions_count; 
    const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extensions_count);
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = config->application_name,
        .pEngineName = config->engine_name,
        .apiVersion = state->vk_core.api_version,
    };
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = required_extensions_count,
        .ppEnabledExtensionNames = required_extensions,
    };
    EXPECT(vkCreateInstance(&createInfo, state->vk_core.allocator, &state->vk_core.instance), "Failed to Create vulkan Instance");
}


void select_physical_device(State *state) {
    uint32_t count;
    EXPECT(vkEnumeratePhysicalDevices(state->vk_core.instance, &count, NULL), "Failed to enumerate physical devices count1")
    EXPECT(count == 0, "Failed to find vulkan supported physical device")

    VkPhysicalDevice devices[count];
    VkResult result = vkEnumeratePhysicalDevices(state->vk_core.instance, &count, devices);
    if (result != VK_INCOMPLETE) {
        EXPECT(result, "Failed to enumerate physical devices")
    }
    state->vk_core.physical_device = devices[0];
}

void create_surface(State *state) {
    EXPECT(glfwCreateWindowSurface(state->vk_core.instance, state->window_data.window, state->vk_core.allocator, &state->vk_core.surface), "Failed to create window surface")
}

void select_queue_family(State *state) {
    state->vk_core.graphics_queue_family = UINT32_MAX;
    uint32_t count;

    vkGetPhysicalDeviceQueueFamilyProperties(state->vk_core.physical_device, &count, NULL);
    VkQueueFamilyProperties *queue_families  = malloc(count * sizeof(VkQueueFamilyProperties));

    EXPECT(queue_families == NULL, "Failed to allocate memmory for queue families")

    vkGetPhysicalDeviceQueueFamilyProperties(state->vk_core.physical_device, &count, queue_families);

    for (uint32_t queue_family_index = 0; queue_family_index < count; ++queue_family_index) {
        VkQueueFamilyProperties properties = queue_families[queue_family_index];
        if(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT && glfwGetPhysicalDevicePresentationSupport(state->vk_core.instance, state->vk_core.physical_device, queue_family_index)){
            state->vk_core.graphics_queue_family = queue_family_index;
            break;
        }
    }
    EXPECT(state->vk_core.graphics_queue_family == UINT32_MAX, "Failed no suitable queue family")

    free(queue_families);
}

void create_device(State *state) {
    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(state->vk_core.physical_device, &supported_features);
    VkPhysicalDeviceFeatures enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
    EXPECT(vkCreateDevice(state->vk_core.physical_device, &(VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &(VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = state->vk_core.graphics_queue_family,
            .queueCount = 1,
            .pQueuePriorities = &(float){1.0}
        }, 
        .queueCreateInfoCount = 1,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &(const char *) {VK_KHR_SWAPCHAIN_EXTENSION_NAME},
        .pEnabledFeatures = &enabledFeatures,
    }, state->vk_core.allocator, &state->vk_core.device) , "failed to create device and queues")
}

void get_queue(State *state) {
    vkGetDeviceQueue(state->vk_core.device, state->vk_core.graphics_queue_family, 0, &state->vk_core.graphics_queue);
}

void init_vk(Config *config, State *state) {
    create_instance(state, config);
    select_physical_device(state);
    create_surface(state);
    select_queue_family(state);
    create_device(state);
    get_queue(state);
}
