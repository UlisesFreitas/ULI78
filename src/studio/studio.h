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

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "uli.h"
#include "api.h"
#include "script.h"
#include "defines.h"
#include "tools.h"
#include "system.h"
#include "anim.h"
#include "ext/png.h"

#define KEYBOARD_HOLD 20
#define KEYBOARD_PERIOD 3

#ifdef BAREMETALPI
#define ULI_LOCAL "../.uli78/"
#else
#define ULI_LOCAL ".local/"
#endif
#define ULI_LOCAL_VERSION ULI_LOCAL ULI_VERSION_HASH "/"
#define ULI_CACHE ULI_LOCAL "cache/"

#define TOOLBAR_SIZE 7
#define STUDIO_TEXT_WIDTH (ULI_FONT_WIDTH)
#define STUDIO_TEXT_HEIGHT (ULI_FONT_HEIGHT+1)
#define STUDIO_TEXT_BUFFER_WIDTH (ULI78_WIDTH / STUDIO_TEXT_WIDTH)
#define STUDIO_TEXT_BUFFER_HEIGHT (ULI78_HEIGHT / STUDIO_TEXT_HEIGHT)
#define STUDIO_TEXT_BUFFER_SIZE (STUDIO_TEXT_BUFFER_WIDTH * STUDIO_TEXT_BUFFER_HEIGHT)
#define STUDIO_ANIM_TIME 8

#define ULI_COLOR_BG uli_color_black

#define CONFIG_TIC "config.uli"
#define CONFIG_ULI_PATH ULI_LOCAL_VERSION CONFIG_TIC

#define CART_EXT ".uli"
#define PNG_EXT ".png"

#if defined(CRT_SHADER_SUPPORT)
#   define CRT_CMD_PARAM(macro)                                 \
    macro(crt, bool, BOOLEAN, "", "enable CRT monitor effect")
#else
#   define CRT_CMD_PARAM(macro)
#endif

#define CMD_PARAMS_LIST(macro)                                                              \
    macro(skip,         int,    BOOLEAN,    "",         "skip startup animation")           \
    macro(volume,       s32,    INTEGER,    "=<int>",   "global volume value [0-15]")       \
    macro(cli,          int,    BOOLEAN,    "",         "console only output")              \
    macro(fullscreen,   int,    BOOLEAN,    "",         "enable fullscreen mode")           \
    macro(vsync,        int,    BOOLEAN,    "",         "enable VSYNC")                     \
    macro(soft,         int,    BOOLEAN,    "",         "use software rendering")           \
    macro(fs,           char*,  STRING,     "=<str>",   "path to the file system folder")   \
    macro(scale,        s32,    INTEGER,    "=<int>",   "main window scale")                \
    macro(cmd,          char*,  STRING,     "=<str>",   "run commands in the console")      \
    macro(keepcmd,      int,    BOOLEAN,    "",         "re-execute commands on every run") \
    macro(version,      int,    BOOLEAN,    "",         "print program version")            \
    CRT_CMD_PARAM(macro)

#define SHOW_TOOLTIP(STUDIO, FORMAT, ...)   \
do{                                         \
    static const char Format[] = FORMAT;    \
    static char buf[sizeof Format];         \
    sprintf(buf, Format, __VA_ARGS__);      \
    showTooltip(STUDIO, buf);               \
}while(0)

typedef struct
{
    char *cart;
#define CMD_PARAMS_DEF(name, ctype, type, post, help) ctype name;
    CMD_PARAMS_LIST(CMD_PARAMS_DEF)
#undef  CMD_PARAMS_DEF

#if defined(BUILD_EDITORS)
    const char *codeexport;
    const char *codeimport;
    s32 delay;
    s32 lowerlimit;
    s32 upperlimit;
    s32 battletime;

    int fft;
    int fftlist;
    int fftcaptureplaybackdevices;
    const char *fftdevice;
#endif
} StartArgs;

typedef enum
{
    ULI_START_MODE,
    ULI_CONSOLE_MODE,
    ULI_RUN_MODE,
    ULI_CODE_MODE,
    ULI_SPRITE_MODE,
    ULI_MAP_MODE,
    ULI_WORLD_MODE,
    ULI_SFX_MODE,
    ULI_MUSIC_MODE,
    ULI_MENU_MODE,
    ULI_SURF_MODE,

    ULI_MODES_COUNT
} EditorMode;

typedef enum
{
    VI_NORMAL,
    VI_INSERT,
    VI_SELECT,
    VI_SEEK,
    VI_SEEK_BACK,
} ViMode;

enum
{
    uli_icon_cut        = 80,
    uli_icon_copy       = 81,
    uli_icon_paste      = 82,
    uli_icon_undo       = 83,
    uli_icon_redo       = 84,
    uli_icon_bank       = 85,
    uli_icon_pin        = 86,
    uli_icon_tab        = 87,
    uli_icon_code       = 88,
    uli_icon_sprite     = 89,
    uli_icon_map        = 90,
    uli_icon_sfx        = 91,
    uli_icon_music      = 92,
    uli_icon_rec        = 93,
    uli_icon_rec2       = 94,
    uli_icon_bookmark   = 95,
    uli_icon_shadow     = 96,
    uli_icon_shadow2    = 97,
    uli_icon_run        = 98,
    uli_icon_hand       = 99,
    uli_icon_find       = 100,
    uli_icon_goto       = 101,
    uli_icon_outline    = 102,
    uli_icon_world      = 103,
    uli_icon_grid       = 104,
    uli_icon_down       = 105,
    uli_icon_up         = 106,
    uli_icon_fill       = 107,
    uli_icon_select     = 108,
    uli_icon_pen        = 109,
    uli_icon_tiles      = 110,
    uli_icon_sprites    = 111,
    uli_icon_left       = 112,
    uli_icon_right      = 113,
    uli_icon_piano      = 114,
    uli_icon_tracker    = 115,
    uli_icon_follow     = 116,
    uli_icon_sustain    = 117,
    uli_icon_playnow    = 118,
    uli_icon_playframe  = 119,
    uli_icon_stop       = 120,
    uli_icon_rgb        = 121,
    uli_icon_tinyleft   = 122,
    uli_icon_pos        = 123,
    uli_icon_tinyright  = 124,
    uli_icon_bigup      = 125,
    uli_icon_bigdown    = 126,
    uli_icon_bigleft    = 127,
    uli_icon_bigright   = 128,
    uli_icon_fliphorz   = 129,
    uli_icon_flipvert   = 130,
    uli_icon_rotate     = 131,
    uli_icon_erase      = 132,
    uli_icon_bigpen     = 133,
    uli_icon_bigpicker  = 134,
    uli_icon_bigselect  = 135,
    uli_icon_bigfill    = 136,
    uli_icon_loop       = 137,
    uli_icon_ai         = 118,
};

