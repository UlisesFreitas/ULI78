// MIT License

// Copyright (c) 2017 Vadim Grigoruk @uli78 // grigoruk@gmail.com

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

#include "studio/system.h"
#include "tools.h"

#include "ext/fft.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>



#if defined(CRT_SHADER_SUPPORT)
#include <SDL_gpu.h>
#else
#include <SDL.h>
#endif





#define TEXTURE_SIZE (ULI78_FULLWIDTH)
#define SCREEN_FORMAT ULI78_PIXEL_COLOR_RGBA8888
#define AXIS_THRESHOLD 0x4000

#if defined(__ULI_WINDOWS__)
#include <windows.h>
#endif



#if defined(TOUCH_INPUT_SUPPORT)
#define TOUCH_TIMEOUT (10 * ULI78_FRAMERATE)
#endif

#define ULI_PACKAGE "com.uli78.uli"

#define KBD_COLS 22
#define KBD_ROWS 17

#define LOCK_MUTEX(MUTEX) SDL_LockMutex(MUTEX); SCOPE(SDL_UnlockMutex(MUTEX))

enum
{
    uli_key_board = uli_keys_count + 1,
    uli_touch_size,
};

typedef union
{
#if defined(CRT_SHADER_SUPPORT)
    GPU_Target* gpu;
#endif
    SDL_Renderer* sdl;
} Renderer;

typedef union
{
#if defined(CRT_SHADER_SUPPORT)
    GPU_Image* gpu;
#endif
    SDL_Texture* sdl;
} Texture;

static struct
{
    Studio* studio;
    uli78_input input;

    SDL_Window* window;

    struct
    {
        Renderer renderer;
        Texture texture;

#if defined(CRT_SHADER_SUPPORT)
        u32 shader;
        GPU_ShaderBlock block;
#endif
    } screen;

    struct
    {
        SDL_GameController* ports[ULI_GAMEPADS];

#if defined(TOUCH_INPUT_SUPPORT)
        struct
        {
            Texture texture;
            void* pixels;
            uli78_gamepads joystick;

            struct
            {
                s32 size;
                SDL_Point axis;
                SDL_Point a;
                SDL_Point b;
                SDL_Point x;
                SDL_Point y;
            } button;

            s32 counter;
        }touch;
#endif

        uli78_gamepads joystick;

    } gamepad;

    struct
    {
        bool state[uli_keys_count];
        bool pressed[uli_keys_count];
        char text;

#if defined(TOUCH_INPUT_SUPPORT)
        struct
        {
            struct
            {
                s32 size;
                SDL_Point pos;
            } button;

            bool state[uli_touch_size];

            struct
            {
                Texture up;
                Texture down;
                void* upPixels;
                void* downPixels;
            } texture;

            bool useText;

        } touch;
#endif

    } keyboard;

    struct
    {
        bool focus;
    } mouse;

    struct
    {
        SDL_mutex           *mutex;
        SDL_AudioSpec       spec;
        SDL_AudioDeviceID   device;
        s32                 bufferRemaining;
    } audio;

    struct
    {
      SDL_AudioSpec       spec;
      SDL_AudioDeviceID   device;
    } audioIn;
} platform
#if defined(TOUCH_INPUT_SUPPORT)
=
{
    .gamepad.touch.counter = TOUCH_TIMEOUT,
    .keyboard.touch.useText = false,
}
#endif
;



static void destoryTexture(Texture texture)
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        GPU_FreeImage(texture.gpu);
    }
    else
#endif
    {
        SDL_DestroyTexture(texture.sdl);
    }
}

static void destoryRenderer(Renderer renderer)
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        GPU_CloseCurrentRenderer();
    }
    else
#endif
    {
        SDL_DestroyRenderer(renderer.sdl);
    }
}

static void renderClear(Renderer renderer)
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        GPU_Clear(renderer.gpu);
    }
    else
#endif
    {
        SDL_RenderClear(renderer.sdl);
    }
}

static void renderPresent(Renderer renderer)
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        GPU_Flip(renderer.gpu);
    }
    else
#endif
    {
        SDL_RenderPresent(renderer.sdl);
    }
}

static void renderCopy(Renderer render, Texture tex, SDL_Rect src, SDL_Rect dst)
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        GPU_Rect gpusrc = {src.x, src.y, src.w, src.h};
        GPU_BlitScale(tex.gpu, &gpusrc, render.gpu, dst.x, dst.y, (float)dst.w / src.w, (float)dst.h / src.h);
    }
    else
#endif
    {
        SDL_RenderCopy(render.sdl, tex.sdl, &src, &dst);
    }
}

static void audioCallback(void* userdata, u8* stream, s32 len)
{
    LOCK_MUTEX(platform.audio.mutex)
    {
        const uli_mem* uli = studio_mem(platform.studio);

        while(len--)
        {
            if (platform.audio.bufferRemaining <= 0)
            {
                studio_sound(platform.studio);
                platform.audio.bufferRemaining = uli->product.samples.count * ULI78_SAMPLESIZE;
            }

            *stream++ = ((u8*)uli->product.samples.buffer)[uli->product.samples.count * ULI78_SAMPLESIZE - platform.audio.bufferRemaining--];
        }
    }
}

static void initSound()
{
    platform.audio.mutex = SDL_CreateMutex();

    SDL_AudioSpec want =
    {
        .freq = ULI78_SAMPLERATE,
        .format = AUDIO_S16,
        .channels = ULI78_SAMPLE_CHANNELS,
        .userdata = NULL,
        .callback = audioCallback,
        .samples = 1024,
    };

    if (studio_config(platform.studio)->fft)
    {
        FFT_Open(studio_config(platform.studio)->fftcaptureplaybackdevices, studio_config(platform.studio)->fftdevice);
    }

    platform.audio.device = SDL_OpenAudioDevice(NULL, 0, &want, &platform.audio.spec, 0);
}

static const u8* getSpritePtr(const uli_tile* tiles, s32 x, s32 y)
{
    enum { SheetCols = (ULI_SPRITESHEET_SIZE / ULI_SPRITESIZE) };
    return tiles[x / ULI_SPRITESIZE + y / ULI_SPRITESIZE * SheetCols].data;
}

static u8 getSpritePixel(const uli_tile* tiles, s32 x, s32 y)
{
    return uli_tool_peek4(getSpritePtr(tiles, x, y), (x % ULI_SPRITESIZE) + (y % ULI_SPRITESIZE) * ULI_SPRITESIZE);
}

static void setWindowIcon()
{
    enum{ Size = 64, TileSize = 16, ColorKey = 14, Cols = TileSize / ULI_SPRITESIZE, Scale = Size/TileSize};

    u32* pixels = SDL_malloc(Size * Size * sizeof(u32));
    SCOPE(SDL_free(pixels))
    {
        uli_blitpal pal = uli_tool_palette_blit(&studio_config(platform.studio)->cart->bank0.palette.vbank0, SCREEN_FORMAT);

        for(s32 j = 0, index = 0; j < Size; j++)
            for(s32 i = 0; i < Size; i++, index++)
            {
                u8 color = getSpritePixel(studio_config(platform.studio)->cart->bank0.tiles.data, i/Scale, j/Scale);
                pixels[index] = color == ColorKey ? 0 : pal.data[color];
            }

        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, Size, Size,
            sizeof(s32) * BITS_IN_BYTE, Size * sizeof(s32),
            0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

        SCOPE(SDL_FreeSurface(surface))
        {
            SDL_SetWindowIcon(platform.window, surface);
        }
    }
}

static void updateTextureBytes(Texture texture, const void* data, s32 width, s32 height)
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        GPU_UpdateImageBytes(texture.gpu, NULL, (const u8*)data, width * sizeof(u32));
    }
    else
#endif
    {
        void* pixels = NULL;
        s32 pitch = 0;
        SDL_LockTexture(texture.sdl, NULL, &pixels, &pitch);
        SDL_memcpy(pixels, data, pitch * height);
        SDL_UnlockTexture(texture.sdl);
    }
}

#if defined(TOUCH_INPUT_SUPPORT)

static void drawKeyboardLabels(uli_mem* uli, s32 shift)
{
    typedef struct {const char* text; s32 x; s32 y; bool alt; const char* shift;} Label;
    static const Label Labels[] =
    {
        #include "kbdlabels.inl"
    };

    for(s32 i = 0; i < COUNT_OF(Labels); i++)
    {
        const Label* label = Labels + i;
        if(label->text)
            uli_api_print(uli, label->text, label->x, label->y + shift, uli_color_grey, true, 1, label->alt);

        if(label->shift)
            uli_api_print(uli, label->shift, label->x + 6, label->y + shift + 2, uli_color_light_grey, true, 1, label->alt);
    }
}

