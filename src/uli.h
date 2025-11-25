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
#include "uli78.h"
#include "defines.h"

#define ULI_VRAM_SIZE (16*1024) //16K
#define ULI_RAM_SIZE (ULI_VRAM_SIZE+80*1024) //16K+80K
#define ULI_WASM_PAGE_COUNT 4 // 256K
#define ULI_FONT_WIDTH 6
#define ULI_FONT_HEIGHT 6
#define ULI_ALTFONT_WIDTH 4
#define ULI_PALETTE_BPP 4
#define ULI_PALETTE_SIZE (1 << ULI_PALETTE_BPP)
#define ULI_PALETTES 2
#define ULI_SPRITESIZE 8

#define ULI_DEFAULT_BIT_DEPTH 4
#define ULI_DEFAULT_BLIT_MODE 2

#define ULI78_OFFSET_LEFT ((ULI78_FULLWIDTH-ULI78_WIDTH)/2)
#define ULI78_OFFSET_TOP ((ULI78_FULLHEIGHT-ULI78_HEIGHT)/2)

#define BITS_IN_BYTE 8
#define ULI_BANK_SPRITES (1 << BITS_IN_BYTE)
#define ULI_SPRITE_BANKS 2
#define ULI_FLAGS (ULI_BANK_SPRITES * ULI_SPRITE_BANKS)
#define ULI_SPRITES (ULI_BANK_SPRITES * ULI_SPRITE_BANKS)

#define ULI_SPRITESHEET_SIZE 128
#define ULI_SPRITESHEET_COLS (ULI_SPRITESHEET_SIZE / ULI_SPRITESIZE)

#define ULI_MAP_ROWS (ULI_SPRITESIZE)
#define ULI_MAP_COLS (ULI_SPRITESIZE)
#define ULI_MAP_SCREEN_WIDTH (ULI78_WIDTH / ULI_SPRITESIZE)
#define ULI_MAP_SCREEN_HEIGHT (ULI78_HEIGHT / ULI_SPRITESIZE)
#define ULI_MAP_WIDTH (ULI_MAP_SCREEN_WIDTH * ULI_MAP_ROWS)
#define ULI_MAP_HEIGHT (ULI_MAP_SCREEN_HEIGHT * ULI_MAP_COLS)

#define ULI_PERSISTENT_SIZE (1024/sizeof(s32)) // 1K
#define ULI_SAVEID_SIZE 64

#define ULI_SOUND_CHANNELS 4
#define SFX_TICKS 30
#define SFX_COUNT_BITS 6
#define SFX_COUNT (1 << SFX_COUNT_BITS)
#define SFX_SPEED_BITS 3
#define SFX_DEF_SPEED (1 << SFX_SPEED_BITS)

#define NOTES 12
#define OCTAVES 8
#define MAX_VOLUME 15
#define MUSIC_PATTERN_ROWS 64
#define MUSIC_PATTERNS 60
#define MUSIC_CMD_BITS 3
#define TRACK_PATTERN_BITS 6
#define TRACK_PATTERN_MASK ((1 << TRACK_PATTERN_BITS) - 1)
#define TRACK_PATTERNS_SIZE (TRACK_PATTERN_BITS * ULI_SOUND_CHANNELS / BITS_IN_BYTE)
#define MUSIC_FRAMES 16
#define MUSIC_TRACKS 8
#define DEFAULT_TEMPO 150
#define DEFAULT_SPEED 6
#define PITCH_DELTA 128
#define NOTES_PER_BEAT 4
#define PATTERN_START 1
#define MUSIC_SFXID_LOW_BITS 5
#define WAVES_COUNT 16
#define WAVE_VALUES 32
#define WAVE_VALUE_BITS 4
#define WAVE_MAX_VALUE ((1 << WAVE_VALUE_BITS) - 1)
#define WAVE_SIZE (WAVE_VALUES * WAVE_VALUE_BITS / BITS_IN_BYTE)
#define ULI_PCM_SIZE 128

