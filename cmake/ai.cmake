################################
# Ai for Lua
################################

# En lugar de buscar en el sistema, apuntamos directamente a la librería curl en 'vendor'.
set(CURL_DIR ${THIRDPARTY_DIR}/curl)

# Definimos las variables que find_package(CURL) normalmente definiría.
set(CURL_INCLUDE_DIR ${CURL_DIR}/include)

# Asumimos que las librerías precompiladas para Windows x64 están en lib/x64
set(CURL_LIBRARY ${CURL_DIR}/lib/x64/libcurl.lib)

# Añade nuestro nuevo archivo fuente del binding C a la lista de fuentes del núcleo.
list(APPEND CORE_SRC
    ${CMAKE_SOURCE_DIR}/src/ai/lua_http.c
)

# Añade el directorio de cabeceras de cURL para que #include <curl/curl.h> funcione.
list(APPEND CORE_INCS
    ${CURL_INCLUDE_DIR}
)

# Añade la librería cURL a la lista de librerías a enlazar con el núcleo.
list(APPEND CORE_LIBS
    ${CURL_LIBRARY}
)
