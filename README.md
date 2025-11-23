# Kuta Game Engine

A lightweight, code-first 3D should be game engine built with Vulkan.

## Demo

https://github.com/user-attachments/assets/345b0d02-4e3b-44c0-9b34-388aa100250b

*Early prototype - Light diffuse rendering example*

## About

Kuta is a personal should be game engine project focused on minimalism and developer control. Unlike commercial game engines with heavy abstraction layers, Kuta prioritizes:

- **Lightweight architecture** - Minimal overhead, maximum control
- **Code-first workflow** - Most work happens in your code editor
- **Low abstraction** - Direct access to rendering pipeline and engine internals

>  **Status**: Very early development - not even at prototype stage

## Features (Current)

-  Vulkan-based rendering backend
-  Window management (GLFW)
-  3D model loading (Assimp)
-  Texture support
-  Basic lighting (diffuse)
-  Descriptor system
-  Buffer management

## Planned Features

### Immediate Priority
-  **PBR Materials** 
-  **Normal mapping**
-  **Shadow Mapping**

## Future
-  GUI level editor (simple, code-complementary)
-  Scene management
-  Physics integration
-  Audio system

## Building

### Prerequisites

**Linux:**
- CMake 3.16+
- GCC or Clang with C17 support
- Vulkan SDK
- GLFW3
- cglm
- Assimp

### Build Instructions

```bash
# Clone the repository
https://github.com/Abdo-main/Kuta.git
cd kuta

# Build the engine library
use the build script for your system


# The library will be automatically copied to examples/*/include/
```

### Windows Setup (First Time)

1. **Prerequisites:**
   - Git
   - CMake 3.16+
   - MinGW-w64 (for GCC)
   - Vulkan SDK

2. **Automated Setup:**
```bash
   setup-windows.bat
```
   This will:
   - Clone and bootstrap vcpkg
   - Install all dependencies automatically (from vcpkg.json)
   - Build the project

3. **Manual Setup (if you prefer):**
```bash
   git clone https://github.com/microsoft/vcpkg.git
   vcpkg\bootstrap-vcpkg.bat
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake
   cmake --build build
```


## Running Examples

Each example is self-contained with its own Makefile:

```bash
cd examples/lightDiffuse
make
./lightDiffuse
```

### Available Examples

- **lightDiffuse** - Basic diffuse lighting with 3D models

## Using Kuta in Your Project

1. Build the engine (generates `libkuta.so` or `kuta.dll`)
2. Copy the library and headers to your project
3. Link against kuta, cglm, and math library:

## Development Goals

This engine is built primarily for personal game development with these priorities:

1. **Understanding over convenience** - Learn how modern rendering works
2. **Performance** - Lightweight design, minimal overhead
3. **Flexibility** - Easy to modify and extend
4. **Simplicity** - Clean, readable C code

## Contributing

This is primarily a personal learning project, but suggestions and feedback are welcome! Feel free to open issues for discussion.

## License

GPL-3.0
