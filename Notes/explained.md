## Phase 1: Core ECS Foundation - Step by Step

### Step 1.1: Understanding the ECS Core Structures

#### What is an Entity?

```c
typedef uint32_t Entity;
```

- **An Entity is just a number** - think of it like a unique ID or barcode
- It has NO data, NO behavior - just identifies "something" in your game
- Example: Entity 1 might be a player, Entity 2 might be a rock, Entity 3 might be a camera

#### What is a Component Type?

```c
typedef enum {
    COMPONENT_TRANSFORM = 0,    // Position, rotation, scale
    COMPONENT_MESH_RENDERER,    // What 3D model to draw
    COMPONENT_CAMERA,          // Camera settings
    COMPONENT_VISIBILITY,      // Is it visible?
    COMPONENT_COUNT
} ComponentType;
```

- **Component Types are like "categories of data"**
- Each type represents a different kind of information an entity can have
- Think of them like "slots" that entities can fill with data

#### What is a Component Signature?

```c
typedef uint64_t ComponentSignature;
```

- **A signature is a bitset** - each bit represents whether an entity has a specific component
- If bit 0 is set, entity has COMPONENT_TRANSFORM
- If bit 1 is set, entity has COMPONENT_MESH_RENDERER
- Example: Signature `0000...0011` means entity has Transform + MeshRenderer components

#### The ECS World Structure

```c
typedef struct {
    Entity entities[MAX_ENTITIES];              // Array of all entity IDs
    ComponentSignature signatures[MAX_ENTITIES]; // What components each entity has
    void* component_pools[COMPONENT_COUNT];     // Arrays storing actual component data
    size_t component_sizes[COMPONENT_COUNT];    // Size of each component type
    uint32_t entity_count;                      // How many entities exist
    uint32_t next_entity_id;                    // Next ID to assign
} World;
```

**Why this structure?**

- `entities[]`: Keeps track of all living entities
- `signatures[]`: Quick lookup - "what components does entity X have?"
- `component_pools[]`: Separate arrays for each component type (better cache performance)
- This layout is called "archetype-based" ECS

### Step 1.2: Component Definitions Explained

#### Transform Component - "Where is it?"

```c
typedef struct {
    vec3 position;    // X, Y, Z coordinates in world space
    vec3 rotation;    // Euler angles (how much rotated around each axis)
    vec3 scale;       // Size multiplier (1.0 = normal size, 2.0 = double size)
    bool dirty;       // "Has this changed since last frame?"
    mat4 matrix;      // Pre-calculated transformation matrix (for GPU)
} TransformComponent;
```

- **Every visible object needs position/rotation/scale**
- `dirty` flag avoids recalculating matrix every frame
- `matrix` is the final transformation sent to GPU shaders

#### Mesh Renderer Component - "What does it look like?"

```c
typedef struct {
    uint32_t model_id;    // Which 3D model to use (index into model array)
    uint32_t texture_id;  // Which texture to apply (index into texture array)
} MeshRendererComponent;
```

- **Pure data** - just IDs pointing to loaded resources
- Multiple entities can share the same model_id (multiple soldiers using same mesh)

#### Visibility Component - "Should we draw it?"

```c
typedef struct {
    bool visible;  // Is it currently visible?
    float alpha;   // Transparency (1.0 = opaque, 0.0 = invisible)
} VisibilityComponent;
```

- Allows hiding objects without destroying them
- Alpha blending support for transparency effects

### Step 1.3: Core ECS Functions Explained

#### World Management

```c
void world_init(World* world) {
    // 1. Zero out the world structure
    memset(world, 0, sizeof(World));

    // 2. Allocate memory for each component pool
    world->component_pools[COMPONENT_TRANSFORM] =
        malloc(sizeof(TransformComponent) * MAX_ENTITIES);
    world->component_pools[COMPONENT_MESH_RENDERER] =
        malloc(sizeof(MeshRendererComponent) * MAX_ENTITIES);
    // ... etc for each component type

    // 3. Store component sizes for later use
    world->component_sizes[COMPONENT_TRANSFORM] = sizeof(TransformComponent);
    world->component_sizes[COMPONENT_MESH_RENDERER] = sizeof(MeshRendererComponent);

    // 4. Initialize counters
    world->entity_count = 0;
    world->next_entity_id = 1;  // Start at 1 (0 = invalid entity)
}
```

#### Entity Creation

