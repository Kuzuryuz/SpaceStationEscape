cmake_minimum_required(VERSION 3.16)
project(SpaceStationEscape)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.2.5
)
FetchContent_MakeAvailable(assimp)

add_executable(SpaceStationEscape
    src/main.cpp
    "src/graphics/Shader.cpp"
    "src/graphics/StaticModel.cpp"
    "src/graphics/Texture.cpp"
    "src/graphics/AnimatedCharacter.cpp"
    "src/graphics/AnimatedObjectPlayer.cpp"
    "src/audio/AudioEngine.cpp"
    "src/world/World.cpp"
    "src/world/SpaceStationScene.cpp"
    "src/graphics/StaticModel.h"
    "src/graphics/Texture.h"
    "src/graphics/AnimatedCharacter.h"
    "src/graphics/AnimatedObjectPlayer.h"
    "src/audio/AudioEngine.h"
    "src/world/SpaceStationScene.h"
    "src/GameState.h")

target_include_directories(SpaceStationEscape PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(SpaceStationEscape PRIVATE
    PROJECT_ROOT="."
)

target_link_libraries(SpaceStationEscape PRIVATE
    glfw
    glad
    glm
    assimp
    windowscodecs
)
