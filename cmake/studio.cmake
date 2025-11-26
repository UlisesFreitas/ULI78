################################
# ULI-78 studio
################################

set(ULI78LIB_DIR ${CMAKE_SOURCE_DIR}/src)
set(ULI78STUDIO_SRC
    ${ULI78LIB_DIR}/studio/screens/run.c
    ${ULI78LIB_DIR}/studio/screens/menu.c
    ${ULI78LIB_DIR}/studio/screens/mainmenu.c
    ${ULI78LIB_DIR}/studio/screens/start.c
    ${ULI78LIB_DIR}/studio/studio.c
    ${ULI78LIB_DIR}/studio/config.c
    ${ULI78LIB_DIR}/studio/fs.c
    ${ULI78LIB_DIR}/ext/md5.c
    ${ULI78LIB_DIR}/ext/json.c
    ${ULI78LIB_DIR}/ext/png.c
)

if(BUILD_EDITORS)
    set(ULI78STUDIO_SRC ${ULI78STUDIO_SRC}
        ${ULI78LIB_DIR}/studio/screens/console.c
        ${ULI78LIB_DIR}/studio/screens/surf.c
        ${ULI78LIB_DIR}/studio/editors/code.c
        ${ULI78LIB_DIR}/studio/editors/sprite.c
        ${ULI78LIB_DIR}/studio/editors/map.c
        ${ULI78LIB_DIR}/studio/editors/world.c
        ${ULI78LIB_DIR}/studio/editors/sfx.c
        ${ULI78LIB_DIR}/studio/editors/music.c
        ${ULI78LIB_DIR}/studio/net.c
        ${ULI78LIB_DIR}/ext/history.c
        ${ULI78LIB_DIR}/ext/gif.c
    )
endif()

if(BUILD_PRO)
    set(ULI78STUDIO_SRC ${ULI78STUDIO_SRC}
        ${ULI78LIB_DIR}/studio/project.c)
endif()

set(ULI78_OUTPUT uli78)

add_library(uli78studio STATIC
    ${ULI78STUDIO_SRC}
    ${CMAKE_SOURCE_DIR}/build/assets/cart.png.dat
    ${CMAKE_SOURCE_DIR}/build/assets/config.uli.dat)

target_include_directories(uli78studio
    PRIVATE ${THIRDPARTY_DIR}/jsmn
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(uli78studio PUBLIC uli78core PRIVATE zip wave_writer argparse giflib png)

if(USE_NAETT)
    target_compile_definitions(uli78studio PRIVATE USE_NAETT)
    target_link_libraries(uli78studio PRIVATE naett)
endif()

if(BUILD_PRO)
    target_compile_definitions(uli78studio PRIVATE ULI78_PRO)
endif()

if(BUILD_SDLGPU)
    target_compile_definitions(uli78studio PUBLIC CRT_SHADER_SUPPORT)
endif()

if(BUILD_EDITORS)
    target_compile_definitions(uli78studio PUBLIC BUILD_EDITORS)
endif()
