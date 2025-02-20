# preferred Clang versions
set(CLANG_VERSIONS 19 18 17)

set(FOUND_CLANG FALSE)
foreach(VERS ${CLANG_VERSIONS})
    find_program(CLANG_BIN NAMES clang-${VERS})
    find_program(CLANGPP_BIN NAMES clang++-${VERS})
    if(CLANG_BIN AND CLANGPP_BIN)
        message(STATUS "Found clang-${VERS} in PATH; using it as compiler.")
        set(CMAKE_C_COMPILER "${CLANG_BIN}" CACHE FILEPATH "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "${CLANGPP_BIN}" CACHE FILEPATH "C++ compiler" FORCE)
        set(FOUND_CLANG TRUE)
        break()
    endif()
endforeach()

# If no versioned Clang was found, try unversioned clang/clang++
if(NOT FOUND_CLANG)
    find_program(CLANG_BIN NAMES clang)
    find_program(CLANGPP_BIN NAMES clang++)
    if(CLANG_BIN AND CLANGPP_BIN)
        message(STATUS "Found clang in PATH; using it as compiler.")
        set(CMAKE_C_COMPILER "${CLANG_BIN}" CACHE FILEPATH "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "${CLANGPP_BIN}" CACHE FILEPATH "C++ compiler" FORCE)
        set(FOUND_CLANG TRUE)
    endif()
endif()

if(NOT FOUND_CLANG)
    message(FATAL_ERROR "No Clang found. Falling back to system default compiler.")
endif()
