#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "buffer_data.h"
#include "descriptors.h"
#include "internal_types.h"
#include "kuta.h"
#include "models.h"
#include "renderer.h"
#include "swapchain.h"
#include "texture_data.h"
#include "types.h"
#include "utils.h"
#include "vulkan_core.h"
#include "window.h"

static KutaContext *kuta_context = NULL;

// Inits the component pools and entitys
void world_init(World *world) {
  memset(world, 0, sizeof(World));

  world->component_pools[COMPONENT_TRANSFORM] =
      malloc(sizeof(TransformComponent) * MAX_ENTITIES);
  world->component_pools[COMPONENT_MESH_RENDERER] =
      malloc(sizeof(MeshRendererComponent) * MAX_ENTITIES);
  world->component_pools[COMPONENT_VISIBILITY] =
      malloc(sizeof(VisibilityComponent) * MAX_ENTITIES);
  world->component_pools[COMPONENT_CAMERA] =
      malloc(sizeof(CameraComponent) * MAX_ENTITIES); // ADD
  world->component_pools[COMPONENT_LIGHT] =
      malloc(sizeof(LightComponent) * MAX_ENTITIES); // ADD

  world->component_sizes[COMPONENT_TRANSFORM] = sizeof(TransformComponent);
  world->component_sizes[COMPONENT_MESH_RENDERER] =
      sizeof(MeshRendererComponent);
  world->component_sizes[COMPONENT_VISIBILITY] = sizeof(VisibilityComponent);
  world->component_sizes[COMPONENT_CAMERA] = sizeof(CameraComponent); // ADD
  world->component_sizes[COMPONENT_LIGHT] = sizeof(LightComponent);   // ADD

  world->entity_count = 0;
  world->next_entity_id = 1;
}

// Cleanup componoents pools
void world_cleanup(World *world) {
  for (int i = 0; i < COMPONENT_COUNT; i++) {
    if (world->component_pools[i]) {
      free(world->component_pools[i]);
      world->component_pools[i] = NULL;
    }
  }
  memset(world, 0, sizeof(World));
}

// Creates an entity and returns its id
Entity create_entity(World *world) {
  if (world->entity_count >= MAX_ENTITIES) {
    return 0;
  }

  Entity new_entity = world->next_entity_id++;

  world->entities[world->entity_count] = new_entity;
  world->entity_count++;

  world->signatures[new_entity] = 0;

  return new_entity;
}

// Returns true if the entity exists
bool entity_exists(World *world, Entity entity) {
  if (entity == 0 || entity >= world->next_entity_id) {
    return false;
  }

  for (uint32_t i = 0; i < world->entity_count; i++) {
    if (world->entities[i] == entity) {
      return true;
    }
  }
  return false;
}

// Adds a desired component to an entity
void add_component(World *world, Entity entity, ComponentType type,
                   void *component) {
  if (!entity_exists(world, entity))
    return;

  void *pool = world->component_pools[type];
  size_t component_size = world->component_sizes[type];

  void *destination = (char *)pool + (entity * component_size);

  memcpy(destination, component, component_size);

  world->signatures[entity] |= (1ULL << type);
}

void *get_component(World *world, Entity entity, ComponentType type) {
  if (!(world->signatures[entity] & (1ULL << type))) {
    return NULL;
  }

  void *pool = world->component_pools[type];
  size_t component_size = world->component_sizes[type];

  return (char *)pool + (entity * component_size);
}

// Updates the Transform data of entites that have it
void transform_system_update(World *world) {
  for (uint32_t i = 0; i < world->entity_count; i++) {
    Entity entity = world->entities[i];

    if (!(world->signatures[entity] &
          COMPONENT_SIGNATURE(COMPONENT_TRANSFORM))) {
      continue;
    }

    TransformComponent *transform =
        get_component(world, entity, COMPONENT_TRANSFORM);

    if (transform->dirty) {
      mat4 translation_matrix, rotation_matrix, scale_matrix;

      glm_translate_make(translation_matrix, transform->position);
      glm_euler_xyz(transform->rotation, rotation_matrix);
      glm_scale_make(scale_matrix, transform->scale);

      mat4 temp;
      glm_mat4_mul(rotation_matrix, scale_matrix, temp);
      glm_mat4_mul(translation_matrix, temp, transform->matrix);

      transform->dirty = false;
    }
  }
}

