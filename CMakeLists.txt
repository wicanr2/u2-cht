cmake_minimum_required(VERSION 3.16)
project(u2_cht C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2IMG REQUIRED SDL2_image)

add_executable(u2_poc
    src/poc_main.c
    src/u2_map.c
    src/u2_render.c
    src/u2_text.c)

target_include_directories(u2_poc PRIVATE
    ${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS} ${SDL2IMG_INCLUDE_DIRS})
target_link_libraries(u2_poc PRIVATE
    ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES} ${SDL2IMG_LIBRARIES})
target_compile_options(u2_poc PRIVATE -Wall -Wextra)
