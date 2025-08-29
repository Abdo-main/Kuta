#pragma once

#include "stdio.h"
#include "stdint.h"
#include <signal.h>
#include "main.h"

#define EXPECT(EXPR, FORMAT, ...) {                                                                                                 \
    int errCode = (EXPR);                                                                                                           \
    if (errCode) {                                                                                                                  \
        fprintf(stderr, "%s -> %s -> %i -> Error(%i):\n\t" FORMAT "\n", __FILE_NAME__, __func__, __LINE__, errCode, ##__VA_ARGS__); \
        raise(SIGABRT);                                                                                                             \
    }                                                                                                                               \
}

void setup_error_handling();

void log_info();

const uint32_t* read_file(const char* filename, size_t* size);

VkImageView create_image_view(State *state, VkImage image, VkFormat format);
uint32_t clamp(uint32_t value, uint32_t min, uint32_t max);

VkCommandBuffer begin_single_time_commands(State *state);
void end_single_time_commands(State *state, VkCommandBuffer command_buffer);
