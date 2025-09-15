if(WIN32 AND NOT CMAKE_TOOLCHAIN_FILE)
    set(VCPKG_PATHS
        "G:/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
    )
    
    foreach(VCPKG_PATH ${VCPKG_PATHS})
        if(EXISTS ${VCPKG_PATH})
            set(CMAKE_TOOLCHAIN_FILE ${VCPKG_PATH})
            message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
            break()
        endif()
    endforeach()
    
    if(NOT CMAKE_TOOLCHAIN_FILE)
        message(STATUS "No vcpkg toolchain found, using system libraries")
    endif()
endif()

cmake_minimum_required(VERSION 3.16)
project(Kuta C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEFINED ENV{CI})
    message(STATUS "Building in CI environment")
    set(CI_BUILD ON)
endif()

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

set(COMMON_DIR src/common)
set(CORE_DIR src/core)
set(GRAPHICS_DIR src/graphics)
set(INCLUDE_DIR include)
set(SHADER_DIR shaders)
set(EXAMPLES_DIR examples)

set(COMMON_SOURCES
    ${COMMON_DIR}/utils.c
)

set(CORE_SOURCES
    ${CORE_DIR}/window.c
    ${CORE_DIR}/vulkan_core.c
    ${CORE_DIR}/kuta.c
)

set(GRAPHICS_SOURCES
    ${GRAPHICS_DIR}/renderer.c
    ${GRAPHICS_DIR}/swapchain.c
    ${GRAPHICS_DIR}/buffer_data.c
    ${GRAPHICS_DIR}/descriptors.c
    ${GRAPHICS_DIR}/texture_data.c
    ${GRAPHICS_DIR}/models.c
    ${GRAPHICS_DIR}/camera.c
)

add_executable(kuta
    ${COMMON_SOURCES}
    ${CORE_SOURCES}
    ${GRAPHICS_SOURCES}
    ${EXAMPLES_DIR}/main.c
)

# Platform-specific settings
if(WIN32)
    # Windows-specific using vcpkg or manual paths
    target_link_libraries(kuta PRIVATE glfw Vulkan::Vulkan)
    
    # Try multiple possible cglm locations for Windows
    set(CGLM_PATHS
        "G:/Dev/cglm-0.9.6/include"
        "C:/cglm/include"
        "${CMAKE_SOURCE_DIR}/third_party/cglm/include"
    )
    
    foreach(CGLM_PATH ${CGLM_PATHS})
        if(EXISTS ${CGLM_PATH})
            target_include_directories(kuta PRIVATE ${CGLM_PATH})
            message(STATUS "Using cglm from: ${CGLM_PATH}")
            set(CGLM_FOUND TRUE)
            break()
        endif()
    endforeach()
    
    if(NOT CGLM_FOUND AND NOT CI_BUILD)
        message(WARNING "cglm not found in expected locations")
    endif()
    
    # Try to find cglm library (if using vcpkg)
    find_package(cglm CONFIG QUIET)
    if(cglm_FOUND)
        target_link_libraries(kuta PRIVATE cglm::cglm)
        message(STATUS "Using cglm via vcpkg")
    endif()
    
    # Try to find assimp
    find_package(assimp CONFIG QUIET)
    if(assimp_FOUND)
        target_link_libraries(kuta PRIVATE assimp::assimp)
        message(STATUS "Using assimp via vcpkg")
    endif()
    
elseif(UNIX)
    # Linux-specific - check for third_party directory first, then system
    set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")
    
    if(EXISTS "${THIRD_PARTY_DIR}")
        message(STATUS "Using third_party libraries")
        
        # Add third_party include directories
        target_include_directories(kuta PRIVATE
            ${THIRD_PARTY_DIR}/cglm/include
            ${THIRD_PARTY_DIR}/assimp/include
            ${THIRD_PARTY_DIR}/glfw/include
            ${THIRD_PARTY_DIR}/stb
        )
        
        # Link third_party libraries
        find_library(CGLM_LIB cglm PATHS ${THIRD_PARTY_DIR}/cglm/lib NO_DEFAULT_PATH)
        find_library(ASSIMP_LIB assimp PATHS ${THIRD_PARTY_DIR}/assimp/lib NO_DEFAULT_PATH)
        find_library(GLFW_LIB glfw PATHS ${THIRD_PARTY_DIR}/glfw/lib NO_DEFAULT_PATH)
        
        target_link_libraries(kuta PRIVATE 
            Vulkan::Vulkan 
            ${GLFW_LIB}
            ${CGLM_LIB}
            ${ASSIMP_LIB}
            m  # Math library
        )
    else()
        # Fallback to system libraries
        message(STATUS "Using system libraries")
        target_link_libraries(kuta PRIVATE glfw Vulkan::Vulkan cglm m assimp)
    endif()
endif()

# Include directories
target_include_directories(kuta PRIVATE
    ${COMMON_DIR}
    ${CORE_DIR}
    ${GRAPHICS_DIR}
    ${INCLUDE_DIR}
    src  # For internal includes like #include "common/internal_types.h"
)

# Add STB include for both Windows and Linux if third_party exists
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/stb")
    target_include_directories(kuta PRIVATE ${CMAKE_SOURCE_DIR}/third_party/stb)
    message(STATUS "Using STB from third_party")
endif()

# Compiler-specific flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kuta PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(kuta PRIVATE DEBUG=1)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        target_compile_options(kuta PRIVATE -g -O0)
    endif()
else()
    target_compile_definitions(kuta PRIVATE NDEBUG=1)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        target_compile_options(kuta PRIVATE -O3)
    endif()
endif()

# Optional: Install target
install(TARGETS kuta DESTINATION bin)