void set_entity_position(World *world, Entity entity, vec3 position) {
  TransformComponent *transform =
      get_component(world, entity, COMPONENT_TRANSFORM);
  if (!transform)
    return;

  glm_vec3_copy(position, transform->position); // cglm copy function
  transform->dirty = true;
}

void set_entity_rotation(World *world, Entity entity, vec3 rotation) {
  TransformComponent *transform =
      get_component(world, entity, COMPONENT_TRANSFORM);
  if (!transform)
    return;

  glm_vec3_copy(rotation, transform->rotation);
  transform->dirty = true;
}

void set_entity_scale(World *world, Entity entity, vec3 scale) {
  TransformComponent *transform =
      get_component(world, entity, COMPONENT_TRANSFORM);
  if (!transform)
    return;

  glm_vec3_copy(scale, transform->scale);
  transform->dirty = true;
}

void move_entity(World *world, Entity entity, vec3 delta) {
  TransformComponent *transform =
      get_component(world, entity, COMPONENT_TRANSFORM);
  if (!transform)
    return;

  glm_vec3_add(transform->position, delta, transform->position);
  transform->dirty = true;
}

// Manages Resource allocation initialization
ResourceManager *get_resource_manager() {
  static ResourceManager rm = {0};
  static bool initialized = false;

  if (!kuta_context) {
    printf("Error: kuta_context is NULL!\n");
    return NULL;
  }

  if (!initialized) {
    rm.geometry_capacity = 4;
    rm.geometry_count = 0;

    rm.geometries = malloc(sizeof(GeometryData) * rm.geometry_capacity);
    rm.vertex_buffers = malloc(sizeof(VkBuffer) * rm.geometry_capacity);
    rm.vertex_memory = malloc(sizeof(VkDeviceMemory) * rm.geometry_capacity);
    rm.index_buffers = malloc(sizeof(VkBuffer) * rm.geometry_capacity);
    rm.index_memory = malloc(sizeof(VkDeviceMemory) * rm.geometry_capacity);

    rm.texture_capacity = 4;
    rm.texture_count = 0;

    rm.textures = malloc(sizeof(TextureData) * rm.texture_capacity);
    rm.texture_memory = malloc(sizeof(VkDeviceMemory) * rm.texture_capacity);

    for (uint32_t i = 0; i < rm.geometry_capacity; i++) {
      rm.vertex_buffers[i] = VK_NULL_HANDLE;
      rm.vertex_memory[i] = VK_NULL_HANDLE;
      rm.index_buffers[i] = VK_NULL_HANDLE;
      rm.index_memory[i] = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < rm.texture_capacity; i++) {
      rm.textures[i].texture_image = VK_NULL_HANDLE;
      rm.textures[i].texture_image_view = VK_NULL_HANDLE;
      rm.textures[i].texture_sampler = VK_NULL_HANDLE;
      rm.texture_memory[i] = VK_NULL_HANDLE;
    }

    initialize(&rm.free_geometry_ids);
    initialize(&rm.free_texture_ids);

    initialized = true;
  }

  return &rm;
}

VkBuffer get_model_vertex_buffer(int model_id) {
  ResourceManager *rm = get_resource_manager();
  return rm->vertex_buffers[model_id];
}

VkBuffer get_model_index_buffer(int model_id) {
  ResourceManager *rm = get_resource_manager();
  return rm->index_buffers[model_id];
}

int get_model_index_count(int model_id) {
  ResourceManager *rm = get_resource_manager();
  return rm->geometries[model_id].index_count;
}

VkDescriptorSet get_texture_descriptor_set(int texture_id) {
  ResourceManager *rm = get_resource_manager();
  size_t frame_index = kuta_context->state.renderer.current_frame;
  size_t set_index = frame_index * rm->geometry_count + texture_id;
  return kuta_context->state.renderer.descriptor_sets[set_index];
}

