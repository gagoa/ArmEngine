cmake_minimum_required(VERSION 3.16)

project(game_engine_gagoa LANGUAGES C CXX)

# Require C++17 across all platforms
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Lua C sources (compiled as C)
file(GLOB LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua/*.c")
add_library(lua_lib STATIC ${LUA_SOURCES})
target_include_directories(lua_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua)

# All game sources (match vcxproj / autograder Makefile)
set(GAME_SOURCES
    main.cpp
    Engine.cpp
    EngineUtils.cpp
    SceneDB.cpp
    TemplateDB.cpp
    ComponentDB.cpp
    ParticleSystem.cpp
)

add_executable(game_engine ${GAME_SOURCES})

# Include directories: project root, third_party, lua, rapidjson (header-only usage)
target_include_directories(game_engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua
        ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson-1.1.0/include
)

target_link_libraries(game_engine PRIVATE lua_lib)

# --- Platform-specific: SDL2, SDL2_image, SDL2_mixer, SDL2_ttf ---
if(WIN32)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    target_link_libraries(game_engine PRIVATE
        SDL2::SDL2
        SDL2_image::SDL2_image
        SDL2_mixer::SDL2_mixer
        SDL2_ttf::SDL2_ttf
    )
    target_include_directories(game_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/windows_sdl_include
    )
elseif(APPLE)
    set(SDL2_FW "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib/SDL2.framework")
    set(SDL2_IMG_FW "${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/lib/SDL2_image.framework")
    set(SDL2_MIX_FW "${CMAKE_CURRENT_SOURCE_DIR}/SDL_mixer/lib/SDL2_mixer.framework")
    set(SDL2_TTF_FW "${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf/lib/SDL2_ttf.framework")
    if(NOT EXISTS "${SDL2_FW}" OR NOT EXISTS "${SDL2_IMG_FW}" OR NOT EXISTS "${SDL2_MIX_FW}" OR NOT EXISTS "${SDL2_TTF_FW}")
        message(FATAL_ERROR "macOS: SDL2 frameworks not found under SDL2/, SDL_image/, SDL_mixer/, SDL2_ttf/.")
    endif()
    target_include_directories(game_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL2
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL_image
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL_mixer
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf
    )
    target_link_libraries(game_engine PRIVATE
        "-framework SDL2"
        "-framework SDL2_image"
        "-framework SDL2_mixer"
        "-framework SDL2_ttf"
    )
    target_link_directories(game_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL_mixer/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/SDL2_ttf/lib
    )
elseif(UNIX AND NOT APPLE)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    target_link_libraries(game_engine PRIVATE
        SDL2::SDL2
        SDL2_image::SDL2_image
        SDL2_mixer::SDL2_mixer
        SDL2_ttf::SDL2_ttf
    )
    target_include_directories(game_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/windows_sdl_include
    )
    set_target_properties(game_engine PROPERTIES OUTPUT_NAME game_engine_linux)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

if(MSVC)
    target_compile_options(game_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(game_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    main.cpp
    Engine.cpp
    Engine.hpp
    EngineUtils.cpp
    EngineUtils.hpp
    SceneDB.cpp
    SceneDB.hpp
    TemplateDB.cpp
    TemplateDB.hpp
    ComponentDB.cpp
    ComponentDB.hpp
    ParticleSystem.cpp
    ParticleSystem.hpp
)

include(GNUInstallDirs)
install(TARGETS game_engine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
