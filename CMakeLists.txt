# CI-specific options
if(DEFINED ENV{CI})
    message(STATUS "Building in CI environment")
    set(ENABLE_TESTING ON)
endif()

if (WIN32)
set(CMAKE_TOOLCHAIN_FILE "G:/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()
cmake_minimum_required(VERSION 3.16)
project(Kuta C)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
set(STB_DIR ${CMAKE_SOURCE_DIR}/third_party/stb)

if (WIN32)
include_directories("G:/Dev/cglm-0.9.6/include")
endif()

# Set source and include directories
set(SRC_DIR src/Base)
set(INCLUDE_DIR include)
set(SHADER_DIR shaders)

add_executable(kuta
    ${SRC_DIR}/main.c
    ${SRC_DIR}/window.c
    ${SRC_DIR}/vulkan_core.c
    ${SRC_DIR}/swapchain.c
    ${SRC_DIR}/renderer.c
    ${SRC_DIR}/vertex_data.c
    ${SRC_DIR}/utils.c
    ${SRC_DIR}/descriptors.c
    ${SRC_DIR}/textures.c
    ${SRC_DIR}/models.c
    ${SRC_DIR}/camera.c
    ${SRC_DIR}/kuta.c
)

# Platform-specific settings
if(WIN32)
    # Windows-specific
    target_link_libraries(kuta PRIVATE glfw Vulkan::Vulkan)
    # If using MinGW, you might need to link extra libraries:
    # target_link_libraries(kuta PRIVATE -lgdi32 -luser32 -lkernel32)
elseif(UNIX)
    # Linux-specific
    target_link_libraries(kuta PRIVATE glfw Vulkan::Vulkan cglm m assimp)
endif()

# Include directories (CMake handles Vulkan/cglm includes if found)
target_include_directories(kuta PRIVATE
    ${SRC_DIR}
    ${INCLUDE_DIR}
)

# Copy shaders to build directory (cross-platform)
file(GLOB SHADER_FILES "${SHADER_DIR}/*.spv")
foreach(SHADER_FILE ${SHADER_FILES})
    add_custom_command(TARGET kuta POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SHADER_FILE} $<TARGET_FILE_DIR:kuta>
    )
endforeach()

# Optional: Install target
install(TARGETS kuta DESTINATION bin)
