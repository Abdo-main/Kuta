cmake_minimum_required(VERSION 3.16)
project(Kuta C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

set(COMMON_SOURCES src/common/utils.c)
set(CORE_SOURCES
    src/core/window.c
    src/core/vulkan_core.c
    src/core/kuta.c
)
set(GRAPHICS_SOURCES
    src/graphics/renderer.c
    src/graphics/swapchain.c
    src/graphics/buffer_data.c
    src/graphics/descriptors.c
    src/graphics/texture_data.c
    src/graphics/models.c
)

add_library(kuta SHARED
    ${COMMON_SOURCES}
    ${CORE_SOURCES}
    ${GRAPHICS_SOURCES}
)

target_include_directories(kuta PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(kuta PRIVATE
    src/common
    src/core
    src/graphics
    src
)

if(WIN32)
    target_link_libraries(kuta PUBLIC glfw Vulkan::Vulkan)
    
    find_package(cglm CONFIG REQUIRED)
    find_package(assimp CONFIG REQUIRED)
    
    target_link_libraries(kuta PUBLIC cglm::cglm assimp::assimp)
    
elseif(UNIX)
    set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")
    
    if(EXISTS "${THIRD_PARTY_DIR}")
        target_include_directories(kuta PRIVATE
            ${THIRD_PARTY_DIR}/cglm/include
            ${THIRD_PARTY_DIR}/assimp/include
            ${THIRD_PARTY_DIR}/glfw/include
            ${THIRD_PARTY_DIR}/stb
        )
        
        find_library(CGLM_LIB cglm PATHS ${THIRD_PARTY_DIR}/cglm/lib NO_DEFAULT_PATH)
        find_library(ASSIMP_LIB assimp PATHS ${THIRD_PARTY_DIR}/assimp/lib NO_DEFAULT_PATH)
        find_library(GLFW_LIB glfw PATHS ${THIRD_PARTY_DIR}/glfw/lib NO_DEFAULT_PATH)
        
        target_link_libraries(kuta PUBLIC 
            Vulkan::Vulkan 
            ${GLFW_LIB}
            ${CGLM_LIB}
            ${ASSIMP_LIB}
            m
        )
    else()
        target_link_libraries(kuta PUBLIC 
            glfw 
            Vulkan::Vulkan 
            cglm 
            assimp 
            m
        )
    endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/stb")
    target_include_directories(kuta PRIVATE ${CMAKE_SOURCE_DIR}/third_party/stb)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(kuta PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(kuta PRIVATE DEBUG=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(kuta PRIVATE -g -O0)
    endif()
else()
    target_compile_definitions(kuta PRIVATE NDEBUG=1)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(kuta PRIVATE -O3)
    endif()
endif()

install(TARGETS kuta 
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

add_custom_command(TARGET kuta POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/examples/lightDiffuse/include
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kuta> ${CMAKE_SOURCE_DIR}/examples/lightDiffuse/include/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/examples/lightDiffuse/include/
)

# Optional: Add more examples here
# add_custom_command(TARGET kuta POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/examples/anotherExample/include
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kuta> ${CMAKE_SOURCE_DIR}/examples/anotherExample/include/
#     COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/examples/anotherExample/include/
#     COMMENT "Copying library and headers to examples/anotherExample/"
# )
