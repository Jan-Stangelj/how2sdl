cmake_minimum_required(VERSION 3.16)

project("how2sdl" LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# Build SDL as a static library
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

# Doint build stuff we dont need
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("vendor/sdl")

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

target_include_directories("how2sdl" PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries("how2sdl" PRIVATE SDL3::SDL3-static)