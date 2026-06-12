cmake_minimum_required(VERSION 3.16)
project(u2_cht C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2IMG REQUIRED SDL2_image)
pkg_check_modules(SDL2MIXER SDL2_mixer)   # 可選:FM Towns CDDA 音樂(無則靜音 fallback)

set(U2_CORE
    src/u2_map.c
    src/u2_mon.c
    src/u2_play.c
    src/u2_render.c
    src/u2_save.c
    src/u2_strings.c
    src/u2_talk.c
    src/u2_tileset.c
    src/u2_dungeon.c
    src/u2_text.c)

add_executable(u2_poc src/poc_main.c ${U2_CORE})
add_executable(u2_demo src/demo_main.c ${U2_CORE})
add_executable(u2_game src/game_main.c ${U2_CORE})
add_executable(u2_sheet src/sheet_main.c ${U2_CORE})
add_executable(u2_dungeon_demo src/dungeon_main.c ${U2_CORE})

# data 層測試 (免 SDL)
add_executable(u2_test
    tests/test_data.c
    src/u2_map.c src/u2_mon.c src/u2_play.c src/u2_talk.c src/u2_strings.c src/u2_save.c)
target_include_directories(u2_test PRIVATE src)
target_compile_options(u2_test PRIVATE -Wall -Wextra)

foreach(tgt u2_poc u2_demo u2_game u2_sheet u2_dungeon_demo)
    target_include_directories(${tgt} PRIVATE
        ${SDL2_INCLUDE_DIRS} ${SDL2TTF_INCLUDE_DIRS} ${SDL2IMG_INCLUDE_DIRS} ${SDL2MIXER_INCLUDE_DIRS})
    target_link_libraries(${tgt} PRIVATE
        ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARIES} ${SDL2IMG_LIBRARIES} ${SDL2MIXER_LIBRARIES} m)
    target_compile_options(${tgt} PRIVATE -Wall -Wextra)
    if(SDL2MIXER_FOUND)
        target_compile_definitions(${tgt} PRIVATE HAVE_SDL_MIXER=1)
    endif()
endforeach()
