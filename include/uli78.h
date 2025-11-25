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

#pragma once

#include "retro_endianness.h"
#include "uli78_config.h"
#include "uli78_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ULI78_WIDTH             240
#define ULI78_HEIGHT            136
#define ULI78_FULLWIDTH_BITS    8
#define ULI78_FULLWIDTH         (1 << ULI78_FULLWIDTH_BITS)
#define ULI78_FULLHEIGHT        (ULI78_FULLWIDTH*9/16)

#define ULI78_MARGIN_TOP        ((ULI78_FULLHEIGHT - ULI78_HEIGHT) / 2)
#define ULI78_MARGIN_BOTTOM     ULI78_MARGIN_TOP
#define ULI78_MARGIN_LEFT       ((ULI78_FULLWIDTH - ULI78_WIDTH) / 2)
#define ULI78_MARGIN_RIGHT      ULI78_MARGIN_LEFT

#define ULI78_KEY_BUFFER        4
#define ULI78_SAMPLERATE        44100
#define ULI78_SAMPLETYPE        s16
#define ULI78_SAMPLESIZE        sizeof(ULI78_SAMPLETYPE)
#define ULI78_SAMPLE_CHANNELS   2
#define ULI78_FRAMERATE         60

typedef enum {
    ULI78_PIXEL_COLOR_ARGB8888 = (1 << 8) | 32,
    ULI78_PIXEL_COLOR_ABGR8888 = (2 << 8) | 32,
    ULI78_PIXEL_COLOR_RGBA8888 = (3 << 8) | 32,
    ULI78_PIXEL_COLOR_BGRA8888 = (4 << 8) | 32
} uli78_pixel_color_format;

typedef struct
{
    struct
    {
        void (*trace)(const char* text, u8 color);
        void (*error)(const char* info);
        void (*exit)();
    } callback;

    struct
    {
        ULI78_SAMPLETYPE* buffer;
        s32 count;
    } samples;

    u32 *screen;
} uli78;

typedef union
{
    struct
    {
#if RETRO_IS_BIG_ENDIAN
        u16 _reserved:1;
        u16 guide:1;
        u16 r2:1;
        u16 l2:1;
        u16 r1:1;
        u16 l1:1;
        u16 select:1;
        u16 start:1;
        u16 y:1;
        u16 x:1;
        u16 b:1;
        u16 a:1;
        u16 right:1;
        u16 left:1;
        u16 down:1;
        u16 up:1;
#else
        u16 up:1;
        u16 down:1;
        u16 left:1;
        u16 right:1;
        u16 a:1;
        u16 b:1;
        u16 x:1;
        u16 y:1;
        u16 start:1;
        u16 select:1;
        u16 l1:1;
        u16 r1:1;
        u16 l2:1;
        u16 r2:1;
        u16 guide:1;
        u16 _reserved:1;
#endif
    };

    u16 data;
} uli78_gamepad;

typedef union
{
    struct
    {
#if RETRO_IS_BIG_ENDIAN
        uli78_gamepad fourth;
        uli78_gamepad third;
        uli78_gamepad second;
        uli78_gamepad first;
#else
        uli78_gamepad first;
        uli78_gamepad second;
        uli78_gamepad third;
        uli78_gamepad fourth;
#endif
    };

    u64 data;
} uli78_gamepads;

typedef struct
{
    union
    {
        // absolute pos
        struct
        {
            u8 x;
            u8 y;
        };

        // relative values
        struct
        {
            s8 rx;
            s8 ry;
        };
    };
    
    union
    {
        struct
        {
            u16 left:1;
            u16 middle:1;
            u16 right:1;

            s16 scrollx:6;
            s16 scrolly:6;

            u16 relative:1;
        };

        u16 btns;
    };
} uli78_mouse;

typedef u8 uli_key;

typedef union
{
    uli_key keys[ULI78_KEY_BUFFER];
    u32 data;
} uli78_keyboard;

typedef enum {
    uli_layout_unknown = 0,
    uli_layout_qwerty,
    uli_layout_azerty,
    uli_layout_qwertz,
    uli_layout_qzerty,
    uli_layout_de_neo,
    uli_layout_de_bone
} uli_layout;

typedef struct
{
    union { uli78_gamepads gamepads; u64 gamepads_data; };
    union { uli78_mouse mouse; u32 mouse_data; };
    union { uli78_keyboard keyboard; u32 keyboard_data; };

} uli78_input;

ULI78_API uli78* uli78_create(s32 samplerate, uli78_pixel_color_format format);
ULI78_API void uli78_load(uli78* uli, void* cart, s32 size);
ULI78_API void uli78_tick(uli78* uli, uli78_input input, u64 (*counter)(), u64 (*freq)());
ULI78_API void uli78_sound(uli78* uli);
ULI78_API void uli78_delete(uli78* uli);

#ifdef __cplusplus
}
#endif
