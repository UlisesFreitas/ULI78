################################
# libretro renderer example
################################

if(BUILD_LIBRETRO)
    set(LIBRETRO_DIR ${ULI78CORE_DIR}/system/libretro)
    set(LIBRETRO_SRC
        ${LIBRETRO_DIR}/uli78_libretro.c
    )

    if (LIBRETRO_STATIC)
        add_library(uli78_libretro STATIC
            ${LIBRETRO_SRC}
        )
        if(EMSCRIPTEN)
            set(LIBRETRO_EXTENSION "bc")
        else()
            set(LIBRETRO_EXTENSION "a")
        endif()

        set_target_properties(uli78_libretro PROPERTIES SUFFIX "${LIBRETRO_SUFFIX}.${LIBRETRO_EXTENSION}")
    else()
        add_library(uli78_libretro SHARED
            ${LIBRETRO_SRC}
        )
    endif()

    target_include_directories(uli78_libretro PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${ULI78CORE_DIR}
    )

    if(MINGW)
        target_link_libraries(uli78_libretro mingw32)
    endif()

    if(ANDROID)
        set_target_properties(uli78_libretro PROPERTIES SUFFIX "_android.so")
    endif()

    # MSYS2 builds libretro to ./bin, despite it being a DLL. This forces it to ./lib.
    set_target_properties(uli78_libretro PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )

    target_compile_definitions(uli78_libretro PRIVATE
        __LIBRETRO__=TRUE
    )
    target_link_libraries(uli78_libretro uli78core)
    set_target_properties(uli78_libretro PROPERTIES PREFIX "")
endif()