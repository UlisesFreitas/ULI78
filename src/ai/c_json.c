//MIT License
//
//Copyright (c) 2024, Ulysses (https://www.youtube.com/@ulysses_w)
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <lua.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lauxlib.h"

// Ensure parent links are enabled for jsmn

#define JSMN_PARENT_LINKS

// By including jsmn.h without JSMN_HEADER, the implementation is included.
#include "vendor/jsmn/jsmn.h"

// Forward declaration for recursive parsing
static void parse_token(lua_State *L, const char *json_str, jsmntok_t *tokens, int token_index);

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

static void parse_object(lua_State *L, const char *json_str, jsmntok_t *tokens, int token_index) {
    jsmntok_t *obj_token = &tokens[token_index];
    lua_newtable(L);

    for (int i = 0; i < obj_token->size; i++) {
        int key_index = token_index + 1 + 2 * i;
        int val_index = key_index + 1;

        jsmntok_t *key_tok = &tokens[key_index];

        // Push key
        lua_pushlstring(L, json_str + key_tok->start, key_tok->end - key_tok->start);

        // Push value by parsing it
        parse_token(L, json_str, tokens, val_index);

        // Set table
        lua_settable(L, -3);
    }
}

static void parse_array(lua_State *L, const char *json_str, jsmntok_t *tokens, int token_index) {
    jsmntok_t *arr_token = &tokens[token_index];
    lua_newtable(L);

    for (int i = 0; i < arr_token->size; i++) {
        int val_index = token_index + 1 + i;

        // Push index (1-based for Lua)
        lua_pushinteger(L, i + 1);

        // Push value by parsing it
        parse_token(L, json_str, tokens, val_index);

        // Set table
        lua_settable(L, -3);
    }
}

static void parse_token(lua_State *L, const char *json_str, jsmntok_t *tokens, int token_index) {
    jsmntok_t *tok = &tokens[token_index];
    const char* str_start = json_str + tok->start;
    int str_len = tok->end - tok->start;

    switch (tok->type) {
        case JSMN_OBJECT:
            parse_object(L, json_str, tokens, token_index);
            break;
        case JSMN_ARRAY:
            parse_array(L, json_str, tokens, token_index);
            break;
        case JSMN_STRING:
            lua_pushlstring(L, str_start, str_len);
            break;
        case JSMN_PRIMITIVE:
            if (*str_start == 't') {
                lua_pushboolean(L, 1);
            } else if (*str_start == 'f') {
                lua_pushboolean(L, 0);
            } else if (*str_start == 'n') {
                lua_pushnil(L);
            } else {
                // It's a number
                char* num_buf = (char*)malloc(str_len + 1);
                if (num_buf) {
                    strncpy(num_buf, str_start, str_len);
                    num_buf[str_len] = '\0';
                    lua_pushnumber(L, atof(num_buf));
                    free(num_buf);
                } else {
                    lua_pushnil(L); // out of memory
                }
            }
            break;
        default:
            // Should not happen with valid JSON
            lua_pushnil(L);
            break;
    }
}

static int l_json_decode(lua_State *L) {
    size_t json_len;
    const char *json_str = luaL_checklstring(L, 1, &json_len);

    jsmn_parser p;
    jsmn_init(&p);

    int num_tokens = jsmn_parse(&p, json_str, json_len, NULL, 0);
    if (num_tokens < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "Failed to parse JSON (token counting)");
        return 2;
    }
    
    jsmntok_t *tokens = (jsmntok_t *)malloc(sizeof(jsmntok_t) * num_tokens);
    if (tokens == NULL) {
        return luaL_error(L, "Out of memory for JSON tokens");
    }

    jsmn_init(&p);
    int r = jsmn_parse(&p, json_str, json_len, tokens, num_tokens);
    if (r < 0) {
        free(tokens);
        lua_pushnil(L);
        lua_pushstring(L, "Failed to parse JSON (parsing)");
        return 2;
    }

    if (r < 1) {
        free(tokens);
        lua_pushnil(L);
        lua_pushstring(L, "Empty JSON");
        return 2;
    }

    parse_token(L, json_str, tokens, 0);

    free(tokens);
    return 1;
}

// Función C para extraer un valor de un string JSON
const char* json_get_string(const char* json_str, const char* key, int* out_len) {
    jsmn_parser p;
    jsmntok_t t[8192]; // Aumentado para soportar respuestas grandes de IA

    jsmn_init(&p);
    int r = jsmn_parse(&p, json_str, strlen(json_str), t, sizeof(t) / sizeof(t[0]));

    if (r < 0) {
        // Error de parseo
        return NULL;
    }

    // Asumimos un objeto JSON simple (no anidado profundamente por ahora)
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        return NULL;
    }

    for (int i = 1; i < r; i++) {
        if (jsoneq(json_str, &t[i], key) == 0) {
            if (i + 1 < r && t[i + 1].type == JSMN_STRING) {
                const char* value_start = json_str + t[i + 1].start;
                *out_len = t[i + 1].end - t[i + 1].start;
                return value_start;
            }
        }
    }

    return NULL; // Clave no encontrada
}


static const struct luaL_Reg json_lib[] = {
    {"decode", l_json_decode},
    {NULL, NULL}
};

int luaopen_json(lua_State *L) {
    luaL_newlib(L, json_lib);
    return 1;
}
