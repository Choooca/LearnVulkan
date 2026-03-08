cmake_minimum_required(VERSION 4.1.2)

set(CMAKE_C_STANDARD 23)

include(FetchContent)

function(include_dependency lib_name git_url git_tag)
    FetchContent_Declare(
    ${lib_name}
    GIT_REPOSITORY  ${git_url}
    GIT_TAG         ${git_tag}
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    )

    FetchContent_MakeAvailable(${lib_name})
endfunction()

set(GLAD_VERSION 2.0.5)

set(VULKAN_VER_MAJOR 1)
set(VULKAN_VER_MINOR 3)
set(VULKAN_VER_PATCH 278)
set(VULKAN_VERSION ${VULKAN_VER_MAJOR}.${VULKAN_VER_MINOR}.${VULKAN_VER_PATCH})

message(STATUS "Include GLFW")

find_package(glfw QUIET)
if(NOT glfw_FOUND)
    message(STATUS "Getting glfw from github")
    include_dependency(glfw https://github.com/glfw/glfw.git 3.4)
else()
    message(STATUS "Using local glfw")
endif()

message(STATUS "Include GLAD")

FetchContent_Declare(
  libglad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v${GLAD_VERSION}
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
  SOURCE_SUBDIR  cmake
)
FetchContent_MakeAvailable(libglad)
glad_add_library(glad_vulkan STATIC REPRODUCIBLE LOADER API vulkan=${VULKAN_VER_MAJOR}.${VULKAN_VER_MINOR})

message(STATUS "Include Vulkan C Headers")

include_dependency(vulkanheaders https://github.com/KhronosGroup/Vulkan-Headers.git v${VULKAN_VERSION})

message(STATUS "Include Vulkan C++ Headers")

include_dependency(vulkanhpp https://github.com/KhronosGroup/Vulkan-Hpp.git v${VULKAN_VERSION})

add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE ${vulkanhpp_SOURCE_DIR})
target_link_libraries(vulkan INTERFACE Vulkan::Headers glfw glad_vulkan)