// Draws all the entites depending on if they are visible
void render_system_draw(World *world, VkCommandBuffer cmd_buffer) {
  ComponentSignature required = COMPONENT_SIGNATURE(COMPONENT_TRANSFORM) |
                                COMPONENT_SIGNATURE(COMPONENT_MESH_RENDERER) |
                                COMPONENT_SIGNATURE(COMPONENT_VISIBILITY);

  for (uint32_t i = 0; i < world->entity_count; i++) {
    Entity entity = world->entities[i];

    if ((world->signatures[entity] & required) != required) {
      continue;
    }

    TransformComponent *transform =
        get_component(world, entity, COMPONENT_TRANSFORM);
    MeshRendererComponent *renderer =
        get_component(world, entity, COMPONENT_MESH_RENDERER);
    VisibilityComponent *visibility =
        get_component(world, entity, COMPONENT_VISIBILITY);

    if (!visibility->visible || visibility->alpha <= 0.0f) {
      continue;
    }

    VkBuffer vertex_buffers[] = {get_model_vertex_buffer(renderer->model_id)};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(cmd_buffer, get_model_index_buffer(renderer->model_id),
                         0, VK_INDEX_TYPE_UINT32);

    VkDescriptorSet descriptor_set =
        get_texture_descriptor_set(renderer->texture_id);

    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            kuta_context->state.renderer.pipeline_layout, 0, 1,
                            &descriptor_set, 0, NULL);

    vkCmdPushConstants(cmd_buffer, kuta_context->state.renderer.pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4),
                       &transform->matrix);

    uint32_t index_count = get_model_index_count(renderer->model_id);
    vkCmdDrawIndexed(cmd_buffer, index_count, 1, 0, 0, 0);
  }
}

// mark camera as dirty
void camera_dirty(World *world) {
  for (uint32_t i = 0; i < world->entity_count; i++) {
    Entity entity = world->entities[i];

    if ((world->signatures[entity] & COMPONENT_SIGNATURE(COMPONENT_CAMERA))) {
      CameraComponent *camera = get_component(world, entity, COMPONENT_CAMERA);
      if (camera) {
        camera->dirty = true;
      }
    }
  }
}

// Update camera vectors based on yaw/pitch
void camera_system_update_vectors(CameraComponent *camera) {
  vec3 front;
  front[0] = cos(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
  front[1] = sin(glm_rad(camera->pitch));
  front[2] = sin(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));

  glm_vec3_normalize_to(front, camera->front);
  glm_vec3_crossn(camera->front, camera->worldUp, camera->right);
  glm_vec3_crossn(camera->right, camera->front, camera->up);
}

// Update camera matrices
void camera_system_update(World *world, State *state) {
  for (uint32_t i = 0; i < world->entity_count; i++) {
    Entity entity = world->entities[i];

    if (!(world->signatures[entity] & COMPONENT_SIGNATURE(COMPONENT_CAMERA))) {
      continue;
    }

    CameraComponent *camera = get_component(world, entity, COMPONENT_CAMERA);

    if (!camera->active) {
      continue;
    }

    if (camera->dirty) {
      vec3 center;
      glm_vec3_add(camera->position, camera->front, center);
      glm_lookat(camera->position, center, camera->up, camera->view);

      float aspect = (float)state->swp_ch.extent.width /
                     (float)state->swp_ch.extent.height;
      glm_perspective(glm_rad(camera->fov), aspect, camera->nearPlane,
                      camera->farPlane, camera->projection);

      // Flip Y for Vulkan
      camera->projection[1][1] *= -1;

      camera->dirty = false;
    }
  }
}

// return active camera
CameraComponent *get_active_camera(World *world) {
  for (uint32_t i = 0; i < world->entity_count; i++) {
    Entity entity = world->entities[i];

    if (!(world->signatures[entity] & COMPONENT_SIGNATURE(COMPONENT_CAMERA))) {
      continue;
    }

    CameraComponent *camera = get_component(world, entity, COMPONENT_CAMERA);
    if (camera->active) {
      return camera;
    }
  }
  return NULL;
}

void set_active_camera(Entity *camera_entity) {
  kuta_context->state.input_state.active_camera_entity = *camera_entity;
}

