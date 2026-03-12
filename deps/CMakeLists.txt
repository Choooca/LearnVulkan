cmake_minimum_required(VERSION 4.1.2)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

function(include_dependency lib_name git_url git_tag source_subdir)
    FetchContent_Declare(
    ${lib_name}
    GIT_REPOSITORY  ${git_url}
    GIT_TAG         ${git_tag}
    GIT_SHALLOW     TRUE
    GIT_PROGRESS    TRUE
    SOURCE_SUBDIR   ${source_subdir}
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
    include_dependency(glfw https://github.com/glfw/glfw.git 3.4 "")
else()
    message(STATUS "Using local glfw")
endif()

message(STATUS "Include Vulkan C Headers")

include_dependency(vulkanheaders https://github.com/KhronosGroup/Vulkan-Headers.git v${VULKAN_VERSION} "")

message(STATUS "Include Vulkan C++ Headers")

include_dependency(vulkanhpp https://github.com/KhronosGroup/Vulkan-Hpp.git v${VULKAN_VERSION} "")

message(STATUS "Include glm headers")

include_dependency(glm https://github.com/g-truc/glm.git 1.0.3 "")

message(STATUS "Include spdlog")

include_dependency(spdlog https://github.com/gabime/spdlog.git v1.17.0 "")

add_library(deps INTERFACE)
target_include_directories(deps INTERFACE ${vulkanhpp_SOURCE_DIR})

find_package(Vulkan REQUIRED)
target_link_libraries(deps INTERFACE Vulkan::Headers Vulkan::Vulkan glfw glm::glm spdlog::spdlog)