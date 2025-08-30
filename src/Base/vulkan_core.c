#include "vulkan_core.h"
#include <cglm/cglm.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "utils.h"

void create_instance(VkCore *vk_core, State *state) {
    uint32_t required_extensions_count; 
    const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extensions_count);
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = state->application_name,
        .pEngineName = state->engine_name,
        .apiVersion = vk_core->api_version,
    };
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = required_extensions_count,
        .ppEnabledExtensionNames = required_extensions,
    };
    EXPECT(vkCreateInstance(&createInfo, vk_core->allocator, &vk_core->instance), "Failed to Create vulkan Instance");
}


void select_physical_device(VkCore *vk_core) {
    uint32_t count;

    EXPECT(vkEnumeratePhysicalDevices(vk_core->instance, &count, NULL), "Failed to enumerate physical devices count1")
    EXPECT(count == 0, "Failed to find vulkan supported physical device")

    VkPhysicalDevice devices[count];
    VkResult result = vkEnumeratePhysicalDevices(vk_core->instance, &count, devices);
    if (result != VK_INCOMPLETE) {
        EXPECT(result, "Failed to enumerate physical devices")
    }
    vk_core->physical_device = devices[0];
}

void create_surface(VkCore *vk_core, State *state) {
    EXPECT(glfwCreateWindowSurface(vk_core->instance, state->window, vk_core->allocator, &vk_core->surface), "Failed to create window surface")
}

void select_queue_family(VkCore *vk_core) {
    vk_core->graphics_queue_family = UINT32_MAX;
    uint32_t count;

    vkGetPhysicalDeviceQueueFamilyProperties(vk_core->physical_device, &count, NULL);
    VkQueueFamilyProperties *queue_families  = malloc(count * sizeof(VkQueueFamilyProperties));

    EXPECT(queue_families == NULL, "Failed to allocate memmory for queue families")

    vkGetPhysicalDeviceQueueFamilyProperties(vk_core->physical_device, &count, queue_families);

    for (uint32_t queue_family_index = 0; queue_family_index < count; ++queue_family_index) {
        VkQueueFamilyProperties properties = queue_families[queue_family_index];
        if(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT && glfwGetPhysicalDevicePresentationSupport(vk_core->instance, vk_core->physical_device, queue_family_index)){
            vk_core->graphics_queue_family = queue_family_index;
            break;
        }
    }
    EXPECT(vk_core->graphics_queue_family == UINT32_MAX, "Failed no suitable queue family")

    free(queue_families);
}

void create_device(VkCore *vk_core) {
    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(vk_core->physical_device, &supported_features);
    VkPhysicalDeviceFeatures enabledFeatures = {
        .samplerAnisotropy = VK_TRUE,
    };
    EXPECT(vkCreateDevice(vk_core->physical_device, &(VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &(VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = vk_core->graphics_queue_family,
            .queueCount = 1,
            .pQueuePriorities = &(float){1.0}
        }, 
        .queueCreateInfoCount = 1,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &(const char *) {VK_KHR_SWAPCHAIN_EXTENSION_NAME},
        .pEnabledFeatures = &enabledFeatures,
    }, vk_core->allocator, &vk_core->device) , "failed to create device and queues")
}

void get_queue(VkCore *vk_core) {
    vkGetDeviceQueue(vk_core->device, vk_core->graphics_queue_family, 0, &vk_core->graphics_queue);
}

void init_vk(VkCore *vk_core, State *state) {
    create_instance(vk_core, state);
    select_physical_device(vk_core);
    create_surface(vk_core, state);
    select_queue_family(vk_core);
    create_device(vk_core);
    get_queue(vk_core);
}