#define ULI_BANKSIZE_BITS 16
#define ULI_BANK_SIZE (1 << ULI_BANKSIZE_BITS) // 64K
#define ULI_BANK_BITS 3
#define ULI_BANKS (1 << ULI_BANK_BITS)

#define ULI_CODE_SIZE (ULI_BANK_SIZE * ULI_BANKS)
#define ULI_BINARY_BANKS 4
#define ULI_BINARY_SIZE (ULI_BINARY_BANKS * ULI_BANK_SIZE) // 4 * 64k = 256K


#define ULI_BUTTONS 16
#define ULI_GAMEPADS (sizeof(uli78_gamepads) / sizeof(uli78_gamepad))

#define SFX_NOTES {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"}
#define ULI_FONT_CHARS 128

#define ULI_UNUSED(x) (void)x

enum
{
    NoteNone = 0,
    NoteStop,
    NoteNone2,
    NoteNone3,
    NoteStart,
};

typedef enum
{
    uli_color_black,
    uli_color_purple,
    uli_color_red,
    uli_color_orange,
    uli_color_yellow,
    uli_color_light_green,
    uli_color_green,
    uli_color_dark_green,
    uli_color_dark_blue,
    uli_color_blue,
    uli_color_light_blue,
    uli_color_cyan,
    uli_color_white,
    uli_color_light_grey,
    uli_color_grey,
    uli_color_dark_grey,
} uli_color;

typedef enum
{
    uli_no_flip = 0,
    uli_horz_flip = 1,
    uli_vert_flip = 2,
} uli_flip;

typedef enum
{
    uli_no_rotate,
    uli_90_rotate,
    uli_180_rotate,
    uli_270_rotate,
} uli_rotate;

typedef enum
{
    uli_bpp_4 = 4,
    uli_bpp_2 = 2,
    uli_bpp_1 = 1,
} uli_bpp;

typedef struct
{
#if RETRO_IS_BIG_ENDIAN
    u8 size:4;
    u8 start:4;
#else
    u8 start:4;
    u8 size:4;
#endif
} uli_sound_loop;

typedef struct
{

    struct
    {
#if RETRO_IS_BIG_ENDIAN
        u8 wave:4;
        u8 volume:4;
        s8 pitch:4;
        u8 chord:4;
#else
        u8 volume:4;
        u8 wave:4;
        u8 chord:4;
        s8 pitch:4;
#endif
    } data[SFX_TICKS];

    struct
    {
#if RETRO_IS_BIG_ENDIAN
        u8 reverse:1; // chord reverse
        s8 speed:SFX_SPEED_BITS;
        u8 pitch16x:1; // pitch factor
        u8 octave:3;
        u8 temp:2;
        u8 stereo_right:1;
        u8 stereo_left:1;
        u8 note:4;
#else
        u8 octave:3;
        u8 pitch16x:1; // pitch factor
        s8 speed:SFX_SPEED_BITS;
        u8 reverse:1; // chord reverse
        u8 note:4;
        u8 stereo_left:1;
        u8 stereo_right:1;
        u8 temp:2;
#endif
    };

    union
    {
        struct
        {
            uli_sound_loop wave;
            uli_sound_loop volume;
            uli_sound_loop chord;
            uli_sound_loop pitch;
        };

        uli_sound_loop loops[4];
    };

} uli_sample;

typedef struct
{
    union
    {
        struct
        {
            s8 wave;
            s8 volume;
            s8 chord;
            s8 pitch;
        };

        s8 data[4];
    };
} uli_sfx_pos;

typedef struct
{
    u8 data[WAVE_SIZE];
}uli_waveform;

typedef struct
{
    uli_waveform items[WAVES_COUNT];
} uli_waveforms;

