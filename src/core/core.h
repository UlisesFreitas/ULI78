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

#include "api.h"
#include "tools.h"
#include "script.h"

#define CLOCKRATE (255<<13)
#define ULI_DEFAULT_COLOR 15
#define ULI_SOUND_RINGBUF_LEN 12 // in worst case, this induces ~ 12 tick delay i.e. 200 ms

typedef struct
{
    s32 time;       /* clock time of next delta */
    s32 phase;      /* position within waveform */
    s32 amp;        /* current amplitude in delta buffer */
}uli_sound_register_data;

typedef struct
{
    s32 tick;
    uli_sfx_pos* pos;
    s32 index;
    s32 note;
    struct
    {
        u8 left:4;
        u8 right:4;
    } volume;
    s8 speed:SFX_SPEED_BITS;
    s32 duration;
} uli_channel_data;

typedef struct
{
    struct
    {
        s32 tick;
        u8 note1:4;
        u8 note2:4;
    } chord;

    struct
    {
        s32 tick;
        u8 period:4;
        u8 depth:4;
    } vibrato;

    struct
    {
        s32 tick;
        u8 note;
        s32 duration;
    } slide;

    struct
    {
        s32 value;
    } finepitch;

    struct
    {
        const uli_track_row* row;
        s32 ticks;
    } delay;

} uli_command_data;

typedef struct
{
    bool active;
    s32 frame;
    s32 beat;
} uli_jump_command;

typedef struct
{

    struct
    {
        uli78_gamepads previous;
        uli78_gamepads now;

        u32 holds[sizeof(uli78_gamepads) * BITS_IN_BYTE];
    } gamepads;

    struct
    {
        uli78_keyboard previous;
        uli78_keyboard now;

        u32 holds[uli_keys_count];
    } keyboard;

    struct
    {
        struct sound_register_data
        {
            uli_sound_register_data data[ULI_SOUND_CHANNELS];
            uli_sound_register_data pcm;
        } left, right;
    } registers;

    struct sound_ring_buf
    {
        uli_sound_register registers[ULI_SOUND_CHANNELS];
        uli_stereo_volume stereo;
        uli_pcm pcm;
    } sound_ringbuf[ULI_SOUND_RINGBUF_LEN];

    u32 sound_ringbuf_head;
    u32 sound_ringbuf_tail;

    struct
    {
        uli_channel_data channels[ULI_SOUND_CHANNELS];
    } sfx;

    struct
    {
        s32 ticks;
        uli_channel_data channels[ULI_SOUND_CHANNELS];
        uli_command_data commands[ULI_SOUND_CHANNELS];
        uli_sfx_pos sfxpos[ULI_SOUND_CHANNELS];
        uli_jump_command jump;
        s32 tempo;
        s32 speed;
    } music;

    uli_tick tick;
    uli_blit_callback callback;

    u32 synced;

    struct
    {
        s32 id;
        uli_vram mem;
    } vbank;

    struct ClipRect
    {
        s32 l, t, r, b;
    } clip;

    bool initialized;
} uli_core_state_data;

typedef struct
{
    uli_mem memory; // it should be first
    uli78_pixel_color_format screen_format;

    void* currentVM;
    const uli_script* currentScript;

    struct
    {
        struct blip_t* left;
        struct blip_t* right;
    } blip;

    s32 samplerate;
    uli_tick_data* data;
    uli_core_state_data state;

    struct
    {
        uli_core_state_data state;
        uli_ram ram;
        u8 input;

        struct
        {
            u64 start;
            u64 paused;
        } time;
    } pause;

    struct
    {
    #define API_FUNC_DEF(name, _, __, ___, ____, _____, ret, ...) ret (*name)(__VA_ARGS__);
        ULI_API_LIST(API_FUNC_DEF)
    #undef  API_FUNC_DEF

#if defined BUILD_DEPRECATED
        void (*textri)(uli_mem* uli, float x1, float y1, float x2, float y2, float x3, float y3, float u1, float v1, float u2, float v2, float u3, float v3, bool use_map, u8* colors, s32 count);
#endif
    } api;

} uli_core;

void uli_core_tick_io(uli_mem* memory);
void uli_core_sound_tick_start(uli_mem* memory);
void uli_core_sound_tick_end(uli_mem* memory);
void resetSfxPos(uli_channel_data* channel);

#if defined(BUILD_DEPRECATED)
// mouse cursor is the same in both modes
// for backward compatibility
#define OVR_COMPAT(CORE, BANK)                                              \
    core->api.vbank(&CORE->memory, BANK),                                     \
    CORE->memory.ram->vram.vars.cursor = CORE->state.vbank.mem.vars.cursor

#define OVR(CORE)                                   \
    s32 MACROVAR(_bank_) = CORE->state.vbank.id;    \
    OVR_COMPAT(CORE, 1);                            \
    core->api.cls(&CORE->memory, 0);                  \
    SCOPE(OVR_COMPAT(CORE, MACROVAR(_bank_)))

#endif
