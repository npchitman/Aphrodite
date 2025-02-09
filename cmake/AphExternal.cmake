include(CPM)

set(VK_SDK_VERSION 1.3.275)

CPMAddPackage(
  NAME tinygltf
  GITHUB_REPOSITORY syoyo/tinygltf
  VERSION 2.8.18
)

CPMAddPackage(
  NAME tracy
  GITHUB_REPOSITORY wolfpld/tracy
  VERSION 0.10
)

CPMAddPackage(
  NAME glm
  GITHUB_REPOSITORY g-truc/glm
  GIT_TAG 1.0.1
  OPTIONS
      "GLM_TEST_ENABLE OFF"
)

CPMAddPackage(
  NAME unordered_dense
  GITHUB_REPOSITORY martinus/unordered_dense
  VERSION 4.1.2
)

CPMAddPackage(
  NAME backward-cpp
  GITHUB_REPOSITORY bombela/backward-cpp
  GIT_TAG master
)

CPMAddPackage(
  NAME vma
  GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
  GIT_TAG master
)

CPMAddPackage(
  NAME mimalloc
  GITHUB_REPOSITORY microsoft/mimalloc
  VERSION 2.1.2
  OPTIONS
      "MI_BUILD_SHARED OFF"
      "MI_BUILD_OBJECT OFF"
      "MI_BUILD_STATIC ON"
      "MI_BUILD_TESTS OFF"
      "MI_USE_CXX ON"
      "MI_OVERRIDE ON"
)

CPMAddPackage(
  NAME stb
  GITHUB_REPOSITORY nothings/stb
  GIT_TAG master
)
add_library(stb INTERFACE IMPORTED)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

CPMAddPackage(
  NAME slang
  URL https://github.com/shader-slang/slang/releases/download/v2024.14.5/slang-2024.14.5-linux-x86_64.tar.gz
  VERSION v2024.14.5
  DOWNLOAD_ONLY YES
)
add_library(slang SHARED IMPORTED)
set_property(TARGET slang PROPERTY IMPORTED_LOCATION ${slang_SOURCE_DIR}/lib/libslang.so)
target_include_directories(slang INTERFACE ${slang_SOURCE_DIR}/include)
target_link_libraries(slang INTERFACE ${slang_SOURCE_DIR}/lib/libslang.so)


CPMAddPackage(
  NAME spirv-cross
  GITHUB_REPOSITORY KhronosGroup/SPIRV-Cross
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
  OPTIONS
      "SPIRV_CROSS_CLI OFF"
      "SPIRV_CROSS_ENABLE_TESTS OFF"
)

CPMAddPackage(
  NAME vulkan-headers
  GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
  DOWNLOAD_ONLY YES
)
add_library(vulkan-registry INTERFACE IMPORTED)
target_include_directories(vulkan-registry INTERFACE ${vulkan-headers_SOURCE_DIR}/include)

CPMAddPackage(
  NAME volk
  GITHUB_REPOSITORY zeux/volk
  GIT_TAG vulkan-sdk-${VK_SDK_VERSION}
)

# wsi backend
set(VALID_WSI_BACKENDS Auto GLFW SDL2)
if(NOT (APH_WSI_BACKEND IN_LIST VALID_WSI_BACKENDS))
    message(FATAL_ERROR "Wrong value passed for APH_WSI_BACKEND, use one of: Auto, GLFW, SDL2")
endif()

if(APH_WSI_BACKEND STREQUAL "Auto")
  message("WSI backend is set to 'Auto', choose the SDL2 by default")
  set(APH_WSI_BACKEND "SDL2")
endif()

if(APH_WSI_BACKEND STREQUAL "GLFW")
  set(APH_WSI_BACKEND_IS_GLFW "ON")
    CPMAddPackage(
            NAME GLFW
            GITHUB_REPOSITORY glfw/glfw
            GIT_TAG 3.3.9
            OPTIONS
                "GLFW_BUILD_TESTS OFF"
                "GLFW_BUILD_EXAMPLES OFF"
                "GLFW_BULID_DOCS OFF"
                "GLFW_INSTALL OFF"
    )
    if(NOT GLFW_ADDED)
        message(FATAL_ERROR "GLFW3 library not found!")
    endif()
elseif(APH_WSI_BACKEND STREQUAL "SDL2")
  set(APH_WSI_BACKEND_IS_SDL2 "ON")
    CPMAddPackage(
      NAME SDL2
      VERSION 2.30.9
      URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz
      OPTIONS
          "SDL_SHARED OFF"
          "SDL_STATIC ON"
          "SDL_TEST OFF"
    )
    if(NOT SDL2_ADDED)
        message(FATAL_ERROR "SDL2 library not found!")
    endif()
endif()

CPMAddPackage(
  NAME imgui
  GITHUB_REPOSITORY ocornut/imgui
  GIT_TAG docking
  DOWNLOAD_ONLY YES
)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp

  $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp>
  $<$<BOOL:${APH_WSI_BACKEND_IS_SDL2}>:${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp>
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui
  PRIVATE
  $<$<BOOL:${APH_WSI_BACKEND_IS_GLFW}>:glfw>
  $<$<BOOL:${APH_WSI_BACKEND_IS_SDL2}>:SDL2::SDL2-static>
)