#define MUSIC_CMD_LIST(macro)                                               \
    macro(empty,    0, "")                                                  \
    macro(volume,   M, "master volume for LEFT=X / RIGHT=Y channel")        \
    macro(chord,    C, "play chord, X=3 Y=7 plays +0,+3,+7 notes")          \
    macro(jump,     J, "jump to FRAME=X / BEAT=Y")                          \
    macro(slide,    S, "slide to note (legato) with TICKS=XY")              \
    macro(pitch,    P, "finepitch UP/DOWN=XY-" DEF2STR(PITCH_DELTA))        \
    macro(vibrato,  V, "vibrato with PERIOD=X and DEPTH=Y")                 \
    macro(delay,    D, "delay triggering a note with TICKS=XY")

typedef enum
{
#define ENUM_ITEM(name, ...) uli_music_cmd_##name,
    MUSIC_CMD_LIST(ENUM_ITEM)
#undef ENUM_ITEM

    uli_music_cmd_count
} uli_music_command;

typedef struct
{
#if RETRO_IS_BIG_ENDIAN
    u8 param1   :4;
    u8 note     :4;
    u8 sfxhi    :1;
    u8 command  :MUSIC_CMD_BITS; // uli_music_command
    u8 param2   :4;
    u8 octave   :3;
    u8 sfxlow   :MUSIC_SFXID_LOW_BITS;
#else
    u8 note     :4;
    u8 param1   :4;
    u8 param2   :4;
    u8 command  :MUSIC_CMD_BITS; // uli_music_command
    u8 sfxhi    :1;
    u8 sfxlow   :MUSIC_SFXID_LOW_BITS;
    u8 octave   :3;
#endif
} uli_track_row;

typedef struct
{
    uli_track_row rows[MUSIC_PATTERN_ROWS];

} uli_track_pattern;

typedef struct
{
    u8 data[MUSIC_FRAMES * TRACK_PATTERNS_SIZE]; // sfx - 6bits per channel = 24 bit

    s8 tempo; // delta value, rel to 120 bpm * 10 [32-255]
    u8 rows; // delta value, rel to 64 rows, can be [1-64]
    s8 speed; // delta value, rel to 6 [1-31]

} uli_track;

typedef struct
{
    uli_track_pattern data[MUSIC_PATTERNS];
} uli_patterns;

typedef struct
{
    uli_track data[MUSIC_TRACKS];
} uli_tracks;

typedef struct
{
    uli_sample data[SFX_COUNT];
} uli_samples;

typedef struct
{
    uli_waveforms waveforms;
    uli_samples samples;
}uli_sfx;

typedef struct
{
    uli_patterns patterns;
    uli_tracks tracks;
}uli_music;

typedef enum
{
    uli_music_stop,
    uli_music_play_frame,
    uli_music_play,
} uli_music_status;

typedef struct
{
    struct
    {
        s8 track;
        s8 frame;
        s8 row;
    } music;

    struct
    {
#if RETRO_IS_BIG_ENDIAN
        u8 unknown:4;
        u8 music_sustain:1;
        u8 music_status:2; // enum uli_music_status
        u8 music_loop:1;
#else
        u8 music_loop:1;
        u8 music_status:2; // enum uli_music_status
        u8 music_sustain:1;
        u8 unknown:4;
#endif
    } flag;

} uli_music_state;

typedef union
{
    struct
    {
#if RETRO_IS_BIG_ENDIAN
        u8 right4:4;
        u8 left4:4;

        u8 right3:4;
        u8 left3:4;

        u8 right2:4;
        u8 left2:4;

        u8 right1:4;
        u8 left1:4;
#else
        u8 left1:4;
        u8 right1:4;

        u8 left2:4;
        u8 right2:4;

        u8 left3:4;
        u8 right3:4;

        u8 left4:4;
        u8 right4:4;
#endif
    };

    u32 data;
} uli_stereo_volume;

typedef struct
{
    struct
    {
	u8 freq_low;
#if RETRO_IS_BIG_ENDIAN
        u8 volume:4;
        u8 freq_high:4;
#else
        u8 freq_high:4;
        u8 volume:4;
#endif
    };

    uli_waveform waveform;
} uli_sound_register;


