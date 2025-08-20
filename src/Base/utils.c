#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "utils.h"

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max) {
    if(value < min) {
        return min;
    } else if(value > max) {
        return max;
    }
    return value;
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
