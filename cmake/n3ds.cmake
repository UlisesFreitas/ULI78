################################
# ULI-78 app (N3DS)
################################

if(NINTENDO_3DS)
    set(ULI78_SRC ${ULI78_SRC}
        ${CMAKE_SOURCE_DIR}/src/system/n3ds/utils.c
        ${CMAKE_SOURCE_DIR}/src/system/n3ds/keyboard.c
        ${CMAKE_SOURCE_DIR}/src/system/n3ds/main.c
    )

    add_executable(uli78 ${ULI78_SRC})

    target_include_directories(uli78 PRIVATE
        ${DEVKITPRO}/portlibs/3ds/include
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src)

    target_link_directories(uli78 PRIVATE ${DEVKITPRO}/libctru/lib ${DEVKITPRO}/portlibs/3ds/lib)
    target_link_libraries(uli78 uli78studio png citro3d)

    ctr_generate_smdh(uli78.smdh
        NAME        "ULI-78 tiny computer"
        DESCRIPTION "Fantasy computer for making, playing and sharing tiny games"
        AUTHOR      "Ulises Freitas"
        ICON        ${CMAKE_SOURCE_DIR}/build/n3ds/icon.png
    )

    ctr_create_3dsx(uli78
        SMDH   uli78.smdh
        ROMFS  ${CMAKE_SOURCE_DIR}/build/n3ds/romfs
        OUTPUT ${CMAKE_SOURCE_DIR}/build/bin/uli78.3dsx
    )

endif()