static INLINE u16 uli_sound_register_get_freq(const uli_sound_register* reg)
{
    return (reg->freq_high << 8) | reg->freq_low;
}

static INLINE void uli_sound_register_set_freq(uli_sound_register* reg, u16 val)
{
    reg->freq_low = val;
    reg->freq_high = val >> 8;
}

typedef struct
{
    u8 data[ULI_MAP_WIDTH * ULI_MAP_HEIGHT];
} uli_map;

typedef struct
{
    u8 data[ULI_SPRITESIZE * ULI_SPRITESIZE * ULI_PALETTE_BPP / BITS_IN_BYTE];
} uli_tile;

typedef struct
{
    char data[ULI_CODE_SIZE];
} uli_code;

typedef struct
{
    char data[ULI_BINARY_SIZE];
    u32 size;
} uli_binary;

typedef struct
{
    u8 r;
    u8 g;
    u8 b;
} uli_rgb;

typedef union
{
    uli_rgb colors[ULI_PALETTE_SIZE];

    u8 data[ULI_PALETTE_SIZE * sizeof(uli_rgb)];
} uli_palette;

typedef struct
{
    u32 data[ULI_PALETTE_SIZE];
} uli_blitpal;

typedef struct
{
    uli_tile data[ULI_BANK_SPRITES];
} uli_tiles, uli_sprites;

typedef struct
{
    u8 data[ULI_FLAGS];
} uli_flags;

typedef struct
{
    uli_palette vbank0;
    uli_palette vbank1;
} uli_palettes;

typedef struct
{
    u8 data[ULI78_WIDTH * ULI78_HEIGHT * ULI_PALETTE_BPP / BITS_IN_BYTE];
} uli_screen;

typedef struct
{
    uli_screen      screen;
    uli_tiles       tiles;
    uli_sprites     sprites;
    uli_map         map;
    uli_sfx         sfx;
    uli_music       music;
    uli_flags       flags;
    uli_palettes    palette;
} uli_bank;

typedef struct
{
    union
    {
        uli_bank bank0;
        uli_bank banks[ULI_BANKS];
    };

    uli_code code;
    uli_binary binary;
    u8 lang;

} uli_cartridge;

typedef struct
{
    u8 data[(ULI_FONT_CHARS - 1) * BITS_IN_BYTE];

    union
    {
        struct
        {
            u8 width;
            u8 height;
        };

        u8 params[BITS_IN_BYTE];
    };
} uli_font_data;

typedef struct
{
    uli_font_data regular;
    uli_font_data alt;
} uli_font;

typedef union
{
    struct
    {
        uli_screen screen;
        uli_palette palette;
        u8 mapping[ULI_PALETTE_SIZE * ULI_PALETTE_BPP / BITS_IN_BYTE];

        struct
        {
            union
            {
		struct {
#if RETRO_IS_BIG_ENDIAN
		    u8 padding_border:4;
		    u8 border:ULI_PALETTE_BPP;
#else
		    u8 border:ULI_PALETTE_BPP;
		    u8 padding_border:4;
#endif
		};
                // clear color for the BANK1
		struct {
#if RETRO_IS_BIG_ENDIAN
		    u8 padding_clear:4;
		    u8 clear:ULI_PALETTE_BPP;
#else
		    u8 clear:ULI_PALETTE_BPP;
		    u8 padding_clear:4;
#endif
		};
            };

            struct
            {
                s8 x;
                s8 y;
            } offset;

            struct
            {
#if RETRO_IS_BIG_ENDIAN
                u8 system:1;
                u8 sprite:7;
#else
                u8 sprite:7;
                u8 system:1;
#endif
            } cursor;
        } vars;

        struct
        {
#if RETRO_IS_BIG_ENDIAN
            u8 reserved:4;
            u8 segment:4;
#else
            u8 segment:4;
            u8 reserved:4;
#endif
        } blit;

        u8 reserved[3];
    };

    u8 data[ULI_VRAM_SIZE];
} uli_vram;

