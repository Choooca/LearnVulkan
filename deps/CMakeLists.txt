cmake_minimum_required(VERSION 4.30)

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

find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
  message(STATUS "Getting SDL3 from github")
  include_dependency(SDL3 https://github.com/libsdl-org/SDL.git release-3.2.18)
else()
  message(STATUS "Using local SDL3")
endif()