static void map2ram(uli_mem* uli)
{
    memcpy(uli->ram->map.data, &studio_config(platform.studio)->cart->bank0.map, sizeof uli->ram->map);
    memcpy(uli->ram->tiles.data, &studio_config(platform.studio)->cart->bank0.tiles, sizeof uli->ram->tiles * ULI_SPRITE_BANKS);
}

static void initTouchKeyboardState(uli_mem* uli, Texture* texture, void** pixels, bool down)
{
    enum{Cols=KBD_COLS, Rows=KBD_ROWS};

    uli_api_map(uli, down ? Cols : 0, 0, Cols, Rows, 0, 0, 0, 0, 1, NULL, NULL);
    drawKeyboardLabels(uli, down ? 2 : 0);
    uli_core_blit(uli);
    *pixels = SDL_malloc(ULI78_FULLWIDTH * ULI78_FULLHEIGHT * sizeof(u32));
    memcpy(*pixels, uli->product.screen, ULI78_FULLWIDTH * ULI78_FULLHEIGHT * sizeof(u32));

#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        texture->gpu = GPU_CreateImage(ULI78_FULLWIDTH, ULI78_FULLHEIGHT, GPU_FORMAT_RGBA);
        GPU_SetAnchor(texture->gpu, 0, 0);
        GPU_SetImageFilter(texture->gpu, GPU_FILTER_NEAREST);
    }
    else
#endif
    {
        texture->sdl = SDL_CreateTexture(platform.screen.renderer.sdl, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ULI78_FULLWIDTH, ULI78_FULLHEIGHT);
    }

    updateTextureBytes(*texture, *pixels, ULI78_FULLWIDTH, ULI78_FULLHEIGHT);
}

static void initTouchKeyboard()
{
    uli_mem *uli = uli_core_create(ULI78_SAMPLERATE, SCREEN_FORMAT);

    SCOPE(uli_core_close(uli))
    {
        memcpy(uli->ram->vram.palette.data, studio_config(platform.studio)->cart->bank0.palette.vbank0.data, sizeof(uli_palette));
        uli_api_cls(uli, 0);
        map2ram(uli);

        initTouchKeyboardState(uli, &platform.keyboard.touch.texture.up, &platform.keyboard.touch.texture.upPixels, false);
        initTouchKeyboardState(uli, &platform.keyboard.touch.texture.down, &platform.keyboard.touch.texture.downPixels, true);

        memset(uli->ram->map.data, 0, sizeof uli->ram->map);
    }
}

static void updateGamepadParts()
{
    s32 tileSize = ULI_SPRITESIZE;
    s32 offset = 0;
    SDL_Rect rect;

    const s32 JoySize = 3;
    SDL_GetWindowSize(platform.window, &rect.w, &rect.h);

    if(rect.w < rect.h)
    {
        tileSize = rect.w / 2 / JoySize;
        offset = (rect.h * 2 - JoySize * tileSize) / 3;
    }
    else
    {
        tileSize = rect.w / 5 / JoySize;
        offset = (rect.h - JoySize * tileSize) / 2;
    }

    platform.gamepad.touch.button.size = tileSize;
    platform.gamepad.touch.button.axis = (SDL_Point){0, offset};
    platform.gamepad.touch.button.a = (SDL_Point){rect.w - 2*tileSize, 2*tileSize + offset};
    platform.gamepad.touch.button.b = (SDL_Point){rect.w - 1*tileSize, 1*tileSize + offset};
    platform.gamepad.touch.button.x = (SDL_Point){rect.w - 3*tileSize, 1*tileSize + offset};
    platform.gamepad.touch.button.y = (SDL_Point){rect.w - 2*tileSize, 0*tileSize + offset};

    platform.keyboard.touch.button.size = rect.w < rect.h ? tileSize : 0;
    platform.keyboard.touch.button.pos = (SDL_Point){rect.w/2 - tileSize, rect.h - 2*tileSize};
}
#endif

#if defined(TOUCH_INPUT_SUPPORT)
static void initTouchGamepad()
{
    if(!platform.gamepad.touch.pixels)
    {
        uli_mem* uli = uli_core_create(ULI78_SAMPLERATE, SCREEN_FORMAT);

        SCOPE(uli_core_close(uli))
        {
            const uli_bank* bank = &studio_config(platform.studio)->cart->bank0;

            {
                memcpy(uli->ram->vram.palette.data, &bank->palette.vbank0, sizeof(uli_palette));
                memcpy(uli->ram->tiles.data, &bank->tiles, sizeof(uli_tiles));
                uli_api_spr(uli, 0, 0, 0, ULI_SPRITESHEET_COLS, ULI_SPRITESHEET_COLS, NULL, 0, 1, uli_no_flip, uli_no_rotate);
            }

            platform.gamepad.touch.pixels = SDL_malloc(TEXTURE_SIZE * TEXTURE_SIZE * sizeof(u32));

            uli_core_blit(uli);

            for(u32* pix = uli->product.screen, *end = pix + ULI78_FULLWIDTH * ULI78_FULLHEIGHT; pix != end; ++pix)
                if(*pix == uli_rgba(&bank->palette.vbank0.colors[0]))
                    *pix = 0;

            memcpy(platform.gamepad.touch.pixels, uli->product.screen, ULI78_FULLWIDTH * ULI78_FULLHEIGHT * sizeof(u32));

            ZEROMEM(uli->ram->vram.palette);
            ZEROMEM(uli->ram->tiles);
        }
    }

    if(!platform.gamepad.touch.texture.sdl)
    {
#if defined(CRT_SHADER_SUPPORT)
        if(!studio_config(platform.studio)->soft)
        {
            platform.gamepad.touch.texture.gpu = GPU_CreateImage(TEXTURE_SIZE, TEXTURE_SIZE, GPU_FORMAT_RGBA);
            GPU_SetAnchor(platform.gamepad.touch.texture.gpu, 0, 0);
            GPU_SetImageFilter(platform.gamepad.touch.texture.gpu, GPU_FILTER_NEAREST);
            GPU_SetRGBA(platform.gamepad.touch.texture.gpu, 0xff, 0xff, 0xff, studio_config(platform.studio)->theme.gamepad.touch.alpha);
        }
        else
#endif
        {
            platform.gamepad.touch.texture.sdl = SDL_CreateTexture(platform.screen.renderer.sdl, SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STREAMING, TEXTURE_SIZE, TEXTURE_SIZE);
            SDL_SetTextureBlendMode(platform.gamepad.touch.texture.sdl, SDL_BLENDMODE_BLEND);
            SDL_SetTextureAlphaMod(platform.gamepad.touch.texture.sdl, studio_config(platform.studio)->theme.gamepad.touch.alpha);
        }

        updateTextureBytes(platform.gamepad.touch.texture, platform.gamepad.touch.pixels, TEXTURE_SIZE, TEXTURE_SIZE);
    }

    updateGamepadParts();
}
#endif

static void initGPU()
{
    bool vsync = studio_config(platform.studio)->options.vsync;
    bool soft = studio_config(platform.studio)->soft;

#if defined(CRT_SHADER_SUPPORT)
    if(!soft)
    {
        s32 w, h;
        SDL_GetWindowSize(platform.window, &w, &h);

        GPU_SetInitWindow(SDL_GetWindowID(platform.window));

        GPU_SetPreInitFlags(vsync ? GPU_INIT_ENABLE_VSYNC : GPU_INIT_DISABLE_VSYNC);
        platform.screen.renderer.gpu = GPU_Init(w, h, GPU_DEFAULT_INIT_FLAGS);

        GPU_SetWindowResolution(w, h);
        GPU_SetVirtualResolution(platform.screen.renderer.gpu, w, h);

        platform.screen.texture.gpu = GPU_CreateImage(ULI78_FULLWIDTH, ULI78_FULLHEIGHT, GPU_FORMAT_RGBA);
        GPU_SetAnchor(platform.screen.texture.gpu, 0, 0);
        GPU_SetImageFilter(platform.screen.texture.gpu, GPU_FILTER_NEAREST);
    }
    else
#endif
    {
        platform.screen.renderer.sdl = SDL_CreateRenderer(platform.window, -1,
#if defined(CRT_SHADER_SUPPORT)
            SDL_RENDERER_SOFTWARE
#else
            (soft ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED)
#endif
            | (!soft && vsync ? SDL_RENDERER_PRESENTVSYNC : 0)
        );

        platform.screen.texture.sdl = SDL_CreateTexture(platform.screen.renderer.sdl, SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_STREAMING, ULI78_FULLWIDTH, ULI78_FULLHEIGHT);
    }

#if defined(TOUCH_INPUT_SUPPORT)
    initTouchGamepad();
    initTouchKeyboard();
#endif
}


