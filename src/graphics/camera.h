#pragma once
#include "GLFW/glfw3.h"
#include "internal_types.h"

void process_input(State *state, float deltaTime);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods);

void camera_process_mouse_movement(Camera *camera, float xoffset,
                                   float yoffset);

void camera_process_keyboard(Camera *camera, int direction, float deltaTime);

void camera_init(Camera *camera);

void camera_get_view_matrix(Camera *camera, mat4 view);

void camera_update_vectors(Camera *camera);
