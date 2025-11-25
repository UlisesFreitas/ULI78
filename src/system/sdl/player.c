// MIT License

// Copyright (c) 2017 Vadim Grigoruk @uli78

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

#include <string.h>
#include <stdio.h>
#include <SDL.h>
#include <uli78.h>

#if defined(__APPLE__)
# if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
#    error SDL for Mac OS X only supports deploying on 10.6 and above.
# endif /* MAC_OS_X_VERSION_MIN_REQUIRED < 1060 */
#endif

#define ULI78_WINDOW_SCALE 3
#define ULI78_WINDOW_TITLE "ULI-78"
#define ULI78_DEFAULT_CART "cart.uli"
#define ULI78_EXECUTABLE_NAME "player-sdl"

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
        fprintf(stderr, "Failed to load cart data.");
        output = 1;
    }
    else
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

        SDL_Window* window = SDL_CreateWindow(ULI78_WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ULI78_FULLWIDTH * ULI78_WINDOW_SCALE, ULI78_FULLHEIGHT * ULI78_WINDOW_SCALE, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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

s32 main(s32 argc, char **argv)
{
    const char* executable = argc > 0 ? argv[0] : ULI78_EXECUTABLE_NAME;
    const char* input = (argc > 1) ? argv[1] : ULI78_DEFAULT_CART;

    // Display help message.
    if(strcmp(input, "--help") == 0 || strcmp(input, "-h") == 0)
    {
        printf("Usage: %s <file>\n", executable);
        return 0;
    }

    // Load the given file.
    FILE* file = fopen(input, "rb");
    if(!file)
    {
        fprintf(stderr, "Error: Could not load %s.\n\nUsage: %s <file>\n", input, argv[0]);
        return 1;
    }

    // Load the file data.
    fseek(file, 0, SEEK_END);
    s32 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read the data into usable memory.
    void* cart = SDL_malloc(size);
    if(cart) fread(cart, size, 1, file);
    fclose(file);

    if (!cart) {
        fprintf(stderr, "Error reading %s.\n", input);
        return 1;
    }

    return runCart(cart, size);
}