static void destroyGPU()
{
    destoryTexture(platform.screen.texture);

#if defined(TOUCH_INPUT_SUPPORT)

    if(platform.gamepad.touch.texture.sdl)
        destoryTexture(platform.gamepad.touch.texture);

    if(platform.keyboard.touch.texture.up.sdl)
        destoryTexture(platform.keyboard.touch.texture.up);

    if(platform.keyboard.touch.texture.down.sdl)
        destoryTexture(platform.keyboard.touch.texture.down);

#endif

    destoryRenderer(platform.screen.renderer);

#if defined(CRT_SHADER_SUPPORT)

    if(!studio_config(platform.studio)->soft)
    {
        if(platform.screen.shader)
        {
            GPU_FreeShaderProgram(platform.screen.shader);
            platform.screen.shader = 0;
        }

        GPU_Quit();
    }

#endif
}

static void calcTextureRect(SDL_Rect* rect)
{
    bool integerScale = studio_config(platform.studio)->options.integerScale;

    s32 sw, sh, w, h;
    SDL_GetWindowSize(platform.window, &sw, &sh);

    enum{Width = ULI78_FULLWIDTH, Height = ULI78_FULLHEIGHT};

    if (sw * Height < sh * Width)
    {
        w = sw - (integerScale ? sw % Width : 0);
        h = Height * w / Width;
    }
    else
    {
        h = sh - (integerScale ? sh % Height : 0);
        w = Width * h / Height;
    }

    *rect = (SDL_Rect)
    {
        (sw - w) / 2,
#if defined (TOUCH_INPUT_SUPPORT)
        // snap the screen up to get a place for the software keyboard
        sw > sh ? (sh - h) / 2 : 0,
#else
        (sh - h) / 2,
#endif
        w, h
    };
}

static void processMouse()
{
    uli_point pt;
    s32 mb = SDL_GetMouseState(&pt.x, &pt.y);

    uli78_input* input = &platform.input;

    if(SDL_GetRelativeMouseMode())
    {
        SDL_GetRelativeMouseState(&pt.x, &pt.y);

        input->mouse.rx = pt.x;
        input->mouse.ry = pt.y;
    }
    else
    {
        input->mouse.x = input->mouse.y = -1;

        if(platform.mouse.focus)
        {
            SDL_Rect rect;
            calcTextureRect(&rect);

            if(rect.w && rect.h)
            {
                uli_point m = {(pt.x - rect.x) * ULI78_FULLWIDTH / rect.w, (pt.y - rect.y) * ULI78_FULLHEIGHT / rect.h};

                if(m.x < 0 || m.y < 0 || m.x >= ULI78_FULLWIDTH || m.y >= ULI78_FULLHEIGHT)
                    SDL_ShowCursor(SDL_ENABLE);
                else
                {
                    SDL_ShowCursor(SDL_DISABLE);
                    input->mouse.x = m.x;
                    input->mouse.y = m.y;
                }
            }
        }
    }

    {
        input->mouse.left = mb & SDL_BUTTON_LMASK ? 1 : 0;
        input->mouse.middle = mb & SDL_BUTTON_MMASK ? 1 : 0;
        input->mouse.right = mb & SDL_BUTTON_RMASK ? 1 : 0;
    }
}

static void processKeyboard()
{
    uli78_input* input = &platform.input;

    {
        SDL_Keymod mod = SDL_GetModState();

        platform.keyboard.state[uli_key_shift] = mod & KMOD_SHIFT;
        platform.keyboard.state[uli_key_ctrl] = mod & (KMOD_CTRL | KMOD_GUI);
        platform.keyboard.state[uli_key_alt] = mod & KMOD_LALT;
        platform.keyboard.state[uli_key_capslock] = mod & KMOD_CAPS;

        // it's weird, but system sends CTRL when you press RALT
        if(mod & KMOD_RALT)
            platform.keyboard.state[uli_key_ctrl] = false;
    }

    for(s32 i = 0, c = 0; i < COUNT_OF(platform.keyboard.state) && c < ULI78_KEY_BUFFER; i++)
        if(platform.keyboard.state[i]
            // Some programmable keyboards will send key down and up events immediately.
            // If the key was pressed and released in the same frame, report it as
            // down for this frame so that it isn't missed. Lying about the key being
            // currently down is the only way to communicate a key press without
            // adding keyboard events to uli78_input.
            || platform.keyboard.pressed[i]
#if defined(TOUCH_INPUT_SUPPORT)
            || platform.keyboard.touch.state[i]
#endif
            )
            input->keyboard.keys[c++] = i;

#if defined(TOUCH_INPUT_SUPPORT)
    if(platform.keyboard.touch.state[uli_key_board])
    {
        *input->keyboard.keys = uli_key_board;
        if(!SDL_IsTextInputActive())
            SDL_StartTextInput();
    }
#endif

    for(s32 i = 0, c = 0; i < COUNT_OF(platform.keyboard.state) && c < ULI78_KEY_BUFFER; i++) {
        platform.keyboard.pressed[i] = false;
    }
}

#if defined(TOUCH_INPUT_SUPPORT)

