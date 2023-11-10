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

# mimalloc
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MI_OVERRIDE ON CACHE BOOL "" FORCE)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


add_subdirectory(${APH_EXTERNAL_DIR})
add_subdirectory(${APH_EXTERNAL_DIR}/mimalloc EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/vma EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/glfw EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/volk EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/spirv-cross EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/imgui EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/slang EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/tinygltf EXCLUDE_FROM_ALL)
add_subdirectory(${APH_EXTERNAL_DIR}/unordered_dense EXCLUDE_FROM_ALL)

find_package(PkgConfig REQUIRED)
pkg_check_modules(xcb REQUIRED IMPORTED_TARGET xcb)
