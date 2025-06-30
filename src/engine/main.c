#include <cglm/cglm.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#define PANIC(ERROR, FORMAT, ...) {if(ERROR) { fprintf(stderr, "%s -> %s -> %i -> Error(%i):\n\t" FORMAT "\n", __FILE_NAME__, __FUNCTION__, __LINE__, ERROR, ##__VA_ARGS__); raise(SIGABRT);}}

typedef struct State {
    const char *window_title;
    int window_width, window_height;
    bool window_resizable;
    GLFWwindow *window;
    uint32_t api_version;

    VkAllocationCallbacks *allocator;
    VkInstance instance;

} State;

void glfwErrorCallback(int error_code, const char *description) {
    PANIC(error_code, "GLFW: %s", description)
}

void exitCallback() {
    glfwTerminate();
}

void setupErrorHandling() {
    glfwSetErrorCallback(glfwErrorCallback);
    atexit(exitCallback);
}

void createWindow(State *state) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, state->window_resizable);

    state->window = glfwCreateWindow(state->window_width, state->window_height, state->window_title, NULL, NULL);
}

void createInstance(State *state) {
    uint32_t required_extensions_count;
    const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extensions_count);

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = state->api_version,
    };
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = required_extensions_count,
        .ppEnabledExtensionNames = required_extensions,
    };
    PANIC(vkCreateInstance(&createInfo, state->allocator, &state->instance), "Failed to Create vulkan Instance");
}

void logInfo() {
    uint32_t instance_api_version;
    vkEnumerateInstanceVersion(&instance_api_version);
    uint32_t api_version_variant = VK_API_VERSION_VARIANT(instance_api_version);
    uint32_t api_version_major = VK_API_VERSION_MAJOR(instance_api_version);
    uint32_t api_version_minor = VK_API_VERSION_MINOR(instance_api_version);
    uint32_t api_version_patch = VK_API_VERSION_PATCH(instance_api_version);

    printf("Vulkan Api %i.%i.%i.%i\n", api_version_variant, api_version_major, api_version_minor, api_version_patch);
    printf("Glfw %s\n", glfwGetVersionString());
}

void init(State *state) {
    setupErrorHandling();
    logInfo();
    createWindow(state);
    createInstance(state);
}

void loop(State *state) {
    while (!glfwWindowShouldClose(state->window)) {
        glfwPollEvents();

    }
}

void cleanup(State *state) {
    glfwDestroyWindow(state->window);
    vkDestroyInstance(state->instance, state->allocator);
    state->window = NULL;
}

int main(void) {
    State state = {
        .window_title = "Hello, Kuta!",
        .window_width = 800,
        .window_height = 600,
        .window_resizable = false,
        .api_version = VK_API_VERSION_1_4,
    };

    init(&state);
    loop(&state);
    cleanup(&state);
    


    return EXIT_SUCCESS;
}

