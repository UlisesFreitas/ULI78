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

#include "studio.h"

#if defined(BUILD_EDITORS)

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "editors/code.h"
#include "editors/sprite.h"
#include "editors/map.h"
#include "editors/world.h"
#include "editors/sfx.h"
#include "editors/music.h"
#include "screens/console.h"
#include "screens/surf.h"
#include "ext/history.h"
#include "net.h"
#include "wave_writer.h"
#include "ext/gif.h"
#define MSF_GIF_IMPL
#include "ext/msf_gif.h"

#include "../fftdata.h"
#include "ext/fft.h"

#endif

#include "ext/md5.h"
#include "config.h"
#include "cart.h"
#include "screens/start.h"
#include "screens/run.h"
#include "screens/menu.h"
#include "screens/mainmenu.h"

#include "fs.h"

#include "argparse.h"

#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#if defined(ULI_MODULE_EXT)
#include <dlfcn.h>
#endif

#define MD5_HASHSIZE 16

// interval between the Windows and Unix epoch
#define UNIX_EPOCH_IN_FILETIME 116444736000000000ULL

#if defined(ULI78_PRO) && defined(BUILD_EDITORS)
#define ULI_EDITOR_BANKS (ULI_BANKS)
#else
#define ULI_EDITOR_BANKS 1
#endif

#ifdef BUILD_EDITORS
typedef struct
{
    u8 data[MD5_HASHSIZE];
} CartHash;

static const EditorMode Modes[] =
{
    ULI_CODE_MODE,
    ULI_SPRITE_MODE,
    ULI_MAP_MODE,
    ULI_SFX_MODE,
    ULI_MUSIC_MODE,
};

static const EditorMode BankModes[] =
{
    ULI_SPRITE_MODE,
    ULI_MAP_MODE,
    ULI_SFX_MODE,
    ULI_MUSIC_MODE,
};

#endif

typedef struct
{
    bool down;
    bool click;

    struct
    {
        s32 start;
        s32 ticks;
        bool click;
    } dbl;

    uli_point start;
    uli_point end;


} MouseState;

struct Studio
{
    uli_mem* uli;

    bool alive;

    EditorMode mode;
    EditorMode prevMode;
    EditorMode toolbarMode;

    struct
    {
        MouseState state[3];
    } mouse;

#if defined(BUILD_EDITORS)
    EditorMode menuMode;
    ViMode viMode;

    struct
    {
        CartHash hash;
        u64 mdate;
    }cart;

    struct
    {
        bool show;
        bool chained;

        union
        {
            struct
            {
                s8 sprites;
                s8 map;
                s8 sfx;
                s8 music;
            } index;

            s8 indexes[COUNT_OF(BankModes)];
        };
    } bank;

    struct
    {
        struct
        {
            s32 popup;
        } pos;

        Movie* movie;

        Movie idle;
        Movie show;
        Movie wait;
        Movie hide;
    } anim;

    struct
    {
        char message[STUDIO_TEXT_BUFFER_WIDTH];
    } popup;

    struct
    {
        char text[STUDIO_TEXT_BUFFER_WIDTH];
    } tooltip;

    struct
    {
        bool record;
        bool screenshot;

        u32* buffer;
        s32 frame;

        MsfGifState gif;

    } video;

    Code*       code;

    struct
    {
        Sprite* sprite[ULI_EDITOR_BANKS];
        Map*    map[ULI_EDITOR_BANKS];
        Sfx*    sfx[ULI_EDITOR_BANKS];
        Music*  music[ULI_EDITOR_BANKS];
    } banks;

    Console*    console;
    World*      world;
    Surf*       surf;

    uli_net* net;

    Bytebattle bytebattle;

#endif

    Start*      start;
    Run*        run;
    Menu*       menu;
    Config*     config;

    StudioMainMenu* mainmenu;

    uli_fs* fs;
    s32 samplerate;
    uli_font systemFont;

};

static void emptyDone(void* data) {}

void fadePalette(uli_palette* pal, s32 value)
{
    for(u8 *i = pal->data, *end = i + sizeof(uli_palette); i < end; i++)
        *i = *i * value >> 8;
}

void map2ram(uli_ram* ram, const uli_map* src)
{
    memcpy(ram->map.data, src, sizeof ram->map);
}

void tiles2ram(uli_ram* ram, const uli_tiles* src)
{
    memcpy(ram->tiles.data, src, sizeof ram->tiles * ULI_SPRITE_BANKS);
}

static inline void sfx2ram(uli_ram* ram, const uli_sfx* src)
{
    memcpy(&ram->sfx, src, sizeof ram->sfx);
}

static inline void music2ram(uli_ram* ram, const uli_music* src)
{
    memcpy(&ram->music, src, sizeof ram->music);
}

s32 calcWaveAnimation(uli_mem* uli, u32 offset, s32 channel)
{
    const uli_sound_register* reg = &uli->ram->registers[channel];

    s32 val = uli_tool_noise(&reg->waveform)
        ? (rand() & 1) * MAX_VOLUME
        : uli_tool_peek4(reg->waveform.data, ((offset * uli_sound_register_get_freq(reg)) >> 7) % WAVE_VALUES);

    return val * reg->volume;
}

#if defined(BUILD_EDITORS)
static const uli_sfx* getSfxSrc(Studio* studio)
{
    uli_mem* uli = studio->uli;
    return &uli->cart.banks[studio->bank.index.sfx].sfx;
}

static const uli_music* getMusicSrc(Studio* studio)
{
    uli_mem* uli = studio->uli;
    return &uli->cart.banks[studio->bank.index.music].music;
}

const char* studioExportSfx(Studio* studio, s32 index, const char* filename)
{
    uli_mem* uli = studio->uli;

    const char* path = uli_fs_path(studio->fs, filename);

    if(wave_open( studio->samplerate, path ))
    {

#if ULI78_SAMPLE_CHANNELS == 2
        wave_enable_stereo();
#endif

        const uli_sfx* sfx = getSfxSrc(studio);

        sfx2ram(uli->ram, sfx);
        music2ram(uli->ram, getMusicSrc(studio));

        {
            const uli_sample* effect = &sfx->samples.data[index];

            enum{Channel = 0};
            sfx_stop(uli, Channel);
            uli_api_sfx(uli, index, effect->note, effect->octave, -1, Channel, MAX_VOLUME, MAX_VOLUME, SFX_DEF_SPEED);

            for(s32 ticks = 0, pos = 0; pos < SFX_TICKS; pos = uli_tool_sfx_pos(effect->speed, ++ticks))
            {
                uli_core_tick_start(uli);
                uli_core_tick_end(uli);
                uli_core_synth_sound(uli);

                wave_write(uli->product.samples.buffer, uli->product.samples.count);
            }

            sfx_stop(uli, Channel);
            memset(uli->ram->registers, 0, sizeof(uli_sound_register));
        }

        wave_close();

        return path;
    }

    return NULL;
}

const char* studioExportMusic(Studio* studio, s32 track, s32 bank, const char* filename)
{
    uli_mem* uli = studio->uli;

    const char* path = uli_fs_path(studio->fs, filename);

    if(wave_open( studio->samplerate, path ))
    {
#if ULI78_SAMPLE_CHANNELS == 2
        wave_enable_stereo();
#endif
#if defined(ULI78_PRO) && defined(BUILD_EDITORS)
        // chained = true in CLI. Set to false if want to use unchained
        bool chained = studio->bank.chained;
        if(chained)
            memset(studio->bank.indexes, bank, sizeof studio->bank.indexes);
        else
            for(s32 i = 0; i < COUNT_OF(BankModes); i++)
                if(BankModes[i] == ULI_MUSIC_MODE)
                    studio->bank.indexes[i] = bank;
#endif
        const uli_sfx* sfx = getSfxSrc(studio);
        const uli_music* music = getMusicSrc(studio);

        sfx2ram(uli->ram, sfx);
        music2ram(uli->ram, music);

        const uli_music_state* state = &uli->ram->music_state;
        const Music* editor = studio->banks.music[bank];

        uli_api_music(uli, track, -1, -1, false, editor->sustain, -1, -1);

        s32 frame = state->music.frame;
        s32 frames = MUSIC_FRAMES * 16;

        while(frames && state->flag.music_status == uli_music_play)
        {
            uli_core_tick_start(uli);

            for (s32 i = 0; i < ULI_SOUND_CHANNELS; i++)
                if(!editor->on[i])
                    uli->ram->registers[i].volume = 0;

            uli_core_tick_end(uli);
            uli_core_synth_sound(uli);

            wave_write(uli->product.samples.buffer, uli->product.samples.count);

            if(frame != state->music.frame)
            {
                --frames;
                frame = state->music.frame;
            }
        }

        uli_api_music(uli, -1, -1, -1, false, false, -1, -1);

        wave_close();
        return path;
    }

    return NULL;
}
#endif

void sfx_stop(uli_mem* uli, s32 channel)
{
    uli_api_sfx(uli, -1, 0, 0, -1, channel, MAX_VOLUME, MAX_VOLUME, SFX_DEF_SPEED);
}

char getKeyboardText(Studio* studio)
{
    char text;
    if(!uli_sys_keyboard_text(&text))
    {
        uli_mem* uli = studio->uli;
        uli78_input* input = &uli->ram->input;

#ifdef KEYBOARD_LAYOUT_ES
        // US KEYS:                     " abcdefghijklmnopqrstuvwxyz0123456789-=[]\\;'`,./< ";
        static const char Symbols[] =   " abcdefghijklmnopqrstuvwxyz0123456789'!`+cn'o,.-< ";
        static const char Shift[] =     " ABCDEFGHIJKLMNOPQRSTUVWXYZ=!\" $%&/()??^*CN\"a;:_> ";
        static const char Alt[] =       "                            |@#        []} {\\     ";
#else
        static const char Symbols[] =   " abcdefghijklmnopqrstuvwxyz0123456789-=[]\\;'`,./ ";
        static const char Shift[] =     " ABCDEFGHIJKLMNOPQRSTUVWXYZ)!@#$%^&*(_+{}|:\"~<>? ";
        static const char Alt[] =       "                                                  ";
#endif

        enum{Count = sizeof Symbols};

        for(s32 i = 0; i < ULI78_KEY_BUFFER; i++)
        {
            uli_key key = input->keyboard.keys[i];

            if(key > 0 && key < Count && uli_api_keyp(uli, key, KEYBOARD_HOLD, KEYBOARD_PERIOD))
            {
                bool caps = uli_api_key(uli, uli_key_capslock);
                bool shift = uli_api_key(uli, uli_key_shift);
                bool alt = uli_api_key(uli, uli_key_alt);

                if (caps && key >= uli_key_a && key <= uli_key_z) shift = !shift;

                return shift ? Shift[key]
                    : alt ? Alt[key]
                    : Symbols[key];
            }
        }

        return '\0';
    }

    return text;
}