typedef struct
{
    u32 data[ULI_PERSISTENT_SIZE];
} uli_persistent;

typedef struct
{
    u8 data[ULI_GAMEPADS * ULI_BUTTONS];
} uli_mapping;

typedef struct
{
    u8 data[ULI_PCM_SIZE];
} uli_pcm;

typedef union
{
    struct
    {
        uli_vram            vram;
        uli_tiles           tiles;
        uli_sprites         sprites;
        uli_map             map;
        uli78_input         input;
        uli_sfx_pos         sfxpos[ULI_SOUND_CHANNELS];
        uli_sound_register  registers[ULI_SOUND_CHANNELS];
        uli_sfx             sfx;
        uli_music           music;
        uli_music_state     music_state;
        uli_stereo_volume   stereo;
        uli_persistent      persistent;
        uli_flags           flags;
        uli_font            font;
        uli_mapping         mapping;
        uli_pcm             pcm;

        u8 free;
    };

    u8 data[ULI_RAM_SIZE];

} uli_ram;

typedef enum
{
    uli_key_unknown,

    uli_key_a,
    uli_key_b,
    uli_key_c,
    uli_key_d,
    uli_key_e,
    uli_key_f,
    uli_key_g,
    uli_key_h,
    uli_key_i,
    uli_key_j,
    uli_key_k,
    uli_key_l,
    uli_key_m,
    uli_key_n,
    uli_key_o,
    uli_key_p,
    uli_key_q,
    uli_key_r,
    uli_key_s,
    uli_key_t,
    uli_key_u,
    uli_key_v,
    uli_key_w,
    uli_key_x,
    uli_key_y,
    uli_key_z,

    uli_key_0,
    uli_key_1,
    uli_key_2,
    uli_key_3,
    uli_key_4,
    uli_key_5,
    uli_key_6,
    uli_key_7,
    uli_key_8,
    uli_key_9,

    uli_key_minus,
    uli_key_equals,
    uli_key_leftbracket,
    uli_key_rightbracket,
    uli_key_backslash,
    uli_key_semicolon,
    uli_key_apostrophe,
    uli_key_grave,
    uli_key_comma,
    uli_key_period,
    uli_key_slash,

    uli_key_space,
    uli_key_tab,

    uli_key_return,
    uli_key_backspace,
    uli_key_delete,
    uli_key_insert,

    uli_key_pageup,
    uli_key_pagedown,
    uli_key_home,
    uli_key_end,
    uli_key_up,
    uli_key_down,
    uli_key_left,
    uli_key_right,

    uli_key_capslock,
    uli_key_ctrl,
    uli_key_shift,
    uli_key_alt,

    uli_key_escape,
    uli_key_f1,
    uli_key_f2,
    uli_key_f3,
    uli_key_f4,
    uli_key_f5,
    uli_key_f6,
    uli_key_f7,
    uli_key_f8,
    uli_key_f9,
    uli_key_f10,
    uli_key_f11,
    uli_key_f12,

    uli_key_numpad0,
    uli_key_numpad1,
    uli_key_numpad2,
    uli_key_numpad3,
    uli_key_numpad4,
    uli_key_numpad5,
    uli_key_numpad6,
    uli_key_numpad7,
    uli_key_numpad8,
    uli_key_numpad9,
    uli_key_numpadplus,
    uli_key_numpadminus,
    uli_key_numpadmultiply,
    uli_key_numpaddivide,
    uli_key_numpadenter,
    uli_key_numpadperiod,

    ////////////////

    uli_keys_count
} uli_keycode;

typedef enum
{
    uli_mouse_left,
    uli_mouse_middle,
    uli_mouse_right,
} uli_mouse_btn;

typedef enum
{
    uli_cursor_arrow,
    uli_cursor_hand,
    uli_cursor_ibeam,
} uli_cursor;
