// MIT License

// Copyright (c) 2017-2022 Vadim Grigoruk @nesbox // grigoruk@gmail.com

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
#include <SDL.h>
#include <uli78.h>

#include "cart.h"
#include "zip.h"
#include "start.h"

#define ULI78_WINDOW_SCALE 3
#define ULI78_WINDOW_TITLE "ULI-78"

static struct
{
    s32 remaining;
    SDL_mutex *mutex;
    bool quit;
} state = {0};

static void onExit()
{
    state.quit = true;
}

static const char* sanitize_path(const char* path)
{
    // On Windows, the path from argv[0] can be quoted if it contains spaces.
    // fopen doesn't like quotes, so we need to remove them.
    static char sanitized_path[ULINAME_MAX];
    const char* p_in = path;
    char* p_out = sanitized_path;
    if (*p_in == '"') p_in++; // Skip leading quote
    while (*p_in) {
        *p_out++ = *p_in++;
    }
    if (*(p_out-1) == '"') *(p_out-1) = '\0'; // Remove trailing quote
    return sanitized_path;
}

static void* read_cart(const char* path, s32* size)
{
    FILE* file = fopen(path, "rb");
    if(!file)
    {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    void* cart = SDL_malloc(*size);
    if(cart)
    {
        fread(cart, *size, 1, file);
    }
    fclose(file);
    return cart;
}

static s32 getCart(const char* exe, u8** cart, s32* size)
{
    *cart = NULL;
    *size = 0;

    FILE* file = fopen(exe, "rb");
    if (!file)
    {
        // Fallback: try to load cart.uli if executable can't be opened (e.g. permissions)
        *cart = read_cart("cart.uli", size);
        return *cart ? 0 : 1;
    }

    fseek(file, -(long)sizeof(EmbedHeader), SEEK_END);

    EmbedHeader header;
    if (fread(&header, sizeof(EmbedHeader), 1, file) == 1 && strncmp(header.sig, CART_SIG, sizeof(header.sig)) == 0)
    {
        // Found embedded cart, proceed to load it
        u8* zipBuffer = (u8*)malloc(header.cartSize);
        if (!zipBuffer)
        {
            fclose(file);
            fprintf(stderr, "failed to allocate memory for zip buffer\n");
            return 1;
        }

        fseek(file, -(long)(sizeof(EmbedHeader) + header.cartSize), SEEK_END);
        if (fread(zipBuffer, header.cartSize, 1, file) != 1)
        {
            fclose(file);
            free(zipBuffer);
            fprintf(stderr, "failed to read embed cart\n");
            return 1;
        }

        fclose(file);

        *cart = (u8*)malloc(sizeof(uli_cartridge));
        if (!*cart)
        {
            free(zipBuffer);
            fprintf(stderr, "failed to allocate memory for cart\n");
            return 1;
        }

        *size = uli_tool_unzip(*cart, sizeof(uli_cartridge), zipBuffer, header.cartSize);
        free(zipBuffer);

        return 0;
    }
    else
    {
        // No embedded cart found, try to load cart.uli from disk
        fclose(file);
        *cart = read_cart("cart.uli", size);
        if (*cart) return 0;

        fprintf(stderr, "error: no cart found\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ULI-78 Error", "No cart found (embedded or cart.uli)", NULL);
        return 1;
    }
}



static u64 uli_sys_counter_get()
{
    return SDL_GetPerformanceCounter();
}

static u64 uli_sys_freq_get()
{
    return SDL_GetPerformanceFrequency();
}

static void audioCallback(void* userdata, u8* stream, s32 len)
{
    SDL_LockMutex(state.mutex);
    {
        uli78* uli = userdata;

        while(len--)
        {
            if (state.remaining <= 0)
            {
                uli78_sound(uli);
                state.remaining = uli->samples.count * ULI78_SAMPLESIZE;
            }

            *stream++ = ((u8*)uli->samples.buffer)[uli->samples.count * ULI78_SAMPLESIZE - state.remaining--];
        }
    }
    SDL_UnlockMutex(state.mutex);
}

s32 runCart(void* cart, s32 size)
{
    s32 output = 0;

    uli78_input input;
    SDL_memset(&input, 0, sizeof input);

    uli78* uli = uli78_create(ULI78_SAMPLERATE, ULI78_PIXEL_COLOR_RGBA8888);
    uli->callback.exit = onExit;
    uli78_load(uli, cart, size);

    if(!uli)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ULI-78 Error", "Failed to load cart data.", NULL);
        output = 1;
    }
    else
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error", SDL_GetError(), NULL);
            return 1;
        }

        SDL_Window* window = SDL_CreateWindow(ULI78_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ULI78_FULLWIDTH * ULI78_WINDOW_SCALE, ULI78_FULLHEIGHT * ULI78_WINDOW_SCALE, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!window)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error", SDL_GetError(), NULL);
            return 1;
        }

        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ULI78_FULLWIDTH, ULI78_FULLHEIGHT);
        SDL_AudioDeviceID audioDevice = 0;
        SDL_AudioSpec audioSpec;

        {
            state.mutex = SDL_CreateMutex();

            SDL_AudioSpec want =
            {
                .freq = ULI78_SAMPLERATE,
                .format = AUDIO_S16,
                .channels = ULI78_SAMPLE_CHANNELS,
                .callback = audioCallback,
                .samples = 1024,
                .userdata = uli,
            };

            audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &audioSpec, 0);
        }

        const u64 Delta = SDL_GetPerformanceFrequency() / ULI78_FRAMERATE;
        u64 nextTick = SDL_GetPerformanceCounter();

        SDL_PauseAudioDevice(audioDevice, 0);

        while(!state.quit)
        {
            SDL_Event event;

            while(SDL_PollEvent(&event))
            {
                switch(event.type)
                {
                case SDL_QUIT:
                    state.quit = true;
                    break;
                case SDL_KEYUP:
                    // Quit when pressing the escape button.
                    if(event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        state.quit = true;
                    }
                    break;
                }
            }

            {
                input.gamepads.data = 0;
                const uint8_t* keyboard = SDL_GetKeyboardState(NULL);

                static const SDL_Scancode Keys[] =
                {
                    SDL_SCANCODE_UP,
                    SDL_SCANCODE_DOWN,
                    SDL_SCANCODE_LEFT,
                    SDL_SCANCODE_RIGHT,

                    SDL_SCANCODE_Z,
                    SDL_SCANCODE_X,
                    SDL_SCANCODE_A,
                    SDL_SCANCODE_S,
                };

                for (u32 i = 0; i < SDL_arraysize(Keys); i++)
                {
                    if (keyboard[Keys[i]])
                    {
                        input.gamepads.first.data |= (1 << i);
                    }
                }
            }

            SDL_LockMutex(state.mutex);
            {
                uli78_tick(uli, input, uli_sys_counter_get, uli_sys_freq_get);
            }
            SDL_UnlockMutex(state.mutex);

            SDL_RenderClear(renderer);

            {
                void* pixels = NULL;
                s32 pitch = 0;
                SDL_Rect destination;
                SDL_LockTexture(texture, NULL, &pixels, &pitch);
                SDL_memcpy(pixels, uli->screen, pitch * ULI78_FULLHEIGHT);
                SDL_UnlockTexture(texture);

                // Render the image in the proper aspect ratio.
                {
                    s32 windowWidth, windowHeight;
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                    float widthRatio = (float)windowWidth / ULI78_FULLWIDTH;
                    float heightRatio = (float)windowHeight / ULI78_FULLHEIGHT;
                    float optimalSize = widthRatio < heightRatio ? widthRatio : heightRatio;
                    destination.w = (s32)(ULI78_FULLWIDTH * optimalSize);
                    destination.h = (s32)(ULI78_FULLHEIGHT * optimalSize);
                    destination.x = windowWidth / 2 - destination.w / 2;
                    destination.y = windowHeight / 2 - destination.h / 2;
                }

                SDL_RenderCopy(renderer, texture, NULL, &destination);
            }

            SDL_RenderPresent(renderer);

            {
                s64 delay = (nextTick += Delta) - SDL_GetPerformanceCounter();

                if(delay > 0)
                    SDL_Delay((u32)(delay * 1000 / SDL_GetPerformanceFrequency()));
            }
        }

        uli78_delete(uli);

        SDL_CloseAudioDevice(audioDevice);
        SDL_DestroyMutex(state.mutex);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }

    SDL_free(cart);
    return output;
}

s32 SDL_main(s32 argc, char** argv)
{
    u8* cart = NULL;
    s32 size = 0;
    if (getCart(sanitize_path(argv[0]), &cart, &size) != 0) return 1;
    return runCart(cart, size);
}