void setCursor(Studio* studio, uli_cursor id);

bool checkMousePos(Studio* studio, const uli_rect* rect);
bool checkMouseClick(Studio* studio, const uli_rect* rect, uli_mouse_btn button);
bool checkMouseDblClick(Studio* studio, const uli_rect* rect, uli_mouse_btn button);
bool checkMouseDown(Studio* studio, const uli_rect* rect, uli_mouse_btn button);

void drawToolbar(Studio* studio, uli_mem* uli, bool bg);
void drawBitIcon(Studio* studio, s32 id, s32 x, s32 y, u8 color);

uli_cartridge* loadPngCart(png_buffer buffer);
void studioRomLoaded(Studio* studio);
void studioRomSaved(Studio* studio);
void studioConfigChanged(Studio* studio);

void setStudioMode(Studio* studio, EditorMode mode);
EditorMode getStudioMode(Studio* studio);
void exitStudio(Studio* studio);

void setStudioViMode(Studio* studio, ViMode mode);
ViMode getStudioViMode(Studio* studio);
bool checkStudioViMode(Studio* studio, ViMode mode);

void toClipboard(const void* data, s32 size, bool flip);
bool fromClipboard(void* data, s32 size, bool flip, bool remove_white_spaces, bool sameSize);

typedef enum
{
    ULI_CLIPBOARD_NONE,
    ULI_CLIPBOARD_CUT,
    ULI_CLIPBOARD_COPY,
    ULI_CLIPBOARD_PASTE,
} ClipboardEvent;

ClipboardEvent getClipboardEvent(Studio* studio);

typedef enum
{
    ULI_TOOLBAR_CUT,
    ULI_TOOLBAR_COPY,
    ULI_TOOLBAR_PASTE,
    ULI_TOOLBAR_UNDO,
    ULI_TOOLBAR_REDO,
} StudioEvent;

void setStudioEvent(Studio* studio, StudioEvent event);
void showTooltip(Studio* studio, const char* text);

void setSpritePixel(uli_tile* tiles, s32 x, s32 y, u8 color);
u8 getSpritePixel(uli_tile* tiles, s32 x, s32 y);

typedef void(*ConfirmCallback)(Studio* studio, bool yes, void* data);
void confirmDialog(Studio* studio, const char** text, s32 rows, ConfirmCallback callback, void* data);
void confirmLoadCart(Studio* studio, ConfirmCallback callback, void* data);

bool studioCartChanged(Studio* studio);
void playSystemSfx(Studio* studio, s32 id);

void gotoMenu(Studio* studio);
void gotoCode(Studio* studio);
void gotoSurf(Studio* studio);

void runGame(Studio* studio);
void exitGame(Studio* studio);
void resumeGame(Studio* studio);
void saveProject(Studio* studio);

uli_tiles* getBankTiles(Studio* studio);
uli_palette* getBankPalette(Studio* studio, bool bank);
uli_flags* getBankFlags(Studio* studio);
uli_map* getBankMap(Studio* studio);

char getKeyboardText(Studio* studio);
bool keyWasPressed(Studio* studio, uli_key key);
bool enterWasPressed(Studio* studio);
bool anyKeyWasPressed(Studio* studio);
bool ticEnterWasPressed(uli_mem* uli, s32 hold, s32 period);

const StudioConfig* getConfig(Studio* studio);
struct Start* getStartScreen(Studio* studio);
struct Sprite* getSpriteEditor(Studio* studio);

const char* studioExportMusic(Studio* studio, s32 track, s32 bank, const char* filename);
const char* studioExportSfx(Studio* studio, s32 sfx, const char* filename);

uli_mem* getMemory(Studio* studio);

const char* md5str(const void* data, s32 length);
void sfx_stop(uli_mem* uli, s32 channel);
s32 calcWaveAnimation(uli_mem* uli, u32 index, s32 channel);
void map2ram(uli_ram* ram, const uli_map* src);
void tiles2ram(uli_ram* ram, const uli_tiles* src);
void fadePalette(uli_palette* pal, s32 value);
bool project_ext(const char* name);

#if defined(BUILD_EDITORS)

typedef struct
{
    char* exp;
    char* imp;

    struct
    {
        uli_code code;
        char postag[32];
    } last;

    s32 delay;
    s32 ticks;

    struct
    {
        s32 lower;
        s32 upper;
        s32 current;
    } limit;

    struct
    {
        s32 started;
        s32 time;
        s32 left;

        bool hidetime;
    } battle;

} Bytebattle;

Bytebattle* getBytebattle(Studio* studio);

#endif
