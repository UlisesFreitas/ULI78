################################
# ULI-78 core
################################

if(WIN32)
    add_library(dlfcn STATIC ${THIRDPARTY_DIR}/dlfcn/src/dlfcn.c)

    target_include_directories(dlfcn
        INTERFACE
            ${THIRDPARTY_DIR}/dirent/include
            ${THIRDPARTY_DIR}/dlfcn/src)
endif()

set(BUILD_DEPRECATED TRUE)

set(ULI78CORE_DIR ${CMAKE_SOURCE_DIR}/src)
set(ULI78CORE_SRC
    ${ULI78CORE_DIR}/fftdata.c
    ${ULI78CORE_DIR}/core/core.c
    ${ULI78CORE_DIR}/core/draw.c
    ${ULI78CORE_DIR}/core/io.c
    ${ULI78CORE_DIR}/core/sound.c
    ${ULI78CORE_DIR}/uli.c
    ${ULI78CORE_DIR}/cart.c
    ${ULI78CORE_DIR}/tools.c
    ${ULI78CORE_DIR}/zip.c
    ${ULI78CORE_DIR}/tilesheet.c
    ${ULI78CORE_DIR}/script.c
    ${ULI78CORE_DIR}/ext/fft.c
    ${ULI78CORE_DIR}/ext/kiss_fft.c
    ${ULI78CORE_DIR}/ext/kiss_fftr.c
    ${ULI78CORE_DIR}/ext/png.c
)

if(BUILD_DEPRECATED)
    set(ULI78CORE_SRC ${ULI78CORE_SRC} ${ULI78CORE_DIR}/ext/gif.c)
endif()

add_library(uli78core STATIC ${ULI78CORE_SRC})

if (FREEBSD)
    target_include_directories(uli78core PRIVATE ${SYSROOT_PATH}/usr/local/include)
    target_link_directories(uli78core PRIVATE ${SYSROOT_PATH}/usr/local/lib)
endif()

if(WIN32)
    target_link_libraries(uli78core PUBLIC dlfcn)
endif()

target_include_directories(uli78core
    PRIVATE
        ${THIRDPARTY_DIR}/moonscript
        ${THIRDPARTY_DIR}/fennel
        ${THIRDPARTY_DIR}/yuescript
        ${POCKETPY_DIR}/src
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(uli78core PRIVATE png)
target_link_libraries(uli78core PRIVATE blipbuf)

if(BUILD_WITH_ZLIB)
    target_link_libraries(uli78core PRIVATE zlib)
endif()

if(BUILD_STATIC)
    if(BUILD_WITH_LUA)
        target_link_libraries(uli78core PRIVATE lua)
    endif()

    if(BUILD_WITH_MOON)
        target_link_libraries(uli78core PRIVATE moon)
    endif()

    if(BUILD_WITH_YUE)
        target_link_libraries(uli78core PRIVATE yuescript)
    endif()

    if(BUILD_WITH_FENNEL)
        target_link_libraries(uli78core PRIVATE fennel)
    endif()

    if(BUILD_WITH_JS)
        target_link_libraries(uli78core PRIVATE js)
    endif()

    if(BUILD_WITH_SCHEME)
        target_link_libraries(uli78core PRIVATE scheme)
    endif()

    if(BUILD_WITH_SQUIRREL)
        target_link_libraries(uli78core PRIVATE squirrel)
    endif()

    if(BUILD_WITH_PYTHON)
        target_link_libraries(uli78core PRIVATE python)
    endif()

    if(BUILD_WITH_WREN)
        target_link_libraries(uli78core PRIVATE wren)
    endif()

    if(BUILD_WITH_RUBY)
        target_link_libraries(uli78core PRIVATE ruby)
    endif()

    if(BUILD_WITH_JANET)
        target_link_libraries(uli78core PRIVATE janet)
    endif()

    if(BUILD_WITH_WASM)
        target_link_libraries(uli78core PRIVATE wasm)
    endif()

    target_link_libraries(uli78core PRIVATE runtime)

endif()

if(BUILD_DEPRECATED)
    target_compile_definitions(uli78core PRIVATE BUILD_DEPRECATED)
    target_link_libraries(uli78core PRIVATE giflib)
endif()

if(LINUX)
    target_link_libraries(uli78core PRIVATE m dl)
endif()
