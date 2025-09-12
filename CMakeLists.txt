# Only set vcpkg toolchain if we're on Windows AND it exists AND it's not already set
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

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CI environment detection
if(DEFINED ENV{CI})
    message(STATUS "Building in CI environment")
    set(CI_BUILD ON)
endif()

# Find required packages
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)


if(WIN32)
    # Try multiple possible cglm locations
    set(CGLM_PATHS
        "G:/Dev/cglm-0.9.6/include"
        "C:/cglm/include"
        "${CMAKE_SOURCE_DIR}/third_party/cglm/include"
    )
    
    foreach(CGLM_PATH ${CGLM_PATHS})
        if(EXISTS ${CGLM_PATH})
            include_directories(${CGLM_PATH})
            message(STATUS "Using cglm from: ${CGLM_PATH}")
            set(CGLM_FOUND TRUE)
            break()
        endif()
    endforeach()
    
    if(NOT CGLM_FOUND AND NOT CI_BUILD)
        message(WARNING "cglm not found in expected locations")
    endif()
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
if(SHADER_FILES)
    foreach(SHADER_FILE ${SHADER_FILES})
        add_custom_command(TARGET kuta POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER_FILE} $<TARGET_FILE_DIR:kuta>
        )
    endforeach()
    message(STATUS "Found ${CMAKE_MATCH_COUNT} shader files to copy")
else()
    message(WARNING "No .spv shader files found in ${SHADER_DIR}")
endif()

# Optional: Install target
install(TARGETS kuta DESTINATION bin)
