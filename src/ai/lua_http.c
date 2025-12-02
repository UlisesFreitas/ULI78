// MIT License

// Copyright (c) 2025 Gemini

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Incluimos las cabeceras de Lua
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

// Incluimos la cabecera de libcurl
#include <curl/curl.h>

// Estructura para almacenar en memoria la respuesta de la petición HTTP
struct MemoryStruct {
  char *memory;
  size_t size;
};

// Callback que libcurl usará para escribir los datos recibidos en nuestra estructura de memoria
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    // ¡Sin memoria!
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

// La función que expondremos a Lua: http.post(url, headers, body)
// headers es una tabla de Lua, ej: { ["Content-Type"] = "application/json" }
static int l_http_post(lua_State *L) {
  const char *url = luaL_checkstring(L, 1);
  const char *body = luaL_checkstring(L, 3);

  CURL *curl;
  CURLcode res;
  struct MemoryStruct chunk;
  chunk.memory = malloc(1); // Crece según sea necesario
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if(curl) {
    struct curl_slist *headers = NULL;

    // 2do argumento: la tabla de cabeceras
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_pushnil(L); // Primer par key-value
    while (lua_next(L, 2) != 0) {
      // `key` está en el índice -2 y `value` en el -1
      const char *key = lua_tostring(L, -2);
      const char *value = lua_tostring(L, -1);
      
      char header_string[256];
      snprintf(header_string, sizeof(header_string), "%s: %s", key, value);
      headers = curl_slist_append(headers, header_string);
      
      // Quita el `value` pero mantiene la `key` para la siguiente iteración
      lua_pop(L, 1);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Enviar todos los datos del callback a esta función
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    
    // Pasamos nuestro 'chunk' de memoria a la función de callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    // Realizar la petición
    res = curl_easy_perform(curl);

    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      lua_pushnil(L);
      lua_pushstring(L, curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      curl_slist_free_all(headers);
      free(chunk.memory);
      return 2; // Retornamos nil y el mensaje de error
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Devolvemos el cuerpo de la respuesta y el código de estado
    lua_pushlstring(L, chunk.memory, chunk.size);
    lua_pushinteger(L, http_code);

    // Limpieza
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(chunk.memory);

    return 2; // Retornamos 2 valores: body y status_code
  }

  lua_pushnil(L);
  lua_pushstring(L, "Failed to initialize curl");
  return 2;
}

// Lista de funciones que nuestra librería 'http' exportará.
static const struct luaL_Reg httplib[] = {
  {"post", l_http_post},
  {NULL, NULL} // Centinela
};

// Función de inicialización de la librería. Lua la llamará cuando se haga require('http').
LUALIB_API int luaopen_http(lua_State *L) {
  luaL_newlib(L, httplib);
  return 1;
}


