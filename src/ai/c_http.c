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
#include "curl/curl.h"
// Estructura para almacenar en memoria la respuesta de la petición HTTP

// Estructura para gestionar el estado de una petición asíncrona
typedef void (*http_c_callback)(const char* body, long http_code, void* userdata);

typedef struct {
    lua_State *L;
    int callback_ref;
    void* userdata;
    http_c_callback c_callback;
    struct MemoryStruct *chunk;
} CurlRequest;

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

static CURLM *curl_multi_handle = NULL;
static int still_running = 0;

// Función para procesar las peticiones pendientes. Debe ser llamada en el bucle principal.
void http_update()
{
    if (!curl_multi_handle) return;

    CURLMcode mc = curl_multi_perform(curl_multi_handle, &still_running);
    if (mc != CURLM_OK && mc != CURLM_CALL_MULTI_PERFORM) {
        fprintf(stderr, "curl_multi_perform() failed, code %d.\n", mc);
        return;
    }

    int msgs_left;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(curl_multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL *easy_handle = msg->easy_handle;
            CurlRequest *req = NULL;
            curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &req);

            if (req) {
                lua_State *L = req->L;
                // Obtener la función de callback desde el registro de Lua
                if (req->c_callback) {
                    long http_code = 0;
                    if (msg->data.result == CURLE_OK) {
                        curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &http_code);
                        req->c_callback(req->chunk->memory, http_code, req->userdata);
                    } else {
                        req->c_callback(curl_easy_strerror(msg->data.result), -1, req->userdata);
                    }
                } else if (req->callback_ref != LUA_NOREF) {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, req->callback_ref);
                    if (msg->data.result == CURLE_OK) {
                        long http_code = 0;
                        curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &http_code);
                        lua_pushlstring(L, req->chunk->memory, req->chunk->size);
                        lua_pushinteger(L, http_code);
                    } else {
                        // Hubo un error
                        lua_pushnil(L);
                        lua_pushstring(L, curl_easy_strerror(msg->data.result));
                    }
                    // Llamar al callback con 2 argumentos (body, status/error)
                    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
                        // Manejar error en el callback si es necesario
                        fprintf(stderr, "Error running http callback: %s\n", lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                    luaL_unref(L, LUA_REGISTRYINDEX, req->callback_ref);
                }

                // Limpieza
                curl_multi_remove_handle(curl_multi_handle, easy_handle);
                curl_easy_cleanup(easy_handle);
                free(req->chunk->memory);
                free(req->chunk);
                free(req);
            }
        }
    }
}

void http_post_c(const char* url, const char* body, http_c_callback callback, void* userdata) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        // No se puede llamar a luaL_error aquí, así que manejamos el error de otra forma
        fprintf(stderr, "Failed to initialize curl easy handle\n");
        return;
    }

    // Crear y configurar el chunk de memoria para la respuesta
    struct MemoryStruct *chunk = malloc(sizeof(struct MemoryStruct));
    chunk->memory = malloc(1);
    chunk->size = 0;

    // Crear la estructura de la petición
    CurlRequest *req = malloc(sizeof(CurlRequest));
    req->L = NULL; // No hay estado de Lua
    req->callback_ref = LUA_NOREF;
    req->userdata = userdata;
    req->c_callback = callback;
    req->chunk = chunk;

    // Configurar cabeceras
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // Imprime todos los detalles de la petición
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L); // Timeout de conexión de 1 segundo
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, req);

    if (!curl_multi_handle) {
        curl_multi_handle = curl_multi_init();
    }

    curl_multi_add_handle(curl_multi_handle, curl);

    // No necesitamos las cabeceras después de setopt
    curl_slist_free_all(headers);
}


// http.post(url, headers, body, callback)
static int l_http_post(lua_State *L) {
    const char *url = luaL_checkstring(L, 1);
    const char *body = luaL_checkstring(L, 3);
    luaL_checktype(L, 4, LUA_TFUNCTION);

    // Guardar el callback en el registro de Lua para que no sea recolectado por el GC
    lua_pushvalue(L, 4);
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    CURL *curl = curl_easy_init();
    if (!curl) {
        luaL_unref(L, LUA_REGISTRYINDEX, callback_ref);
        return luaL_error(L, "Failed to initialize curl easy handle");
    }

    // Crear y configurar el chunk de memoria para la respuesta
    struct MemoryStruct *chunk = malloc(sizeof(struct MemoryStruct));
    chunk->memory = malloc(1);
    chunk->size = 0;

    // Crear la estructura de la petición
    CurlRequest *req = malloc(sizeof(CurlRequest));
    req->L = L;
    req->callback_ref = callback_ref;
    req->userdata = NULL;
    req->c_callback = NULL;
    req->chunk = chunk;

    // Configurar cabeceras
    struct curl_slist *headers = NULL;
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
        const char *key = lua_tostring(L, -2);
        const char *value = lua_tostring(L, -1);
        char header_string[256];
        snprintf(header_string, sizeof(header_string), "%s: %s", key, value);
        headers = curl_slist_append(headers, header_string);
        lua_pop(L, 1);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Nota: curl hace su propia copia
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, req);

    curl_multi_add_handle(curl_multi_handle, curl);

    // No necesitamos las cabeceras después de setopt
    curl_slist_free_all(headers);

    return 0; // No devolvemos nada inmediatamente
}

// Lista de funciones que nuestra librería 'http' exportará.
static const struct luaL_Reg httplib[] = {
  {"post", l_http_post},
  {"update", http_update}, // Exponemos update para ser llamado desde C
  {NULL, NULL} // Centinela
};

// Función de inicialización de la librería. Lua la llamará cuando se haga require('http').
LUALIB_API int luaopen_http(lua_State *L) {
  if (!curl_multi_handle) {
      curl_multi_handle = curl_multi_init();
  }
  luaL_newlib(L, httplib);
  return 1;
}

void http_cleanup() {
    if (curl_multi_handle) {
        curl_multi_cleanup(curl_multi_handle);
        curl_multi_handle = NULL;
    }
}