```c
Entity create_entity(World* world) {
    // 1. Check if we have space
    if (world->entity_count >= MAX_ENTITIES) {
        return 0;  // Error: no more entities available
    }

    // 2. Assign new ID
    Entity new_entity = world->next_entity_id++;

    // 3. Add to entity list
    world->entities[world->entity_count] = new_entity;
    world->entity_count++;

    // 4. Initialize empty signature (no components yet)
    world->signatures[new_entity] = 0;

    return new_entity;
}
```

#### Adding Components - The Key Function

```c
void add_component(World* world, Entity entity, ComponentType type, void* component) {
    // 1. Validate entity exists
    if (!entity_exists(world, entity)) return;

    // 2. Get the component pool for this type
    void* pool = world->component_pools[type];
    size_t component_size = world->component_sizes[type];

    // 3. Calculate where to store this component
    // Simple approach: store at entity ID index
    void* destination = (char*)pool + (entity * component_size);

    // 4. Copy the component data
    memcpy(destination, component, component_size);

    // 5. Update entity's signature (set the bit for this component type)
    world->signatures[entity] |= (1ULL << type);
}
```

**Why this works:**

- Each component type has its own array
- Entity ID is used as index into these arrays
- Signature bitset tracks which components an entity has

#### Getting Components

```c
void* get_component(World* world, Entity entity, ComponentType type) {
    // 1. Check if entity has this component
    if (!(world->signatures[entity] & (1ULL << type))) {
        return NULL;  // Entity doesn't have this component
    }

    // 2. Calculate location in component pool
    void* pool = world->component_pools[type];
    size_t component_size = world->component_sizes[type];

    return (char*)pool + (entity * component_size);
}
```

### Step 1.4: How It All Works Together

```c
// Example: Creating a renderable object
World world;
world_init(&world);

// 1. Create entity (just gets an ID)
Entity player = create_entity(&world);  // Returns something like 1

// 2. Add transform component
TransformComponent transform = {
    .position = {0.0f, 0.0f, 0.0f},
    .rotation = {0.0f, 0.0f, 0.0f},
    .scale = {1.0f, 1.0f, 1.0f},
    .dirty = true
};
add_component(&world, player, COMPONENT_TRANSFORM, &transform);
// Now player entity has signature: 0000...0001 (bit 0 set)

// 3. Add mesh renderer component
MeshRendererComponent renderer = {
    .model_id = 0,     // First loaded model
    .texture_id = 0    // First loaded texture
};
add_component(&world, player, COMPONENT_MESH_RENDERER, &renderer);
// Now player entity has signature: 0000...0011 (bits 0 and 1 set)

// 4. Add visibility component
VisibilityComponent visibility = {.visible = true, .alpha = 1.0f};
add_component(&world, player, COMPONENT_VISIBILITY, &visibility);
// Now player entity has signature: 0000...1011 (bits 0, 1, and 3 set)
```

**Memory Layout After This:**

```
entities[0] = 1 (player entity ID)
signatures[1] = 0000...1011 (has Transform, MeshRenderer, Visibility)

component_pools[COMPONENT_TRANSFORM][1] = {pos:{0,0,0}, rot:{0,0,0}, scale:{1,1,1}}
component_pools[COMPONENT_MESH_RENDERER][1] = {model_id:0, texture_id:0}
component_pools[COMPONENT_VISIBILITY][1] = {visible:true, alpha:1.0}
```

---

## Phase 2: Transform & Rendering Components - Step by Step

### Step 2.1: Understanding Systems

**What is a System?**

- A system is a function that processes entities with specific components
- Systems contain ALL the logic - components are just data
- Systems run every frame to update the world state

#### Transform System Explained

```c
void transform_system_update(World* world) {
    // 1. Find all entities that have transform components
    for (uint32_t i = 0; i < world->entity_count; i++) {
        Entity entity = world->entities[i];

        // 2. Check if this entity has a transform component
        if (!(world->signatures[entity] & COMPONENT_SIGNATURE(COMPONENT_TRANSFORM))) {
            continue;  // Skip entities without transform
        }

        // 3. Get the transform component
        TransformComponent* transform = get_component(world, entity, COMPONENT_TRANSFORM);

        // 4. Only update if transform changed (dirty flag)
        if (transform->dirty) {
            // 5. Calculate new world matrix
            mat4 translation = mat4_translate(transform->position);
            mat4 rotation = mat4_rotate_xyz(transform->rotation);
            mat4 scale = mat4_scale(transform->scale);

            // 6. Combine transformations: Scale * Rotation * Translation
            transform->matrix = mat4_multiply(translation,
                                mat4_multiply(rotation, scale));

            // 7. Mark as clean
            transform->dirty = false;
        }
    }
}
```