static bool checkTouch(const SDL_Rect* rect, s32* x, s32* y)
{
    s32 devices = SDL_GetNumTouchDevices();
    s32 width = 0, height = 0;
    SDL_GetWindowSize(platform.window, &width, &height);

    for (s32 i = 0; i < devices; i++)
    {
        SDL_TouchID id = SDL_GetTouchDevice(i);

        {
            s32 fingers = SDL_GetNumTouchFingers(id);

            for (s32 f = 0; f < fingers; f++)
            {
                SDL_Finger* finger = SDL_GetTouchFinger(id, f);

                if (finger && finger->pressure > 0.0f)
                {
                    SDL_Point point = { (s32)(finger->x * width), (s32)(finger->y * height) };
                    if (SDL_PointInRect(&point, rect))
                    {
                        *x = point.x;
                        *y = point.y;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

static bool isGamepadVisible()
{
    return studio_mem(platform.studio)->input.gamepad;
}

static bool isKbdVisible()
{
    if(!studio_mem(platform.studio)->input.keyboard)
        return false;

    s32 w, h;
    SDL_GetWindowSize(platform.window, &w, &h);

    SDL_Rect rect;
    calcTextureRect(&rect);

    return h - rect.h - KBD_ROWS * w / KBD_COLS >= 0
#if defined(__ULI_ANDROID__)
        && !SDL_IsTextInputActive()
#endif
        ;
}

static const uli_key KbdLayout[] =
{
    #include "kbdlayout.inl"
};

static void processTouchKeyboardButton(SDL_Point pt)
{
    enum{Cols = KBD_COLS, Rows = KBD_ROWS};

    s32 w, h;
    SDL_GetWindowSize(platform.window, &w, &h);

    SDL_Rect kbd = {0, h - Rows * w / Cols, w, Rows * w / Cols};

    if(SDL_PointInRect(&pt, &kbd))
    {
        uli_point pos = {(pt.x - kbd.x) * Cols / w, (pt.y - kbd.y) * Cols / w};
        platform.keyboard.touch.state[KbdLayout[pos.x + pos.y * Cols]] = true;
        platform.keyboard.touch.useText = true;
    }
}

static void processTouchKeyboard()
{
    if(!isKbdVisible()) return;

    {
        SDL_Point pt;
        if(SDL_GetMouseState(&pt.x, &pt.y) & SDL_BUTTON_LMASK)
            processTouchKeyboardButton(pt);
    }

    s32 w, h;
    SDL_GetWindowSize(platform.window, &w, &h);

    s32 devices = SDL_GetNumTouchDevices();
    for (s32 i = 0; i < devices; i++)
    {
        SDL_TouchID id = SDL_GetTouchDevice(i);
        s32 fingers = SDL_GetNumTouchFingers(id);

        for (s32 f = 0; f < fingers; f++)
        {
            SDL_Finger* finger = SDL_GetTouchFinger(id, f);

            if (finger && finger->pressure > 0.0f)
            {
                SDL_Point pt = {finger->x * w, finger->y * h};
                processTouchKeyboardButton(pt);
            }
        }
    }
}

static void processTouchGamepad()
{
    if(!platform.gamepad.touch.counter)
        return;

    platform.gamepad.touch.counter--;

    const s32 size = platform.gamepad.touch.button.size;
    s32 x = 0, y = 0;

    uli78_gamepad* joystick = &platform.gamepad.touch.joystick.first;

    {
        SDL_Rect axis = {platform.gamepad.touch.button.axis.x, platform.gamepad.touch.button.axis.y, size*3, size*3};

        if(checkTouch(&axis, &x, &y))
        {
            x -= axis.x;
            y -= axis.y;

            s32 xt = x / size;
            s32 yt = y / size;

            if(yt == 0) joystick->up = true;
            else if(yt == 2) joystick->down = true;

            if(xt == 0) joystick->left = true;
            else if(xt == 2) joystick->right = true;

            if(xt == 1 && yt == 1)
            {
                xt = (x - size)/(size/3);
                yt = (y - size)/(size/3);

                if(yt == 0) joystick->up = true;
                else if(yt == 2) joystick->down = true;

                if(xt == 0) joystick->left = true;
                else if(xt == 2) joystick->right = true;
            }
        }
    }

    {
        SDL_Rect a = {platform.gamepad.touch.button.a.x, platform.gamepad.touch.button.a.y, size, size};
        if(checkTouch(&a, &x, &y)) joystick->a = true;
    }

    {
        SDL_Rect b = {platform.gamepad.touch.button.b.x, platform.gamepad.touch.button.b.y, size, size};
        if(checkTouch(&b, &x, &y)) joystick->b = true;
    }

    {
        SDL_Rect xb = {platform.gamepad.touch.button.x.x, platform.gamepad.touch.button.x.y, size, size};
        if(checkTouch(&xb, &x, &y)) joystick->x = true;
    }

    {
        SDL_Rect yb = {platform.gamepad.touch.button.y.x, platform.gamepad.touch.button.y.y, size, size};
        if(checkTouch(&yb, &x, &y)) joystick->y = true;
    }
}

#endif

static u8 getAxis(SDL_GameController* controller, SDL_GameControllerAxis axis, s32 dir)
{
    return SDL_GameControllerHasAxis(controller, axis)
        ? SDL_GameControllerGetAxis(controller, axis) * dir > AXIS_THRESHOLD ? 1 : 0
        : 0;
}

static u8 getButton(SDL_GameController* controller, SDL_GameControllerButton button)
{
    return SDL_GameControllerHasButton(controller, button)
        ? SDL_GameControllerGetButton(controller, button)
        : 0;
}

static void processGamepad()
{
    {
        platform.gamepad.joystick.data = 0;
        s32 index = 0;

        for(s32 i = 0; i < COUNT_OF(platform.gamepad.ports); i++)
        {
            SDL_GameController* controller = platform.gamepad.ports[i];

            if(controller && SDL_GameControllerGetAttached(controller))
            {
                uli78_gamepad* gamepad = NULL;

                switch(index)
                {
                case 0: gamepad = &platform.gamepad.joystick.first; break;
                case 1: gamepad = &platform.gamepad.joystick.second; break;
                case 2: gamepad = &platform.gamepad.joystick.third; break;
                case 3: gamepad = &platform.gamepad.joystick.fourth; break;
                }

                if(gamepad)
                {
                    gamepad->up = getAxis(controller, SDL_CONTROLLER_AXIS_LEFTY, -1)
                        || getAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY, -1)
                        || getButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);

                    gamepad->down = getAxis(controller, SDL_CONTROLLER_AXIS_LEFTY, +1)
                        || getAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY, +1)
                        || getButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);

                    gamepad->left = getAxis(controller, SDL_CONTROLLER_AXIS_LEFTX, -1)
                        || getAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX, -1)
                        || getButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);

                    gamepad->right = getAxis(controller, SDL_CONTROLLER_AXIS_LEFTX, +1)
                        || getAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX, +1)
                        || getButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

                    gamepad->a = getButton(controller, SDL_CONTROLLER_BUTTON_A);
                    gamepad->b = getButton(controller, SDL_CONTROLLER_BUTTON_B);
                    gamepad->x = getButton(controller, SDL_CONTROLLER_BUTTON_X);
                    gamepad->y = getButton(controller, SDL_CONTROLLER_BUTTON_Y);
                    gamepad->start = getButton(controller, SDL_CONTROLLER_BUTTON_START);
                    gamepad->select = getButton(controller, SDL_CONTROLLER_BUTTON_BACK);
                    gamepad->guide = getButton(controller, SDL_CONTROLLER_BUTTON_GUIDE);
                    gamepad->l1 = getButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                    gamepad->r1 = getButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                    gamepad->l2 = getAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT, +1);
                    gamepad->r2 = getAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, +1);

                    // !TODO: We have to find a better way to handle gamepad MENU button
                    // atm we show game menu for only Pause Menu button on XBox one controller
                    // issue #1220
                    if(getButton(controller, SDL_CONTROLLER_BUTTON_GUIDE))
                    {
                        uli78_input* input = &platform.input;
                        input->keyboard.keys[0] = uli_key_escape;
                    }

                    index++;
                }
            }
        }
    }

    {
        uli78_input* input = &platform.input;

        input->gamepads.data = 0;

#if defined(TOUCH_INPUT_SUPPORT)
        input->gamepads.data |= platform.gamepad.touch.joystick.data;
#endif
        input->gamepads.data |= platform.gamepad.joystick.data;
    }
}

#if defined(TOUCH_INPUT_SUPPORT)
static void processTouchInput()
{
    s32 devices = SDL_GetNumTouchDevices();

    for (s32 i = 0; i < devices; i++)
        if(SDL_GetNumTouchFingers(SDL_GetTouchDevice(i)) > 0)
        {
            platform.gamepad.touch.counter = TOUCH_TIMEOUT;
            break;
        }

    if(isGamepadVisible())
        processTouchGamepad();
    else
        processTouchKeyboard();
}
#endif

static const u32 KeyboardCodes[uli_keys_count] =
{
    #include "keycodes.inl"
};

static void handleKeydown(SDL_Keycode keycode, bool down, bool* state, bool* pressed)
{
    for(uli_key i = 0; i < COUNT_OF(KeyboardCodes); i++)
    {
        if(KeyboardCodes[i] == keycode)
        {
            if (down && pressed) {
                pressed[i] = true;
            }
            state[i] = down;
            break;
        }
    }

#if defined(__ULI_ANDROID__)
    if(keycode == SDLK_AC_BACK)
        state[uli_key_escape] = down;
#elif defined(__ULI_MACOSX__)
    // SDLK_KP_ENTER is the equivalent of fn+enter on a Macbook keyboard
    if(keycode == SDLK_KP_ENTER)
        state[uli_key_insert] = down;
#endif
}

uli_layout detect_keyboard_layout()
{
    char q = SDL_GetKeyFromScancode(SDL_SCANCODE_Q);
    char w = SDL_GetKeyFromScancode(SDL_SCANCODE_W);
    char y = SDL_GetKeyFromScancode(SDL_SCANCODE_Y);

    uli_layout layout = uli_layout_unknown;

    if (q == 'q' && w == 'w' && y == 'y') layout = uli_layout_qwerty; // US etc.
    if (q == 'a' && w == 'z' && y == 'y') layout = uli_layout_azerty; // French
    if (q == 'q' && w == 'w' && y == 'z') layout = uli_layout_qwertz; // German etc.
    if (q == 'q' && w == 'z' && y == 'y') layout = uli_layout_qzerty; // Italian
    // Don't ask me why it detects k instead of l
    if (q == 'x' && w == 'v' && y == 'k') layout = uli_layout_de_neo; // xvlcwk - German Neo
    // ...or why it detects p instead of u
    if (q == 'j' && w == 'd' && y == 'p') layout = uli_layout_de_bone; // jduaxp - German Bone

    return layout;
}

