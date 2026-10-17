cmake_minimum_required(VERSION 3.20)
project(untitled5 CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.2
)
FetchContent_MakeAvailable(SDL2)

add_executable(main main.cpp
)

# Schimbăm aici din SDL2::SDL2 în SDL2-static:
target_link_libraries(main PRIVATE SDL2::SDL2-static)