bool keyWasPressed(Studio* studio, uli_key key)
{
    uli_mem* uli = studio->uli;
    return uli_api_keyp(uli, key, KEYBOARD_HOLD, KEYBOARD_PERIOD);
}

bool enterWasPressed(Studio* studio)
{
    uli_mem* uli = studio->uli;
    return ticEnterWasPressed(uli, KEYBOARD_HOLD, KEYBOARD_PERIOD);
}

bool ticEnterWasPressed(uli_mem* uli, s32 hold, s32 period)
{
    return uli_api_keyp(uli, uli_key_return, hold, period) ||
           uli_api_keyp(uli, uli_key_numpadenter, hold, period);
}


bool anyKeyWasPressed(Studio* studio)
{
    uli_mem* uli = studio->uli;

    for(s32 i = 0; i < ULI78_KEY_BUFFER; i++)
    {
        uli_key key = uli->ram->input.keyboard.keys[i];

        if(uli_api_keyp(uli, key, KEYBOARD_HOLD, KEYBOARD_PERIOD))
            return true;
    }

    return false;
}

#if defined(BUILD_EDITORS)
uli_tiles* getBankTiles(Studio* studio)
{
    return &studio->uli->cart.banks[studio->bank.index.sprites].tiles;
}

uli_map* getBankMap(Studio* studio)
{
    return &studio->uli->cart.banks[studio->bank.index.map].map;
}

uli_palette* getBankPalette(Studio* studio, bool vbank)
{
    uli_bank* bank = &studio->uli->cart.banks[studio->bank.index.sprites];
    return vbank ? &bank->palette.vbank1 : &bank->palette.vbank0;
}

uli_flags* getBankFlags(Studio* studio)
{
    return &studio->uli->cart.banks[studio->bank.index.sprites].flags;
}
#endif

void playSystemSfx(Studio* studio, s32 id)
{
    const uli_sample* effect = &studio->config->cart->bank0.sfx.samples.data[id];
    uli_api_sfx(studio->uli, id, effect->note, effect->octave, -1, 0, MAX_VOLUME, MAX_VOLUME, effect->speed);
}

static void md5(const void* voidData, s32 length, u8 digest[MD5_HASHSIZE])
{
    enum {Size = 512};

    const u8* data = voidData;

    MD5_CTX c;
    MD5_Init(&c);

    while (length > 0)
    {
        MD5_Update(&c, data, length > Size ? Size: length);

        length -= Size;
        data += Size;
    }

    MD5_Final(digest, &c);
}

const char* md5str(const void* data, s32 length)
{
    static char res[MD5_HASHSIZE * 2 + 1];

    u8 digest[MD5_HASHSIZE];

    md5(data, length, digest);

    for (s32 n = 0; n < MD5_HASHSIZE; ++n)
        snprintf(res + n*2, sizeof("ff"), "%02x", digest[n]);

    return res;
}

static u8* getSpritePtr(uli_tile* tiles, s32 x, s32 y)
{
    enum { SheetCols = (ULI_SPRITESHEET_SIZE / ULI_SPRITESIZE) };
    return tiles[x / ULI_SPRITESIZE + y / ULI_SPRITESIZE * SheetCols].data;
}


void setSpritePixel(uli_tile* tiles, s32 x, s32 y, u8 color)
{
    // TODO: check spritesheet rect
    uli_tool_poke4(getSpritePtr(tiles, x, y), (x % ULI_SPRITESIZE) + (y % ULI_SPRITESIZE) * ULI_SPRITESIZE, color);
}

u8 getSpritePixel(uli_tile* tiles, s32 x, s32 y)
{
    // TODO: check spritesheet rect
    return uli_tool_peek4(getSpritePtr(tiles, x, y), (x % ULI_SPRITESIZE) + (y % ULI_SPRITESIZE) * ULI_SPRITESIZE);
}

#if defined(BUILD_EDITORS)
void toClipboard(const void* data, s32 size, bool flip)
{
    if(data)
    {
        enum {Len = 2};

        char* clipboard = (char*)malloc(size*Len + 1);

        if(clipboard)
        {
            char* ptr = clipboard;

            uli_tool_buf2str(data, size, clipboard, flip);
            uli_sys_clipboard_set(clipboard);
            free(clipboard);
        }
    }
}

static void removeWhiteSpaces(char* str)
{
    s32 i = 0;
    s32 len = (s32)strlen(str);

    for (s32 j = 0; j < len; j++)
        if(!isspace(str[j]))
            str[i++] = str[j];

    str[i] = '\0';
}

bool fromClipboard(void* data, s32 size, bool flip, bool remove_white_spaces, bool sameSize)
{
    if(uli_sys_clipboard_has())
    {
        char* clipboard = uli_sys_clipboard_get();

        SCOPE(uli_sys_clipboard_free(clipboard))
        {
            if (remove_white_spaces)
                removeWhiteSpaces(clipboard);

            bool valid = sameSize
                ? strlen(clipboard) == size * 2
                : strlen(clipboard) <= size * 2;

            if(valid) uli_tool_str2buf(clipboard, (s32)strlen(clipboard), data, flip);

            return valid;
        }
    }

    return false;
}

void showTooltip(Studio* studio, const char* text)
{
    strncpy(studio->tooltip.text, text, sizeof studio->tooltip.text - 1);
}

static void drawExtrabar(Studio* studio, uli_mem* uli)
{
    enum {Size = 7};

    s32 x = (COUNT_OF(Modes) + 1) * Size + 17 * ULI_FONT_WIDTH;
    s32 y = 0;

    static struct Icon {u8 id; StudioEvent event; const char* tip;} Icons[] =
    {
        {uli_icon_cut,      ULI_TOOLBAR_CUT,    "CUT [ctrl+x]"},
        {uli_icon_copy,     ULI_TOOLBAR_COPY,   "COPY [ctrl+c]"},
        {uli_icon_paste,    ULI_TOOLBAR_PASTE,  "PASTE [ctrl+v]"},
        {uli_icon_undo,     ULI_TOOLBAR_UNDO,   "UNDO [ctrl+z]"},
        {uli_icon_redo,     ULI_TOOLBAR_REDO,   "REDO [ctrl+y]"},
    };

    u8 color = uli_color_red;
    FOR(const struct Icon*, icon, Icons)
    {
        uli_rect rect = {x, y, Size, Size};

        u8 bg = uli_color_white;
        u8 fg = uli_color_light_grey;

        if(checkMousePos(studio, &rect))
        {
            setCursor(studio, uli_cursor_hand);

            fg = color;
            showTooltip(studio, icon->tip);

            if(checkMouseDown(studio, &rect, uli_mouse_left))
            {
                bg = fg;
                fg = uli_color_white;
            }
            else if(checkMouseClick(studio, &rect, uli_mouse_left))
            {
                setStudioEvent(studio, icon->event);
            }
        }

        uli_api_rect(uli, x, y, Size, Size, bg);
        drawBitIcon(studio, icon->id, x, y, fg);

        x += Size;
        color++;
    }
}

struct Sprite* getSpriteEditor(Studio* studio)
{
    return studio->banks.sprite[studio->bank.index.sprites];
}
#endif

const StudioConfig* studio_config(Studio* studio)
{
    return &studio->config->data;
}

const StudioConfig* getConfig(Studio* studio)
{
    return studio_config(studio);
}

struct Start* getStartScreen(Studio* studio)
{
    return studio->start;
}

#if defined(ULI78_PRO) && defined(BUILD_EDITORS)

static void drawBankIcon(Studio* studio, s32 x, s32 y)
{
    uli_mem* uli = studio->uli;

    uli_rect rect = {x, y, ULI_FONT_WIDTH, ULI_FONT_HEIGHT};

    bool over = false;
    EditorMode mode = 0;

    for(s32 i = 0; i < COUNT_OF(BankModes); i++)
        if(BankModes[i] == studio->mode)
        {
            mode = i;
            break;
        }

    if(checkMousePos(studio, &rect))
    {
        setCursor(studio, uli_cursor_hand);

        over = true;

        showTooltip(studio, "SWITCH BANK");

        if(checkMouseClick(studio, &rect, uli_mouse_left))
            studio->bank.show = !studio->bank.show;
    }

    if(studio->bank.show)
    {
        drawBitIcon(studio, uli_icon_bank, x, y, uli_color_red);

        enum{Size = TOOLBAR_SIZE};

        for(s32 i = 0; i < ULI_EDITOR_BANKS; i++)
        {
            uli_rect rect = {x + 2 + (i+1)*Size, 0, Size, Size};

            bool over = false;
            if(checkMousePos(studio, &rect))
            {
                setCursor(studio, uli_cursor_hand);
                over = true;

                if(checkMouseClick(studio, &rect, uli_mouse_left))
                {
                    if(studio->bank.chained)
                        memset(studio->bank.indexes, i, sizeof studio->bank.indexes);
                    else studio->bank.indexes[mode] = i;
                }
            }

            if(i == studio->bank.indexes[mode])
                uli_api_rect(uli, rect.x, rect.y, rect.w, rect.h, uli_color_red);

            uli_api_print(uli, (char[]){'0' + i, '\0'}, rect.x+1, rect.y+1,
                i == studio->bank.indexes[mode]
                    ? uli_color_white
                    : over
                        ? uli_color_red
                        : uli_color_light_grey,
                false, 1, false);

        }

        {
            uli_rect rect = {x + 4 + (ULI_EDITOR_BANKS+1)*Size, 0, Size, Size};

            bool over = false;

            if(checkMousePos(studio, &rect))
            {
                setCursor(studio, uli_cursor_hand);

                over = true;

                if(checkMouseClick(studio, &rect, uli_mouse_left))
                {
                    studio->bank.chained = !studio->bank.chained;

                    if(studio->bank.chained)
                        memset(studio->bank.indexes, studio->bank.indexes[mode], sizeof studio->bank.indexes);
                }
            }

            drawBitIcon(studio, uli_icon_pin, rect.x, rect.y, studio->bank.chained ? uli_color_red : over ? uli_color_grey : uli_color_light_grey);
        }
    }
    else
    {
        drawBitIcon(studio, uli_icon_bank, x, y, over ? uli_color_red : uli_color_light_grey);
    }
}

#endif

static inline s32 lerp(s32 a, s32 b, float d)
{
    return (b - a) * d + a;
}

