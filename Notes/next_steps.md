# Kuta 3D Renderer - ECS Development Roadmap

A structured approach to building a complete 3D rendering framework using Entity-Component-System architecture.

## Table of Contents

- [Phase 1: Core ECS Foundation](#phase-1-core-ecs-foundation)
- [Phase 2: Transform & Rendering Components](#phase-2-transform--rendering-components)
- [Phase 3: Dynamic Resource System](#phase-3-dynamic-resource-system)
- [Phase 4: Camera System](#phase-4-camera-system)
- [Phase 5: Render System](#phase-5-render-system)
- [Future Enhancements](#future-enhancements)

---

## Phase 1: Core ECS Foundation

> **Priority: CRITICAL** - This is the foundation for all other systems

### Current Problem

- Models are hardcoded by index
- No flexible entity management
- Scene composition is inflexible

### Implementation Steps

#### 1.1 Create Core ECS Structures

```c
// Maximum entities and components
#define MAX_ENTITIES 1024
#define MAX_COMPONENT_TYPES 32

// Entity is just an ID
typedef uint32_t Entity;

// Component type enumeration
typedef enum {
    COMPONENT_TRANSFORM = 0,
    COMPONENT_MESH_RENDERER,
    COMPONENT_CAMERA,
    COMPONENT_VISIBILITY,
    COMPONENT_COUNT
} ComponentType;

// Component signature (bitset)
typedef uint64_t ComponentSignature;

// ECS World structure
typedef struct {
    Entity entities[MAX_ENTITIES];
    ComponentSignature signatures[MAX_ENTITIES];
    void* component_pools[COMPONENT_COUNT];
    size_t component_sizes[COMPONENT_COUNT];
    uint32_t entity_count;
    uint32_t next_entity_id;
} World;
```

#### 1.2 Component Definitions

```c
// Transform Component
typedef struct {
    vec3 position;
    vec3 rotation;  // Euler angles in radians
    vec3 scale;
    bool dirty;     // Needs matrix recalculation
    mat4 matrix;    // Cached world matrix
} TransformComponent;

// Mesh Renderer Component
typedef struct {
    uint32_t model_id;
    uint32_t texture_id;
} MeshRendererComponent;

// Visibility Component
typedef struct {
    bool visible;
    float alpha;
} VisibilityComponent;
```

#### 1.3 Core ECS Functions

```c
// World management
void world_init(World* world);
void world_cleanup(World* world);

// Entity management
Entity create_entity(World* world);
void destroy_entity(World* world, Entity entity);
bool entity_exists(World* world, Entity entity);

// Component management
void add_component(World* world, Entity entity, ComponentType type, void* component);
void remove_component(World* world, Entity entity, ComponentType type);
void* get_component(World* world, Entity entity, ComponentType type);
bool has_component(World* world, Entity entity, ComponentType type);

// Component signature helpers
#define COMPONENT_SIGNATURE(type) (1ULL << (type))
ComponentSignature get_entity_signature(World* world, Entity entity);
```

#### 1.4 Expected User Experience

```c
World world;
world_init(&world);

// Create an entity with transform and mesh renderer
Entity player = create_entity(&world);

TransformComponent transform = {
    .position = {0.0f, 0.0f, 0.0f},
    .rotation = {0.0f, 0.0f, 0.0f},
    .scale = {1.0f, 1.0f, 1.0f},
    .dirty = true
};
add_component(&world, player, COMPONENT_TRANSFORM, &transform);

MeshRendererComponent renderer = {
    .model_id = player_model,
    .texture_id = player_texture
};
add_component(&world, player, COMPONENT_MESH_RENDERER, &renderer);

VisibilityComponent visibility = {.visible = true, .alpha = 1.0f};
add_component(&world, player, COMPONENT_VISIBILITY, &visibility);
```

---

## Phase 2: Transform & Rendering Components

> **Priority: CRITICAL** - Core rendering functionality

### Implementation Steps

#### 2.1 Transform System

```c
// System for updating transform matrices
void transform_system_update(World* world);

// Query entities with transform components
typedef struct {
    Entity entities[MAX_ENTITIES];
    TransformComponent* transforms;
    uint32_t count;
} TransformQuery;

TransformQuery query_transforms(World* world);
```

#### 2.2 Rendering System

```c
// System for rendering mesh components
void render_system_draw(World* world, VkCommandBuffer cmd_buffer);

// Query for renderable entities
typedef struct {
    Entity entities[MAX_ENTITIES];
    TransformComponent* transforms;
    MeshRendererComponent* renderers;
    VisibilityComponent* visibility;
    uint32_t count;
} RenderQuery;

RenderQuery query_renderables(World* world);
```

#### 2.3 Helper Functions for Components

```c
// Transform helpers
void set_entity_position(World* world, Entity entity, vec3 position);
void set_entity_rotation(World* world, Entity entity, vec3 rotation);
void set_entity_scale(World* world, Entity entity, vec3 scale);
vec3 get_entity_position(World* world, Entity entity);
vec3 get_entity_forward(World* world, Entity entity);

// Rendering helpers
void set_entity_model(World* world, Entity entity, uint32_t model_id);
void set_entity_texture(World* world, Entity entity, uint32_t texture_id);
void set_entity_visibility(World* world, Entity entity, bool visible);
```

#### 2.4 Expected User Experience

```c
while (running()) {
    begin_frame();

    // Update all transform matrices
    transform_system_update(&world);

    // Render all visible entities
    render_system_draw(&world, command_buffer);

    end_frame();
}

// Move entities around
set_entity_position(&world, player, (vec3){5.0f, 0.0f, 0.0f});
set_entity_rotation(&world, enemy, (vec3){0.0f, time * 2.0f, 0.0f});
```

---

## Phase 3: Dynamic Resource System

> **Priority: HIGH** - Removes hardcoded limitations

### Current Problem

- Fixed array sizes for models/textures
- No runtime resource loading
- Memory waste for unused slots

### Implementation Steps

#### 3.1 Resource Components

```c
// Resource handle component (for sharing resources)
typedef struct {
    uint32_t resource_id;
    uint32_t ref_count;
} ResourceHandle;

// Material component (combines texture + properties)
typedef struct {
    uint32_t diffuse_texture;
    uint32_t normal_texture;
    vec3 color_tint;
    float metallic;
    float roughness;
} MaterialComponent;
```

#### 3.2 Resource Management System

```c
// Resource manager (global)
typedef struct {
    GeometryData* geometries;
    TextureData* textures;
    MaterialData* materials;

    uint32_t geometry_count;
    uint32_t geometry_capacity;
    uint32_t texture_count;
    uint32_t texture_capacity;
    uint32_t material_count;
    uint32_t material_capacity;

    // Free lists for ID reuse
    uint32_t* free_geometry_ids;
    uint32_t* free_texture_ids;
    uint32_t* free_material_ids;
} ResourceManager;

// Resource loading system
uint32_t load_geometry(const char* filepath);
uint32_t load_texture(const char* filepath);
uint32_t create_material(uint32_t diffuse_tex, vec3 color);
void unload_resource(uint32_t resource_id, ComponentType type);
```

#### 3.3 Expected User Experience

```c
world_init(&world);

// Load resources dynamically
uint32_t player_mesh = load_geometry("./models/player.glb");
uint32_t player_tex = load_texture("./textures/player.png");
uint32_t player_mat = create_material(player_tex, (vec3){1.0f, 1.0f, 1.0f});

// Create multiple entities with shared resources
for (int i = 0; i < 10; i++) {
    Entity soldier = create_entity(&world);

    TransformComponent transform = {
        .position = {i * 2.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    };
    add_component(&world, soldier, COMPONENT_TRANSFORM, &transform);

    MeshRendererComponent renderer = {
        .model_id = player_mesh,
        .texture_id = player_tex
    };
    add_component(&world, soldier, COMPONENT_MESH_RENDERER, &renderer);
}
```

---

## Phase 4: Camera System

> **Priority: MEDIUM** - Essential for view control

### Implementation Steps

#### 4.1 Camera Component

```c
typedef enum {
    CAMERA_PROJECTION_PERSPECTIVE,
    CAMERA_PROJECTION_ORTHOGRAPHIC
} CameraProjection;

typedef struct {
    CameraProjection projection;
    float fov;          // For perspective
    float near_plane;
    float far_plane;
    float ortho_size;   // For orthographic
    bool is_main;       // Main camera flag
    mat4 view_matrix;
    mat4 proj_matrix;
} CameraComponent;
```

#### 4.2 Camera System

```c
// Camera system update
void camera_system_update(World* world, uint32_t window_width, uint32_t window_height);

// Camera queries
typedef struct {
    Entity entities[MAX_ENTITIES];
    TransformComponent* transforms;
    CameraComponent* cameras;
    uint32_t count;
} CameraQuery;

CameraQuery query_cameras(World* world);
CameraComponent* get_main_camera(World* world);
```

#### 4.3 Camera Helper Functions

```c
// Camera creation helpers
Entity create_fps_camera(World* world, vec3 position);
Entity create_orbital_camera(World* world, vec3 target, float distance);

// Camera control
void set_camera_fov(World* world, Entity camera, float fov);
void set_camera_target(World* world, Entity camera, vec3 target);
void move_camera(World* world, Entity camera, vec3 delta);
```

#### 4.4 Expected User Experience

```c
// Create main camera
Entity main_cam = create_fps_camera(&world, (vec3){0.0f, 5.0f, 10.0f});

while (running()) {
    begin_frame();

    // Update camera matrices
    camera_system_update(&world, window_width, window_height);

    // Move camera programmatically
    float time = glfwGetTime();
    vec3 cam_pos = {cos(time) * 10.0f, 5.0f, sin(time) * 10.0f};
    set_entity_position(&world, main_cam, cam_pos);

    // Systems update
    transform_system_update(&world);
    render_system_draw(&world, command_buffer);

    end_frame();
}
```

---

## Phase 5: Render System

> **Priority: MEDIUM** - Advanced rendering features

### Implementation Steps

#### 5.1 Render Command Components

```c
// Render command component for custom rendering
typedef enum {
    RENDER_LAYER_BACKGROUND = 0,
    RENDER_LAYER_OPAQUE = 100,
    RENDER_LAYER_TRANSPARENT = 200,
    RENDER_LAYER_UI = 300
} RenderLayer;

typedef struct {
    RenderLayer layer;
    uint32_t sort_order;
    bool cast_shadows;
    bool receive_shadows;
} RenderComponent;
```

#### 5.2 Advanced Render System

```c
// Render system with sorting
void render_system_collect_commands(World* world);
void render_system_sort_commands(void);
void render_system_execute_commands(VkCommandBuffer cmd_buffer);

// Render queries with filtering
typedef struct {
    ComponentSignature required;
    ComponentSignature excluded;
    RenderLayer layer_filter;
} RenderFilter;

uint32_t query_entities_filtered(World* world, RenderFilter filter, Entity* out_entities);
```

#### 5.3 Expected User Experience

```c
// Create entities with different render layers
Entity background = create_entity(&world);
add_component(&world, background, COMPONENT_TRANSFORM, &bg_transform);
add_component(&world, background, COMPONENT_MESH_RENDERER, &bg_renderer);
RenderComponent bg_render = {.layer = RENDER_LAYER_BACKGROUND, .sort_order = 0};
add_component(&world, background, COMPONENT_RENDER, &bg_render);

Entity ui_element = create_entity(&world);
// ... setup ui entity
RenderComponent ui_render = {.layer = RENDER_LAYER_UI, .sort_order = 10};
add_component(&world, ui_element, COMPONENT_RENDER, &ui_render);

while (running()) {
    begin_frame();

    // Collect and sort render commands by layer and sort order
    render_system_collect_commands(&world);
    render_system_sort_commands();
    render_system_execute_commands(command_buffer);

    end_frame();
}
```

---

## Future Enhancements

### Additional Components

- [ ] Animation Component (bone transforms, keyframes)
- [ ] Physics Component (rigidbody, collider)
- [ ] Audio Component (3D positioned sound)
- [ ] Light Component (point, directional, spot)
- [ ] Particle Component (emitters, systems)

### Advanced Systems

- [ ] Animation System (skeletal animation)
- [ ] Physics System (collision detection/response)
- [ ] Audio System (3D positional audio)
- [ ] Lighting System (shadow mapping, GI)
- [ ] Culling System (frustum, occlusion)
- [ ] LOD System (level-of-detail switching)

### Performance Features

- [ ] Component archetype optimization
- [ ] System parallelization
- [ ] GPU-driven rendering
- [ ] Instancing system for similar entities

### Developer Tools

- [ ] Entity inspector/debugger
- [ ] Component serialization (save/load scenes)
- [ ] Hot-reloading components
- [ ] Performance profiler for systems

---

## Implementation Order

1. **Week 1**: Core ECS Foundation (Phase 1)
2. **Week 2**: Transform & Rendering Components (Phase 2)
3. **Week 3**: Dynamic Resource System (Phase 3)
4. **Week 4**: Camera System (Phase 4)
5. **Week 5**: Render System (Phase 5)

### Key ECS Principles

- **Entities** are just IDs - no behavior, just containers for components
- **Components** are pure data - no methods, just state
- **Systems** contain all logic - operate on entities with specific component combinations
- **Queries** efficiently find entities with required component signatures
- **Composition over inheritance** - mix and match components to create different entity types

Each phase should be fully tested with the ECS architecture before moving to the next. The ECS approach will make your engine much more flexible and performant than traditional OOP approaches.
