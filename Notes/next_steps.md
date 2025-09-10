# Kuta 3D Renderer - Development Roadmap

A structured approach to building a complete 3D rendering framework.

## Table of Contents

- [Phase 1: Scene Management](#phase-1-scene-management)
- [Phase 2: Dynamic Resource System](#phase-2-dynamic-resource-system)
- [Phase 3: Camera API](#phase-3-camera-api)
- [Phase 4: Render Commands](#phase-4-render-commands)
- [Phase 5: Transform System](#phase-5-transform-system)
- [Future Enhancements](#future-enhancements)

---

## Phase 1: Scene Management

> **Priority: CRITICAL** - This is the foundation for all other systems

### Current Problem
- Models are hardcoded by index
- No way to position/transform objects individually
- Scene composition is inflexible

### Implementation Steps

#### 1.1 Create GameObject Structure
```c
typedef struct {
    int id;
    int model_id;
    int texture_id;
    vec3 position;
    vec3 rotation;
    vec3 scale;
    bool visible;
    bool dirty;  // For transform updates
} GameObject;

typedef struct {
    GameObject* objects;
    int count;
    int capacity;
    int next_id;
} Scene;
```

#### 1.2 Add Scene Functions to API
```c
// kuta.h additions
int create_object(int model_id, int texture_id, vec3 position);
void destroy_object(int object_id);
void set_object_position(int object_id, vec3 position);
void set_object_rotation(int object_id, vec3 rotation);
void set_object_scale(int object_id, vec3 scale);
void set_object_visible(int object_id, bool visible);
void clear_scene(void);
```

#### 1.3 Update Render Loop
- Modify `record_command_buffer()` to iterate through scene objects
- Apply per-object transforms via uniform buffers or push constants
- Skip invisible objects

#### 1.4 Expected User Experience
```c
kuta_init(&settings);
int model = load_model("player.glb");
int texture = load_texture("player.png");

int player = create_object(model, texture, (vec3){0, 0, 0});
set_object_scale(player, (vec3){2, 2, 2});

while (running()) {
    begin_frame();
    // Objects render automatically based on scene
    end_frame();
}
```

---

## Phase 2: Dynamic Resource System

> **Priority: HIGH** - Removes hardcoded limitations

### Current Problem
- Fixed array sizes (`MODELS_COUNT`, `TEXTURE_COUNT`)
- No way to load resources at runtime
- Memory waste for unused slots

### Implementation Steps

#### 2.1 Dynamic Resource Arrays
```c
typedef struct {
    GeometryData* geometries;
    TextureData* textures;
    VkBuffer* vertex_buffers;
    VkDeviceMemory* vertex_memory;
    VkBuffer* index_buffers;
    VkDeviceMemory* index_memory;
    
    int model_count;
    int model_capacity;
    int texture_count;
    int texture_capacity;
    
    int* free_model_slots;    // For reusing IDs
    int* free_texture_slots;
} ResourceManager;
```

#### 2.2 Resource Management Functions
```c
// Replace current loading functions
int load_model(const char* filepath);
int load_texture(const char* filepath);
void unload_model(int model_id);
void unload_texture(int texture_id);
bool is_model_valid(int model_id);
bool is_texture_valid(int texture_id);
```

#### 2.3 Update Initialization
- Remove hardcoded model/texture loading from `main()`
- Resources loaded on-demand
- Automatic memory expansion when needed

#### 2.4 Expected User Experience
```c
kuta_init(&settings);

// Load resources dynamically
int player_model = load_model("./models/player.glb");
int enemy_model = load_model("./models/enemy.glb");
int grass_texture = load_texture("./textures/grass.png");

// Resources can be shared across objects
int player1 = create_object(player_model, grass_texture, (vec3){-2, 0, 0});
int player2 = create_object(player_model, grass_texture, (vec3){2, 0, 0});
```

---

## Phase 3: Camera API

> **Priority: MEDIUM** - Essential for user control

### Current Problem
- Camera input hardcoded in callbacks
- No programmatic camera control
- Users can't customize camera behavior

### Implementation Steps

#### 3.1 Camera Control Functions
```c
void set_camera_position(vec3 position);
void set_camera_rotation(float yaw, float pitch);
void set_camera_target(vec3 target);  // Look-at style
void set_camera_fov(float fov_degrees);
void set_camera_near_far(float near, float far);

// For advanced users
Camera* get_camera(void);
void set_camera_mode(CameraMode mode);  // FPS, ORBITAL, FIXED
```

#### 3.2 Camera Modes
```c
typedef enum {
    CAMERA_MODE_FPS,      // Current behavior
    CAMERA_MODE_ORBITAL,  // Rotate around target
    CAMERA_MODE_FIXED,    // No input processing
    CAMERA_MODE_FREE      // Free movement
} CameraMode;
```

#### 3.3 Input System Refactor
- Separate input processing from camera updates
- Allow users to disable automatic input handling
- Add custom input binding support

#### 3.4 Expected User Experience
```c
// Set initial camera
set_camera_position((vec3){0, 5, 10});
set_camera_target((vec3){0, 0, 0});

while (running()) {
    begin_frame();
    
    // Programmatic camera control
    float time = glfwGetTime();
    set_camera_position((vec3){cos(time) * 10, 5, sin(time) * 10});
    
    end_frame();
}
```

---

## Phase 4: Render Commands

> **Priority: MEDIUM** - Flexibility for complex scenes

### Current Problem
- All objects rendered automatically
- No control over render order
- No way to do custom rendering

### Implementation Steps

#### 4.1 Render Queue System
```c
typedef enum {
    RENDER_CMD_DRAW_OBJECT,
    RENDER_CMD_DRAW_MODEL_AT,
    RENDER_CMD_SET_CLEAR_COLOR,
    RENDER_CMD_SET_WIREFRAME
} RenderCommandType;

typedef struct {
    RenderCommandType type;
    union {
        struct { int object_id; } draw_object;
        struct { int model_id; vec3 pos; vec3 rot; vec3 scale; } draw_model;
        struct { float r, g, b, a; } clear_color;
    } data;
} RenderCommand;
```

#### 4.2 Command Functions
```c
void clear_render_queue(void);
void draw_object(int object_id);
void draw_model_at(int model_id, vec3 position, vec3 rotation, vec3 scale);
void set_clear_color(float r, float g, float b);
void set_wireframe_mode(bool enabled);

// Advanced: custom sorting
void set_render_mode(RenderMode mode);  // IMMEDIATE, DEFERRED
```

#### 4.3 Expected User Experience
```c
while (running()) {
    begin_frame();
    
    set_clear_color(0.2, 0.3, 0.8);
    
    // Explicit rendering control
    draw_object(background);
    draw_object(player);
    draw_model_at(bullet_model, bullet_pos, bullet_rot, bullet_scale);
    
    end_frame();
}
```

---

## Phase 5: Transform System

> **Priority: LOW** - Nice-to-have math utilities

### Implementation Steps

#### 5.1 Math Library
```c
// Vector operations
vec3 vec3_add(vec3 a, vec3 b);
vec3 vec3_multiply(vec3 v, float scalar);
float vec3_length(vec3 v);
vec3 vec3_normalize(vec3 v);

// Matrix operations
mat4 mat4_identity(void);
mat4 mat4_translate(vec3 position);
mat4 mat4_rotate_xyz(vec3 rotation);
mat4 mat4_scale(vec3 scale);
mat4 mat4_multiply(mat4 a, mat4 b);
```

#### 5.2 Transform Helpers
```c
void move_object(int object_id, vec3 delta);
void rotate_object(int object_id, vec3 delta_rotation);
void scale_object(int object_id, float scale_factor);
vec3 get_object_forward(int object_id);
```

---

## Future Enhancements

### Rendering Features
- [ ] Multiple lights support
- [ ] Shadow mapping
- [ ] Post-processing pipeline
- [ ] Instanced rendering
- [ ] Level-of-detail (LOD)

### Performance
- [ ] Frustum culling
- [ ] Occlusion culling
- [ ] GPU-driven rendering
- [ ] Multithreaded command recording

### Assets
- [ ] Material system
- [ ] Animation support
- [ ] Texture atlasing
- [ ] Compressed texture formats

### Developer Experience
- [ ] Debug rendering (wireframes, normals)
- [ ] Performance profiler
- [ ] Hot-reloading of shaders/assets
- [ ] Error reporting improvements

---

## Implementation Order

1. **Week 1-2**: Scene Management (Phase 1)
2. **Week 3**: Dynamic Resources (Phase 2) 
3. **Week 4**: Camera API (Phase 3)
4. **Week 5**: Render Commands (Phase 4)
5. **Week 6**: Transform System (Phase 5)

Each phase should be fully tested before moving to the next. Focus on getting Phase 1 working perfectly as it's the foundation for everything else.