static void pollEvents()
{
    // check if releative mode was enabled
    {
        const uli_mem* uli = studio_mem(platform.studio);
        if((bool)uli->ram->input.mouse.relative != (bool)SDL_GetRelativeMouseMode())
            SDL_SetRelativeMouseMode(uli->ram->input.mouse.relative ? SDL_TRUE : SDL_FALSE);
    }

    ZEROMEM(platform.input);
    uli78_input* input = &platform.input;

    // keep relative mode enabled
    input->mouse.relative = SDL_GetRelativeMouseMode() ? 1 : 0;

#if defined(TOUCH_INPUT_SUPPORT)
    ZEROMEM(platform.gamepad.touch.joystick);
    ZEROMEM(platform.keyboard.touch.state);
#endif

#if defined(__ULI_ANDROID__)
    // SDL2 doesn't send SDL_KEYUP for backspace button on Android sometimes
    platform.keyboard.state[uli_key_backspace] = false;
#endif

    SDL_Event event;

// workaround for issue #1332 to not process keyboard when window gained focus
#if defined(__LINUX__)
    static s32 lockInput = 0;

    if(lockInput)
        lockInput--;
#endif

    // Workaround for freeze on fullscreen under macOS #819
    SDL_PumpEvents();
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_MOUSEWHEEL:
            {
                input->mouse.scrollx = event.wheel.x;
                input->mouse.scrolly = event.wheel.y;
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            {
                s32 id = event.cdevice.which;

                const char* name = SDL_GameControllerNameForIndex(id);

                if(name && SDL_strcmp(name, "Serial/Keyboard/Mouse/Joystick") == 0)
                    break;

                if(SDL_IsGameController(id))
                {
                    if (id < ULI_GAMEPADS)
                    {
                        if(platform.gamepad.ports[id])
                            SDL_GameControllerClose(platform.gamepad.ports[id]);

                        platform.gamepad.ports[id] = SDL_GameControllerOpen(id);
                    }
                }
            }
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            {
                s32 id = event.cdevice.which;

                if (id < ULI_GAMEPADS && platform.gamepad.ports[id])
                {
                    SDL_GameControllerClose(platform.gamepad.ports[id]);
                    platform.gamepad.ports[id] = NULL;
                }
            }
            break;
        case SDL_WINDOWEVENT:
            switch(event.window.event)
            {
            case SDL_WINDOWEVENT_ENTER:
                platform.mouse.focus = true;
                break;
            case SDL_WINDOWEVENT_LEAVE:
                platform.mouse.focus = false;
                break;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                {

#if defined(CRT_SHADER_SUPPORT)
                    if(!studio_config(platform.studio)->soft)
                    {
                        s32 w, h;
                        SDL_GetWindowSize(platform.window, &w, &h);
                        GPU_SetWindowResolution(w, h);
                        GPU_SetVirtualResolution(platform.screen.renderer.gpu, w, h);
                    }
#endif

#if defined(TOUCH_INPUT_SUPPORT)
                    updateGamepadParts();
#endif
                }
                break;
#if defined(__LINUX__)
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                // lock input for 10 ticks
                lockInput = 10;
                break;
#endif
            }
            break;

        case SDL_KEYDOWN:

#if defined(TOUCH_INPUT_SUPPORT)
            platform.keyboard.touch.useText = false;
            handleKeydown(event.key.keysym.sym, true, platform.keyboard.touch.state, NULL);
#endif

            handleKeydown(event.key.keysym.sym, true, platform.keyboard.state, platform.keyboard.pressed);
            break;
        case SDL_KEYUP:
            handleKeydown(event.key.keysym.sym, false, platform.keyboard.state, platform.keyboard.pressed);
            break;
        case SDL_KEYMAPCHANGED:
            studio_keymapchanged(platform.studio, detect_keyboard_layout());
            break;
        case SDL_TEXTINPUT:
            if(strlen(event.text.text) == 1)
                platform.keyboard.text = event.text.text[0];
            break;
        case SDL_DROPFILE:
            studio_load(platform.studio, event.drop.file);
            break;
        case SDL_QUIT:
            studio_exit(platform.studio);
            break;
        default:
            break;
        }
    }

    processMouse();

#if defined(TOUCH_INPUT_SUPPORT)
    processTouchInput();
#endif

    SCOPE(processKeyboard())
    {
#if defined(__LINUX__)
        if(lockInput)
            return;
#endif
    }

    processGamepad();
}

bool uli_sys_keyboard_text(char* text)
{
#if defined(TOUCH_INPUT_SUPPORT)
    if(platform.keyboard.touch.useText)
        return false;
#endif

    *text = platform.keyboard.text;
    return true;
}

#if defined(TOUCH_INPUT_SUPPORT)

static void renderKeyboard()
{
    if(!isKbdVisible()) return;

    SDL_Rect rect;
    SDL_GetWindowSize(platform.window, &rect.w, &rect.h);

    SDL_Rect src = {ULI78_OFFSET_LEFT, ULI78_OFFSET_TOP, KBD_COLS*ULI_SPRITESIZE, KBD_ROWS*ULI_SPRITESIZE};
    SDL_Rect dst = {0, rect.h - src.h * rect.w / src.w, rect.w, src.h * rect.w / src.w};

    renderCopy(platform.screen.renderer, platform.keyboard.touch.texture.up, src, dst);

    const uli78_input* input = &studio_mem(platform.studio)->ram->input;

    enum{Cols=KBD_COLS, Rows=KBD_ROWS};

    for(s32 i = 0; i < COUNT_OF(input->keyboard.keys); i++)
    {
        uli_key key = input->keyboard.keys[i];

        if(key > uli_key_unknown)
        {
            for(s32 k = 0; k < COUNT_OF(KbdLayout); k++)
            {
                if(key == KbdLayout[k])
                {
                    SDL_Rect src2 =
                    {
                        (k % Cols) * ULI_SPRITESIZE + ULI78_OFFSET_LEFT,
                        (k / Cols) * ULI_SPRITESIZE + ULI78_OFFSET_TOP,
                        ULI_SPRITESIZE,
                        ULI_SPRITESIZE,
                    };

                    SDL_Rect dst2 =
                    {
                        (src2.x - ULI78_OFFSET_LEFT) * rect.w/src.w,
                        (src2.y - ULI78_OFFSET_TOP) * rect.w/src.w + dst.y,
                        ULI_SPRITESIZE * rect.w/src.w,
                        ULI_SPRITESIZE * rect.w/src.w,
                    };

                    renderCopy(platform.screen.renderer, platform.keyboard.touch.texture.down, src2, dst2);
                }
            }
        }
    }
}

static void renderGamepad()
{
    if(!platform.gamepad.touch.counter)
        return;

    const s32 tileSize = platform.gamepad.touch.button.size;
    const SDL_Point axis = platform.gamepad.touch.button.axis;
    typedef struct { bool press; s32 x; s32 y;} Tile;
    const uli78_input* input = &studio_mem(platform.studio)->ram->input;
    const Tile Tiles[] =
    {
        {input->gamepads.first.up,     axis.x + 1*tileSize, axis.y + 0*tileSize},
        {input->gamepads.first.down,   axis.x + 1*tileSize, axis.y + 2*tileSize},
        {input->gamepads.first.left,   axis.x + 0*tileSize, axis.y + 1*tileSize},
        {input->gamepads.first.right,  axis.x + 2*tileSize, axis.y + 1*tileSize},

        {input->gamepads.first.a,      platform.gamepad.touch.button.a.x, platform.gamepad.touch.button.a.y},
        {input->gamepads.first.b,      platform.gamepad.touch.button.b.x, platform.gamepad.touch.button.b.y},
        {input->gamepads.first.x,      platform.gamepad.touch.button.x.x, platform.gamepad.touch.button.x.y},
        {input->gamepads.first.y,      platform.gamepad.touch.button.y.x, platform.gamepad.touch.button.y.y},
    };

    enum { Left = ULI78_MARGIN_LEFT + 8 * ULI_SPRITESIZE};

    for(s32 i = 0; i < COUNT_OF(Tiles); i++)
    {
        const Tile* tile = Tiles + i;

#if defined(CRT_SHADER_SUPPORT)
        if(!studio_config(platform.studio)->soft)
        {
            GPU_Rect src = { (float)i * ULI_SPRITESIZE + Left, (float)(tile->press ? ULI_SPRITESIZE : 0) + ULI78_MARGIN_TOP, (float)ULI_SPRITESIZE, (float)ULI_SPRITESIZE};
            GPU_Rect dest = { (float)tile->x, (float)tile->y, (float)tileSize, (float)tileSize};

            GPU_BlitScale(platform.gamepad.touch.texture.gpu, &src, platform.screen.renderer.gpu, dest.x, dest.y,
                (float)dest.w / ULI_SPRITESIZE, (float)dest.h / ULI_SPRITESIZE);
        }
#else
        {
            SDL_Rect src = {i * ULI_SPRITESIZE + Left, (tile->press ? ULI_SPRITESIZE : 0) + ULI78_MARGIN_TOP, ULI_SPRITESIZE, ULI_SPRITESIZE};
            SDL_Rect dest = {tile->x, tile->y, tileSize, tileSize};
            SDL_RenderCopy(platform.screen.renderer.sdl, platform.gamepad.touch.texture.sdl, &src, &dest);
        }
#endif
    }
}

#endif

static const char* getAppFolder()
{
    static char appFolder[ULINAME_MAX];

#if defined(__EMSCRIPTEN__)

        strcpy(appFolder, "/" ULI_PACKAGE "/" ULI_NAME "/");

#elif defined(__ULI_ANDROID__)

        strcpy(appFolder, SDL_AndroidGetExternalStoragePath());
        const char AppFolder[] = "/" ULI_NAME "/";
        strcat(appFolder, AppFolder);
        mkdir(appFolder, 0777);

#else

        char* path = SDL_GetPrefPath(ULI_PACKAGE, ULI_NAME);
        strcpy(appFolder, path);
        SDL_free(path);

#endif

    return appFolder;
}

