################################
# SDL2
################################
if(PREFER_SYSTEM_LIBRARIES)
    find_package(SDL2)
    if(SDL2_FOUND)
        add_library(SDL2 ALIAS SDL2::SDL2)
        add_library(SDL2-static ALIAS SDL2::SDL2)
        message(STATUS "Use system library: SDL2")
    else()
        message(WARNING "System library SDL2 not found")
    endif()
endif()


if(BUILD_SDL AND NOT EMSCRIPTEN AND NOT PREFER_SYSTEM_LIBRARIES)

    if(WIN32)
        set(HAVE_LIBC TRUE)
    endif()

    add_subdirectory(${THIRDPARTY_DIR}/sdl2)

    if(MSVC)
        # CMake policy CMP0079
        # This allows linking libraries to targets not built in the current directory.
        cmake_policy(SET CMP0079 NEW)

        target_link_libraries(SDL2 PRIVATE
            libcmt.lib
            libvcruntime.lib
            libucrt.lib
        )
    endif()

endif()

################################
# SDL2 standalone cart player
################################

if(BUILD_SDL AND BUILD_PLAYER)

    add_executable(player-sdl WIN32 ${CMAKE_SOURCE_DIR}/src/system/sdl/player.c)

    target_include_directories(player-sdl PRIVATE
        ${THIRDPARTY_DIR}/sdl2/include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src)

    if(MINGW)
        target_link_libraries(player-sdl PRIVATE mingw32)
        target_link_options(player-sdl PRIVATE -static)
    endif()

    target_link_libraries(player-sdl PRIVATE uli78core SDL2main)

    if(BUILD_STATIC)
        target_link_libraries(player-sdl PRIVATE SDL2-static)
    else()
        target_link_libraries(player-sdl PRIVATE SDL2)
    endif()
endif()

################################
# SDL2 local export player
################################

if(BUILD_SDL AND BUILD_PLAYER)

    add_executable(localplayer-sdl WIN32 ${CMAKE_SOURCE_DIR}/src/system/sdl/localplayer.c)



    target_include_directories(localplayer-sdl PRIVATE
        ${THIRDPARTY_DIR}/sdl2/include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${THIRDPARTY_DIR}/zip/src
        ${CMAKE_SOURCE_DIR}/src/studio/screens)

    if(MINGW)
        target_link_libraries(localplayer-sdl PRIVATE mingw32)
        target_link_options(localplayer-sdl PRIVATE -static)
    endif()

    target_link_libraries(localplayer-sdl PRIVATE uli78core SDL2main)

    if(BUILD_STATIC)
        target_link_libraries(localplayer-sdl PRIVATE SDL2-static)
        if(WIN32)
            target_link_libraries(localplayer-sdl PRIVATE
                imm32 version winmm gdi32 ole32 oleaut32 shell32 setupapi user32 advapi32
            )
        endif()
    else()
        target_link_libraries(localplayer-sdl PRIVATE SDL2)
    endif()
endif()


################################
# SDL GPU
################################

if(BUILD_SDLGPU)

set(SDLGPU_DIR ${THIRDPARTY_DIR}/sdl-gpu/src)
set(SDLGPU_SRC
    ${SDLGPU_DIR}/renderer_GLES_2.c
    ${SDLGPU_DIR}/SDL_gpu.c
    ${SDLGPU_DIR}/SDL_gpu_matrix.c
    ${SDLGPU_DIR}/SDL_gpu_renderer.c
    ${SDLGPU_DIR}/externals/stb_image/stb_image.c
    ${SDLGPU_DIR}/externals/stb_image_write/stb_image_write.c
)


    list(APPEND SDLGPU_SRC
        ${SDLGPU_DIR}/renderer_GLES_1.c
        ${SDLGPU_DIR}/renderer_GLES_3.c
        ${SDLGPU_DIR}/renderer_OpenGL_1.c
        ${SDLGPU_DIR}/renderer_OpenGL_1_BASE.c
        ${SDLGPU_DIR}/renderer_OpenGL_2.c
        ${SDLGPU_DIR}/renderer_OpenGL_3.c
        ${SDLGPU_DIR}/renderer_OpenGL_4.c
        ${SDLGPU_DIR}/SDL_gpu_shapes.c
        ${SDLGPU_DIR}/externals/glew/glew.c
    )


add_library(sdlgpu STATIC ${SDLGPU_SRC})


target_compile_definitions(sdlgpu PRIVATE GLEW_STATIC SDL_GPU_DISABLE_GLES SDL_GPU_DISABLE_OPENGL_3 SDL_GPU_DISABLE_OPENGL_4)
target_include_directories(sdlgpu PUBLIC ${THIRDPARTY_DIR}/sdl-gpu/include)
target_include_directories(sdlgpu PRIVATE ${THIRDPARTY_DIR}/sdl-gpu/src/externals/glew)
target_include_directories(sdlgpu PRIVATE ${THIRDPARTY_DIR}/sdl-gpu/src/externals/glew/GL)
target_include_directories(sdlgpu PRIVATE ${THIRDPARTY_DIR}/sdl-gpu/src/externals/stb_image)
target_include_directories(sdlgpu PRIVATE ${THIRDPARTY_DIR}/sdl-gpu/src/externals/stb_image_write)

if(WIN32)
    target_link_libraries(sdlgpu opengl32)
endif()

if(NOT EMSCRIPTEN)
    if(BUILD_STATIC)
        target_link_libraries(sdlgpu SDL2-static)
    else()
        target_link_libraries(sdlgpu SDL2)
    endif()
endif()

endif()

################################
# ULI-78 app
################################

if(BUILD_SDL)

    set(ULI78_SRC src/system/sdl/main.c)

    if(WIN32)
        configure_file("${PROJECT_SOURCE_DIR}/build/windows/uli78.rc.in" "${PROJECT_SOURCE_DIR}/build/windows/uli78.rc")
        set(ULI78_SRC ${ULI78_SRC} "${PROJECT_SOURCE_DIR}/build/windows/uli78.rc")

        add_executable(${ULI78_TARGET} ${SYSTEM_TYPE} ${ULI78_SRC})
    else()
        add_executable(${ULI78_TARGET} ${ULI78_SRC})
    endif()

    if(MINGW)
        target_link_libraries(${ULI78_TARGET} mingw32)
        target_link_options(${ULI78_TARGET} PRIVATE -static -mconsole)
    endif()

    target_link_libraries(${ULI78_TARGET} SDL2main)

    target_link_libraries(${ULI78_TARGET} uli78studio)

    if(BUILD_TOUCH_INPUT)
        target_compile_definitions(${ULI78_TARGET} PRIVATE TOUCH_INPUT_SUPPORT)
    endif()



    if(BUILD_SDLGPU)
        target_link_libraries(${ULI78_TARGET} sdlgpu)
    else()
        if(BUILD_STATIC)
            target_link_libraries(${ULI78_TARGET} SDL2-static)
        else()
            target_link_libraries(${ULI78_TARGET} SDL2)
        endif()
    endif()


endif()