void camera_move(World *world, Entity camera_entity, vec3 direction,
                 float deltaTime) {
  CameraComponent *camera =
      get_component(world, camera_entity, COMPONENT_CAMERA);
  if (!camera)
    return;

  vec3 movement;
  glm_vec3_scale(direction, deltaTime * 5.0f,
                 movement); // 5.0f = movement speed
  glm_vec3_add(camera->position, movement, camera->position);
  camera->dirty = true;
}

void camera_rotate(World *world, Entity camera_entity, float yaw_delta,
                   float pitch_delta) {
  CameraComponent *camera =
      get_component(world, camera_entity, COMPONENT_CAMERA);
  if (!camera)
    return;

  camera->yaw += yaw_delta * 0.1f; // 0.1f = sensitivity
  camera->pitch += pitch_delta * 0.1f;

  // Clamp pitch
  if (camera->pitch > 89.0f)
    camera->pitch = 89.0f;
  if (camera->pitch < -89.0f)
    camera->pitch = -89.0f;

  camera_system_update_vectors(camera);
  camera->dirty = true;
}

// Process input for camera entity
void camera_system_process_input(World *world, State *state, float deltaTime) {
  Entity camera_entity = state->input_state.active_camera_entity;
  if (camera_entity == 0)
    return; // No active camera

  CameraComponent *camera =
      get_component(world, camera_entity, COMPONENT_CAMERA);
  if (!camera || !camera->active)
    return;

  // Process keyboard movement
  vec3 movement = {0.0f, 0.0f, 0.0f};
  float speed = 5.0f * deltaTime;

  if (state->input_state.keys[GLFW_KEY_W]) {
    vec3 temp;
    glm_vec3_scale(camera->front, speed, temp);
    glm_vec3_add(movement, temp, movement);
  }
  if (state->input_state.keys[GLFW_KEY_S]) {
    vec3 temp;
    glm_vec3_scale(camera->front, speed, temp);
    glm_vec3_sub(movement, temp, movement);
  }
  if (state->input_state.keys[GLFW_KEY_A]) {
    vec3 temp;
    glm_vec3_scale(camera->right, speed, temp);
    glm_vec3_sub(movement, temp, movement);
  }
  if (state->input_state.keys[GLFW_KEY_D]) {
    vec3 temp;
    glm_vec3_scale(camera->right, speed, temp);
    glm_vec3_add(movement, temp, movement);
  }

  // Apply movement
  if (glm_vec3_norm(movement) > 0.0f) {
    glm_vec3_add(camera->position, movement, camera->position);
    camera->dirty = true;
  }
}

void camera_system_process_mouse(World *world, State *state) {
  Entity camera_entity = state->input_state.active_camera_entity;
  if (camera_entity == 0)
    return;

  CameraComponent *camera =
      get_component(world, camera_entity, COMPONENT_CAMERA);
  if (!camera || !camera->active)
    return;

  if (state->input_state.mouse_delta_x != 0.0f ||
      state->input_state.mouse_delta_y != 0.0f) {

    camera->yaw += state->input_state.mouse_delta_x;
    camera->pitch += state->input_state.mouse_delta_y;

    // Clamp pitch
    if (camera->pitch > 89.0f)
      camera->pitch = 89.0f;
    if (camera->pitch < -89.0f)
      camera->pitch = -89.0f;

    camera_system_update_vectors(camera);
    camera->dirty = true;

    // Reset deltas
    state->input_state.mouse_delta_x = 0.0f;
    state->input_state.mouse_delta_y = 0.0f;
  }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  State *state = (State *)glfwGetWindowUserPointer(window);
  if (action == GLFW_PRESS)
    state->input_state.keys[key] = true;
  else if (action == GLFW_RELEASE)
    state->input_state.keys[key] = false;
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  State *state = (State *)glfwGetWindowUserPointer(window);

  if (state->input_state.firstMouse) {
    state->input_state.lastX = xpos;
    state->input_state.lastY = ypos;
    state->input_state.firstMouse = false;
  }

  float xoffset = xpos - state->input_state.lastX;
  float yoffset = state->input_state.lastY - ypos;

  state->input_state.lastX = xpos;
  state->input_state.lastY = ypos;

  state->input_state.mouse_delta_x = xoffset;
  state->input_state.mouse_delta_y = yoffset;
}

