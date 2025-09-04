#include "main.h"

void camera_update_vectors(Camera* camera) {
    vec3 front;
    front[0] = cos(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    front[1] = sin(glm_rad(camera->pitch));
    front[2] = sin(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    
    glm_vec3_normalize_to(front, camera->front);
    
    glm_vec3_crossn(camera->front, camera->worldUp, camera->right);
    glm_vec3_crossn(camera->right, camera->front, camera->up);
}

void camera_get_view_matrix(Camera* camera, mat4 view) {
    vec3 center;
    glm_vec3_add(camera->position, camera->front, center);
    glm_lookat(camera->position, center, camera->up, view);
}

void camera_init(Camera* camera) {
    glm_vec3_copy((vec3){0.0f, 0.0f, 3.0f}, camera->position);
    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, camera->worldUp);
    camera->yaw = -90.0f;
    camera->pitch = 0.0f;
    camera->movementSpeed = 5.0f;
    camera->mouseSensitivity = 0.1f;
    
    camera_update_vectors(camera);
}

void camera_process_keyboard(Camera* camera, int direction, float deltaTime) {
    float velocity = camera->movementSpeed * deltaTime;
    vec3 temp;
    
    if (direction == 0) { // W - Forward
        glm_vec3_scale(camera->front, velocity, temp);
        glm_vec3_add(camera->position, temp, camera->position);
    }
    if (direction == 1) { // S - Backward
        glm_vec3_scale(camera->front, velocity, temp);
        glm_vec3_sub(camera->position, temp, camera->position);
    }
    if (direction == 2) { // A - Left
        glm_vec3_scale(camera->right, velocity, temp);
        glm_vec3_sub(camera->position, temp, camera->position);
    }
    if (direction == 3) { // D - Right
        glm_vec3_scale(camera->right, velocity, temp);
        glm_vec3_add(camera->position, temp, camera->position);
    }
}

void camera_process_mouse_movement(Camera* camera, float xoffset, float yoffset) {
    xoffset *= camera->mouseSensitivity;
    yoffset *= camera->mouseSensitivity;

    camera->yaw += xoffset;
    camera->pitch += yoffset;

    // Constrain pitch
    if (camera->pitch > 89.0f)
        camera->pitch = 89.0f;
    if (camera->pitch < -89.0f)
        camera->pitch = -89.0f;

    camera_update_vectors(camera);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    State* state = (State*)glfwGetWindowUserPointer(window);
    if (action == GLFW_PRESS)
        state->input_state.keys[key] = true;
    else if (action == GLFW_RELEASE)
        state->input_state.keys[key] = false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    State* state = (State*)glfwGetWindowUserPointer(window);
    if (state->input_state.firstMouse) {
        state->input_state.lastX = xpos;
        state->input_state.lastY = ypos;
        state->input_state.firstMouse = false;
    }
    float xoffset = xpos - state->input_state.lastX;
    float yoffset = state->input_state.lastY - ypos;
    
    state->input_state.lastX = xpos;
    state->input_state.lastY = ypos;
    camera_process_mouse_movement(&state->input_state.camera, xoffset, yoffset);
}

void process_input(State* state, float deltaTime) {
    if (state->input_state.keys[GLFW_KEY_W])
        camera_process_keyboard(&state->input_state.camera, 0, deltaTime);
    if (state->input_state.keys[GLFW_KEY_S])
        camera_process_keyboard(&state->input_state.camera, 1, deltaTime);
    if (state->input_state.keys[GLFW_KEY_A])
        camera_process_keyboard(&state->input_state.camera, 2, deltaTime);
    if (state->input_state.keys[GLFW_KEY_D])
        camera_process_keyboard(&state->input_state.camera, 3, deltaTime);
}
