#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "camera.h"
#include "descriptors.h"
#include "kuta.h"
#include "main.h"
#include "models.h"
#include "renderer.h"
#include "swapchain.h"
#include "textures.h"
#include "utils.h"
#include "vertex_data.h"
#include "vulkan_core.h"
#include "window.h"

static KutaContext *kuta_context = NULL;

void world_init(World *world) {
  memset(world, 0, sizeof(World));

  world->component_pools[COMPONENT_TRANSFORM] =
      malloc(sizeof(TransformComponent) * MAX_ENTITIES);
  world->component_pools[COMPONENT_MESH_RENDERER] =
      malloc(sizeof(MeshRendererComponent) * MAX_ENTITIES);
  world->component_pools[COMPONENT_VISIBILITY] = // ADD THIS
      malloc(sizeof(VisibilityComponent) * MAX_ENTITIES);

  world->component_sizes[COMPONENT_TRANSFORM] = sizeof(TransformComponent);
  world->component_sizes[COMPONENT_MESH_RENDERER] =
      sizeof(MeshRendererComponent);
  world->component_sizes[COMPONENT_VISIBILITY] =
      sizeof(VisibilityComponent); // ADD THIS

  world->entity_count = 0;
  world->next_entity_id = 1;
}

void world_cleanup(World *world) {
  for (int i = 0; i < COMPONENT_COUNT; i++) {
    if (world->component_pools[i]) {
      free(world->component_pools[i]);
      world->component_pools[i] = NULL;
    }
  }
  memset(world, 0, sizeof(World));
}

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

bool entity_exists(World *world, Entity entity) {
  // Check if entity is within valid range
  if (entity == 0 || entity >= world->next_entity_id) {
    return false;
  }

  // Search through the entities array to see if this entity exists
  for (uint32_t i = 0; i < world->entity_count; i++) {
    if (world->entities[i] == entity) {
      return true;
    }
  }
  return false;
}

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
      // Use cglm functions to build transform matrix
      mat4 translation_matrix, rotation_matrix, scale_matrix;

      // Create individual transformation matrices
      glm_translate_make(translation_matrix, transform->position);
      glm_euler_xyz(transform->rotation, rotation_matrix);
      glm_scale_make(scale_matrix, transform->scale);

      // Combine: Translation * Rotation * Scale
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

// Convenience functions
void move_entity(World *world, Entity entity, vec3 delta) {
  TransformComponent *transform =
      get_component(world, entity, COMPONENT_TRANSFORM);
  if (!transform)
    return;

  glm_vec3_add(transform->position, delta, transform->position);
  transform->dirty = true;
}

ResourceManager *get_resource_manager() {
  static ResourceManager rm = {0};
  static bool initialized = false;

  if (!kuta_context) {
    printf("Error: kuta_context is NULL!\n");
    return NULL;
  }

  if (!initialized) {
    // Start with initial capacity
    rm.geometry_capacity = 4; // Start small, will grow as needed
    rm.geometry_count = 0;

    // Allocate our OWN arrays (don't point to old ones)
    rm.geometries = malloc(sizeof(GeometryData) * rm.geometry_capacity);
    rm.vertex_buffers = malloc(sizeof(VkBuffer) * rm.geometry_capacity);
    rm.vertex_memory = malloc(sizeof(VkDeviceMemory) * rm.geometry_capacity);
    rm.index_buffers = malloc(sizeof(VkBuffer) * rm.geometry_capacity);
    rm.index_memory = malloc(sizeof(VkDeviceMemory) * rm.geometry_capacity);

    rm.texture_capacity = 4; // Start small, will grow as needed
    rm.texture_count = 0;

    // Allocate texture arrays with memory tracking
    rm.textures = malloc(sizeof(TextureData) * rm.texture_capacity);
    rm.texture_memory =
        malloc(sizeof(VkDeviceMemory) * rm.texture_capacity); // ADD THIS

    // Initialize all handles to VK_NULL_HANDLE
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

    // Bind vertex/index buffers
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

bool kuta_init(Settings *settings) {
  if (kuta_context != NULL) {
    return false;
  }
  kuta_context = calloc(1, sizeof(KutaContext));
  if (!kuta_context)
    return false;

  setup_error_handling();
  log_info();

  kuta_context->state.window_data.width = settings->window_width;
  kuta_context->state.window_data.height = settings->window_height;
  kuta_context->state.window_data.title = settings->window_title;
  kuta_context->state.vk_core.api_version = settings->api_version;
  kuta_context->settings.background_color = settings->background_color;

  create_window(&kuta_context->state.window_data);
  // Set the state as window user pointer so callbacks can access it
  glfwSetWindowUserPointer(kuta_context->state.window_data.window,
                           &kuta_context->state);

  // Initialize input state
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

  camera_init(&kuta_context->state.input_state.camera);
  init_vk(&kuta_context->settings, &kuta_context->state);

  create_swapchain(&kuta_context->state);

  return true;
}

void renderer_init(void) {
  create_render_pass(&kuta_context->state);
  create_descriptor_set_layout(&kuta_context->state);
  create_graphics_pipeline(&kuta_context->state);
  create_command_pool(&kuta_context->state);
  create_depth_resources(&kuta_context->state);
  create_frame_buffers(&kuta_context->state);
}

void renderer_deinit(void) {
  ResourceManager *rm = get_resource_manager();
  create_descriptor_pool(&kuta_context->state, rm);
  create_uniform_buffers(&kuta_context->state, &kuta_context->buffer_data);
  create_descriptor_sets(&kuta_context->buffer_data, rm, &kuta_context->state);
  allocate_command_buffer(&kuta_context->state);
  create_sync_objects(&kuta_context->state);
}

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

  // Proper texture loading:
  VkDeviceMemory texture_memory; // Not a pointer, actual variable
  Texture_image__memory tx =
      create_texture_image(texture_file, &kuta_context->state, &texture_memory);

  // Store in ResourceManager
  rm->textures[id].texture_image = tx.texture_image;
  rm->texture_memory[id] = texture_memory; // Store the actual memory handle

  rm->textures[id].texture_image_view = create_texture_image_view(
      &kuta_context->state, rm->textures[id].texture_image);
  rm->textures[id].texture_sampler =
      create_texture_sampler(&kuta_context->state);

  return id;
}

float lastFrame = 0.0f;
void begin_frame(void) {
  float currentFrame = glfwGetTime();
  float deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  glfwPollEvents();

  process_input(&kuta_context->state, deltaTime);

  uint32_t frame = kuta_context->state.renderer.current_frame;
  vkWaitForFences(kuta_context->state.vk_core.device, 1,
                  &kuta_context->state.renderer.in_flight_fence[frame], VK_TRUE,
                  UINT64_MAX);
  vkResetFences(kuta_context->state.vk_core.device, 1,
                &kuta_context->state.renderer.in_flight_fence[frame]);

  acquire_next_swapchain_image(&kuta_context->state);
}

void end_frame(World *world) {

  transform_system_update(world);

  record_command_buffer(&kuta_context->buffer_data, &kuta_context->settings,
                        &kuta_context->state, world);

  submit_command_buffer(&kuta_context->buffer_data, &kuta_context->state);

  present_swapchain_image(&kuta_context->state);

  kuta_context->state.renderer.current_frame =
      (kuta_context->state.renderer.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

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

bool running() {
  return !glfwWindowShouldClose(kuta_context->state.window_data.window);
}