void lighting_system_gather(World *world, LightingUBO *lighting_ubo) {
  memset(lighting_ubo, 0, sizeof(LightingUBO));

  // Set ambient - INCREASE THIS if too dark
  lighting_ubo->ambientColor[0] = 0.2f;
  lighting_ubo->ambientColor[1] = 0.2f;
  lighting_ubo->ambientColor[2] = 0.2f;
  lighting_ubo->ambientIntensity = 1.0f;

  // Find first light entity
  bool found_light = false;

  for (uint32_t i = 0; i < world->entity_count; i++) {
    Entity entity = world->entities[i];

    ComponentSignature required = COMPONENT_SIGNATURE(COMPONENT_LIGHT) |
                                  COMPONENT_SIGNATURE(COMPONENT_TRANSFORM);

    if ((world->signatures[entity] & required) != required) {
      continue;
    }

    LightComponent *light = get_component(world, entity, COMPONENT_LIGHT);
    TransformComponent *transform =
        get_component(world, entity, COMPONENT_TRANSFORM);

    if (!light->enabled || found_light) {
      continue;
    }

    // Use transform position for light
    glm_vec3_copy(transform->position, lighting_ubo->lightPos);
    glm_vec3_copy(light->color, lighting_ubo->lightColor);
    lighting_ubo->intensity = light->intensity;

    found_light = true;
    break; // Only use first light for now
  }

  // Get camera position for specular
  CameraComponent *camera = get_active_camera(world);
  if (camera) {
    glm_vec3_copy(camera->position, lighting_ubo->viewPos);
  }
}

// This Inits the renderer all loading happens after this
void renderer_init(void) {
  create_render_pass(&kuta_context->state);
  create_descriptor_set_layout(&kuta_context->state);
  create_graphics_pipeline(&kuta_context->state);
  create_command_pool(&kuta_context->state);
  create_color_resources(&kuta_context->state);
  create_depth_resources(&kuta_context->state,
                         kuta_context->texture_data.mip_levels);
  create_frame_buffers(&kuta_context->state);
}

// This Deinits the renderer
void renderer_deinit(void) {
  ResourceManager *rm = get_resource_manager();

  create_descriptor_pool(&kuta_context->state, rm);

  // CREATE UNIFORM BUFFERS (both camera and lighting!)
  create_uniform_buffers(&kuta_context->state, &kuta_context->buffer_data);
  create_lighting_buffers(&kuta_context->state);

  create_descriptor_sets(&kuta_context->buffer_data, rm, &kuta_context->state);
  allocate_command_buffer(&kuta_context->state);
  create_sync_objects(&kuta_context->state);
}

// Takes the path to a model returns its id
uint32_t load_geometry(const char *filepath) {
  ResourceManager *rm = get_resource_manager();

  if (rm->geometry_count >= rm->geometry_capacity &&
      isEmpty(&rm->free_geometry_ids)) {
    uint32_t new_capacity = rm->geometry_capacity * 2;

    rm->geometries =
        realloc(rm->geometries, sizeof(GeometryData) * new_capacity);
    rm->vertex_buffers =
        realloc(rm->vertex_buffers, sizeof(VkBuffer) * new_capacity);
    rm->vertex_memory =
        realloc(rm->vertex_memory, sizeof(VkDeviceMemory) * new_capacity);
    rm->index_buffers =
        realloc(rm->index_buffers, sizeof(VkBuffer) * new_capacity);
    rm->index_memory =
        realloc(rm->index_memory, sizeof(VkDeviceMemory) * new_capacity);

    rm->geometry_capacity = new_capacity;
  }
  uint32_t id = !isEmpty(&rm->free_geometry_ids) ? pop(&rm->free_geometry_ids)
                                                 : rm->geometry_count++;
  if (id >= rm->geometry_capacity) {
    printf("Error: Geometry ID %u exceeds capacity %u!\n", id,
           rm->geometry_capacity);
    return UINT32_MAX;
  }

  GeometryData geometry = load_models(filepath);
  rm->geometries[id] = geometry;

  create_vertex_buffer(&kuta_context->state, &kuta_context->buffer_data,
                       geometry.vertices, geometry.vertex_count,
                       &rm->vertex_buffers[id], &rm->vertex_memory[id]);

  create_index_buffer(&kuta_context->state, &kuta_context->buffer_data,
                      geometry.indices, geometry.index_count,
                      &rm->index_buffers[id], &rm->index_memory[id]);
  return id;
}

