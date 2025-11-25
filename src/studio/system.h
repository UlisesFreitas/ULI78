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
#include "version.h"

#if defined(ULI78_PRO)
#define ULI_VERSION_POST " PRO"
#else
#define ULI_VERSION_POST ""
#endif

#define ULI_VERSION DEF2STR(ULI_VERSION_MAJOR) "." DEF2STR(ULI_VERSION_MINOR) "." DEF2STR(ULI_VERSION_REVISION) ULI_VERSION_STATUS ULI_VERSION_BUILD ULI_VERSION_POST " (" ULI_VERSION_HASH ")"
#define ULI_PACKAGE "com.uli78.uli"
#define ULI_NAME "ULI-78"
#define ULI_NAME_FULL ULI_NAME " "
#define ULI_TITLE ULI_NAME_FULL " " ULI_VERSION
#define ULI_HOST "uli78.com"
#if defined(__ULI_WIN7__)
    #define ULI_WEBSITE_PROTOCOL "http://"
#else
    #define ULI_WEBSITE_PROTOCOL "https://"
#endif
#define ULI_WEBSITE ULI_WEBSITE_PROTOCOL ULI_HOST
#define ULI_COPYRIGHT ULI_WEBSITE " (C) 2017-" ULI_VERSION_YEAR

#define ULINAME_MAX 256

#ifdef __cplusplus
extern "C" {
#endif

void    uli_sys_clipboard_set(const char* text);
bool    uli_sys_clipboard_has();
char*   uli_sys_clipboard_get();
void    uli_sys_clipboard_free(const char* text);
u64     uli_sys_counter_get();
u64     uli_sys_freq_get();
bool    uli_sys_fullscreen_get();
void    uli_sys_fullscreen_set(bool value);
void    uli_sys_message(const char* title, const char* message);
void    uli_sys_title(const char* title);
void    uli_sys_open_path(const char* path);
void    uli_sys_open_url(const char* path);
void    uli_sys_preseed();
bool    uli_sys_keyboard_text(char* text);
void    uli_sys_update_config();
void    uli_sys_default_mapping(uli_mapping* mapping);

#define CODE_COLORS_LIST(macro) \
    macro(BG)       \
    macro(FG)       \
    macro(STRING)   \
    macro(NUMBER)   \
    macro(KEYWORD)  \
    macro(API)      \
    macro(COMMENT)  \
    macro(SIGN)

enum KeybindMode {
    KEYBIND_STANDARD,
    KEYBIND_EMACS,
    KEYBIND_VI
};

enum TabMode {
    TAB_AUTO,
    TAB_TAB,
    TAB_SPACE
};

typedef struct
{
    struct
    {
        struct
        {
#define     CODE_COLOR_DEF(VAR) u8 VAR;
            CODE_COLORS_LIST(CODE_COLOR_DEF)
#undef      CODE_COLOR_DEF

            u8 select;
            u8 cursor;
            bool shadow;
            bool altFont;
            bool altCaret;
            bool matchDelimiters;
            bool autoDelimiters;

        } code;

        struct
        {
            struct
            {
                u8 alpha;
            } touch;

        } gamepad;

    } theme;

    bool checkNewVersion;
    bool cli;
    bool soft;
    bool trim;

    struct StudioOptions
    {
#if defined(CRT_SHADER_SUPPORT)
        bool crt;
#endif

        bool fullscreen;
        bool vsync;
        bool integerScale;
        s32 volume;
        bool autosave;
        uli_mapping mapping;
#if defined(BUILD_EDITORS)
        enum KeybindMode keybindMode;
        enum TabMode tabMode;
        s32 tabSize;
#endif
    } options;

    const uli_cartridge* cart;

    s32 uiScale;

    int fft;
    int fftcaptureplaybackdevices;
    const char *fftdevice;

    uli_layout keyboardLayout;
} StudioConfig;

typedef struct Studio Studio;

void setJustSwitchedToCodeMode(Studio* studio, bool value);
bool hasJustSwitchedToCodeMode(Studio* studio);
const uli_mem* studio_mem(Studio* studio);
void studio_tick(Studio* studio, uli78_input input);
void studio_sound(Studio* studio);
void studio_load(Studio* studio, const char* file);
void studio_keymapchanged(Studio *studio, uli_layout keyboardLayout);
bool studio_alive(Studio* studio);
void studio_exit(Studio* studio);
void studio_delete(Studio* studio);
const StudioConfig* studio_config(Studio* studio);

Studio* studio_create(s32 argc, char **argv, s32 samplerate, uli78_pixel_color_format format, const char* appFolder, s32 maxscale, uli_layout keyboardLayout);

#ifdef __cplusplus
}
#endif
