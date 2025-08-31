#include <stdbool.h>
#include <GLFW/glfw3.h>

#include "window.h"
#include "main.h"

void create_window(WindowData *window_data) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if(window_data->fullscreen) {
        window_data->monitor = glfwGetPrimaryMonitor();

        const GLFWvidmode *mode = glfwGetVideoMode(window_data->monitor);
        window_data->width = mode->width;
        window_data->height = mode->height;
    }

    window_data->window = glfwCreateWindow(window_data->width, window_data->height, window_data->title, NULL, NULL);
}
