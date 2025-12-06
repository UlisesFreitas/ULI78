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

#include "ai.h"
#include "code.h" // Para acceder al estado del editor
#include "ai/c_json.h"
#include "curl/curl.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(_WIN32)
typedef HANDLE Thread;
#else
typedef pthread_t Thread;
#endif

#if defined(_WIN32)
static CRITICAL_SECTION response_mutex;
#else
static pthread_mutex_t response_mutex;
#endif
static char* shared_response_buffer = NULL;
static bool new_response_available = false;

// Función para escapar caracteres especiales para una cadena JSON
static char* escape_json_string(const char* input) {
    if (!input) return strdup("");

    size_t len = strlen(input);
    // Asignar memoria extra para los posibles caracteres de escape
    char* escaped = malloc(len * 2 + 1);
    if (!escaped) return NULL;

    char* out = escaped;
    for (size_t i = 0; i < len; ++i) {
        char c = input[i];
        switch (c) {
            case '\"': *out++ = '\\'; *out++ = '\"'; break;
            case '\\': *out++ = '\\'; *out++ = '\\'; break;
            case '\b': *out++ = '\\'; *out++ = 'b'; break;
            case '\f': *out++ = '\\'; *out++ = 'f'; break;
            case '\n': *out++ = '\\'; *out++ = 'n'; break;
            case '\r': *out++ = '\\'; *out++ = 'r'; break;
            case '\t': *out++ = '\\'; *out++ = 't'; break;
            default:
                if ((unsigned char)c < 0x20) {
                    // No imprimibles, se podrían manejar si es necesario, por ahora se omiten
                } else {
                    *out++ = c;
                }
                break;
        }
    }
    *out = '\0';
    return escaped;
}

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

typedef struct {
    char body[4096];
} thread_data_t;

int network_thread_func(void* data)
{
    thread_data_t* thread_data = (thread_data_t*)data;
    CURL *curl = curl_easy_init();
    if (!curl) {
        free(thread_data);
        return -1;
    }

    struct MemoryStruct chunk = { .memory = malloc(1), .size = 0 };
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    const char* url = "http://localhost:11434/api/generate";
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, thread_data->body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // 60 segundos de timeout

    CURLcode res = curl_easy_perform(curl);

#if defined(_WIN32)
    EnterCriticalSection(&response_mutex);
#else
    pthread_mutex_lock(&response_mutex);
#endif
    free(shared_response_buffer); // Liberar buffer anterior
    if (res != CURLE_OK) {
        shared_response_buffer = strdup(curl_easy_strerror(res));
    } else {
        shared_response_buffer = chunk.memory;
    }
    new_response_available = true;
#if defined(_WIN32)
    LeaveCriticalSection(&response_mutex);
#else
    pthread_mutex_unlock(&response_mutex);
#endif

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(thread_data);

    return 0;
}

void ai_send_prompt(void* code_ptr, const char* prompt, const char* context)
{
    Code* code = (Code*)code_ptr;
    if (!code) return;

    // No hacer nada si ya hay una petición en curso
    if (code->ai.thinking) return;

    code->ai.thinking = true;
    char* escaped_prompt = escape_json_string(prompt);
    // Por ahora, no enviaremos el contexto para evitar peticiones muy grandes
    char* escaped_context = escape_json_string("");

    char body[4096]; // Aumentamos el tamaño por si acaso
    snprintf(body, sizeof(body), "{\"model\": \"codellama:13b-instruct\", \"stream\": false, \"prompt\": \"%s\"}", escaped_prompt);

    free(escaped_prompt);
    free(escaped_context);

    thread_data_t* thread_data = malloc(sizeof(thread_data_t));
    strcpy(thread_data->body, body);

    Thread thread_id;
#if defined(_WIN32)
    thread_id = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)network_thread_func, thread_data, 0, NULL);
    if (thread_id == NULL) {
#else
    if (pthread_create(&thread_id, NULL, (void* (*)(void*))network_thread_func, thread_data) != 0) {
#endif
        fprintf(stderr, "Error creating network thread.\n");
        code->ai.thinking = false;
        free(thread_data);
    } else {
#if !defined(_WIN32)
        pthread_detach(thread_id);
#endif
    }
}

void ai_init()
{
#if defined(_WIN32)
    InitializeCriticalSection(&response_mutex);
#else
    pthread_mutex_init(&response_mutex, NULL);
#endif
}

void ai_update(Code* code)
{
    if (new_response_available)
    {
#if defined(_WIN32)
        EnterCriticalSection(&response_mutex);
#else
        pthread_mutex_lock(&response_mutex);
#endif
        if (new_response_available) // Doble chequeo por si acaso
        {
            int len = 0;
            const char* response = json_get_string(shared_response_buffer, "response", &len);
            if (!response) {
                response = shared_response_buffer;
                len = strlen(response);
            }

            size_t current_len = code->ai.history ? strlen(code->ai.history) : 0;
            code->ai.history = realloc(code->ai.history, current_len + len + 2);
            memcpy(code->ai.history + current_len, response, len);
            code->ai.history[current_len + len] = '\n';
            code->ai.history[current_len + len + 1] = '\0';

            new_response_available = false;
            code->ai.thinking = false;
        }
#if defined(_WIN32)
        LeaveCriticalSection(&response_mutex);
#else
        pthread_mutex_unlock(&response_mutex);
#endif
    }
}
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

// Declaración del callback para la respuesta HTTP
typedef void (*http_c_callback)(const char* body, long http_code);

// Funciones que expondremos desde ai.c
void ai_send_prompt(const char* prompt, const char* context);
void ai_receive_response(const char* response_text, long http_code);

// Función para ser llamada desde el bucle principal para procesar eventos de red
void http_update();
void http_post_c(const char* url, const char* body, http_c_callback callback);