void free_geometry_buffers(ResourceManager *rm, State *state, uint32_t id) {
  if (rm->vertex_buffers[id] != VK_NULL_HANDLE)
    vkDestroyBuffer(state->vk_core.device, rm->vertex_buffers[id],
                    state->vk_core.allocator);

  if (rm->vertex_memory[id] != VK_NULL_HANDLE)
    vkFreeMemory(state->vk_core.device, rm->vertex_memory[id],
                 state->vk_core.allocator);

  if (rm->index_buffers[id] != VK_NULL_HANDLE)
    vkDestroyBuffer(state->vk_core.device, rm->index_buffers[id],
                    state->vk_core.allocator);

  if (rm->index_memory[id] != VK_NULL_HANDLE)
    vkFreeMemory(state->vk_core.device, rm->index_memory[id],
                 state->vk_core.allocator);

  push(&rm->free_geometry_ids, id);
}

// Takes a path to the texture returns its id
uint32_t load_texture(const char *texture_file) {
  ResourceManager *rm = get_resource_manager();

  if (rm->texture_count >= rm->texture_capacity &&
      isEmpty(&rm->free_texture_ids)) {
    uint32_t new_capacity = rm->texture_capacity * 2;

    rm->textures = realloc(rm->textures, sizeof(TextureData) * new_capacity);
    if (!rm->textures) {
      printf("Error: Failed to reallocate texture arrays!\n");
      return UINT32_MAX;
    }

    rm->texture_capacity = new_capacity;
    printf("Expanded texture capacity to %u\n", new_capacity);
  }

  uint32_t id = !isEmpty(&rm->free_texture_ids) ? pop(&rm->free_texture_ids)
                                                : rm->texture_count++;

  if (id >= rm->texture_capacity) {
    printf("Error: Texture ID %u exceeds capacity %u!\n", id,
           rm->texture_capacity);
    return UINT32_MAX;
  }

  VkDeviceMemory texture_memory;
  Texture_image__memory tx =
      create_texture_image(texture_file, &kuta_context->state, &texture_memory);

  rm->textures[id].texture_image = tx.texture_image;
  rm->texture_memory[id] = texture_memory;

  rm->textures[id].texture_image_view = create_texture_image_view(
      &kuta_context->state, rm->textures[id].texture_image, tx.mipLevels);
  rm->textures[id].texture_sampler =
      create_texture_sampler(&kuta_context->state);

  return id;
}

// This Inits the context and applys the settings struct
bool kuta_init(Settings *settings) {
  if (kuta_context != NULL) {
    return false;
  }
  kuta_context = calloc(1, sizeof(KutaContext));
  if (!kuta_context)
    return false;

  setup_error_handling();
  log_info();

  kuta_context->state.renderer.msaa_samples = VK_SAMPLE_COUNT_1_BIT;
  kuta_context->state.window_data.width = settings->window_width;
  kuta_context->state.window_data.height = settings->window_height;
  kuta_context->state.window_data.title = settings->window_title;
  kuta_context->state.vk_core.api_version = settings->api_version;
  kuta_context->settings.background_color = settings->background_color;

  create_window(&kuta_context->state.window_data);

  // Set the state as window user pointer so callbacks can access it
  glfwSetWindowUserPointer(kuta_context->state.window_data.window,
                           &kuta_context->state);

  // Initialize input state for mouse tracking
  kuta_context->state.input_state.firstMouse = true;
  kuta_context->state.input_state.lastX =
      kuta_context->state.window_data.width / 2.0f;
  kuta_context->state.input_state.lastY =
      kuta_context->state.window_data.height / 2.0f;

  // Set callbacks
  glfwSetKeyCallback(kuta_context->state.window_data.window, key_callback);
  glfwSetCursorPosCallback(kuta_context->state.window_data.window,
                           mouse_callback);
  glfwSetInputMode(kuta_context->state.window_data.window, GLFW_CURSOR,
                   GLFW_CURSOR_DISABLED);

  init_vk(&kuta_context->settings, &kuta_context->state);
  create_swapchain(&kuta_context->state);

  return true;
}

