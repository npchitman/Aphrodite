# spirv-headers
set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "" FORCE)

# spirv-tools
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)

# spirv-cross
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

# glslang
set(ENABLE_GLSLANG_INSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)

# shaderc
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
set(SHADERC_THIRD_PARTY_ROOT_DIR "${CMAKE_SOURCE_DIR}/external" CACHE STRING "Third party path." FORCE)

# mimalloc
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


add_subdirectory(external)
add_subdirectory(external/mimalloc EXCLUDE_FROM_ALL)
add_subdirectory(external/glfw EXCLUDE_FROM_ALL)
add_subdirectory(external/reckless EXCLUDE_FROM_ALL)
add_subdirectory(external/imgui EXCLUDE_FROM_ALL)
add_subdirectory(external/volk EXCLUDE_FROM_ALL)
add_subdirectory(external/spirv-cross EXCLUDE_FROM_ALL)
add_subdirectory(external/shaderc EXCLUDE_FROM_ALL)
# add_subdirectory(external/mmgr EXCLUDE_FROM_ALL)

find_library(vulkan_lib NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
if (vulkan_lib)
    message(STATUS "Using bundled Vulkan library version")
else()
    message(FATAL_ERROR "Could not find Vulkan library!")
endif()