**Why separate the transform system?**

- Only recalculates matrices when objects move (performance)
- Centralized transform logic (easier to debug)
- Can be optimized later (SIMD, multithreading)

#### Queries - Finding Entities Efficiently

```c
typedef struct {
    Entity entities[MAX_ENTITIES];       // Matching entity IDs
    TransformComponent* transforms;      // Pointers to their transform components
    uint32_t count;                      // How many matches found
} TransformQuery;

TransformQuery query_transforms(World* world) {
    TransformQuery query = {0};

    // 1. Loop through all entities
    for (uint32_t i = 0; i < world->entity_count; i++) {
        Entity entity = world->entities[i];

        // 2. Check if entity has transform component
        if (world->signatures[entity] & COMPONENT_SIGNATURE(COMPONENT_TRANSFORM)) {
            // 3. Add to query results
            query.entities[query.count] = entity;
            query.transforms[query.count] = get_component(world, entity, COMPONENT_TRANSFORM);
            query.count++;
        }
    }

    return query;
}
```

**Using queries:**

```c
// Instead of manually looping, use a query
TransformQuery transforms = query_transforms(&world);

for (uint32_t i = 0; i < transforms.count; i++) {
    Entity entity = transforms.entities[i];
    TransformComponent* transform = transforms.transforms[i];

    // Do something with this transform...
    printf("Entity %u is at position (%.2f, %.2f, %.2f)\n",
           entity, transform->position.x, transform->position.y, transform->position.z);
}
```

### Step 2.2: Rendering System Explained

```c
void render_system_draw(World* world, VkCommandBuffer cmd_buffer) {
    // 1. Find all entities that can be rendered
    // They need: Transform + MeshRenderer + Visibility components
    ComponentSignature required = COMPONENT_SIGNATURE(COMPONENT_TRANSFORM) |
                                  COMPONENT_SIGNATURE(COMPONENT_MESH_RENDERER) |
                                  COMPONENT_SIGNATURE(COMPONENT_VISIBILITY);

    for (uint32_t i = 0; i < world->entity_count; i++) {
        Entity entity = world->entities[i];

        // 2. Check if entity has all required components
        if ((world->signatures[entity] & required) != required) {
            continue;  // Skip entities missing required components
        }

        // 3. Get all the components we need
        TransformComponent* transform = get_component(world, entity, COMPONENT_TRANSFORM);
        MeshRendererComponent* renderer = get_component(world, entity, COMPONENT_MESH_RENDERER);
        VisibilityComponent* visibility = get_component(world, entity, COMPONENT_VISIBILITY);

        // 4. Skip invisible objects
        if (!visibility->visible || visibility->alpha <= 0.0f) {
            continue;
        }

        // 5. Issue Vulkan draw commands
        // Bind the model's vertex/index buffers
        VkBuffer vertex_buffers[] = {get_model_vertex_buffer(renderer->model_id)};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(cmd_buffer, get_model_index_buffer(renderer->model_id), 0, VK_INDEX_TYPE_UINT32);

        // Bind texture
        VkDescriptorSet descriptor_sets[] = {get_texture_descriptor_set(renderer->texture_id)};
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline_layout, 0, 1, descriptor_sets, 0, NULL);

        // Send transform matrix to GPU via push constants
        vkCmdPushConstants(cmd_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
                          0, sizeof(mat4), &transform->matrix);

        // Draw the mesh
        uint32_t index_count = get_model_index_count(renderer->model_id);
        vkCmdDrawIndexed(cmd_buffer, index_count, 1, 0, 0, 0);
    }
}
```

**What this accomplishes:**

- Only renders entities that have ALL required components
- Automatically skips invisible objects
- Uses transform matrices calculated by transform system
- Issues actual Vulkan draw commands

### Step 2.3: Helper Functions Explained

```c
void set_entity_position(World* world, Entity entity, vec3 position) {
    // 1. Get the transform component (if it exists)
    TransformComponent* transform = get_component(world, entity, COMPONENT_TRANSFORM);
    if (!transform) {
        return;  // Entity doesn't have a transform component
    }

    // 2. Update position
    transform->position = position;

    // 3. Mark as dirty so transform system recalculates matrix
    transform->dirty = true;
}

vec3 get_entity_position(World* world, Entity entity) {
    TransformComponent* transform = get_component(world, entity, COMPONENT_TRANSFORM);
    if (!transform) {
        return (vec3){0.0f, 0.0f, 0.0f};  // Default position if no transform
    }
    return transform->position;
}
```