void uli_sys_clipboard_set(const char* text)
{
    SDL_SetClipboardText(text);
}

bool uli_sys_clipboard_has()
{
    return SDL_HasClipboardText();
}

char* uli_sys_clipboard_get()
{
    return SDL_GetClipboardText();
}

void uli_sys_clipboard_free(const char* text)
{
    SDL_free((void*)text);
}

u64 uli_sys_counter_get()
{
    return SDL_GetPerformanceCounter();
}

u64 uli_sys_freq_get()
{
    return SDL_GetPerformanceFrequency();
}

bool uli_sys_fullscreen_get()
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        return GPU_GetFullscreen() == GPU_TRUE ? true : false;
    }
    else
#endif
    {
        return SDL_GetWindowFlags(platform.window) & SDL_WINDOW_FULLSCREEN_DESKTOP
            ? true : false;
    }
}

void uli_sys_fullscreen_set(bool value)
{
#if defined(CRT_SHADER_SUPPORT)
    if(!studio_config(platform.studio)->soft)
    {
        GPU_SetFullscreen(value ? GPU_TRUE : GPU_FALSE, GPU_TRUE);
    }
    else
#endif
    {
        SDL_SetWindowFullscreen(platform.window,
            value ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }
}

void uli_sys_message(const char* title, const char* message)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title, message, NULL);
}

void uli_sys_title(const char* title)
{
    if(platform.window)
        SDL_SetWindowTitle(platform.window, title);
}

void uli_sys_open_path(const char* path)
{
    char command[ULINAME_MAX];

#if defined(__WINDOWS__)

    sprintf(command, "explorer \"%s\"", path);

    wchar_t wcommand[ULINAME_MAX];
    MultiByteToWideChar(CP_UTF8, 0, command, ULINAME_MAX, wcommand, ULINAME_MAX);

    _wsystem(wcommand);

#elif defined(__LINUX__)

    sprintf(command, "xdg-open \"%s\"", path);
    if(system(command)){}

#elif defined(__MACOSX__)

    sprintf(command, "open \"%s\"", path);
    system(command);

#endif
}

void uli_sys_open_url(const char* url)
{
#if defined(__EMSCRIPTEN__)
    EM_ASM_(
    {
        window.open(UTF8ToString($0))
    }, url);
#else
    SDL_OpenURL(url);
#endif
}

void uli_sys_preseed()
{
#if defined(__MACOSX__)
    srandom(time(NULL));
    random();
#else
    srand((u32)time(NULL));
    rand();
#endif
}

#if defined(CRT_SHADER_SUPPORT)

