#pragma once

#include "internal_types.h"
#include "stdint.h"
#include "stdio.h"
#include <signal.h>

#pragma once

#include "internal_types.h"
#include "stdint.h"
#include "stdio.h"
#include <signal.h>
#include <string.h>

#ifdef _MSC_VER
#define __FILENAME__                                                           \
  (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#elif defined(__GNUC__) || defined(__clang__)
#ifdef __FILE_NAME__
#define __FILENAME__ __FILE_NAME__
#else
#define __FILENAME__ __FILE__
#endif
#else
#define __FILENAME__ __FILE__
#endif

#define EXPECT(EXPR, FORMAT, ...)                                              \
  {                                                                            \
    int errCode = (EXPR);                                                      \
    if (errCode) {                                                             \
      fprintf(stderr, "%s -> %s -> %i -> Error(%i):\n\t" FORMAT "\n",          \
              __FILENAME__, __func__, __LINE__, errCode, ##__VA_ARGS__);       \
      raise(SIGABRT);                                                          \
    }                                                                          \
  }

void initialize(Stack *stack);

bool isEmpty(Stack *stack);

bool isFull(Stack *stack);

void push(Stack *stack, uint32_t value);

uint32_t pop(Stack *stack);

void setup_error_handling();

void log_info();

const uint32_t *read_file(const char *filename, size_t *size);

VkImageView create_image_view(VkImage image, VkFormat format,
                              VkImageAspectFlags aspect_Flags,
                              uint32_t mipLevels, State *state);

uint32_t clamp(uint32_t value, uint32_t min, uint32_t max);

VkCommandBuffer begin_single_time_commands(State *state);

void end_single_time_commands(VkCommandBuffer command_buffer, State *state);