static inline float bounceOut(float x)
{
    const float n1 = 7.5625;
    const float d1 = 2.75;

    if (x < 1 / d1) return n1 * x * x;
    else if (x < 2 / d1)
    {
        x -= 1.5 / d1;
        return n1 * x * x + 0.75;
    }
    else if (x < 2.5 / d1)
    {
        x -= 2.25 / d1;
        return n1 * x * x + 0.9375;
    }
    else
    {
        x -= 2.625 / d1;
        return n1 * x * x + 0.984375;
    }
}

static inline float animEffect(AnimEffect effect, float x)
{
    const float c1 = 1.70158;
    const float c2 = c1 * 1.525;
    const float c3 = c1 + 1;
    const float c4 = (2 * M_PI) / 3;
    const float c5 = (2 * M_PI) / 4.5;

    switch(effect)
    {
    case AnimLinear:
        return x;

    case AnimEaseIn:
        return x * x;

    case AnimEaseOut:
        return 1 - (1 - x) * (1 - x);

    case AnimEaseInOut:
        return x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;

    case AnimEaseInCubic:
        return x * x * x;

    case AnimEaseOutCubic:
        return 1 - pow(1 - x, 3);

    case AnimEaseInOutCubic:
        return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;

    case AnimEaseInQuart:
        return x * x * x * x;

    case AnimEaseOutQuart:
        return 1 - pow(1 - x, 4);

    case AnimEaseInOutQuart:
        return x < 0.5 ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2;

    case AnimEaseInQuint:
        return x * x * x * x * x;

    case AnimEaseOutQuint:
        return 1 - pow(1 - x, 5);

    case AnimEaseInOutQuint:
        return x < 0.5 ? 16 * x * x * x * x * x : 1 - pow(-2 * x + 2, 5) / 2;

    case AnimEaseInSine:
        return 1 - cos((x * M_PI) / 2);

    case AnimEaseOutSine:
        return sin((x * M_PI) / 2);

    case AnimEaseInOutSine:
        return -(cos(M_PI * x) - 1) / 2;

    case AnimEaseInExpo:
        return x == 0 ? 0 : pow(2, 10 * x - 10);

    case AnimEaseOutExpo:
        return x == 1 ? 1 : 1 - pow(2, -10 * x);

    case AnimEaseInOutExpo:
        return x == 0
            ? 0
            : x == 1
                ? 1
                : x < 0.5
                    ? pow(2, 20 * x - 10) / 2
                    : (2 - pow(2, -20 * x + 10)) / 2;

    case AnimEaseInCirc:
        return 1 - sqrt(1 - pow(x, 2));

    case AnimEaseOutCirc:
        return sqrt(1 - pow(x - 1, 2));

    case AnimEaseInOutCirc:
        return x < 0.5
            ? (1 - sqrt(1 - pow(2 * x, 2))) / 2
            : (sqrt(1 - pow(-2 * x + 2, 2)) + 1) / 2;

    case AnimEaseInBack:
        return c3 * x * x * x - c1 * x * x;

    case AnimEaseOutBack:
        return 1 + c3 * pow(x - 1, 3) + c1 * pow(x - 1, 2);

    case AnimEaseInOutBack:
        return x < 0.5
            ? (pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
            : (pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;

    case AnimEaseInElastic:
        return x == 0
            ? 0
            : x == 1
                ? 1
                : -pow(2, 10 * x - 10) * sin((x * 10 - 10.75) * c4);

    case AnimEaseOutElastic:
        return x == 0
            ? 0
            : x == 1
                ? 1
                : pow(2, -10 * x) * sin((x * 10 - 0.75) * c4) + 1;

    case AnimEaseInOutElastic:
        return x == 0
            ? 0
            : x == 1
                ? 1
                : x < 0.5
                    ? -(pow(2, 20 * x - 10) * sin((20 * x - 11.125) * c5)) / 2
                    : (pow(2, -20 * x + 10) * sin((20 * x - 11.125) * c5)) / 2 + 1;

    case AnimEaseInBounce:
        return 1 - bounceOut(1 - x);

    case AnimEaseOutBounce:
        return bounceOut(x);

    case AnimEaseInOutBounce:
        return x < 0.5
            ? (1 - bounceOut(1 - 2 * x)) / 2
            : (1 + bounceOut(2 * x - 1)) / 2;
    }

    return x;
}

static void animTick(Movie* movie)
{
    for(Anim* it = movie->items, *end = it + movie->count; it != end; ++it)
    {
	float tick = (float)(movie->tick < it->time ? movie->tick : it->time);
        *it->value = lerp(it->start, it->end, animEffect(it->effect, tick / it->time));
    }
}

void processAnim(Movie* movie, void* data)
{
    animTick(movie);

    if(movie->tick == movie->time)
        movie->done(data);

    movie->tick++;
}

Movie* resetMovie(Movie* movie)
{
    movie->tick = 0;
    animTick(movie);

    return movie;
}

#if defined(BUILD_EDITORS)

static void drawPopup(Studio* studio)
{
    if(studio->anim.movie != &studio->anim.idle)
    {
        enum{Width = ULI78_WIDTH, Height = ULI_FONT_HEIGHT + 1};

        uli_api_rect(studio->uli, 0, studio->anim.pos.popup, Width, Height, uli_color_red);
        uli_api_print(studio->uli, studio->popup.message,
            (s32)(Width - strlen(studio->popup.message) * ULI_FONT_WIDTH)/2,
            studio->anim.pos.popup + 1, uli_color_white, true, 1, false);

        // render popup message
        {
            uli_mem* uli = studio->uli;
            const uli_bank* bank = &getConfig(studio)->cart->bank0;
            u32* dst = uli->product.screen + ULI78_MARGIN_LEFT + ULI78_MARGIN_TOP * ULI78_FULLWIDTH;

            for(s32 i = 0, y = 0; y < (Height + studio->anim.pos.popup); y++, dst += ULI78_MARGIN_RIGHT + ULI78_MARGIN_LEFT)
                for(s32 x = 0; x < Width; x++)
                *dst++ = uli_rgba(&bank->palette.vbank0.colors[uli_tool_peek4(uli->ram->vram.screen.data, i++)]);
        }
    }
}

void drawToolbar(Studio* studio, uli_mem* uli, bool bg)
{
    if(bg)
        uli_api_rect(uli, 0, 0, ULI78_WIDTH, TOOLBAR_SIZE, uli_color_white);

    enum {Size = 7};

    static const u8 Icons[] = {uli_icon_code, uli_icon_sprite, uli_icon_map, uli_icon_sfx, uli_icon_music};
    static const char* Tips[] = {"CODE EDITOR [f1]", "SPRITE EDITOR [f2]", "MAP EDITOR [f3]", "SFX EDITOR [f4]", "MUSIC EDITOR [f5]",};

    s32 mode = -1;

    for(s32 i = 0; i < COUNT_OF(Modes); i++)
    {
        uli_rect rect = {i * Size, 0, Size, Size};

        bool over = false;

        if(checkMousePos(studio, &rect))
        {
            setCursor(studio, uli_cursor_hand);

            over = true;

            showTooltip(studio, Tips[i]);

            if(checkMouseClick(studio, &rect, uli_mouse_left))
                studio->toolbarMode = Modes[i];
        }

        if(getStudioMode(studio) == Modes[i]) mode = i;

        if (mode == i)
        {
            drawBitIcon(studio, uli_icon_tab, i * Size, 0, uli_color_grey);
            drawBitIcon(studio, Icons[i], i * Size, 1, uli_color_black);
        }

        drawBitIcon(studio, Icons[i], i * Size, 0, mode == i ? uli_color_white : (over ? uli_color_grey : uli_color_light_grey));
    }

    if(mode >= 0) drawExtrabar(studio, uli);

    static const char* Names[] =
    {
        "CODE EDITOR",
        "SPRITE EDITOR",
        "MAP EDITOR",
        "SFX EDITOR",
        "MUSIC EDITOR",
    };

#if defined (ULI78_PRO) && defined(BUILD_EDITORS)
    enum {TextOffset = (COUNT_OF(Modes) + 2) * Size - 2};
    if(mode >= 1)
        drawBankIcon(studio, COUNT_OF(Modes) * Size + 2, 0);
#else
    enum {TextOffset = (COUNT_OF(Modes) + 1) * Size};
#endif

    if(mode == 0 || (mode >= 1 && !studio->bank.show))
    {
        if(strlen(studio->tooltip.text))
        {
            uli_api_print(uli, studio->tooltip.text, TextOffset, 1, uli_color_dark_grey, false, 1, false);
        }
        else
        {
            uli_api_print(uli, Names[mode], TextOffset, 1, uli_color_grey, false, 1, false);
        }
    }
}

void setStudioEvent(Studio* studio, StudioEvent event)
{
    switch(studio->mode)
    {
    case ULI_CODE_MODE:
        {
            Code* code = studio->code;
            code->event(code, event);
        }
        break;
    case ULI_SPRITE_MODE:
        {
            Sprite* sprite = studio->banks.sprite[studio->bank.index.sprites];
            sprite->event(sprite, event);
        }
    break;
    case ULI_MAP_MODE:
        {
            Map* map = studio->banks.map[studio->bank.index.map];
            map->event(map, event);
        }
        break;
    case ULI_SFX_MODE:
        {
            Sfx* sfx = studio->banks.sfx[studio->bank.index.sfx];
            sfx->event(sfx, event);
        }
        break;
    case ULI_MUSIC_MODE:
        {
            Music* music = studio->banks.music[studio->bank.index.music];
            music->event(music, event);
        }
        break;
    default: break;
    }
}

ClipboardEvent getClipboardEvent(Studio* studio)
{
    uli_mem* uli = studio->uli;

    bool shift = uli_api_key(uli, uli_key_shift);
    bool ctrl = uli_api_key(uli, uli_key_ctrl);

    if(ctrl)
    {
        if(keyWasPressed(studio, uli_key_insert) || keyWasPressed(studio, uli_key_c)) return ULI_CLIPBOARD_COPY;
        else if(keyWasPressed(studio, uli_key_x)) return ULI_CLIPBOARD_CUT;
        else if(keyWasPressed(studio, uli_key_v)) return ULI_CLIPBOARD_PASTE;
    }
    else if(shift)
    {
        if(keyWasPressed(studio, uli_key_delete)) return ULI_CLIPBOARD_CUT;
        else if(keyWasPressed(studio, uli_key_insert)) return ULI_CLIPBOARD_PASTE;
    }

    return ULI_CLIPBOARD_NONE;
}

static void showPopupMessage(Studio* studio, const char* text)
{
    memset(studio->popup.message, '\0', sizeof studio->popup.message);
    strncpy(studio->popup.message, text, sizeof(studio->popup.message) - 1);

    for(char* c = studio->popup.message; c < studio->popup.message + sizeof studio->popup.message; c++)
        if(*c) *c = toupper(*c);

    studio->anim.movie = resetMovie(&studio->anim.show);
}
#endif

static void exitConfirm(Studio* studio, bool yes, void* data)
{
    studio->alive = yes;
}

void studio_exit(Studio* studio)
{
#if defined(BUILD_EDITORS)
    if(studio->mode != ULI_START_MODE && studioCartChanged(studio))
    {
        static const char* Rows[] =
        {
            "WARNING!",
            "You have unsaved changes",
            "Do you really want to exit?",
        };

        confirmDialog(studio, Rows, COUNT_OF(Rows), exitConfirm, NULL);
    }
    else
#endif
        exitConfirm(studio, true, NULL);
}

void exitStudio(Studio* studio)
{
    studio_exit(studio);
}

void drawBitIcon(Studio* studio, s32 id, s32 x, s32 y, u8 color)
{
    uli_mem* uli = studio->uli;

    const uli_tile* tile = &getConfig(studio)->cart->bank0.tiles.data[id];

    for(s32 i = 0, sx = x, ex = sx + ULI_SPRITESIZE; i != ULI_SPRITESIZE * ULI_SPRITESIZE; ++i, ++x)
    {
        if(x == ex)
        {
            x = sx;
            y++;
        }

        if(uli_tool_peek4(tile, i))
            uli_api_pix(uli, x, y, color, false);
    }
}

static void initRunMode(Studio* studio)
{
    initRun(studio->run,
#if defined(BUILD_EDITORS)
        studio->console,
#else
        NULL,
#endif
        studio->fs, studio);
}

#if defined(BUILD_EDITORS)
static void initWorldMap(Studio* studio)
{
    initWorld(studio->world, studio, studio->banks.map[studio->bank.index.map]);
}

static void initSurfMode(Studio* studio)
{
    initSurf(studio->surf, studio, studio->console);
}

void gotoSurf(Studio* studio)
{
    initSurfMode(studio);
    setStudioMode(studio, ULI_SURF_MODE);
}

void gotoCode(Studio* studio)
{
    setStudioMode(studio, ULI_CODE_MODE);
}

#endif

void setStudioMode(Studio* studio, EditorMode mode)
{
    if(mode != studio->mode)
    {
        EditorMode prev = studio->mode;

        if(prev == ULI_RUN_MODE)
            uli_core_pause(studio->uli);

        if(mode != ULI_RUN_MODE)
            uli_api_reset(studio->uli);

        switch (prev)
        {
        case ULI_START_MODE:
        case ULI_CONSOLE_MODE:
        case ULI_MENU_MODE:
            break;
        default: studio->prevMode = prev; break;
        }

#if defined(BUILD_EDITORS)
        switch(mode)
        {
        case ULI_RUN_MODE: initRunMode(studio); break;
        case ULI_CONSOLE_MODE:
            if (prev == ULI_SURF_MODE)
                studio->console->done(studio->console);
            break;
        case ULI_WORLD_MODE: initWorldMap(studio); break;
        case ULI_SURF_MODE: studio->surf->resume(studio->surf); break;
        default: break;
        }

        studio->mode = mode;
#else
        switch (mode)
        {
        case ULI_START_MODE:
        case ULI_MENU_MODE:
            studio->mode = mode;
            break;
        default:
            studio->mode = ULI_RUN_MODE;
        }
#endif
    }

#if defined(BUILD_EDITORS)
    else if(mode == ULI_MUSIC_MODE)
    {
        Music* music = studio->banks.music[studio->bank.index.music];
        music->tab = (music->tab + 1) % MUSIC_TAB_COUNT;
    }
#endif

#if defined(BUILD_EDITORS)
    studio->viMode = 0;
#endif
}

EditorMode getStudioMode(Studio* studio)
{
    return studio->mode;
}

#if defined(BUILD_EDITORS)
static void changeStudioMode(Studio* studio, s32 dir)
{
    for(size_t i = 0; i < COUNT_OF(Modes); i++)
    {
        if(studio->mode == Modes[i])
        {
            setStudioMode(studio, Modes[(i+dir+ COUNT_OF(Modes)) % COUNT_OF(Modes)]);
            return;
        }
    }
}
#endif

void resumeGame(Studio* studio)
{
    uli_core_resume(studio->uli);
    studio->mode = ULI_RUN_MODE;
}

#if defined(BUILD_EDITORS)
void setStudioViMode(Studio* studio, ViMode mode) {
    studio->viMode = mode;
}

ViMode getStudioViMode(Studio* studio) {
    return studio->viMode;
}

bool checkStudioViMode(Studio* studio, ViMode mode) {
    return (
        getConfig(studio)->options.keybindMode == KEYBIND_VI
        && getStudioViMode(studio) == mode
    );
}
#endif

static inline bool pointInRect(const uli_point* pt, const uli_rect* rect)
{
    return (pt->x >= rect->x)
        && (pt->x < (rect->x + rect->w))
        && (pt->y >= rect->y)
        && (pt->y < (rect->y + rect->h));
}

bool checkMousePos(Studio* studio, const uli_rect* rect)
{
    uli_point pos = uli_api_mouse(studio->uli);
    return pointInRect(&pos, rect);
}

bool checkMouseClick(Studio* studio, const uli_rect* rect, uli_mouse_btn button)
{
    MouseState* state = &studio->mouse.state[button];

    bool value = state->click
        && pointInRect(&state->start, rect)
        && pointInRect(&state->end, rect);

    if(value) state->click = false;

    return value;
}

bool checkMouseDblClick(Studio* studio, const uli_rect* rect, uli_mouse_btn button)
{
    MouseState* state = &studio->mouse.state[button];

    bool value = state->dbl.click
        && pointInRect(&state->start, rect)
        && pointInRect(&state->end, rect);

    if(value) state->dbl.click = false;

    return value;
}

bool checkMouseDown(Studio* studio, const uli_rect* rect, uli_mouse_btn button)
{
    MouseState* state = &studio->mouse.state[button];

    return state->down && pointInRect(&state->start, rect);
}

void setCursor(Studio* studio, uli_cursor id)
{
    uli_mem* uli = studio->uli;

    VBANK(uli, 0)
    {
        uli->ram->vram.vars.cursor.sprite = id;
    }
}

#if defined(BUILD_EDITORS)

typedef struct
{
    Studio* studio;
    ConfirmCallback callback;
    void* data;
} ConfirmData;

static void confirmHandler(bool yes, void* data)
{
    ConfirmData* confirmData = data;
    SCOPE(free(confirmData))
    {
        Studio* studio = confirmData->studio;

        if(studio->menuMode == ULI_RUN_MODE)
        {
            resumeGame(studio);
        }
        else setStudioMode(studio, studio->menuMode);

        confirmData->callback(studio, yes, confirmData->data);
    }
}

static void confirmNo(void* data, s32 pos)
{
    confirmHandler(false, data);
}

static void confirmYes(void* data, s32 pos)
{
    confirmHandler(true, data);
}

void confirmDialog(Studio* studio, const char** text, s32 rows, ConfirmCallback callback, void* data)
{
    if(studio->mode != ULI_MENU_MODE)
    {
        studio->menuMode = studio->mode;
    }

    setStudioMode(studio, ULI_MENU_MODE);

    static MenuItem Answers[] =
    {
        {"",    NULL},
        {"(N)O",  confirmNo},
        {"(Y)ES", confirmYes},
    };

    Answers[1].hotkey = uli_key_n;
    Answers[2].hotkey = uli_key_y;

    s32 count = rows + COUNT_OF(Answers);
    MenuItem* items = malloc(sizeof items[0] * count);
    SCOPE(free(items))
    {
        for(s32 i = 0; i != rows; ++i)
            items[i] = (MenuItem){text[i], NULL};

        memcpy(items + rows, Answers, sizeof Answers);

        studio_menu_init(studio->menu, items, count, count - 2, 0,
            NULL, MOVE((ConfirmData){studio, callback, data}));

        playSystemSfx(studio, 0);
    }
}

static void resetBanks(Studio* studio)
{
    memset(studio->bank.indexes, 0, sizeof studio->bank.indexes);
}

static void initModules(Studio* studio)
{
    uli_mem* uli = studio->uli;

    resetBanks(studio);

    initCode(studio->code, studio);

    for(s32 i = 0; i < ULI_EDITOR_BANKS; i++)
    {
        initSprite(studio->banks.sprite[i], studio, &uli->cart.banks[i].tiles);
        initMap(studio->banks.map[i], studio, &uli->cart.banks[i].map);
        initSfx(studio->banks.sfx[i], studio, &uli->cart.banks[i].sfx);
        initMusic(studio->banks.music[i], studio, &uli->cart.banks[i].music);
    }

    initWorldMap(studio);
}

static void updateHash(Studio* studio)
{
    md5(&studio->uli->cart, sizeof(uli_cartridge), studio->cart.hash.data);
}

static void updateMDate(Studio* studio)
{
    studio->cart.mdate = fs_date(studio->console->rom.path);
}
#endif

static void updateTitle(Studio* studio)
{
    char name[ULINAME_MAX] = ULI_TITLE;

#if defined(BUILD_EDITORS)
    if(strlen(studio->console->rom.name))
        snprintf(name, ULINAME_MAX, "%s [%s]", ULI_TITLE, studio->console->rom.name);
#endif

    uli_sys_title(name);
}

#if defined(BUILD_EDITORS)

bool project_ext(const char* name)
{
    FOREACH_LANG(script)
    {
        if(uli_tool_has_ext(name, script->fileExtension))
            return true;
    }

    return false;
}

void studioRomSaved(Studio* studio)
{
    updateTitle(studio);
    updateHash(studio);
    updateMDate(studio);
}

void studioRomLoaded(Studio* studio)
{
    initModules(studio);

    updateTitle(studio);
    updateHash(studio);
    updateMDate(studio);
}

bool studioCartChanged(Studio* studio)
{
    CartHash hash;
    md5(&studio->uli->cart, sizeof(uli_cartridge), hash.data);

    return memcmp(hash.data, studio->cart.hash.data, sizeof(CartHash)) != 0;
}
#endif

void runGame(Studio* studio)
{
#if defined(BUILD_EDITORS)

    if (studio->config->data.fft) {
        // initialize FFT data structures
        fPeakMinValue = 0.01f;
        fPeakSmoothing = 0.995f;
        fPeakSmoothValue = 0.0f;
        fAmplification = 1.0f;
        memset(fftData, 0, sizeof(fftData[0]) * FFT_SIZE);
        memset(fftSmoothingData, 0, sizeof(fftSmoothingData[0]) * FFT_SIZE);
        memset(fftNormalizedData, 0, sizeof(fftNormalizedData[0]) * FFT_SIZE);
        memset(fftNormalizedMaxData, 0, sizeof(fftNormalizedMaxData[0]) * FFT_SIZE);
    }

    if(studio->console->args.keepcmd
        && studio->console->commands.count
        && studio->console->commands.current >= studio->console->commands.count)
    {
        studio->console->commands.current = 0;
        setStudioMode(studio, ULI_CONSOLE_MODE);
    }
    else
#endif
    {
        uli_api_reset(studio->uli);

        if(studio->mode == ULI_RUN_MODE)
        {
            initRunMode(studio);
            return;
        }

        setStudioMode(studio, ULI_RUN_MODE);

#if defined(BUILD_EDITORS)
        if(studio->mode == ULI_SURF_MODE)
            studio->prevMode = ULI_SURF_MODE;
#endif
    }
}

#if defined(BUILD_EDITORS)
void saveProject(Studio* studio)
{
    if(getConfig(studio)->trim) trimWhitespace(studio->code);

    CartSaveResult rom = studio->console->save(studio->console);

    if(rom == CART_SAVE_OK)
    {
        char buffer[STUDIO_TEXT_BUFFER_WIDTH];
        char str_saved[] = " saved :)";

        s32 name_len = (s32)strlen(studio->console->rom.name);
        if (name_len + strlen(str_saved) > sizeof(buffer)){
            char subbuf[sizeof(buffer) - sizeof(str_saved) - 5];
            memset(subbuf, '\0', sizeof subbuf);
            strncpy(subbuf, studio->console->rom.name, sizeof subbuf-1);

            snprintf(buffer, sizeof buffer, "%s[...]%s", subbuf, str_saved);
        }
        else
        {
            snprintf(buffer, sizeof buffer, "%s%s", studio->console->rom.name, str_saved);
        }

        showPopupMessage(studio, buffer);
    }
    else if(rom == CART_SAVE_MISSING_NAME) showPopupMessage(studio, "error: missing cart name :(");
    else showPopupMessage(studio, "error: file not saved :(");
}

static void setCoverImage(Studio* studio)
{
    uli_mem* uli = studio->uli;

    if(studio->mode == ULI_RUN_MODE)
    {
        uli_api_sync(uli, uli_sync_screen, 0, true);
        showPopupMessage(studio, "cover image saved :)");
    }
}

static void generateScreenshotName(Studio* studio, const char* extension, char filenameOut[ULINAME_MAX])
{
    const char* romName = studio->console->rom.name;

    // --- Strip extension ---
    const char* dot = strrchr(romName, '.');
    size_t baseLen = dot ? (size_t)(dot - romName) : strlen(romName);

    // --- Get current time with millisecond precision ---
    time_t sec = 0;
    long msec = 0;

#if defined(_WIN32)
    // Windows: Use GetSystemTimeAsFileTime for high precision.
    // It provides time in 100-nanosecond intervals since Jan 1, 1601.
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // Convert FILETIME to Unix seconds and milliseconds
    sec = (time_t)((uli.QuadPart - UNIX_EPOCH_IN_FILETIME) / 10000000ULL);
    msec = (long)((uli.QuadPart / 10000) % 1000);
#else
    // Other systems (Linux, macOS, 3DS, etc.): Use gettimeofday, which is widely available.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sec = tv.tv_sec;
    msec = (long)(tv.tv_usec / 1000);
#endif

    // --- Convert seconds to a local time structure (thread-safe) ---
    struct tm tm_local;

    // Thread-safe localtime
#if defined(_WIN32)
    // Windows secure version
    localtime_s(&tm_local, &sec);
#else
    // POSIX re-entrant version
    localtime_r(&sec, &tm_local);
#endif

    // --- Format the timestamp string ---
    char timestamp[32] = {0};

    // Format as -yymmdd-hhmmss
    strftime(timestamp, sizeof(timestamp), "-%y%m%d-%H%M%S", &tm_local);

    // Append milliseconds
    size_t len = strlen(timestamp);
    snprintf(timestamp + len, sizeof(timestamp) - len, "-%03ld", msec);

    // --- Adjust baseLen to prevent overflow ---
    size_t maxBaseLen = ULINAME_MAX
                      - strlen(timestamp)
                      - strlen(extension)
                      - 1; // for null terminator

    if (baseLen > maxBaseLen)
        baseLen = maxBaseLen;

    // --- Build the final filename string ---
    snprintf(filenameOut, ULINAME_MAX, "%.*s%s%s",
             (int)baseLen, romName, timestamp, extension);
}

static void stopVideoRecord(Studio* studio)
{
    MsfGifResult result = msf_gif_end(&studio->video.gif);

    char filename[ULINAME_MAX];
    generateScreenshotName(studio, ".gif", filename);

    // Now that it has found an available filename, save it.
    if(uli_fs_save(studio->fs, filename, result.data, result.dataSize, true))
    {
        char msg[ULINAME_MAX];
        sprintf(msg, "%s saved :)", filename);
        showPopupMessage(studio, msg);

        uli_sys_open_path(uli_fs_path(studio->fs, filename));
    }
    else showPopupMessage(studio, "error: file not saved :(");

    msf_gif_free(result);

    studio->video.record = false;
}

static void startVideoRecord(Studio* studio)
{
    if(studio->video.record)
    {
        stopVideoRecord(studio);
    }
    else
    {
        studio->video.record = true;
        studio->video.frame = 0;

        s32 scale = studio->config->data.uiScale;
        msf_gif_begin(&studio->video.gif, ULI78_FULLWIDTH * scale, ULI78_FULLHEIGHT * scale);
    }
}

static void takeScreenshot(Studio* studio)
{
    startVideoRecord(studio);
    studio->video.screenshot = true;
}
#endif

static inline bool keyWasPressedOnce(Studio* studio, s32 key)
{
    uli_mem* uli = studio->uli;

    return uli_api_keyp(uli, key, -1, -1);
}

static void gotoFullscreen(Studio* studio)
{
    uli_sys_fullscreen_set(studio->config->data.options.fullscreen = !uli_sys_fullscreen_get());
}

#if defined(CRT_SHADER_SUPPORT)
static void switchCrtMonitor(Studio* studio)
{
    studio->config->data.options.crt = !studio->config->data.options.crt;
}
#endif

#if defined(BUILD_EDITORS)
static u32 getTime()
{
    return uli_sys_counter_get() * 1000 / uli_sys_freq_get();
}

static void hideBattleTime(Studio* studio)
{
    studio->bytebattle.battle.hidetime = !studio->bytebattle.battle.hidetime;
}

static void startBattle(Studio* studio)
{
    studio->bytebattle.battle.started = getTime();
}
#endif

#if defined(ULI78_PRO) && defined(BUILD_EDITORS)

static void switchBank(Studio* studio, s32 bank)
{
    for(s32 i = 0; i < COUNT_OF(BankModes); i++)
        if(BankModes[i] == studio->mode)
        {
            if(studio->bank.chained)
                memset(studio->bank.indexes, bank, sizeof studio->bank.indexes);
            else studio->bank.indexes[i] = bank;
            break;
        }
}

#endif

void gotoMenu(Studio* studio)
{
    setStudioMode(studio, ULI_MENU_MODE);
    studio_mainmenu_free(studio->mainmenu);
    studio->mainmenu = studio_mainmenu_init(studio->menu, studio->config);
}

static bool enterWasPressedOnce(Studio* studio)
{
    return keyWasPressedOnce(studio, uli_key_return) ||
           keyWasPressedOnce(studio, uli_key_numpadenter);
}

#if defined(BUILD_EDITORS)
// These three are to fix the f1 hotkey conflict

// Add a static variable in studio.c to track the mode switch
static bool justSwitchedToCodeMode = false;

void setJustSwitchedToCodeMode(Studio* studio, bool value)
{
    justSwitchedToCodeMode = value;
}

bool hasJustSwitchedToCodeMode(Studio* studio)
{
    return justSwitchedToCodeMode;
}
#endif

#if defined(BUILD_EDITORS)

static bool showGameMenu(Studio* studio)
{
    uli_mem* uli = studio->uli;

    char tag[ULINAME_MAX];
    snprintf(tag, sizeof tag, "\n%s menu:", uli_get_script(uli)->singleComment);

    return strstr(uli->cart.code.data, tag);
}

#endif

static void processShortcuts(Studio* studio)
{
    uli_mem* uli = studio->uli;

    if(studio->mode == ULI_START_MODE) return;

#if defined(BUILD_EDITORS)
    if(!studio->console->active) return;
#endif

    if(studio_mainmenu_keyboard(studio->mainmenu)) return;

    bool alt = uli_api_key(uli, uli_key_alt);
    bool ctrl = uli_api_key(uli, uli_key_ctrl);

#if defined(CRT_SHADER_SUPPORT)
    if(keyWasPressedOnce(studio, uli_key_f6)) switchCrtMonitor(studio);
#endif

    if(alt)
    {
        if (enterWasPressedOnce(studio)) gotoFullscreen(studio);
#if defined(BUILD_EDITORS)
        else if(studio->mode != ULI_RUN_MODE && studio->config->data.keyboardLayout != uli_layout_azerty)
        {
#ifndef KEYBOARD_LAYOUT_ES
            if(keyWasPressedOnce(studio, uli_key_grave)) setStudioMode(studio, ULI_CONSOLE_MODE);
            else if(keyWasPressedOnce(studio, uli_key_1))
            {
                if(studio->mode != ULI_CODE_MODE)
                {
                    setStudioMode(studio, ULI_CODE_MODE);
                    setJustSwitchedToCodeMode(studio, true);
                }
            }
            else if(keyWasPressedOnce(studio, uli_key_2)) setStudioMode(studio, ULI_SPRITE_MODE);
            else if(keyWasPressedOnce(studio, uli_key_3)) setStudioMode(studio, ULI_MAP_MODE);
            else if(keyWasPressedOnce(studio, uli_key_4)) setStudioMode(studio, ULI_SFX_MODE);
            else if(keyWasPressedOnce(studio, uli_key_5)) setStudioMode(studio, ULI_MUSIC_MODE);
#endif
        }
#endif
    }
    else if(ctrl)
    {
        if(keyWasPressedOnce(studio, uli_key_q)) studio_exit(studio);
#if defined(BUILD_EDITORS)
        else if(keyWasPressedOnce(studio, uli_key_pageup)) changeStudioMode(studio, -1);
        else if(keyWasPressedOnce(studio, uli_key_pagedown)) changeStudioMode(studio, +1);
        else if(enterWasPressedOnce(studio)) runGame(studio);
        else if(keyWasPressedOnce(studio, uli_key_r)) runGame(studio);
        else if(keyWasPressedOnce(studio, uli_key_s)) saveProject(studio);
#endif

#if defined(ULI78_PRO) && defined(BUILD_EDITORS)

        else
            for(s32 bank = 0; bank < ULI_BANKS; bank++)
                if(keyWasPressedOnce(studio, uli_key_0 + bank))
                    switchBank(studio, bank);

#endif

    }
    else
    {
        if (keyWasPressedOnce(studio, uli_key_f11)) gotoFullscreen(studio);
#if defined(BUILD_EDITORS)
        else if(keyWasPressedOnce(studio, uli_key_escape))
        {
            if(
                getConfig(studio)->options.keybindMode == KEYBIND_VI
                && getStudioViMode(studio) != VI_NORMAL
            )
                return;

            switch(studio->mode)
            {
            case ULI_MENU_MODE:
                showGameMenu(studio)
                    ? studio_menu_back(studio->menu)
                    : setStudioMode(studio, studio->prevMode == ULI_RUN_MODE
                        ? ULI_CONSOLE_MODE
                        : studio->prevMode);
                break;
            case ULI_RUN_MODE:
                showGameMenu(studio)
                    ? gotoMenu(studio)
                    : setStudioMode(studio, studio->prevMode == ULI_RUN_MODE
                        ? ULI_CONSOLE_MODE
                        : studio->prevMode);
                break;
            case ULI_CONSOLE_MODE:
                setStudioMode(studio, ULI_CODE_MODE);
                break;
            case ULI_CODE_MODE:
                if(studio->code->mode != TEXT_EDIT_MODE)
                {
                    studio->code->escape(studio->code);
                    return;
                }
            default:
                setStudioMode(studio, ULI_CONSOLE_MODE);
            }
        }
        else if(keyWasPressedOnce(studio, uli_key_f8)) takeScreenshot(studio);
        else if(keyWasPressedOnce(studio, uli_key_f9)) startVideoRecord(studio);
        else if(keyWasPressedOnce(studio, uli_key_f10)) hideBattleTime(studio);
        else if(keyWasPressedOnce(studio, uli_key_f12)) startBattle(studio);
        else if(studio->mode == ULI_RUN_MODE && keyWasPressedOnce(studio, uli_key_f7))
            setCoverImage(studio);

        if(!showGameMenu(studio) || studio->mode != ULI_RUN_MODE)
        {
			if(keyWasPressedOnce(studio, uli_key_f1))
			{
				if(studio->mode != ULI_CODE_MODE)
				{
					setStudioMode(studio, ULI_CODE_MODE);
					setJustSwitchedToCodeMode(studio, true);
				}
			}
            else if(keyWasPressedOnce(studio, uli_key_f2)) setStudioMode(studio, ULI_SPRITE_MODE);
            else if(keyWasPressedOnce(studio, uli_key_f3)) setStudioMode(studio, ULI_MAP_MODE);
            else if(keyWasPressedOnce(studio, uli_key_f4)) setStudioMode(studio, ULI_SFX_MODE);
            else if(keyWasPressedOnce(studio, uli_key_f5)) setStudioMode(studio, ULI_MUSIC_MODE);
        }
#else
        else if(keyWasPressedOnce(studio, uli_key_escape))
        {
            switch(studio->mode)
            {
            case ULI_MENU_MODE: studio_menu_back(studio->menu); break;
            case ULI_RUN_MODE: gotoMenu(studio); break;
            default: break;
            }
        }
#endif
    }
}

#if defined(BUILD_EDITORS)
static void reloadConfirm(Studio* studio, bool yes, void* data)
{
    if(yes)
        studio->console->updateProject(studio->console);
    else
        updateMDate(studio);
}

static void checkChanges(Studio* studio)
{
    switch(studio->mode)
    {
    case ULI_START_MODE:
        break;
    default:
        {
            Console* console = studio->console;

            u64 date = fs_date(console->rom.path);

            if(studio->cart.mdate && date > studio->cart.mdate)
            {
                if(studioCartChanged(studio) && studio->mode != ULI_MENU_MODE)
                {
                    static const char* Rows[] =
                    {
                        "WARNING!",
                        "The cart has changed!",
                        "Do you want to reload it?"
                    };

                    confirmDialog(studio, Rows, COUNT_OF(Rows), reloadConfirm, NULL);
                }
                else console->updateProject(console);
            }
        }
    }
}

static void drawBitIconRaw(Studio* studio, u32* frame, s32 sx, s32 sy, s32 id, uli_color color)
{
    const uli_bank* bank = &getConfig(studio)->cart->bank0;

    u32 *dst = frame + sx + sy * ULI78_FULLWIDTH;
    for(s32 src = 0; src != ULI_SPRITESIZE * ULI_SPRITESIZE; dst += ULI78_FULLWIDTH - ULI_SPRITESIZE)
        for(s32 i = 0; i != ULI_SPRITESIZE; ++i, ++dst)
            if(uli_tool_peek4(&bank->tiles.data[id].data, src++))
                *dst = uli_rgba(&bank->palette.vbank0.colors[color]);
}

static void drawRecordLabel(Studio* studio, u32* frame, s32 sx, s32 sy)
{
    drawBitIconRaw(studio, frame, sx, sy, uli_icon_rec, uli_color_red);
    drawBitIconRaw(studio, frame, sx + ULI_SPRITESIZE, sy, uli_icon_rec2, uli_color_red);
}

static bool isRecordFrame(Studio* studio)
{
    return studio->video.record;
}

static void recordFrame(Studio* studio, u32* pixels)
{
    if(studio->video.record)
    {
        if(studio->video.frame % 2 == 0)
        {
            s32 scale = studio->config->data.uiScale;
            s32 w = ULI78_FULLWIDTH * scale;
            s32 h = ULI78_FULLHEIGHT * scale;

            u32 *ptr = studio->video.buffer;
            const u32 *src = pixels;
            for(s32 y = 0; y < h; y++, src = pixels + (y / scale) * ULI78_FULLWIDTH)
                for(s32 x = 0; x < w; x++)
                    *ptr++ = src[x / scale];

            // with centiSecondsPerFame == 3 we have 1000/(3*10)=~33.3fps, so we have to save every second frame
            msf_gif_frame(&studio->video.gif, (u8*)studio->video.buffer, 3, 16, w * sizeof(u32));
        }

        if(studio->video.screenshot)
        {
            studio->video.screenshot = false;
            stopVideoRecord(studio);
            return;
        }

        if(studio->video.frame % ULI78_FRAMERATE < ULI78_FRAMERATE / 2)
        {
            drawRecordLabel(studio, pixels, ULI78_WIDTH - 24, 8);
        }

        studio->video.frame++;
    }
}

#endif

static void renderStudio(Studio* studio)
{
    uli_mem* uli = studio->uli;

#if defined(BUILD_EDITORS)
    showTooltip(studio, "");
#endif

    {
        const uli_sfx* sfx = NULL;
        const uli_music* music = NULL;

        switch(studio->mode)
        {
        case ULI_RUN_MODE:
            sfx = &studio->uli->ram->sfx;
            music = &studio->uli->ram->music;
            break;
        case ULI_START_MODE:
        case ULI_MENU_MODE:
        case ULI_SURF_MODE:
            sfx = &studio->config->cart->bank0.sfx;
            music = &studio->config->cart->bank0.music;
            break;
        default:
#if defined(BUILD_EDITORS)
            sfx = getSfxSrc(studio);
            music = getMusicSrc(studio);
#endif
            break;
        }

        sfx2ram(uli->ram, sfx);
        music2ram(uli->ram, music);

        // restore mapping
        studio->uli->ram->mapping = getConfig(studio)->options.mapping;

        uli_core_tick_start(uli);
    }

    // SECURITY: It's important that this comes before `tick` and not after
    // to prevent misbehaving cartridges from having an opportunity to
    // tamper with the keyboard input.
    processShortcuts(studio);

    // clear screen for all the modes except the Run mode
    if(studio->mode != ULI_RUN_MODE)
    {
        VBANK(uli, 1)
        {
            uli_api_cls(uli, 0);
        }
    }

    switch(studio->mode)
    {
    case ULI_START_MODE:    studio->start->tick(studio->start); break;
    case ULI_RUN_MODE:      studio->run->tick(studio->run); break;
    case ULI_MENU_MODE:     studio_menu_tick(studio->menu); break;

#if defined(BUILD_EDITORS)
    case ULI_CONSOLE_MODE:  studio->console->tick(studio->console); break;
    case ULI_CODE_MODE:
        {
            Code* code = studio->code;
            code->tick(code);
        }
        break;
    case ULI_SPRITE_MODE:
        {
            Sprite* sprite = studio->banks.sprite[studio->bank.index.sprites];
            sprite->tick(sprite);
        }
        break;
    case ULI_MAP_MODE:
        {
            Map* map = studio->banks.map[studio->bank.index.map];
            map->tick(map);
        }
        break;
    case ULI_SFX_MODE:
        {
            Sfx* sfx = studio->banks.sfx[studio->bank.index.sfx];
            sfx->tick(sfx);
        }
        break;
    case ULI_MUSIC_MODE:
        {
            Music* music = studio->banks.music[studio->bank.index.music];
            music->tick(music);
        }
        break;

    case ULI_WORLD_MODE:    studio->world->tick(studio->world); break;
    case ULI_SURF_MODE:     studio->surf->tick(studio->surf); break;
#endif
    default: break;
    }

    uli_core_tick_end(uli);

    switch(studio->mode)
    {
    case ULI_RUN_MODE: break;
    case ULI_SURF_MODE:
    case ULI_MENU_MODE:
        uli->input.data = -1;
        break;
    default:
        uli->input.data = -1;
        uli->input.gamepad = 0;
    }
}

static void updateSystemFont(Studio* studio)
{
    uli_mem* uli = studio->uli;

    studio->systemFont = (uli_font)
    {
        .regular =
        {
	    { 0 },
	    {
		{
		    .width       = ULI_FONT_WIDTH,
		    .height      = ULI_FONT_HEIGHT,
		},
	    },
        },
        .alt =
        {
	    { 0 },
	    {
		{
		    .width       = ULI_ALTFONT_WIDTH,
		    .height      = ULI_FONT_HEIGHT,
		},
	    },
        }
    };

    u8* dst = (u8*)&studio->systemFont;

    for(s32 i = 0; i < ULI_FONT_CHARS * 2; i++)
        for(s32 y = 0; y < ULI_SPRITESIZE; y++)
            for(s32 x = 0; x < ULI_SPRITESIZE; x++)
                if(uli_tool_peek4(&studio->config->cart->bank0.sprites.data[i], ULI_SPRITESIZE*y + x))
                    dst[i*BITS_IN_BYTE+y] |= 1 << x;

    uli->ram->font = studio->systemFont;
}

void studioConfigChanged(Studio* studio)
{
#if defined(BUILD_EDITORS)
    Code* code = studio->code;
    if(code->update)
        code->update(code);

    s32 scale = studio->config->data.uiScale;
    studio->video.buffer = realloc(studio->video.buffer, ULI78_FULLWIDTH * scale * ULI78_FULLHEIGHT * scale * sizeof(u32));
#endif

    updateSystemFont(studio);
    uli_sys_update_config();
}

static void processMouseStates(Studio* studio)
{
    for(s32 i = 0; i < COUNT_OF(studio->mouse.state); i++)
        studio->mouse.state[i].dbl.click = studio->mouse.state[i].click = false;

    uli_mem* uli = studio->uli;

    uli->ram->vram.vars.cursor.sprite = uli_cursor_arrow;
    uli->ram->vram.vars.cursor.system = true;

    for(s32 i = 0; i < COUNT_OF(studio->mouse.state); i++)
    {
        MouseState* state = &studio->mouse.state[i];

        s32 down = uli->ram->input.mouse.btns & (1 << i);

        if(!state->down && down)
        {
            state->down = true;
            state->start = uli_api_mouse(uli);
        }
        else if(state->down && !down)
        {
            state->end = uli_api_mouse(uli);

            state->click = true;
            state->down = false;

            state->dbl.click = state->dbl.ticks - state->dbl.start < 1000 / ULI78_FRAMERATE;
            state->dbl.start = state->dbl.ticks;
        }

        state->dbl.ticks++;
    }
    uli->ram->input.mouse.scrollx *= -1;
}

#if defined(BUILD_EDITORS)
static void doCodeExport(Studio* studio)
{
#ifndef BAREMETALPI
    char pos[sizeof studio->bytebattle.last.postag];
    {
        s32 x = 0, y = 0;

        if(studio->mode != ULI_RUN_MODE)
        {
            codeGetPos(studio->code, &x, &y);
            x++; y++;
        }

        sprintf(pos, "-- pos: %i,%i\n", x, y);
    }

    if(strcmp(studio->bytebattle.last.postag, pos) || strcmp(studio->bytebattle.last.code.data, studio->code->src))
    {
        FILE* file = fopen(studio->bytebattle.exp, "wb");

        if(file)
        {
            strcpy(studio->bytebattle.last.postag, pos);
            strcpy(studio->bytebattle.last.code.data, studio->code->src);

            fwrite(pos, 1, strlen(pos), file);
            fwrite(studio->code->src, 1, strlen(studio->code->src), file);
            fclose(file);
        }
    }
#endif
}

static void doCodeImport(Studio* studio)
{
#ifndef BAREMETALPI
    FILE* file = fopen(studio->bytebattle.imp, "rb");

    if(file)
    {
        static uli_code code;
        code.data[fread(code.data, 1, sizeof(uli_code), file)] = '\0';

        char* end = strchr(code.data, '\n');

        if(end)
        {
            static const char PosTag[] = "-- pos: ";
            enum{TagSize = sizeof PosTag - 1};

            if(memcmp(code.data, PosTag, TagSize) == 0)
            {
                char* start = code.data + TagSize;
                char* sep = strchr(start, ',');

                if(sep)
                {
                    *sep = *end = '\0';
                    s32 x = atoi(start);
                    s32 y = atoi(sep + 1);

                    if(x == 0 && y == 0)
                    {
                        if(studio->mode != ULI_RUN_MODE)
                            runGame(studio);
                    }
                    else
                    {
                        s32 offset = end - code.data + 1;
                        memcpy(studio->code->src, code.data + offset, sizeof(uli_code) - offset);
                        codeSetPos(studio->code, x - 1, y - 1);

                        if(studio->mode == ULI_RUN_MODE)
                            setStudioMode(studio, ULI_CODE_MODE);
                    }
                }
            }
        }

        fclose(file);
    }
#endif
}
#endif

static void blitCursor(Studio* studio)
{
    uli_mem* uli = studio->uli;
    uli78_mouse* m = &uli->ram->input.mouse;

    if(uli->input.mouse && !m->relative && (s32)m->x < ULI78_FULLWIDTH && (s32)m->y < ULI78_FULLHEIGHT)
    {
        s32 sprite = CLAMP(uli->ram->vram.vars.cursor.sprite, 0, ULI_BANK_SPRITES - 1);
        const uli_bank* bank = &uli->cart.bank0;

        uli_point hot = {0};

        if(uli->ram->vram.vars.cursor.system)
        {
            bank = &getConfig(studio)->cart->bank0;
            hot = (uli_point[])
            {
                {0, 0},
                {3, 0},
                {2, 3},
            }[CLAMP(sprite, 0, 2)];
        }
        else if(sprite == 0) return;

        const uli_palette* pal = &bank->palette.vbank0;
        const uli_tile* tile = &bank->sprites.data[sprite];

        uli_point s = {m->x - hot.x, m->y - hot.y};
        u32* dst = uli->product.screen + ULI78_FULLWIDTH * s.y + s.x;

        for(s32 y = s.y, endy = MIN(y + ULI_SPRITESIZE, ULI78_FULLHEIGHT), i = 0; y != endy; ++y, dst += ULI78_FULLWIDTH - ULI_SPRITESIZE)
            for(s32 x = s.x, endx = x + ULI_SPRITESIZE; x != endx; ++x, ++i, ++dst)
                if(x < ULI78_FULLWIDTH)
                {
                    u8 c = uli_tool_peek4(tile->data, i);
                    if(c)
                        *dst = uli_rgba(&pal->colors[c]);
                }
    }
}

uli_mem* getMemory(Studio* studio)
{
    return studio->uli;
}

const uli_mem* studio_mem(Studio* studio)
{
    return getMemory(studio);
}

void studio_tick(Studio* studio, uli78_input input)
{
    uli_mem* uli = studio->uli;
    uli->ram->input = input;

#if defined(BUILD_EDITORS)
    processAnim(studio->anim.movie, studio);
    checkChanges(studio);
    uli_net_start(studio->net);
#endif

    if(studio->toolbarMode)
    {
        setStudioMode(studio, studio->toolbarMode);
        studio->toolbarMode = 0;
    }

    processMouseStates(studio);
    renderStudio(studio);

    {
#if defined(BUILD_EDITORS)
        Sprite* sprite = studio->banks.sprite[studio->bank.index.sprites];
        Map* map = studio->banks.map[studio->bank.index.map];
#endif

        uli_blit_callback callback[ULI_MODES_COUNT] =
        {
            [ULI_MENU_MODE]     = {studio_menu_anim_scanline, NULL, NULL, studio->menu},

#if defined(BUILD_EDITORS)
            [ULI_SPRITE_MODE]   = {sprite->scanline,        NULL, NULL, sprite},
            [ULI_MAP_MODE]      = {map->scanline,           NULL, NULL, map},
            [ULI_WORLD_MODE]    = {studio->world->scanline,    NULL, NULL, studio->world},
            [ULI_SURF_MODE]     = {studio->surf->scanline,     NULL, NULL, studio->surf},
#endif
        };

        if(studio->mode != ULI_RUN_MODE)
        {
            memcpy(uli->ram->vram.palette.data, getConfig(studio)->cart->bank0.palette.vbank0.data, sizeof(uli_palette));
            uli->ram->font = studio->systemFont;
        }

        callback[studio->mode].data
            ? uli_core_blit_ex(uli, callback[studio->mode])
            : uli_core_blit(uli);

        blitCursor(studio);

#if defined(BUILD_EDITORS)
        if(isRecordFrame(studio))
            recordFrame(studio, uli->product.screen);

        drawPopup(studio);
#endif
    }

#if defined(BUILD_EDITORS)
    uli_net_end(studio->net);

    {
        Bytebattle* bb = &(studio->bytebattle);
        if(bb->battle.started)
        {
            u32 passed = getTime() - bb->battle.started;
            bb->battle.left = bb->battle.time - passed;

            if(bb->battle.left > 0)
            {
                s32 delta = bb->battle.time / (bb->limit.upper - bb->limit.lower);
                bb->limit.current = bb->limit.upper - passed / delta;
            }
            else
            {
                bb->battle.left = 0;
                bb->limit.current = bb->limit.lower;
            }
        }

        if(bb->delay)
            if(bb->ticks++ < bb->delay)
                return;

        if(bb->exp)
            doCodeExport(studio);
        else if(bb->imp)
            doCodeImport(studio);

        bb->ticks = 0;
    }
#endif
}

void studio_sound(Studio* studio)
{
    uli_mem* uli = studio->uli;
    uli_core_synth_sound(uli);

    s32 volume = getConfig(studio)->options.volume;

    if(volume != MAX_VOLUME)
    {
        s32 size = uli->product.samples.count;
        for(s16* it = uli->product.samples.buffer, *end = it + size; it != end; ++it)
            *it = *it * volume / MAX_VOLUME;
    }
}

#if defined(BUILD_EDITORS)
static void onStudioLoadConfirmed(Studio* studio, bool yes, void* data)
{
    if(yes)
    {
        const char* file = data;
        showPopupMessage(studio, studio->console->loadCart(studio->console, file)
            ? "cart successfully loaded :)"
            : "error: cart not loaded :(");
    }
}

void confirmLoadCart(Studio* studio, ConfirmCallback callback, void* data)
{
    static const char* Warning[] =
    {
        "WARNING!",
        "You have unsaved changes",
        "Do you really want to load cart?",
    };

    confirmDialog(studio, Warning, COUNT_OF(Warning), callback, data);
}

#endif

void studio_load(Studio* studio, const char* file)
{
#if defined(BUILD_EDITORS)
    studioCartChanged(studio)
        ? confirmLoadCart(studio, onStudioLoadConfirmed, (void*)file)
        : onStudioLoadConfirmed(studio, true, (void*)file);
#endif
}

void exitGame(Studio* studio)
{
    if(studio->prevMode == ULI_SURF_MODE)
    {
        setStudioMode(studio, ULI_SURF_MODE);
    }
    else
    {
        setStudioMode(studio, ULI_CONSOLE_MODE);
    }
}

void studio_delete(Studio* studio)
{
    {
#if defined(BUILD_EDITORS)
        for(s32 i = 0; i < ULI_EDITOR_BANKS; i++)
        {
            freeSprite  (studio->banks.sprite[i]);
            freeMap     (studio->banks.map[i]);
            freeSfx     (studio->banks.sfx[i]);
            freeMusic   (studio->banks.music[i]);
        }

        freeCode    (studio->code);
        freeConsole (studio->console);
        freeWorld   (studio->world);
        freeSurf    (studio->surf);

        FREE(studio->anim.show.items);
        FREE(studio->anim.hide.items);

#endif

        freeStart   (studio->start);
        freeRun     (studio->run);
        freeConfig  (studio->config);

        studio_mainmenu_free(studio->mainmenu);
        studio_menu_free(studio->menu);
    }

    uli_core_close(studio->uli);

#if defined(BUILD_EDITORS)
    uli_net_close(studio->net);
    free(studio->video.buffer);
    if(studio->bytebattle.exp) free(studio->bytebattle.exp);
    if(studio->bytebattle.imp) free(studio->bytebattle.imp);
#endif

    free(studio->fs);
    free(studio);
}

#if defined(BUILD_EDITORS)
Bytebattle* getBytebattle(Studio* studio)
{
    return studio->bytebattle.exp || studio->bytebattle.imp ? &(studio->bytebattle) : NULL;
}
#endif

static StartArgs parseArgs(s32 argc, char **argv)
{
    static const char *const usage[] =
    {
        "uli78 [cart] [options]",
        NULL,
    };

    StartArgs args = {0};
    args.volume = -1;

#if defined(BUILD_EDITORS)
    args.lowerlimit = 256;
    args.upperlimit = 512;
    args.fftlist = 0;
#endif

    struct argparse_option options[] =
    {
        OPT_HELP(),
#define CMD_PARAMS_DEF(name, ctype, type, post, help) OPT_##type('\0', #name, &args.name, help),
        CMD_PARAMS_LIST(CMD_PARAMS_DEF)
#undef  CMD_PARAMS_DEF
#if defined(BUILD_EDITORS)
        OPT_GROUP("Byte battle options:\n"),
        OPT_STRING('\0',    "codeexport",    &args.codeexport,   "export code to filename"),
        OPT_STRING('\0',    "codeimport",    &args.codeimport,   "import code from filename"),
        OPT_INTEGER('\0',   "delay",         &args.delay,        "codeexport / codeimport update interval in ticks"),
        OPT_INTEGER('\0',   "lowerlimit",    &args.lowerlimit,   "lower limit for code size (256 by default)"),
        OPT_INTEGER('\0',   "upperlimit",    &args.upperlimit,   "upper limit for code size (512 by default)"),
        OPT_INTEGER('\0',   "battletime",    &args.battletime,   "battletime in minutes"),
        OPT_GROUP("FFT options:\n"),
        OPT_BOOLEAN('\0', "fft", &args.fft, "enable FFT support"),
        OPT_BOOLEAN('\0', "fftlist", &args.fftlist, "list FFT devices"),
        OPT_BOOLEAN('\0', "fftcaptureplaybackdevices", &args.fftcaptureplaybackdevices, "Capture playback devices for loopback (Windows only)"),
        OPT_STRING('\0', "fftdevice", &args.fftdevice, "name of the device to use with FFT"),
#endif
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\n" ULI_NAME " startup options:", NULL);
    argc = argparse_parse(&argparse, argc, (const char**)argv);

    if(argc == 1)
        args.cart = argv[0];

    return args;
}

#if defined(BUILD_EDITORS)

static void setIdle(void* data)
{
    Studio* studio = data;
    studio->anim.movie = resetMovie(&studio->anim.idle);
}

static void setPopupWait(void* data)
{
    Studio* studio = data;
    studio->anim.movie = resetMovie(&studio->anim.wait);
}

static void setPopupHide(void* data)
{
    Studio* studio = data;
    studio->anim.movie = resetMovie(&studio->anim.hide);
}

#endif

void studio_keymapchanged(Studio* studio, uli_layout keyboardLayout)
{
    studio->config->data.keyboardLayout = keyboardLayout;
}

bool studio_alive(Studio* studio)
{
    return studio->alive;
}

#if defined(ULI_MODULE_EXT)
static bool onEnumModule(const char* name, const char* title, const char* hash, s32 id, void* data, bool dir)
{
    if(strstr(name, ULI_MODULE_EXT))
    {
        void* module = dlopen(name, RTLD_NOW | RTLD_LOCAL);

        if(module)
        {
            const uli_script *config = dlsym(module, DEF2STR(SCRIPT_CONFIG));

            if(config)
            {
                uli_add_script(config);
            }
            else
            {
                dlclose(module);
            }
        }
    }

    return true;
}
#endif

Studio* studio_create(s32 argc, char **argv, s32 samplerate, uli78_pixel_color_format format, const char* folder, s32 maxscale, uli_layout keyboardLayout)
{
    setbuf(stdout, NULL);

    StartArgs args = parseArgs(argc, argv);

    if (args.version)
    {
        printf("%s\n", ULI_VERSION);
        exit(0);
    }

#if defined(ULI_MODULE_EXT)
    // load script modules
    fs_enum(fs_appfolder(), onEnumModule, NULL);
#endif

    Studio* studio = NEW(Studio);
    *studio = (Studio)
    {
        .mode = ULI_START_MODE,
        .prevMode = ULI_CODE_MODE,

#if defined(BUILD_EDITORS)
        .menuMode = ULI_CONSOLE_MODE,

        .bank =
        {
            .chained = true,
        },

        .anim =
        {
            .pos =
            {
                .popup = -TOOLBAR_SIZE,
            },
            .idle = {.done = emptyDone,}
        },

        .popup =
        {
            .message = "\0",
        },

        .tooltip =
        {
            .text = "\0",
        },

        .samplerate = samplerate,
        .net = uli_net_create(ULI_WEBSITE),

        .bytebattle = {0},
#endif
        .uli = uli_core_create(samplerate, format),
    };

    {
        const char *path = args.fs ? args.fs : folder;

        if (fs_isdir(path))
        {
            studio->fs = uli_fs_create(path,
#if defined(BUILD_EDITORS)
                studio->net
#else
                NULL
#endif
            );
        }
        else
        {
            fprintf(stderr, "error: `%s` is not a folder\n", path);
            exit(1);
        }
    }

    {

#if defined(BUILD_EDITORS)
        for(s32 i = 0; i < ULI_EDITOR_BANKS; i++)
        {
            studio->banks.sprite[i]   = calloc(1, sizeof(Sprite));
            studio->banks.map[i]      = calloc(1, sizeof(Map));
            studio->banks.sfx[i]      = calloc(1, sizeof(Sfx));
            studio->banks.music[i]    = calloc(1, sizeof(Music));
        }

        studio->code       = calloc(1, sizeof(Code));
        studio->console    = calloc(1, sizeof(Console));
        studio->world      = calloc(1, sizeof(World));
        studio->surf       = calloc(1, sizeof(Surf));

        studio->anim.show = (Movie)MOVIE_DEF(STUDIO_ANIM_TIME, setPopupWait,
        {
            {-TOOLBAR_SIZE, 0, STUDIO_ANIM_TIME, &studio->anim.pos.popup, AnimEaseIn},
        });

        studio->anim.wait = (Movie){.time = ULI78_FRAMERATE * 2, .done = setPopupHide};
        studio->anim.hide = (Movie)MOVIE_DEF(STUDIO_ANIM_TIME, setIdle,
        {
            {0, -TOOLBAR_SIZE, STUDIO_ANIM_TIME, &studio->anim.pos.popup, AnimEaseIn},
        });

        studio->anim.movie = resetMovie(&studio->anim.idle);
#endif

        studio->start      = calloc(1, sizeof(Start));
        studio->run        = calloc(1, sizeof(Run));
        studio->menu       = studio_menu_create(studio);
        studio->config     = calloc(1, sizeof(Config));
    }
    studio->mainmenu = NULL;
    uli_fs_makedir(studio->fs, ULI_LOCAL);
    uli_fs_makedir(studio->fs, ULI_LOCAL_VERSION);

    initConfig(studio->config, studio, studio->fs);

    if (studio->config->data.uiScale > maxscale)
    {
        printf("Overriding specified uiScale of %i; the maximum your screen will accommodate is %i", studio->config->data.uiScale, maxscale);
        studio->config->data.uiScale = maxscale;
    }

    initStart(studio->start, studio, args.cart);
    initRunMode(studio);

#if defined(BUILD_EDITORS)
    initConsole(studio->console, studio, studio->fs, studio->net, studio->config, args);
    initSurfMode(studio);
    initModules(studio);
#endif

    if(args.scale)
        studio->config->data.uiScale = args.scale;

    if(args.volume >= 0)
        studio->config->data.options.volume = args.volume & 0x0f;

#if defined(CRT_SHADER_SUPPORT)
    studio->config->data.options.crt        |= args.crt;
#endif

    studio->config->data.options.fullscreen |= args.fullscreen;
    studio->config->data.options.vsync      |= args.vsync;
    studio->config->data.soft               |= args.soft;
    studio->config->data.cli                |= args.cli;

#if defined(BUILD_EDITORS)
    if(args.codeexport)
        studio->bytebattle.exp = strdup(args.codeexport);
    else if(args.codeimport)
        studio->bytebattle.imp = strdup(args.codeimport);

    studio->bytebattle.delay = args.delay;
    studio->bytebattle.limit.lower = args.lowerlimit;

    studio->bytebattle.limit.current
        = studio->bytebattle.limit.upper
        = args.upperlimit;

    studio->bytebattle.battle.left
        = studio->bytebattle.battle.time
        = args.battletime * 60 * 1000;

    if (args.fftlist)
    {
        FFT_EnumerateDevices();
        exit(0);
    }
    studio->config->data.fft = args.fft;
    studio->config->data.fftcaptureplaybackdevices = args.fftcaptureplaybackdevices;
    studio->config->data.fftdevice = args.fftdevice;
    studio->config->data.keyboardLayout = keyboardLayout;
#endif

    studioConfigChanged(studio);

    if(args.cli)
        args.skip = true;

#if defined(BUILD_EDITORS)
    if(args.skip)
    {
        if(getBytebattle(studio))
        {
            studio->console->tick(studio->console);
            gotoCode(studio);
        }
        else
            setStudioMode(studio, ULI_CONSOLE_MODE);
    }
#endif

    return studio;
}
