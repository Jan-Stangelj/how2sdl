cmake_minimum_required(VERSION 3.16)

project("how2sdl" LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# SDL3
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_UNINSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("vendor/sdl" EXCLUDE_FROM_ALL)

# SDL3_image
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)

set(SDL3IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory("vendor/sdl_image" EXCLUDE_FROM_ALL)

file(GLOB_RECURSE "SOURCES" CONFIGURE_DEPENDS "src/*.cpp")
add_executable("how2sdl" "${SOURCES}")

# Enable ALL warnings
if(MSVC)
  target_compile_options(how2sdl PRIVATE /W4)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(how2sdl PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(how2sdl PRIVATE -Wall -Wextra -Wpedantic)
endif()


# hello nigga

target_include_directories("how2sdl" PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries("how2sdl" PRIVATE SDL3::SDL3-static SDL3_image::SDL3_image-static)

# Compile the shaders
find_program(GLSLC glslc REQUIRED)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SHADER_FILES
    "${SHADER_SOURCE_DIR}/*"
)

set(SPIRV_FILES)

foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)

    set(SPIRV
        "${SHADER_BINARY_DIR}/${FILE_NAME}.spv"
    )

    add_custom_command(
        OUTPUT ${SPIRV}

        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${SHADER_BINARY_DIR}

        COMMAND ${GLSLC}
            ${SHADER}
            -o ${SPIRV}

        DEPENDS ${SHADER}

        COMMENT "Compiling shader ${FILE_NAME}"
    )

    list(APPEND SPIRV_FILES ${SPIRV})
endforeach()

add_custom_target(shaders
    DEPENDS ${SPIRV_FILES}
)

add_dependencies("how2sdl" shaders)