**Why helper functions?**

- Hide ECS complexity from users
- Provide safe access to component data
- Handle error cases (entity doesn't exist, missing components)

---

## Phase 3: Dynamic Resource System - Step by Step

### Step 3.1: The Problem with Fixed Resources

**Current approach (problematic):**

```c
#define MODELS_COUNT 2
#define TEXTURE_COUNT 2

const char *models_files[MODELS_COUNT] = {
    "./models/aatrox.glb",
    NULL  // Wasted slot
};
```

**Problems:**

- Must know resource count at compile time
- Wastes memory for unused slots
- Can't load new resources during gameplay
- Hard to manage resource lifetimes

### Step 3.2: Dynamic Resource Manager

```c
typedef struct {
    // Dynamic arrays that grow as needed
    GeometryData* geometries;        // Array of loaded 3D models
    TextureData* textures;           // Array of loaded textures

    // Vulkan resources (parallel arrays)
    VkBuffer* vertex_buffers;        // GPU vertex data for each model
    VkDeviceMemory* vertex_memory;   // GPU memory handles
    VkBuffer* index_buffers;         // GPU index data
    VkDeviceMemory* index_memory;

    // Current counts and capacities
    uint32_t geometry_count;         // How many models loaded
    uint32_t geometry_capacity;      // How many models we have space for
    uint32_t texture_count;
    uint32_t texture_capacity;

    // Free lists for ID reuse
    uint32_t* free_geometry_ids;     // Stack of unused model IDs
    uint32_t* free_texture_ids;      // Stack of unused texture IDs
} ResourceManager;
```

#### Loading Resources Dynamically

```c
uint32_t load_geometry(const char* filepath) {
    ResourceManager* rm = get_resource_manager();

    // 1. Check if we need to expand the arrays
    if (rm->geometry_count >= rm->geometry_capacity) {
        // Double the capacity
        uint32_t new_capacity = rm->geometry_capacity * 2;

        // Reallocate all parallel arrays
        rm->geometries = realloc(rm->geometries, sizeof(GeometryData) * new_capacity);
        rm->vertex_buffers = realloc(rm->vertex_buffers, sizeof(VkBuffer) * new_capacity);
        rm->vertex_memory = realloc(rm->vertex_memory, sizeof(VkDeviceMemory) * new_capacity);
        // ... etc for all parallel arrays

        rm->geometry_capacity = new_capacity;
    }

    // 2. Get next available ID
    uint32_t geometry_id;
    if (rm->free_geometry_ids && stack_has_elements(rm->free_geometry_ids)) {
        geometry_id = stack_pop(rm->free_geometry_ids);  // Reuse old ID
    } else {
        geometry_id = rm->geometry_count;  // Use new ID
        rm->geometry_count++;
    }

    // 3. Load the actual geometry data
    GeometryData geometry = load_glb_file(filepath);  // Your existing function
    rm->geometries[geometry_id] = geometry;

    // 4. Create Vulkan buffers
    create_vertex_buffer(&rm->vertex_buffers[geometry_id],
                        &rm->vertex_memory[geometry_id],
                        geometry.vertices, geometry.vertex_count);
    create_index_buffer(&rm->index_buffers[geometry_id],
                       &rm->index_memory[geometry_id],
                       geometry.indices, geometry.index_count);

    return geometry_id;
}
```

#### Resource Sharing Example

```c
// Load resources once
uint32_t soldier_model = load_geometry("./models/soldier.glb");
uint32_t soldier_texture = load_texture("./textures/soldier.png");

// Create 100 soldiers that all share the same model and texture
for (int i = 0; i < 100; i++) {
    Entity soldier = create_entity(&world);

    // Each soldier has its own transform (different positions)
    TransformComponent transform = {
        .position = {i % 10 * 2.0f, 0.0f, i / 10 * 2.0f},  // Grid layout
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f},
        .dirty = true
    };
    add_component(&world, soldier, COMPONENT_TRANSFORM, &transform);

    // But they all share the same model and texture (saves GPU memory)
    MeshRendererComponent renderer = {
        .model_id = soldier_model,     // Same for all soldiers
        .texture_id = soldier_texture  // Same for all soldiers
    };
    add_component(&world, soldier, COMPONENT_MESH_RENDERER, &renderer);

    VisibilityComponent visibility = {.visible = true, .alpha = 1.0f};
    add_component(&world, soldier, COMPONENT_VISIBILITY, &visibility);
}
```

**Benefits:**

- 1 model loaded in GPU memory, used by 100 entities
- Each entity can still have different positions/rotations
- Much more memory efficient

### Step 3.3: Material Component System

```c
typedef struct {
    uint32_t diffuse_texture;    // Base color texture
    uint32_t normal_texture;     // Surface detail texture
    vec3 color_tint;            // Multiply color (for variations)
    float metallic;             // How metallic the surface is (0.0 - 1.0)
    float roughness;            // How rough the surface is (0.0 - 1.0)
} MaterialComponent;

// Instead of just texture_id in MeshRenderer, use material_id
typedef struct {
    uint32_t model_id;
    uint32_t material_id;  // Points to a material instead of just texture
} MeshRendererComponent;
```

**Creating material variations:**

```c
uint32_t base_texture = load_texture("soldier_diffuse.png");
uint32_t normal_map = load_texture("soldier_normal.png");

// Create different material variants
uint32_t red_soldier_material = create_material(base_texture, normal_map,
                                               (vec3){1.0f, 0.5f, 0.5f}, // Red tint
                                               0.1f, 0.8f);  // Low metallic, high roughness

uint32_t blue_soldier_material = create_material(base_texture, normal_map,
                                                (vec3){0.5f, 0.5f, 1.0f}, // Blue tint
                                                0.1f, 0.8f);

uint32_t shiny_soldier_material = create_material(base_texture, normal_map,
                                                 (vec3){1.0f, 1.0f, 1.0f}, // No tint
                                                 0.9f, 0.1f);  // High metallic, low roughness
```

Now you can have visual variety without loading multiple textures!

---

## Phase 4: Camera System - Step by Step

### Step 4.1: Camera as an Entity

**Key insight: The camera is just another entity!**

```c
typedef struct {
    CameraProjection projection;  // Perspective or orthographic?
    float fov;                   // Field of view (for perspective)
    float near_plane;            // Near clipping plane
    float far_plane;             // Far clipping plane
    float ortho_size;            // Size (for orthographic)
    bool is_main;                // Is this the main camera?
    mat4 view_matrix;            // Camera's view transformation
    mat4 proj_matrix;            // Projection transformation
} CameraComponent;
```

#### Creating a Camera Entity

```c
Entity create_fps_camera(World* world, vec3 position) {
    // 1. Create entity like any other
    Entity camera = create_entity(world);

    // 2. Add transform component (cameras need position/rotation too!)
    TransformComponent transform = {
        .position = position,
        .rotation = {0.0f, 0.0f, 0.0f},  // Looking forward initially
        .scale = {1.0f, 1.0f, 1.0f},     // Scale doesn't matter for cameras
        .dirty = true
    };
    add_component(world, camera, COMPONENT_TRANSFORM, &transform);

    // 3. Add camera component
    CameraComponent cam = {
        .projection = CAMERA_PROJECTION_PERSPECTIVE,
        .fov = 45.0f,        // 45 degree field of view
        .near_plane = 0.1f,   // Objects closer than 10cm are clipped
        .far_plane = 1000.0f, // Objects farther than 1km are clipped
        .is_main = true,      // This is our main camera
        .view_matrix = mat4_identity(),
        .proj_matrix = mat4_identity()
    };
    add_component(world, camera, COMPONENT_CAMERA, &cam);

    return camera;
}
```

### Step 4.2: Camera System Update

```c
void camera_system_update(World* world, uint32_t window_width, uint32_t window_height) {
    // 1. Find all camera entities
    ComponentSignature camera_signature = COMPONENT_SIGNATURE(COMPONENT_TRANSFORM) |
                                          COMPONENT_SIGNATURE(COMPONENT_CAMERA);

    for (uint32_t i = 0; i < world->entity_count; i++) {
        Entity entity = world->entities[i];

        // 2. Skip non-camera entities
        if ((world->signatures[entity] & camera_signature) != camera_signature) {
            continue;
        }

        // 3. Get components
        TransformComponent* transform = get_component(world, entity, COMPONENT_TRANSFORM);
        CameraComponent* camera = get_component(world, entity, COMPONENT_CAMERA);

        // 4. Update projection matrix if needed
        float aspect_ratio = (float)window_width / (float)window_height;

        if (camera->projection == CAMERA_PROJECTION_PERSPECTIVE) {
            camera->proj_matrix = mat4_perspective(camera->fov, aspect_ratio,
                                                  camera->near_plane, camera->far_plane);
        } else {
            float half_size = camera->ortho_size * 0.5f;
            camera->view_matrix = mat4_orthographic(-half_size * aspect_ratio,
                                                   half_size * aspect_ratio,
                                                   -half_size, half_size,
                                                   camera->near_plane, camera->far_plane);
        }

        // 5. Update view matrix from transform
        // View matrix is inverse of camera's world transform
        mat4 camera_world_matrix = transform->matrix;
        camera->view_matrix = mat4_inverse(camera_world_matrix);
    }
}
```

#### Using Camera Matrices in Rendering

```c
void render_system_draw(World* world, VkCommandBuffer cmd_buffer) {
    // 1. Find main camera
    CameraComponent* main_camera = get_main_camera(world);
    if (!main_camera) return;  // No camera to render from

    // 2. Create view-projection matrix
    mat4 view_proj = mat4_multiply(main_camera->proj_matrix, main_camera->view_matrix);

    // 3. Render all objects from this camera's perspective
    for (each renderable entity) {
        // Combine object transform with camera view-projection
        mat4 mvp = mat4_multiply(view_proj, object_transform->matrix);

        // Send to GPU
        vkCmdPushConstants(cmd_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT,
                          0, sizeof(mat4), &mvp);

        // Draw...
    }
}
```

### Step 4.3: Multiple Cameras

```c
// Create multiple cameras for different purposes
Entity main_camera = create_fps_camera(&world, (vec3){0, 5, 10});
Entity security_camera = create_orbital_camera(&world, (vec3){0, 0, 0}, 20.0f);
Entity minimap_camera = create_top_down_camera(&world, player_position, 50.0f);

// Only one can be main at a time
CameraComponent* main_cam = get_component(&world, main_camera, COMPONENT_CAMERA);
CameraComponent* sec_cam = get_component(&world, security_camera, COMPONENT_CAMERA);
CameraComponent* mini_cam = get_component(&world, minimap_camera, COMPONENT_CAMERA);

main_cam->is_main = true;
sec_cam->is_main = false;  // Render to texture later
mini_cam->is_main = false; // Render to texture later
```

---

## Phase 5: Render System - Step by Step

### Step 5.1: Render Layers and Sorting

```c
typedef struct {
    RenderLayer layer;        // Which layer this object belongs to
    uint32_t sort_order;      // Sort within layer (lower = rendered first)
    bool cast_shadows;        // Does this object cast shadows?
    bool receive_shadows;     // Does this object receive shadows?
} RenderComponent;
```

**Why layers matter:**

- **BACKGROUND**: Skyboxes, far terrain (render first, no depth testing needed)
- **OPAQUE**: Normal solid objects (render front-to-back for early Z rejection)
- **TRANSPARENT**: Glass, particles (render back-to-front for correct blending)
- **UI**: Interface elements (render last, on top of everything)

#### Render Command Collection

```c
// Global render command buffer
typedef struct {
    Entity entity;
    float depth;              // Distance from camera (for sorting)
    RenderComponent* render;
    TransformComponent* transform;
    MeshRendererComponent* mesh_renderer;
} RenderCommand;

static RenderCommand render_commands[MAX_ENTITIES];
static uint32_t render_command_count = 0;

void render_system_collect_commands(World* world) {
    render_command_count = 0;

    // Find main camera for depth calculations
    CameraComponent* camera = get_main_camera(world);
    vec3 camera_pos = get_main_camera_position(world);

    // Required components for rendering
    ComponentSignature required = COMPONENT_SIGNATURE(COMPONENT_TRANSFORM) |
                                  COMPONENT_SIGNATURE(COMPONENT_MESH_RENDERER) |
                                  COMPONENT_SIGNATURE(COMPONENT_RENDER) |
                                  COMPONENT_SIGNATURE(COMPONENT_VISIBILITY);

    for (uint32_t i = 0; i < world->entity_count; i++) {
        Entity entity = world->entities[i];

        if ((world->signatures[entity] & required) != required) {
            continue;  // Skip entities without all required components
        }

        // Get all components
        TransformComponent* transform = get_component(world, entity, COMPONENT_TRANSFORM);
        MeshRendererComponent* mesh_renderer = get_component(world, entity, COMPONENT_MESH_RENDERER);
        RenderComponent* render_comp = get_component(world, entity, COMPONENT_RENDER);
        VisibilityComponent* visibility = get_component(world, entity, COMPONENT_VISIBILITY);

        // Skip invisible objects
```