static void loadCrtShader()
{
    static const char VertextShader[] =
#if !defined (EMSCRIPTEN)
        "#version 110"                                                              "\n"
#endif
        "attribute vec3 gpu_Vertex;"                                                "\n"
        "attribute vec2 gpu_TexCoord;"                                              "\n"
        "attribute vec4 gpu_Color;"                                                 "\n"
        "uniform mat4 gpu_ModelViewProjectionMatrix;"                               "\n"
        "varying vec4 color;"                                                       "\n"
        "varying vec2 texCoord;"                                                    "\n"
        "void main(void)"                                                           "\n"
        "{"                                                                         "\n"
        "    color = gpu_Color;"                                                    "\n"
        "    texCoord = vec2(gpu_TexCoord);"                                        "\n"
        "    gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 1.0);"  "\n"
        "}"                                                                         "\n"
    ;

    static const char PixelShader[] =
#if !defined (EMSCRIPTEN)
        "#version 110"                                                                      "\n"
#else
        "precision highp float;"                                                            "\n"
#endif
        "varying vec2 texCoord;"                                                            "\n"
        "uniform sampler2D source;"                                                         "\n"
        "uniform float trg_x;"                                                              "\n"
        "uniform float trg_y;"                                                              "\n"
        "uniform float trg_w;"                                                              "\n"
        "uniform float trg_h;"                                                              "\n"
        ""                                                                                  "\n"
        "// Emulated input resolution."                                                     "\n"
        "vec2 res=vec2(256.0,144.0);"                                                       "\n"
        ""                                                                                  "\n"
        "// Hardness of scanline."                                                          "\n"
        "//  -8.0 = soft"                                                                   "\n"
        "// -16.0 = medium"                                                                 "\n"
        "float hardScan=-8.0;"                                                              "\n"
        ""                                                                                  "\n"
        "// Hardness of pixels in scanline."                                                "\n"
        "// -2.0 = soft"                                                                    "\n"
        "// -4.0 = hard"                                                                    "\n"
        "float hardPix=-3.0;"                                                               "\n"
        ""                                                                                  "\n"
        "// Display warp."                                                                  "\n"
        "// 0.0 = none"                                                                     "\n"
        "// 1.0/8.0 = extreme"                                                              "\n"
        "vec2 warp=vec2(1.0/64.0,1.0/48.0); "                                               "\n"
        ""                                                                                  "\n"
        "// Amount of shadow mask."                                                         "\n"
        "float maskDark=0.5;"                                                               "\n"
        "float maskLight=1.5;"                                                              "\n"
        ""                                                                                  "\n"
        "//------------------------------------------------------------------------"        "\n"
        ""                                                                                  "\n"
        "// sRGB to Linear."                                                                "\n"
        "// Assuing using sRGB typed textures this should not be needed."                   "\n"
        "float ToLinear1(float c){return(c<=0.04045)?c/12.92:pow((c+0.055)/1.055,2.4);}"    "\n"
        "vec3 ToLinear(vec3 c){return vec3(ToLinear1(c.r),ToLinear1(c.g),ToLinear1(c.b));}" "\n"
        ""                                                                                  "\n"
        "// Linear to sRGB."                                                                "\n"
        "// Assuing using sRGB typed textures this should not be needed."                   "\n"
        "float ToSrgb1(float c){return(c<0.0031308?c*12.92:1.055*pow(c,0.41666)-0.055);}"   "\n"
        "vec3 ToSrgb(vec3 c){return vec3(ToSrgb1(c.r),ToSrgb1(c.g),ToSrgb1(c.b));}"         "\n"
        ""                                                                                  "\n"
        "// Nearest emulated sample given floating point position and texel offset."        "\n"
        "// Also zero's off screen."                                                        "\n"
        "vec3 Fetch(vec2 pos,vec2 off){"                                                    "\n"
        "    pos=(floor(pos*res+off)+vec2(0.5,0.5))/res;"                                   "\n"
        "    return ToLinear(1.2 * texture2D(source,pos.xy,-16.0).rgb);}"                   "\n"
        ""                                                                                  "\n"
        "// Distance in emulated pixels to nearest texel."                                  "\n"
        "vec2 Dist(vec2 pos){pos=pos*res;return -((pos-floor(pos))-vec2(0.5));}"            "\n"
        "        "                                                                          "\n"
        "// 1D Gaussian."                                                                   "\n"
        "float Gaus(float pos,float scale){return exp2(scale*pos*pos);}"                    "\n"
        ""                                                                                  "\n"
        "// 3-tap Gaussian filter along horz line."                                         "\n"
        "vec3 Horz3(vec2 pos,float off){"                                                   "\n"
        "    vec3 b=Fetch(pos,vec2(-1.0,off));"                                             "\n"
        "    vec3 c=Fetch(pos,vec2( 0.0,off));"                                             "\n"
        "    vec3 d=Fetch(pos,vec2( 1.0,off));"                                             "\n"
        "    float dst=Dist(pos).x;"                                                        "\n"
        "    // Convert distance to weight."                                                "\n"
        "    float scale=hardPix;"                                                          "\n"
        "    float wb=Gaus(dst-1.0,scale);"                                                 "\n"
        "    float wc=Gaus(dst+0.0,scale);"                                                 "\n"
        "    float wd=Gaus(dst+1.0,scale);"                                                 "\n"
        "    // Return filtered sample."                                                    "\n"
        "    return (b*wb+c*wc+d*wd)/(wb+wc+wd);}"                                          "\n"
        ""                                                                                  "\n"
        "// 5-tap Gaussian filter along horz line."                                         "\n"
        "vec3 Horz5(vec2 pos,float off){"                                                   "\n"
        "    vec3 a=Fetch(pos,vec2(-2.0,off));"                                             "\n"
        "    vec3 b=Fetch(pos,vec2(-1.0,off));"                                             "\n"
        "    vec3 c=Fetch(pos,vec2( 0.0,off));"                                             "\n"
        "    vec3 d=Fetch(pos,vec2( 1.0,off));"                                             "\n"
        "    vec3 e=Fetch(pos,vec2( 2.0,off));"                                             "\n"
        "    float dst=Dist(pos).x;"                                                        "\n"
        "    // Convert distance to weight."                                                "\n"
        "    float scale=hardPix;"                                                          "\n"
        "    float wa=Gaus(dst-2.0,scale);"                                                 "\n"
        "    float wb=Gaus(dst-1.0,scale);"                                                 "\n"
        "    float wc=Gaus(dst+0.0,scale);"                                                 "\n"
        "    float wd=Gaus(dst+1.0,scale);"                                                 "\n"
        "    float we=Gaus(dst+2.0,scale);"                                                 "\n"
        "    // Return filtered sample."                                                    "\n"
        "    return (a*wa+b*wb+c*wc+d*wd+e*we)/(wa+wb+wc+wd+we);}"                          "\n"
        ""                                                                                  "\n"
        "// Return scanline weight."                                                        "\n"
        "float Scan(vec2 pos,float off){"                                                   "\n"
        "    float dst=Dist(pos).y;"                                                        "\n"
        "    return Gaus(dst+off,hardScan);}"                                               "\n"
        ""                                                                                  "\n"
        "// Allow nearest three lines to effect pixel."                                     "\n"
        "vec3 Tri(vec2 pos){"                                                               "\n"
        "    vec3 a=Horz3(pos,-1.0);"                                                       "\n"
        "    vec3 b=Horz5(pos, 0.0);"                                                       "\n"
        "    vec3 c=Horz3(pos, 1.0);"                                                       "\n"
        "    float wa=Scan(pos,-1.0);"                                                      "\n"
        "    float wb=Scan(pos, 0.0);"                                                      "\n"
        "    float wc=Scan(pos, 1.0);"                                                      "\n"
        "    return a*wa+b*wb+c*wc;}"                                                       "\n"
        ""                                                                                  "\n"
        "// Distortion of scanlines, and end of screen alpha."                              "\n"
        "vec2 Warp(vec2 pos){"                                                              "\n"
        "    pos=pos*2.0-1.0;    "                                                          "\n"
        "    pos*=vec2(1.0+(pos.y*pos.y)*warp.x,1.0+(pos.x*pos.x)*warp.y);"                 "\n"
        "    return pos*0.5+0.5;}"                                                          "\n"
        ""                                                                                  "\n"
        "// Shadow mask."                                                                   "\n"
        "vec3 Mask(vec2 pos){"                                                              "\n"
        "    pos.x+=pos.y*3.0;"                                                             "\n"
        "    vec3 mask=vec3(maskDark,maskDark,maskDark);"                                   "\n"
        "    pos.x=fract(pos.x/6.0);"                                                       "\n"
        "    if(pos.x<0.333)mask.r=maskLight;"                                              "\n"
        "    else if(pos.x<0.666)mask.g=maskLight;"                                         "\n"
        "    else mask.b=maskLight;"                                                        "\n"
        "    return mask;}    "                                                             "\n"
        ""                                                                                  "\n"
        "void main() {"                                                                     "\n"
        "    hardScan=-12.0;"                                                               "\n"
        "    //maskDark=maskLight;"                                                         "\n"
        "    vec2 start=gl_FragCoord.xy-vec2(trg_x, trg_y);"                                "\n"
        "    start.y=trg_h-start.y;"                                                        "\n"
        ""                                                                                  "\n"
        "    vec2 pos=Warp(start/vec2(trg_w, trg_h));"                                      "\n"
        ""                                                                                  "\n"
        "    gl_FragColor.rgb=Tri(pos)*Mask(gl_FragCoord.xy);"                              "\n"
        "    gl_FragColor = vec4(ToSrgb(gl_FragColor.rgb), 1.0);"                           "\n"
        "}"                                                                                 "\n"
    ;

    u32 vertex = GPU_CompileShader(GPU_VERTEX_SHADER, VertextShader);

    if(!vertex)
    {
        printf("Failed to load vertex shader: %s\n", GPU_GetShaderMessage());
        return;
    }

    u32 pixel = GPU_CompileShader(GPU_PIXEL_SHADER, PixelShader);

    if(!pixel)
    {
        printf("Failed to load pixel shader: %s\n", GPU_GetShaderMessage());
        return;
    }

    if(platform.screen.shader)
        GPU_FreeShaderProgram(platform.screen.shader);

    platform.screen.shader = GPU_LinkShaders(vertex, pixel);

    if(platform.screen.shader)
    {
        platform.screen.block = GPU_LoadShaderBlock(platform.screen.shader, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");
        GPU_ActivateShaderProgram(platform.screen.shader, &platform.screen.block);
    }
    else
    {
        printf("Failed to link shader program: %s\n", GPU_GetShaderMessage());
    }
}
#endif

void uli_sys_update_config()
{
#if defined(TOUCH_INPUT_SUPPORT)
    if(platform.screen.renderer.sdl)
        initTouchGamepad();
#endif
}

void uli_sys_default_mapping(uli_mapping* mapping)
{
    static const SDL_Scancode Scancodes[] =
    {
        SDL_SCANCODE_UP,
        SDL_SCANCODE_DOWN,
        SDL_SCANCODE_LEFT,
        SDL_SCANCODE_RIGHT,
        SDL_SCANCODE_Z,
        SDL_SCANCODE_X,
        SDL_SCANCODE_A,
        SDL_SCANCODE_S,
        SDL_SCANCODE_V,         // START
        SDL_SCANCODE_C,         // SELECT
        SDL_SCANCODE_W,         // L1/LB
        SDL_SCANCODE_E,         // R1/RB
        SDL_SCANCODE_Q,         // L2/LT
        SDL_SCANCODE_R,         // R2/RT
        SDL_SCANCODE_G,         // GUIDE
    };

    for(s32 s = 0; s < COUNT_OF(Scancodes); ++s)
    {
        SDL_Keycode keycode = SDL_GetKeyFromScancode(Scancodes[s]);

        for(uli_key i = 0; i < COUNT_OF(KeyboardCodes); i++)
        {
            if(i != uli_key_unknown && KeyboardCodes[i] == keycode)
            {
                mapping->data[s] = i;
                break;
            }
        }
    }
}

static void gpuTick()
{
    const uli_mem* uli = studio_mem(platform.studio);

    pollEvents();

    if(studio_alive(platform.studio))
    {
#if defined __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    LOCK_MUTEX(platform.audio.mutex)
    {
        studio_tick(platform.studio, platform.input);
    }

    renderClear(platform.screen.renderer);
    updateTextureBytes(platform.screen.texture, uli->product.screen, ULI78_FULLWIDTH, ULI78_FULLHEIGHT);

    SDL_Rect rect;
    calcTextureRect(&rect);

#if defined(CRT_SHADER_SUPPORT)

    if(!studio_config(platform.studio)->soft && studio_config(platform.studio)->options.crt)
    {
        if(platform.screen.shader == 0)
            loadCrtShader();

        GPU_ActivateShaderProgram(platform.screen.shader, &platform.screen.block);

        static const char* Uniforms[] = {"trg_x", "trg_y", "trg_w", "trg_h"};

        for(s32 i = 0; i < COUNT_OF(Uniforms); ++i)
            GPU_SetUniformf(GPU_GetUniformLocation(platform.screen.shader, Uniforms[i]), (&rect.x)[i]);

        GPU_BlitScale(platform.screen.texture.gpu, NULL, platform.screen.renderer.gpu, rect.x, rect.y,
            (float)rect.w / ULI78_FULLWIDTH, (float)rect.h / ULI78_FULLHEIGHT);
        GPU_DeactivateShaderProgram();
    }
    else

#endif

    {
        s32 w, h;
        SDL_GetWindowSize(platform.window, &w, &h);

        s32 offset = uli->ram->input.mouse.x < ULI78_FULLHEIGHT / 2
            ? ULI78_FULLWIDTH-ULI78_OFFSET_LEFT : 0;

        const SDL_Rect Src[] =
        {
            {offset, 0, ULI78_OFFSET_LEFT, ULI78_OFFSET_TOP},                                   // top border
            {offset, ULI78_FULLHEIGHT-ULI78_OFFSET_TOP, ULI78_OFFSET_LEFT, ULI78_OFFSET_TOP},   // bottom border
            {offset, 0, ULI78_OFFSET_LEFT, ULI78_FULLHEIGHT},                                   // left border
            {offset, 0, ULI78_OFFSET_LEFT, ULI78_FULLHEIGHT},                                   // right border
            {0, 0, ULI78_FULLWIDTH, ULI78_FULLHEIGHT},                                          // center
        };

        const SDL_Rect Dst[] =
        {
            {0, 0, w, rect.y},                                          // top border
            {0, rect.y + rect.h, w, h - (rect.y + rect.h)},             // bottom border
            {0, rect.y, rect.x, rect.h},                                // left border
            {rect.x + rect.w, rect.y, w - (rect.x + rect.w), rect.h},   // right border
            {rect.x, rect.y, rect.w, rect.h},                           // screen
        };

        for(s32 i = 0; i < COUNT_OF(Src); ++i)
            renderCopy(platform.screen.renderer, platform.screen.texture, Src[i], Dst[i]);
    }

#if defined(TOUCH_INPUT_SUPPORT)

    if(isGamepadVisible())
        renderGamepad();
    else
        renderKeyboard();
#endif

    renderPresent(platform.screen.renderer);

    platform.keyboard.text = '\0';
}

#if defined(__EMSCRIPTEN__)

static void emsGpuTick()
{
    static double nextTick = -1.0;

    bool vsync = studio_config(platform.studio)->options.vsync;

    if(!vsync)
    {
        if(nextTick < 0.0)
            nextTick = emscripten_get_now();

        nextTick += 1000.0/ULI78_FRAMERATE;
    }

    gpuTick();

    EM_ASM(
    {
        if(FS.syncFSRequests == 0 && Module.syncFSRequests)
        {
            Module.syncFSRequests = 0;
            FS.syncfs(false,function(){});
        }
    });

    if(!vsync)
    {
        double delay = nextTick - emscripten_get_now();

        if(delay > 0.0)
            emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, delay);
    }
}

#endif

s32 determineMaximumScale()
{
    SDL_DisplayMode current;
    int result = SDL_GetCurrentDisplayMode(0, &current);
    s32 maxScale;

    if (result != 0)
    {
        SDL_Log("Unable to SDL_GetCurrentDisplayMode: %s", SDL_GetError());
        return INT32_MAX;
    }

    int maxScaleByW = current.w / ULI78_WIDTH;
    int maxScaleByH = current.h / ULI78_HEIGHT;

    if (maxScaleByH < maxScaleByW)
    {
        maxScale = maxScaleByW;
    }
    else
    {
        maxScale = maxScaleByH;
    }

    if (maxScale <= 1)
    {
        return 1;
    }
    else
    {
        return maxScale;
    }
}

static s32 start(s32 argc, char **argv, const char* folder)
{
#if defined(__MACOSX__)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
    int result = SDL_Init(SDL_INIT_VIDEO);
    if (result != 0)
    {
        SDL_Log("Unable to initialize SDL Video: %i, %s\n", result, SDL_GetError());
        return result;
    }

    result = SDL_Init(SDL_INIT_AUDIO);
    if (result != 0)
    {
        SDL_Log("Unable to initialize SDL Audio: %i, %s\n", result, SDL_GetError());
    }

    result = SDL_Init(SDL_INIT_GAMECONTROLLER);
    if (result != 0)
    {
        SDL_Log("Unable to initialize SDL Game Controller: %i, %s\n", result, SDL_GetError());
    }

    platform.studio = studio_create(argc, argv, ULI78_SAMPLERATE, SCREEN_FORMAT, folder, determineMaximumScale(), detect_keyboard_layout());

    SCOPE(studio_delete(platform.studio))
    {
        if (studio_config(platform.studio)->cli)
        {
            while (!studio_alive(platform.studio))
                studio_tick(platform.studio, platform.input);
        }
        else
        {
            initSound();

            {
                const s32 Width = ULI78_FULLWIDTH * studio_config(platform.studio)->uiScale;
                const s32 Height = ULI78_FULLHEIGHT * studio_config(platform.studio)->uiScale;

                s32 flags = SDL_WINDOW_SHOWN
#if !defined(__EMSCRIPTEN__) && !defined(__MACOSX__)
                        | SDL_WINDOW_ALLOW_HIGHDPI
#endif
                        | SDL_WINDOW_RESIZABLE;

#if defined(CRT_SHADER_SUPPORT)

                if(!studio_config(platform.studio)->soft)
                    flags |= SDL_WINDOW_OPENGL;
#endif

                platform.window = SDL_CreateWindow(ULI_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height, flags);

                setWindowIcon();
                initGPU();

#if defined(__ULI_ANDROID__)
                // The SDLActivity from SDL v2.32 starts with text input active.
                // We must explicitly stop it to show our custom keyboard by default.
                SDL_StopTextInput();
#endif

                if(studio_config(platform.studio)->options.fullscreen)
                    uli_sys_fullscreen_set(true);
            }

            SDL_PauseAudioDevice(platform.audio.device, 0);

#if defined(__EMSCRIPTEN__)
            emscripten_set_main_loop(emsGpuTick, 0, 1);
#else
            {
                const u64 Delta = SDL_GetPerformanceFrequency() / ULI78_FRAMERATE;
                u64 nextTick = SDL_GetPerformanceCounter();

                while (!studio_alive(platform.studio))
                {
                    gpuTick();

                    s64 delay = (nextTick += Delta) - SDL_GetPerformanceCounter();

                    if(delay > 0)
                        SDL_Delay((u32)(delay * 1000 / SDL_GetPerformanceFrequency()));
                    else if(delay < 0)
                        nextTick = SDL_GetPerformanceCounter();
                }
            }
#endif

#if defined(TOUCH_INPUT_SUPPORT)
            if(SDL_IsTextInputActive())
                SDL_StopTextInput();
#endif

            {
                destroyGPU();

#if defined(TOUCH_INPUT_SUPPORT)
                if(platform.gamepad.touch.pixels)
                    SDL_free(platform.gamepad.touch.pixels);

                if(platform.keyboard.touch.texture.upPixels)
                    SDL_free(platform.keyboard.touch.texture.upPixels);

                if(platform.keyboard.touch.texture.downPixels)
                    SDL_free(platform.keyboard.touch.texture.downPixels);
#endif

                SDL_DestroyWindow(platform.window);
                SDL_CloseAudioDevice(platform.audio.device);

                if (studio_config(platform.studio)->fft)
                {
                    FFT_Close();
                }
            }

            SDL_DestroyMutex(platform.audio.mutex);
        }
    }

    return 0;
}

#if defined(__EMSCRIPTEN__)

static void checkPreloadedFile()
{
    EM_ASM_(
    {
        if(Module.filePreloaded)
        {
            _emscripten_cancel_main_loop();
            var start = Module.startArg;
            dynCall('iiii', $0, [start.argc, start.argv, start.folder]);
        }
    }, start);
}

static s32 emsStart(s32 argc, char **argv, const char* folder)
{
    if (argc >= 2)
    {
        s32 pos = strlen(argv[1]) - strlen(".uli");
        if (pos >= 0 && strcmp(&argv[1][pos], ".uli") == 0)
        {
            const char* url = argv[1];

            {
                static char path[ULINAME_MAX];
                strcpy(path, folder);
                strcat(path, argv[1]);
                argv[1] = path;
            }

            EM_ASM_(
            {
                var start = {};
                start.argc = $0;
                start.argv = $1;
                start.folder = $2;

                Module.startArg = start;

                var file = PATH_FS.resolve(UTF8ToString($4));
                var dir = PATH.dirname(file);

                Module.filePreloaded = false;

                FS.createPreloadedFile(dir, PATH.basename(file), UTF8ToString($3), true, true,
                    function()
                    {
                        Module.filePreloaded = true;
                    },
                    function(){}, false, false, function()
                    {
                        try{FS.unlink(file);}catch(e){}
                        FS.mkdirTree(dir);
                    });

            }, argc, argv, folder, url, argv[1]);

            emscripten_set_main_loop(checkPreloadedFile, 0, 0);

            return 0;
        }
    }

    return start(argc, argv, folder);
}

#endif

s32 main(s32 argc, char **argv)
{
#if defined(__ULI_WINDOWS__)
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) && !info.dwCursorPosition.X && !info.dwCursorPosition.Y)
            FreeConsole();
    }
#elif defined(__ULI_LINUX__)
    signal(SIGPIPE, SIG_IGN);
#endif

    const char* folder = getAppFolder();

#if defined(__EMSCRIPTEN__)

    EM_ASM_
    (
        {
            Module.syncFSRequests = 0;

            var dir = UTF8ToString($0);

            FS.mkdirTree(dir);

            FS.mount(IDBFS, {}, dir);
            FS.syncfs(true, function(e)
            {
                dynCall('iiii', $1, [$2, $3, $0]);
            });

         }, folder, emsStart, argc, argv
    );

#else

    return start(argc, argv, folder);

#endif
}

// workaround to build app on Raspbian
#if defined(__RPI__)

#include <fcntl.h>
int fcntl64(int fd, int cmd, ...)
{
    return fcntl(fd, cmd);
}

#endif