float lastFrame = 0.0f;
// Begins the update loop
void begin_frame(World *world) {
  float currentFrame = glfwGetTime();
  float deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  glfwPollEvents();

  // Process input for active camera
  camera_system_process_input(world, &kuta_context->state, deltaTime);
  camera_system_process_mouse(world, &kuta_context->state);

  // Update camera matrices
  camera_system_update(world, &kuta_context->state);

  uint32_t frame = kuta_context->state.renderer.current_frame;
  vkWaitForFences(kuta_context->state.vk_core.device, 1,
                  &kuta_context->state.renderer.in_flight_fence[frame], VK_TRUE,
                  UINT64_MAX);
  vkResetFences(kuta_context->state.vk_core.device, 1,
                &kuta_context->state.renderer.in_flight_fence[frame]);

  acquire_next_swapchain_image(&kuta_context->state,
                               kuta_context->texture_data.mip_levels);
}

// Ends the loop submits the draw commands and updates the transform system
void end_frame(World *world) {
  transform_system_update(world);

  record_command_buffer(&kuta_context->buffer_data, &kuta_context->settings,
                        &kuta_context->state, world);

  submit_command_buffer(&kuta_context->buffer_data, &kuta_context->state,
                        world);

  present_swapchain_image(&kuta_context->state,
                          kuta_context->texture_data.mip_levels);

  kuta_context->state.renderer.current_frame =
      (kuta_context->state.renderer.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// End of the program Cleanup
void kuta_deinit(void) {
  if (!kuta_context)
    return;

  vkDeviceWaitIdle(kuta_context->state.vk_core.device); // Wait before cleanup

  ResourceManager *rm = get_resource_manager();
  destroy_renderer(&kuta_context->state);
  cleanup_swapchain(&kuta_context->state);

  for (uint32_t i = 0; i < rm->texture_count; i++) {
    if (rm->textures[i].texture_sampler != VK_NULL_HANDLE) {
      vkDestroySampler(kuta_context->state.vk_core.device,
                       rm->textures[i].texture_sampler,
                       kuta_context->state.vk_core.allocator);
    }
    if (rm->textures[i].texture_image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(kuta_context->state.vk_core.device,
                         rm->textures[i].texture_image_view,
                         kuta_context->state.vk_core.allocator);
    }
    if (rm->textures[i].texture_image != VK_NULL_HANDLE) {
      vkDestroyImage(kuta_context->state.vk_core.device,
                     rm->textures[i].texture_image,
                     kuta_context->state.vk_core.allocator);
    }
    if (rm->texture_memory[i] != VK_NULL_HANDLE) {
      vkFreeMemory(kuta_context->state.vk_core.device, rm->texture_memory[i],
                   kuta_context->state.vk_core.allocator);
    }
  }

  destroy_lighting_buffers(&kuta_context->state);
  destroy_uniform_buffers(&kuta_context->buffer_data, &kuta_context->state);
  destroy_descriptor_sets(&kuta_context->state);
  destroy_descriptor_set_layout(&kuta_context->state);
  for (uint32_t i = 0; i < rm->geometry_count; i++) {
    free_geometry_buffers(rm, &kuta_context->state, i);
  }
  if (kuta_context->state.vk_core.device != VK_NULL_HANDLE)
    vkDestroyDevice(kuta_context->state.vk_core.device,
                    kuta_context->state.vk_core.allocator);

  if (kuta_context->state.vk_core.surface != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(kuta_context->state.vk_core.instance,
                        kuta_context->state.vk_core.surface,
                        kuta_context->state.vk_core.allocator);

  if (kuta_context->state.window_data.window)
    glfwDestroyWindow(kuta_context->state.window_data.window);

  if (kuta_context->state.vk_core.instance != VK_NULL_HANDLE)
    vkDestroyInstance(kuta_context->state.vk_core.instance,
                      kuta_context->state.vk_core.allocator);
}

// Checks if the program is still running DUH
bool running() {
  return !glfwWindowShouldClose(kuta_context->state.window_data.window);
}

float get_time() { return glfwGetTime(); }
