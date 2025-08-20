#include <stdbool.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include "window.h"
#include "main.h"

void create_window(State *state) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if(state->window_fullscreen) {
        state->windowMonitor = glfwGetPrimaryMonitor();

        const GLFWvidmode *mode = glfwGetVideoMode(state->windowMonitor);
        state->window_width = mode->width;
        state->window_height = mode->height;
    }

    state->window = glfwCreateWindow(state->window_width, state->window_height, state->window_title, NULL, NULL);
}
