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

#include "console.h"
#include "start.h"
#include "tools.h"
#include "studio/fs.h"
#include "studio/net.h"
#include "studio/config.h"
#include "ext/png.h"
#include "ext/json.h"
#include "zip.h"
#include "endians.h"

#if defined(ULI78_PRO)
#include "studio/project.h"
#else
#include "cart.h"
#endif

#include <ctype.h>
#include <string.h>

#if !defined(__ULI_MACOSX__)
#include <malloc.h>
#endif

#include <sys/stat.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#define CONSOLE_CURSOR_COLOR        uli_color_red
#define CONSOLE_INPUT_COLOR         uli_color_white
#define CONSOLE_BACK_TEXT_COLOR     uli_color_grey
#define CONSOLE_FRONT_TEXT_COLOR    uli_color_light_grey
#define CONSOLE_ERROR_TEXT_COLOR    uli_color_red
#define CONSOLE_LINK_TEXT_COLOR     uli_color_blue
#define CONSOLE_CURSOR_BLINK_PERIOD ULI78_FRAMERATE
#define CONSOLE_CURSOR_DELAY        (ULI78_FRAMERATE / 2)
#define CONSOLE_BUFFER_WIDTH        (STUDIO_TEXT_BUFFER_WIDTH)
#define CONSOLE_BUFFER_HEIGHT       (STUDIO_TEXT_BUFFER_HEIGHT)
#define CONSOLE_BUFFER_SCREENS      64
#define CONSOLE_BUFFER_SCREEN       (CONSOLE_BUFFER_WIDTH * CONSOLE_BUFFER_HEIGHT)
#define CONSOLE_BUFFER_SIZE         (CONSOLE_BUFFER_SCREEN * CONSOLE_BUFFER_SCREENS)
#define CONSOLE_BUFFER_ROWS         (CONSOLE_BUFFER_SIZE / CONSOLE_BUFFER_WIDTH)
#define DEFAULT_CHMOD               0755

#define HELP_CMD_LIST(macro)    \
    macro(version)              \
    macro(welcome)              \
    macro(spec)                 \
    macro(ram)                  \
    macro(vram)                 \
    macro(commands)             \
    macro(api)                  \
    macro(keys)                 \
    macro(buttons)              \
    macro(startup)              \
    macro(hotkeys)              \
    macro(terms)                \
    macro(license)

#define IMPORT_CMD_LIST(macro)  \
    macro(binary)               \
    macro(tiles)                \
    macro(sprites)              \
    macro(map)                  \
    macro(code)                 \
    macro(screen)               \
    macro(mapimg)

#define IMPORT_KEYS_LIST(macro) \
    macro(bank)                 \
    macro(x)                    \
    macro(y)                    \
    macro(w)                    \
    macro(h)                    \
    macro(vbank)                \
    macro(bpp)

#define EXPORT_CMD_LIST(macro)  \
    macro(win)                  \
    macro(winxp)                \
    macro(linux)                \
    macro(rpi)                  \
    macro(mac)                  \
    macro(html)                 \
    macro(binary)               \
    macro(tiles)                \
    macro(sprites)              \
    macro(map)                  \
    macro(mapimg)               \
    macro(sfx)                  \
    macro(music)                \
    macro(screen)               \
    macro(help)

#if defined(ULI78_PRO)
#   define ALONE_KEY(macro) macro(alone)
#else
#   define ALONE_KEY(macro)
#endif

#define EXPORT_KEYS_LIST(macro) \
    macro(bank)                 \
    macro(vbank)                \
    macro(id)                   \
    ALONE_KEY(macro)

static const char* WelcomeText =
    "ULI-78 is a fantasy computer for making, playing and sharing tiny games.\n\n"
    "It has built-in tools for development: code, sprites, maps, sound editors and the command line, "
    "which is enough to create a mini retro game.\n"
    "In the end, you will get a cartridge file, which can be stored and played on the website.\n\n"
    "Also, the game can be packed into a player that works on all popular platforms and distributed as you wish.\n"
    "To make a retro-style game, the whole creation process takes place under some technical limitations: "
    "240x136 pixels display, 16 color palette, 256 8x8 color sprites, 4 channel sound, etc.";

static const struct SpecRow {const char* section; const char* info;} SpecText1[] =
{
    {"DISPLAY", "240x136 pixels, 16 colors palette."},
    {"INPUT",   "4 gamepads with 8 buttons / mouse / keyboard."},
    {"SPRITES", "256 8x8 tiles and 256 8x8 sprites."},
    {"MAP",     "240x136 cells, 1920x1088 pixels."},
    {"SOUND",   "4 channels with configurable waveforms."},
    {"CODE",    "64KB of $LANG_NAMES$.",
    },
};

static const struct HotkeysRowGeneral {const char* section; const char* info;} HotkeysTextGeneral[] =
{
    {"CTRL+R/ENTER",  "Run current project."},
    {"CTRL+S",        "Save cart."},
    {"CTRL+X/C/V",    "Cut/copy/paste in the editors."},
    {"CTRL+Z/Y",      "Undo/redo changes in the editors."},
    {"F6",            "Toggle CRT filter."},
    {"F7",            "Assign cover image while in game."},
    {"F8",            "Take a screenshot."},
    {"F9",            "Start/stop GIF video recording."},
    {"F11/ALT+ENTER", "Fullscreen/window mode."},
    {"CTRL+Q",        "Quit the application.",
    },
};

static const struct HotkeysRowNavigation {const char* section; const char* info;} HotkeysTextNavigation[] =
{
    {"ESC",              "Switch console/editor or open menu while in game."},
    {"ESC+F1",           "Switch to code editor while in game."},
    {"ALT+~",            "Show console."},
    {"ALT+1/F1",         "Show code editor."},
    {"ALT+2/F2",         "Show sprite editor."},
    {"ALT+3/F3",         "Show map editor."},
    {"ALT+4/F4",         "Show sfx editor."},
    {"ALT+5/F5",         "Show music editor."},
    {"CTRL+PGUP/PGDOWN", "Switch to previous/next editor mode.",
    },
};

static const struct HotkeysRowCodeEditor {const char* section; const char* info;} HotkeysTextCodeEditor[] =
{
    {"CTRL+F",             "Find."},
    {"CTRL+G",             "Go to line."},
    {"CTRL+P/N",           "Move to previous/next line."},
    {"ALT/CTRL+LEFT",      "Move to previous word."},
    {"ALT/CTRL+RIGHT",     "Move to next word."},
    {"ALT/CTRL+BACKSPACE", "Delete previous word."},
    {"ALT/CTRL+DELETE",    "Delete next word."},
    {"CTRL+K",             "Delete end of line."},
    {"CTRL+D",             "Duplicate current line."},
    {"CTRL+J",             "Newline."},
    {"CTRL+A",             "Select all."},
    {"CTRL+F1",            "Bookmark current line."},
    {"F1",                 "Move to next bookmark."},
    {"CTRL+B",             "Show bookmark list."},
    {"CTRL+O",             "Show code outline and navigate functions."},
    {"CTRL+TAB",           "Indent line."},
    {"CTRL+SHIFT+TAB",     "Unindent line."},
    {"CTRL+/",             "Comment/Uncomment line."},
    {"RIGHT CLICK",        "Drag."},
    {"SCROLL WHEEL",       "Vertical scrolling."},
    {"SHIFT+SCROLL WHEEL", "Horizontal scrolling."},
    {"CTRL+L",             "Center screen on cursor.",
    },
};

static const struct HotkeysRowSpriteEditor {const char* section; const char* info;} HotkeysTextSpriteEditor[] =
{
    {"TAB",      "Switch tiles/sprites."},
    {"[]",       "Choose previous/next palette color."},
    {"-/=",      "Change brush size."},
    {"SCROLL",   "Canvas zoom."},
    {"1",        "Select brush."},
    {"2",        "Select color picker."},
    {"3",        "Select selection tool."},
    {"4",        "Select filling tool."},
    {"5",        "Flip horizontally."},
    {"6",        "Flip vertically."},
    {"7",        "Rotate."},
    {"8/DELETE", "Erase.",
    },
};

static const struct HotkeysRowMapEditor {const char* section; const char* info;} HotkeysTextMapEditor[] =
{
    {"SHIFT",      "Show tilesheet."},
    {"CTRL+CLICK", "Replace all identical tiles (when the Fill tool [4] is selected)."},
    {"`",          "Show/hide grid."},
    {"TAB/SCROLL", "Switch to full world map."},
    {"1",          "Select draw."},
    {"2",          "Select drag map."},
    {"3",          "Select selection tool."},
    {"4",          "Select filling tool.",
    },
};

static const struct HotkeysRowSFXEditor {const char* section; const char* info;} HotkeysTextSFXEditor[] =
{
    {"SPACE",         "Play last played note."},
    {"Z,X,C,V,B,N,M", "Play notes corresponding to one octave (bottom row of QWERTY layout)."},
    {"S,D,G,H,J",     "Play notes corresponding to sharps and flats (home row of QWERTY layout).",
    },
};

static const struct HotkeysRowMusicEditor {const char* section; const char* info;} HotkeysTextMusicEditor[] =
{
    {"SHIFT+ENTER",   "Play pattern from cursor position in the music editor."},
    {"ENTER",         "Play frame."},
    {"SPACE",         "Play track."},
    {"CTRL+F",        "Follow."},
    {"Z,X,C,V,B,N,M", "Play notes corresponding to one octave (bottom row of QWERTY layout) in tracker mode."},
    {"S,D,G,H,J",     "Play notes corresponding to sharps and flats (home row of QWERTY layout) in tracker mode."},
    {"A",             "Insert note break (or 'stop')."},
    {"DELETE",        "Delete selection / selected row."},
    {"BACKSPACE",     "Delete the row above."},
    {"INSERT",        "Insert rows below."},
    {"CTRL+F1",       "Decrease notes by Semitone."},
    {"CTRL+F2",       "Increase notes by Semitone."},
    {"CTRL+F3",       "Decrease octaves."},
    {"CTRL+F4",       "Increase octaves."},
    {"CTRL+RIGHT",    "Jump forward one frame."},
    {"CTRL+LEFT",     "Jump backward one frame."},
    {"TAB",           "Go to next channel."},
    {"SHIFT+TAB",     "Go to previous channel."},
    {"+",             "Next pattern."},
    {"-",             "Previous pattern."},
    {"CTRL+UP",       "Next instrument."},
    {"CTRL+DOWN",     "Previous instrument."},
    {"F5",            "Switch piano/tracker mode.",
    },
};

static const char* TermsText =
    "## Terms of Use\n"
    "- All cartridges posted on the " ULI_WEBSITE " website are the property of their authors.\n"
    "- Do not redistribute the cartridge without permission, directly from the author.\n"
    "- By uploading cartridges to the site, you grant Nesbox the right to freely use and distribute them. "
    "All other rights by default remain with the author.\n"
    "- Do not post material that violates copyright, obscenity or any other laws.\n"
    "- Nesbox reserves the right to remove or filter any material without prior notice.\n\n"
    "## Privacy Policy\n"
    "We store only the user's email and password in encrypted form and will not transfer any personal "
    "information to third parties without explicit permission.";

static const char* LicenseText =
    "## MIT License\n"
    "\n"
    "Copyright (c) 2017-" ULI_VERSION_YEAR " Vadim Grigoruk @uli78 // grigoruk@gmail.com\n"
    "\n"
    "Permission is hereby granted, free of charge, to any person obtaining a copy "
    "of this software and associated documentation files (the 'Software'), to deal "
    "in the Software without restriction, including without limitation the rights "
    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
    "copies of the Software, and to permit persons to whom the Software is "
    "furnished to do so, subject to the following conditions: "
    "The above copyright notice and this permission notice shall be included in all "
    "copies or substantial portions of the Software.\n"
    "\n"
    "THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
    "SOFTWARE.";

static const struct StartupOption {const char* name; const char* help;} StartupOptions[] =
{
#define CMD_PARAMS_DEF(name, ctype, type, post, help) {#name post, help},
    CMD_PARAMS_LIST(CMD_PARAMS_DEF)
#undef CMD_PARAMS_DEF
};

struct CommandDesc
{
    char* command;

    struct Param
    {
        char* key;
        char* val;
    }* params;

    s32 count;

    char* src;
};

static const char* PngExt = PNG_EXT;

#if defined(__EMSCRIPTEN__)
#define CAN_ADDGET_FILE 1
#endif


// You must free the result if result is non-NULL. TODO: find a better place for this function?
char *str_replace(const char *orig, char *rep, char *with) {
    char *result; // the return string
    const char *ins;    // the next insert point
    char *tmp;    // varies
    s32 len_rep;  // length of rep (the string to remove)
    s32 len_with; // length of with (the string to replace rep with)
    s32 len_front; // distance between rep and end of last rep
    s32 count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

static char* replaceHelpTokens(const char* text)
{
    char langnames[ULINAME_MAX] = {0};
    char langextensions[ULINAME_MAX] = {0};

    char langnamespipe[ULINAME_MAX] = {0};

    for(const uli_script **it = uli_scripts(); *it; ++it)
    {
        bool isLast = *(it + 1) == NULL;
        bool isSecondToLast = *(it + 2) == NULL;

        strcat(langnames, (*it)->name);
        if (!isLast)
            strcat(langnames, isSecondToLast ? " or " : ", ");

        strcat(langextensions, (*it)->fileExtension);
        strcat(langextensions, " ");

        strcat(langnamespipe, (*it)->name);
        if (!isLast)
            strcat(langnamespipe, "|");
    }

    char* replaced1 = str_replace(text, "$LANG_NAMES$", langnames);
    char* replaced2 = str_replace(replaced1, "$LANG_EXTENSIONS$", langextensions);
    char* replaced3 = str_replace(replaced2, "$LANG_NAMES_PIPE$", langnamespipe);
    free(replaced2);
    free(replaced1);
    return replaced3;
}


static const char* getName(const char* name, const char* ext)
{
    static char path[ULINAME_MAX];

    strcpy(path, name);

    size_t ps = strlen(path);
    size_t es = strlen(ext);

    if(!(ps > es && strstr(path, ext) + es == path + ps))
        strcat(path, ext);

    return path;
}

static const char* getCartName(const char* name)
{
    return getName(name, CART_EXT);
}

static void scrollBuffer(char* buffer)
{
    memmove(buffer, buffer + CONSOLE_BUFFER_WIDTH, CONSOLE_BUFFER_SIZE - CONSOLE_BUFFER_WIDTH);
    memset(buffer + CONSOLE_BUFFER_SIZE - CONSOLE_BUFFER_WIDTH, 0, CONSOLE_BUFFER_WIDTH);
}

static void scrollConsole(Console* console)
{
    while(console->cursor.pos.y >= CONSOLE_BUFFER_HEIGHT * CONSOLE_BUFFER_SCREENS)
    {
        scrollBuffer(console->text);
        scrollBuffer((char*)console->color);

        console->cursor.pos.y--;
    }

    size_t inputLines = (console->cursor.pos.x + console->input.pos) / CONSOLE_BUFFER_WIDTH;
    s32 minScroll = console->cursor.pos.y + inputLines - CONSOLE_BUFFER_HEIGHT + 1;
    if(console->scroll.pos < minScroll)
        console->scroll.pos = minScroll;
}

static void setSymbol(Console* console, char sym, u8 color, s32 offset)
{
    console->text[offset] = sym;
    console->color[offset] = color;
}

static s32 cursorOffset(Console* console)
{
    return console->cursor.pos.x + console->cursor.pos.y * CONSOLE_BUFFER_WIDTH;
}

static uli_point cursorPos(Console* console)
{
    s32 offset = cursorOffset(console) + console->input.pos;
    return (uli_point)
    {
        offset % CONSOLE_BUFFER_WIDTH,
        offset / CONSOLE_BUFFER_WIDTH
    };
}

static void nextLine(Console* console)
{
    console->cursor.pos.x = 0;
    console->cursor.pos.y++;
}

static bool iswrap(char sym)
{
    switch(sym)
    {
    case '|': return true;
    }

    return isspace(sym);
}

static void consolePrintOffset(Console* console, const char* text, u8 color, s32 wrapLineOffset)
{
#ifndef BAREMETALPI
    printf("%s", text);
#endif

    console->cursor.pos = cursorPos(console);

    for(const char* ptr = text, *next = ptr; *ptr; ptr++)
    {
        char symbol = *ptr;

        scrollConsole(console);

        if (symbol == '\n')
            nextLine(console);
        else
        {
            if(!iswrap(symbol))
            {
                const char* cur = ptr;
                s32 len = CONSOLE_BUFFER_WIDTH;

                while(*cur && !iswrap(*cur++)) len--;

                if(len > 0 && len <= console->cursor.pos.x)
                {
                    nextLine(console);
                    console->cursor.pos.x = wrapLineOffset;
                }
            }

            setSymbol(console, symbol, iswrap(symbol) ? uli_color_dark_grey : color, cursorOffset(console));

            console->cursor.pos.x++;

            if (console->cursor.pos.x >= CONSOLE_BUFFER_WIDTH)
            {
                nextLine(console);
                console->cursor.pos.x = wrapLineOffset;
            }
        }
    }

    console->input.text = console->text + cursorOffset(console);
    console->input.pos = 0;
}

static void consolePrint(Console* console, const char* text, u8 color)
{
    consolePrintOffset(console, text, color, 0);
}

static void printBack(Console* console, const char* text)
{
    consolePrint(console, text, CONSOLE_BACK_TEXT_COLOR);
}

static void printFront(Console* console, const char* text)
{
    consolePrint(console, text, CONSOLE_FRONT_TEXT_COLOR);
}

static void printLink(Console* console, const char* text)
{
    consolePrint(console, text, CONSOLE_LINK_TEXT_COLOR);
}

static void printError(Console* console, const char* text)
{
    consolePrint(console, text, CONSOLE_ERROR_TEXT_COLOR);
}

static void printLine(Console* console)
{
    consolePrint(console, "\n", 0);
}

static void clearSelection(Console* console)
{
    ZEROMEM(console->select);
}

static void commandDoneLine(Console* console, bool newLine)
{
    if(!console->args.cli)
    {
        if(newLine)
            printLine(console);

        char dir[ULINAME_MAX];
        uli_fs_dir(console->fs, dir);
        if(strlen(dir))
            printBack(console, dir);

        printFront(console, ">");
    }

    console->active = true;

    clearSelection(console);

    FREE(console->desc->src);
    FREE(console->desc->command);
    FREE(console->desc->params);

    memset(console->desc, 0, sizeof(CommandDesc));
}

static void commandDone(Console* console)
{
    commandDoneLine(console, true);
}

static inline void drawChar(uli_mem* uli, char symbol, s32 x, s32 y, u8 color, bool alt)
{
    uli_api_print(uli, (char[]){symbol, '\0'}, x, y, color, true, 1, alt);
}

static void drawCursor(Console* console)
{
    if(!console->active)
        return;

    uli_point pos = cursorPos(console);
    pos.x *= STUDIO_TEXT_WIDTH;
    pos.y -= console->scroll.pos;
    pos.y *= STUDIO_TEXT_HEIGHT;

    u8 symbol = console->input.text[console->input.pos];

    bool inverse = console->cursor.delay || console->tickCounter % CONSOLE_CURSOR_BLINK_PERIOD < CONSOLE_CURSOR_BLINK_PERIOD / 2;

    if(inverse)
        uli_api_rect(console->uli, pos.x - 1, pos.y - 1, ULI_FONT_WIDTH + 1, ULI_FONT_HEIGHT + 1, CONSOLE_CURSOR_COLOR);

    drawChar(console->uli, symbol, pos.x, pos.y, inverse ? ULI_COLOR_BG : CONSOLE_INPUT_COLOR, false);
}

static void drawConsoleText(Console* console)
{
    uli_mem* uli = console->uli;
    const char* ptr = console->text + console->scroll.pos * CONSOLE_BUFFER_WIDTH;
    const u8* colorPointer = console->color + console->scroll.pos * CONSOLE_BUFFER_WIDTH;

    const char* end = ptr + CONSOLE_BUFFER_SCREEN;
    uli_point pos = {0};

    struct
    {
        const char* start;
        const char* end;
    } select =
    {
        console->select.start,
        console->select.end
    };

    if(select.start > select.end)
        SWAP(select.start, select.end, const char*);

    while(ptr < end)
    {
        char symbol = *ptr++;
        u8 color = *colorPointer++;
        bool hasSymbol = symbol && symbol != ' ';
        bool drawSelection = ptr > select.start && ptr <= select.end;
        s32 x = pos.x * STUDIO_TEXT_WIDTH;
        s32 y = pos.y * STUDIO_TEXT_HEIGHT;

        if(drawSelection)
            uli_api_rect(uli, x, y - 1, STUDIO_TEXT_WIDTH, STUDIO_TEXT_HEIGHT, hasSymbol ? color : CONSOLE_INPUT_COLOR);

        if(hasSymbol)
            drawChar(console->uli, symbol, x, y, drawSelection ? ULI_COLOR_BG : color, false);

        if(++pos.x == CONSOLE_BUFFER_WIDTH)
        {
            pos.y++;
            pos.x = 0;
        }
    }
}

static void processConsoleHome(Console* console)
{
    console->input.pos = 0;
}

static void processConsoleEnd(Console* console)
{
    console->input.pos = strlen(console->input.text);
}

static s32 getInputOffset(Console* console)
{
    return (console->input.text - console->text) + console->input.pos;
}

static void deleteText(Console* console, s32 start, s32 end)
{
    s32 offset = console->input.text - console->text;
    s32 size = CONSOLE_BUFFER_SIZE - offset - end;
    memmove(console->input.text + start, console->input.text + end, size);

    u8* color = console->color + offset;
    memmove(color + start, color + end, size);
}

static void processConsoleDel(Console* console)
{
    deleteText(console, console->input.pos, console->input.pos + 1);
}

static void processConsoleBackspace(Console* console)
{
    if(console->input.pos > 0)
    {
        console->input.pos--;

        processConsoleDel(console);
    }
}

static void onHelpCommand(Console* console);

static void onExitCommand(Console* console)
{
    exitStudio(console->studio);
    commandDone(console);
}

static void onEditCommand(Console* console)
{
    gotoCode(console->studio);
    commandDone(console);
}

static void loadCartSection(Console* console, const uli_cartridge* cart, const char* section)
{
    uli_mem* uli = console->uli;

    static const struct Section
    {
        const char* name;
        s32 offset;
        s32 size;
    } Sections[] =
    {
#define SECTION_DEF(name, ...) {#name, offsetof(uli_bank, name), sizeof(uli_ ## name)},
        ULI_SYNC_LIST(SECTION_DEF)
#undef  SECTION_DEF
    };

    if(section)
    {
        if(strcmp(section, "code") == 0)
            memcpy(&uli->cart.code, &cart->code, sizeof(uli_code));
        else
            FOR(const struct Section*, it, Sections)
                if(strcmp(section, it->name) == 0)
                {
                    memcpy((u8*)&uli->cart.bank0 + it->offset, (const u8*)&cart->bank0 + it->offset, it->size);
                    break;
                }
    }
    else
        memcpy(&uli->cart, cart, sizeof(uli_cartridge));
}

static char* getDemoCartPath(char* path, const uli_script* script)
{
    strcpy(path, ULI_LOCAL_VERSION "default_");

    if(script && script->name)
        strcat(path, script->name);

    strcat(path, ".uli");

    return path;
}

static void* getDemoCart(Console* console, const uli_script* script, s32* size)
{
    char path[1024];
    getDemoCartPath(path, script);

    {
        void* data = uli_fs_loadroot(console->fs, path, size);

        if(data && *size)
            return data;
    }

    u8* data = calloc(1, sizeof(uli_cartridge));

    if(data)
    {
        *size = uli_tool_unzip(data, sizeof(uli_cartridge), script->demo.data, script->demo.size);

        if(*size)
            uli_fs_saveroot(console->fs, path, data, *size, false);
    }

    return data;
}

static void setCartName(Console* console, const char* name, const char* path)
{
    if(console->rom.name != name)
        strcpy(console->rom.name, name);

    if(console->rom.path != path)
        strcpy(console->rom.path, path);
}

static void onLoadDemoCommandConfirmed(Console* console, const uli_script* script)
{
    void* data = NULL;
    s32 size = 0;

    {
        char path[1024];
        getDemoCartPath(path, script);
        const char* name = getCartName(path);
        setCartName(console, name, uli_fs_path(console->fs, name));
    }

    data = getDemoCart(console, script, &size);
    uli_cart_load(&console->uli->cart, data, size);
    uli_api_reset(console->uli);

    studioRomLoaded(console->studio);

    printBack(console, "\ncart ");
    printFront(console, console->rom.name);
    printBack(console, " loaded!\n");

    free(data);
}

static void onCartLoaded(Console* console, const char* name, const char* section)
{
    uli_api_reset(console->uli);

    if(!section)
        setCartName(console, name, uli_fs_path(console->fs, name));

    studioRomLoaded(console->studio);

    printBack(console, "\ncart ");
    printFront(console, console->rom.name);
    printBack(console, " loaded!\nuse ");
    printFront(console, "RUN");
    printBack(console, " command to run it\n");

}

static inline uli_cartridge* newCart()
{
    return malloc(sizeof(uli_cartridge));
}

static void updateProject(Console* console)
{
    uli_mem* uli = console->uli;
    const char* path = console->rom.path;

    if(*path)
    {
        s32 size = 0;
        void* data = fs_read(path, &size);

        if(data) SCOPE(free(data))
        {
#if defined(ULI78_PRO)
            if(project_ext(path))
                uli_project_load(console->rom.name, data, size, &uli->cart);
            else
#endif
                uli_cart_load(&uli->cart, data, size);

            studioRomLoaded(console->studio);
        }
    }
}

typedef struct
{
    Console* console;
    char* name;
    char* section;
    fs_done_callback callback;
    void* calldata;
} LoadByHashData;

static void loadByHashDone(const u8* buffer, s32 size, void* data)
{
    LoadByHashData* loadByHashData = data;
    Console* console = loadByHashData->console;

    uli_cartridge* cart = newCart();

    SCOPE(free(cart))
    {
        uli_cart_load(cart, buffer, size);
        loadCartSection(console, cart, loadByHashData->section);
        onCartLoaded(console, loadByHashData->name, loadByHashData->section);
    }

    if (loadByHashData->callback)
        loadByHashData->callback(loadByHashData->calldata);

    FREE(loadByHashData->name);
    FREE(loadByHashData->section);
    FREE(loadByHashData);

    commandDone(console);
}

static void loadByHash(Console* console, const char* name, const char* hash, const char* section, fs_done_callback callback, void* data)
{
    console->active = false;

    LoadByHashData loadByHashData = { console, strdup(name), section ? strdup(section) : NULL, callback, data};
    uli_fs_hashload(console->fs, name, hash, loadByHashDone, MOVE(loadByHashData));
}

typedef struct
{
    Console* console;
    char* name;
    char* hash;
    char* section;
} LoadPublicCartData;

static bool compareFilename(const char* name, const char* title, const char* hash, s32 id, void* data, bool dir)
{
    LoadPublicCartData* loadPublicCartData = data;
    Console* console = loadPublicCartData->console;

    if (strcmp(name, loadPublicCartData->name) == 0 && hash && strlen(hash))
    {
        loadPublicCartData->hash = strdup(hash);
        return false;
    }

    return true;
}

static void fileFound(void* data)
{
    LoadPublicCartData* loadPublicCartData = data;
    Console* console = loadPublicCartData->console;

    if (loadPublicCartData->hash)
        loadByHash(console, loadPublicCartData->name, loadPublicCartData->hash, loadPublicCartData->section, NULL, NULL);
    else
    {
        char msg[ULINAME_MAX];
        sprintf(msg, "\nerror: `%s` file not loaded", loadPublicCartData->name);
        printError(console, msg);
        commandDone(console);
    }

    FREE(loadPublicCartData->name);
    FREE(loadPublicCartData->hash);
    FREE(loadPublicCartData->section);
    FREE(loadPublicCartData);
}

static bool printUsage(Console* console, const char* command);

static void onLoadCommandConfirmed(Console* console)
{
    if(console->desc->count > 0)
    {
        uli_mem* uli = console->uli;

        const char* param = console->desc->params->key;
        const char* name = getCartName(param);
        const char* section = console->desc->count > 1 ? console->desc->params[1].key : NULL;

        if(section)
        {
            static const char* Sections[] =
            {
                "code",
#define         SECTION_DEF(name, ...) #name,
                ULI_SYNC_LIST(SECTION_DEF)
#undef          SECTION_DEF
            };

            bool found = false;
            for(const char** it = Sections, **end = it + COUNT_OF(Sections); it != end; ++it)
                if(strcmp(*it, section) == 0)
                {
                    found = true;
                    break;
                }

            if(!found)
            {
                printError(console, "\nunknown section: ");
                printError(console, section);
                printLine(console);
                printUsage(console, console->desc->command);
                commandDone(console);
                return;
            }
        }

        if (uli_fs_ispubdir(console->fs))
        {
            LoadPublicCartData loadPublicCartData = { console, strdup(name), NULL, section ? strdup(section) : NULL };
            uli_fs_enum(console->fs, compareFilename, fileFound, MOVE(loadPublicCartData));

            return;
        }
        else
        {
            s32 size = 0;
            void* data = strcmp(name, CONFIG_ULI_PATH) == 0
                ? uli_fs_loadroot(console->fs, name, &size)
                : uli_fs_load(console->fs, name, &size);

            if(data) SCOPE(free(data))
            {
                uli_cartridge* cart = newCart();

                SCOPE(free(cart))
                {
                    uli_cart_load(cart, data, size);
                    loadCartSection(console, cart, section);
                    onCartLoaded(console, name, section);
                }
            }
            else if(uli_tool_has_ext(param, PngExt) && uli_fs_exists(console->fs, param))
            {
                png_buffer buffer;
                buffer.data = uli_fs_load(console->fs, param, &buffer.size);

                SCOPE(free(buffer.data))
                {
                    uli_cartridge* cart = loadPngCart(buffer);

                    if(cart) SCOPE(free(cart))
                    {
                        loadCartSection(console, cart, section);
                        onCartLoaded(console, param, section);
                    }
                    else printError(console, "\npng cart loading error");
                }
            }
            else
            {
                const char* name = param;

#if defined(ULI78_PRO)
                if(project_ext(name))
                {
                    void* data = uli_fs_load(console->fs, name, &size);

                    if(data) SCOPE(free(data))
                    {
                        uli_cartridge* cart = newCart();

                        SCOPE(free(cart))
                        {
                            uli_project_load(name, data, size, cart);
                            loadCartSection(console, cart, section);
                            onCartLoaded(console, name, section);
                        }
                    }
                    else printError(console, "\nproject loading error");

                }
                else printError(console, "\nfile not found");
#else
                if(project_ext(name)) {
                    printError(console, "\nproject loading error");
                    printFront(console, "\nThis version only supports binary .png or .uli cartridges.");
                    printLine(console);
                    printFront(console, "\nULI-78 ");
                    consolePrint(console,"PRO",uli_color_light_blue);
                    printFront(console, " is needed for text files.");
                    printLine(console);
                    printFront(console, "\nLearn more:\n");
                    printLink(console, "https://uli78.com/pro");
                } else {
                    printError(console, "\ncart loading error");
                }

#endif
            }
        }
    }
    else
        printUsage(console, console->desc->command);

    commandDone(console);
}

typedef void(*ConsoleConfirmCallback)(Console* console);

typedef struct
{
    Console* console;
    ConsoleConfirmCallback callback;
} CommandConfirmData;

static void onConfirm(Studio* studio, bool yes, void* data)
{
    CommandConfirmData* confirmData = (CommandConfirmData*)data;

    if(yes)
    {
        confirmData->callback(confirmData->console);
    }
    else commandDone(confirmData->console);

    free(confirmData);
}

static void confirmCommand(Console* console, const char** text, s32 rows, ConsoleConfirmCallback callback)
{
    if(console->args.cli)
    {
        for(s32 i = 0; i < rows; i++)
        {
            printError(console, text[i]);
            printLine(console);
        }

        commandDone(console);
    }
    else
    {
        CommandConfirmData data = {console, callback};
        confirmDialog(console->studio, text, rows, onConfirm, MOVE(data));
    }
}

typedef void(*LoadDemoConfirmCallback)(Console* console, const uli_script* script);

typedef struct
{
    Console* console;
    LoadDemoConfirmCallback callback;
    const uli_script* script;
} LoadDemoConfirmData;

static void onLoadDemoConfirm(Studio* studio, bool yes, void* data)
{
    LoadDemoConfirmData* demoData = (LoadDemoConfirmData*)data;

    if(yes)
    {
        demoData->callback(demoData->console, demoData->script);
    }
    else commandDone(demoData->console);

    free(demoData);
}

static const char* LoadWarningRows[] =
{
    "WARNING!",
    "You have unsaved changes",
    "Do you really want to load cart?",
};

static void onLoadDemoCommand(Console* console, const uli_script* script)
{
    if(studioCartChanged(console->studio))
    {
        LoadDemoConfirmData data = {console, onLoadDemoCommandConfirmed, script};
        confirmDialog(console->studio, LoadWarningRows, COUNT_OF(LoadWarningRows), onLoadDemoConfirm, MOVE(data));
    }
    else
    {
        onLoadDemoCommandConfirmed(console, script);
    }
}

static void onLoadCommand(Console* console)
{
    if(studioCartChanged(console->studio))
    {
        confirmCommand(console, LoadWarningRows, COUNT_OF(LoadWarningRows), onLoadCommandConfirmed);
    }
    else
    {
        onLoadCommandConfirmed(console);
    }
}

static void loadDemo(Console* console, const uli_script* script)
{
    s32 size = 0;
    u8* data = getDemoCart(console, script, &size);

    if(data)
    {
        uli_cart_load(&console->uli->cart, data, size);
        uli_api_reset(console->uli);

        free(data);
    }

    memset(console->rom.name, 0, sizeof console->rom.name);

    studioRomLoaded(console->studio);
}

static void onNewCommandConfirmed(Console* console)
{
    bool done = false;

    s32 count = 0;
    FOREACH_LANG(_) count++;

    if(count == 0)
    {
        printError(console, "\nerror: not found any language.");
    }
    else if(count == 1)
    {
        loadDemo(console, uli_get_script(console->uli));
        done = true;
    }
    else if(console->desc->count)
    {
        const char* param = console->desc->params->key;

        FOREACH_LANG(ln)
        {
            if(strcmp(param, ln->name) == 0)
            {
                loadDemo(console, ln);
                done = true;
            }
        }

        if(!done)
        {
            printError(console, "\nunknown parameter: ");
            printError(console, param);
            commandDone(console);
            return;
        }
    }
    else
    {
        printError(console, "\nerror: choose a language for the new cart.");
        printUsage(console, console->desc->command);
    }

    if(done) printBack(console, "\nnew cart has been created");
    else printError(console, "\ncart not created");

    commandDone(console);
}

static void onNewCommand(Console* console)
{
    if(studioCartChanged(console->studio))
    {
        static const char* Rows[] =
        {
            "WARNING!",
            "You have unsaved changes",
            "Do you really want to create new cart?",
        };

        confirmCommand(console, Rows, COUNT_OF(Rows), onNewCommandConfirmed);
    }
    else
    {
        onNewCommandConfirmed(console);
    }
}

static void insertInputText(Console* console, const char* text)
{
    s32 size = strlen(text);
    s32 offset = getInputOffset(console);

    if(size < CONSOLE_BUFFER_SIZE - offset)
    {
        char* pos = console->text + offset;
        u8* color = console->color + offset;

        {
            s32 len = strlen(pos);
            memmove(pos + size, pos, len);
            memmove(color + size, color, len);
        }

        memcpy(pos, text, size);
        memset(color, CONSOLE_INPUT_COLOR, size);

        console->input.pos += size;
    }

    clearSelection(console);
}

typedef struct
{
    Console* console;
    char* incompleteWord; // Original word that's being completed.
    char* options; // Options to show to the user.
    char* commonPrefix; // Common prefix of all options.
} TabCompleteData;

static void addTabCompleteOption(TabCompleteData* data, const char* option)
{
    if (strstr(option, data->incompleteWord) == option)
    {
        // Possibly reduce the common prefix of all possible options.
        if (strlen(data->options) == 0)
        {
            // This is the first option to be added. Initialize the prefix.
            strncpy(data->commonPrefix, option, CONSOLE_BUFFER_SCREEN);
            // strncpy does not null-terminate if the source string is too long so
            // null terminate in the end just to be sure
            data->commonPrefix[CONSOLE_BUFFER_SCREEN - 1] = 0;
        }
        else
        {
            // Only leave the longest common prefix.
            char* tmpCommonPrefix = data->commonPrefix;
            char* tmpOption = (char*) option;

            while (*tmpCommonPrefix && *tmpOption && *tmpCommonPrefix == *tmpOption) {
                tmpCommonPrefix++;
                tmpOption++;
            }

            *tmpCommonPrefix = 0;
        }

        // The option matches the incomplete word, add it to the list.
        // the last parameter of strcat is the maximum number of characters to be
        // copied, and it adds a null terminator in the end. data->options having
        // capacity CONSOLE_BUFFER_SCREEN, we can append CONSOLE_BUFFER_SCREEN -
        // strlen(data->options) - 1 characters to its end at most
        strncat(data->options, option, CONSOLE_BUFFER_SCREEN - strlen(data->options) - 1);
        strncat(data->options, " ", CONSOLE_BUFFER_SCREEN - strlen(data->options) - 1);
    }
}

// Used to show tab-complete options, for example.
static void provideHint(Console* console, const char* hint)
{
    char* input = malloc(CONSOLE_BUFFER_SCREEN);
    strncpy(input, console->input.text, CONSOLE_BUFFER_SCREEN);
    // strncpy does not null-terminate if the source string is too long so
    // null terminate in the end just to be sure
    input[CONSOLE_BUFFER_SCREEN - 1] = 0;

    printLine(console);
    printBack(console, hint);
    commandDone(console);
    insertInputText(console, input);

    free(input);
}

static void finishTabComplete(const TabCompleteData* data)
{
    bool anyOptions = strlen(data->options) > 0;
    if (anyOptions) {
        // Adding one at the right because all options end with a space.
        bool justOneOptionLeft = strlen(data->options) == strlen(data->commonPrefix)+1;

        if (strlen(data->commonPrefix) == strlen(data->incompleteWord) && !justOneOptionLeft)
        {
            provideHint(data->console, data->options);
        }
        processConsoleEnd(data->console);
        insertInputText(data->console, data->commonPrefix+strlen(data->incompleteWord));

        if (justOneOptionLeft)
        {
            insertInputText(data->console, " ");
        }
    }

    free(data->options);
    free(data->commonPrefix);
}

static void tabCompleteLanguages(TabCompleteData* data)
{
    FOREACH_LANG(ln)
    {
        addTabCompleteOption(data, ln->name);
    }

    finishTabComplete(data);
}

static void tabCompleteExport(TabCompleteData* data)
{
#define EXPORT_CMD_DEF(name) addTabCompleteOption(data, #name);
    EXPORT_CMD_LIST(EXPORT_CMD_DEF)
#undef  EXPORT_CMD_DEF
    finishTabComplete(data);
}

static void tabCompleteImport(TabCompleteData* data)
{
#define IMPORT_CMD_DEF(name) addTabCompleteOption(data, #name);
    IMPORT_CMD_LIST(IMPORT_CMD_DEF)
#undef  IMPORT_CMD_DEF
    finishTabComplete(data);
}

static bool addFileAndDirToTabComplete(const char* name, const char* title, const char* hash, s32 id, void* data, bool dir)
{
    addTabCompleteOption(data, name);

    return true;
}

static bool addFilenameToTabComplete(const char* name, const char* title, const char* hash, s32 id, void* data, bool dir)
{
    if (!dir)
        addTabCompleteOption(data, name);

    return true;
}

static bool addDirToTabComplete(const char* name, const char* title, const char* hash, s32 id, void* data, bool dir)
{
    if (dir)
        addTabCompleteOption(data, name);

    return true;
}

static void finishTabCompleteAndFreeData(void* data) {
    finishTabComplete((const TabCompleteData *) data);
    free(data);
}

static void tabCompleteFiles(TabCompleteData* data)
{
    uli_fs_enum(data->console->fs, addFilenameToTabComplete, finishTabCompleteAndFreeData, MOVE(*data));
}

static void tabCompleteDirs(TabCompleteData* data)
{
    uli_fs_enum(data->console->fs, addDirToTabComplete, finishTabCompleteAndFreeData, MOVE(*data));
}

static void tabCompleteFilesAndDirs(TabCompleteData* data)
{
    uli_fs_enum(data->console->fs, addFileAndDirToTabComplete, finishTabCompleteAndFreeData, MOVE(*data));
}

static void tabCompleteConfig(TabCompleteData* data)
{
    addTabCompleteOption(data, "reset");
    addTabCompleteOption(data, "default");
    finishTabComplete(data);
}

typedef struct
{
    const char* name;
    bool dir;
} FileItem;

typedef struct
{
    Console* console;
    FileItem* items;
    s32 count;
} PrintFileNameData;

static bool printFilename(const char* name, const char* title, const char* hash, s32 id, void* ctx, bool dir)
{
    PrintFileNameData* data = ctx;

    data->items = realloc(data->items, (data->count + 1) * sizeof *data->items);
    data->items[data->count++] = (FileItem){strdup(name), dir};

    return true;
}

static s32 casecmp(const char *str1, const char *str2)
{
    while (*str1 && *str2)
    {
        if (tolower((u8) *str1) != tolower((u8) *str2))
            break;

        ++str1;
        ++str2;
    }

    return (s32) ((u8) tolower(*str1) - (u8) tolower(*str2));
}

static inline int itemcmp(const void* a, const void* b)
{
    const FileItem* item1 = a;
    const FileItem* item2 = b;

    if(item1->dir != item2->dir)
        return item1->dir ? -1 : 1;

    return casecmp(item1->name, item2->name);
}

static void onDirDone(void* ctx)
{
    PrintFileNameData* data = ctx;
    Console* console = data->console;

    qsort(data->items, data->count, sizeof *data->items, itemcmp);

    for(const FileItem *item = data->items, *end = item + data->count; item < end; item++)
    {
        printLine(console);

        if(item->dir)
        {
            printBack(console, "[");
            printBack(console, item->name);
            printBack(console, "]");
        }
        else printFront(console, item->name);

        free((void*)item->name);
    }

    if (data->count == 0)
    {
        printBack(console, "\n\nuse ");
        printFront(console, "DEMO");
        printBack(console, " command to install demo carts");
    }
    else free(data->items);

    printLine(console);
    commandDone(console);

    free(ctx);
}

typedef struct
{
    Console* console;
    char* name;
} ChangeDirData;

static void onChangeDirectoryDone(bool dir, void* data)
{
    ChangeDirData* changeDirData = data;
    Console* console = changeDirData->console;

    if (dir)
    {
        uli_fs_changedir(console->fs, changeDirData->name);
    }
    else printBack(console, "\ndir doesn't exist");

    free(changeDirData->name);
    free(changeDirData);

    commandDone(console);
}

static void onChangeDirectory(Console* console)
{
    if(console->desc->count)
    {
        const char* param = console->desc->params->key;

        if(strcmp(param, "/") == 0)
        {
            uli_fs_homedir(console->fs);
        }
        else if(strcmp(param, "..") == 0)
        {
            uli_fs_dirback(console->fs);
        }
        else
        {
            ChangeDirData data = { console, strdup(param) };
            uli_fs_isdir_async(console->fs, param, onChangeDirectoryDone, MOVE(data));
            return;
        }
    } else {
        uli_fs_homedir(console->fs);
    }

    commandDone(console);
}

static void onMakeDirectory(Console* console)
{
    if(console->desc->count)
    {
        char msg[ULINAME_MAX];
        const char* param = console->desc->params->key;

        if (uli_fs_exists(console->fs, param)) {
            sprintf(msg, "\nerror, [%s] already exists :(", param);
            printError(console, msg);
            commandDone(console);
            return;
        }

        sprintf(msg, "\ncreated [%s] folder :)", param);

        printBack(console, uli_fs_makedir(console->fs, param)
            ? "\nerror, dir not created :("
            : msg);

    }
    else printError(console, "\ninvalid dir name");

    commandDone(console);
}

static void onDirCommand(Console* console)
{
    printLine(console);

    PrintFileNameData data = {console};
    uli_fs_enum(console->fs, printFilename, onDirDone, MOVE(data));
}

static void onFolderCommand(Console* console)
{

    printBack(console, "\nStorage path:\n");
    printFront(console, uli_fs_pathroot(console->fs, ""));

    uli_fs_openfolder(console->fs);

    commandDone(console);
}

static void onClsCommand(Console* console)
{
    memset(console->text, 0, CONSOLE_BUFFER_SIZE);
    memset(console->color, ULI_COLOR_BG, CONSOLE_BUFFER_SIZE);

    ZEROMEM(console->scroll);
    ZEROMEM(console->cursor);
    ZEROMEM(console->input);

    printf("\r");

    commandDoneLine(console, false);
}

static void onInstallDemosCommand(Console* console)
{
    uli_fs* fs = console->fs;
    u8* data = (u8*)newCart();

    SCOPE(free(data))
    {
        printBack(console, "\nadded carts:\n\n");

        FOREACH_LANG(ln)
        {
            for(const struct uli_demo *demo = ln->demos; demo && demo->data; demo++)
            {
                uli_fs_save(fs, demo->name, data, uli_tool_unzip(data, sizeof(uli_cartridge), demo->data, demo->size), true);
                printFront(console, demo->name);
                printLine(console);
            }
        }

        static const char* Bunny = "bunny";

        uli_fs_makedir(fs, Bunny);
        uli_fs_changedir(fs, Bunny);

        FOREACH_LANG(ln)
        {
            // having a Mark is not mandatory
            if (ln->mark.data != NULL)
            {
                uli_fs_save(fs, ln->mark.name, data, uli_tool_unzip(data, sizeof(uli_cartridge), ln->mark.data, ln->mark.size), true);
                printFront(console, Bunny);
                printFront(console, "/");
                printFront(console, ln->mark.name);
                printLine(console);
            }
        }

        uli_fs_dirback(fs);
    }

    commandDone(console);
}

static void onGameMenuCommand(Console* console)
{
    gotoMenu(console->studio);
    commandDone(console);
}

static void onSurfCommand(Console* console)
{
    gotoSurf(console->studio);
}

static void loadExternal(Console* console, const char* path)
{
    CommandDesc desc =
    {
        .params = malloc(sizeof *desc.params),
        .count = 1,
    };

    *desc.params = (struct Param){.key = strdup(path)};
    *console->desc = desc;

    onLoadCommandConfirmed(console);
}

static void onConfigCommand(Console* console)
{
    if(console->desc->count)
    {
        if(strcmp(console->desc->params->key, "reset") == 0)
        {
            console->config->reset(console->config);
            printBack(console, "\nconfiguration reset :)");
        }
        else if(strcmp(console->desc->params->key, "default") == 0)
        {
            if (console->desc->count == 1)
            {
                onLoadDemoCommand(console, uli_get_script(console->uli));
            }
            else
            {
                FOREACH_LANG(script)
                {
                    if (strcmp(console->desc->params[1].key, script->name) == 0)
                        onLoadDemoCommand(console, script);
                }
            }
        }
        else
        {
            printError(console, "\nunknown parameter:\n");
            printError(console, console->desc->params->key);
        }
    }
    else
    {
        CommandDesc desc =
        {
            .params = malloc(sizeof *desc.params),
            .count = 1,
        };

        *desc.params = (struct Param){.key = strdup(CONFIG_ULI_PATH)};
        *console->desc = desc;

        onLoadCommand(console);

        return;
    }

    commandDone(console);
}

typedef struct
{
#define IMPORT_KEYS_DEF(key) s32 key;
    IMPORT_KEYS_LIST(IMPORT_KEYS_DEF)
#undef IMPORT_KEYS_DEF
} ImportParams;

static void onFileImported(Console* console, const char* filename, bool result)
{
    if(result)
    {
        printLine(console);
        printBack(console, filename);
        printBack(console, " imported :)");
    }
    else
    {
        char buf[ULINAME_MAX];
        sprintf(buf, "\nerror: %s not imported :(", filename);
        printError(console, buf);
    }

    commandDone(console);
}

static inline uli_bank* getBank(Console* console, s32 bank)
{
    return &console->uli->cart.banks[bank];
}

static inline const uli_palette* getPalette(Console* console, s32 bank, s32 vbank)
{
    return vbank
        ? &getBank(console, bank)->palette.vbank1
        : &getBank(console, bank)->palette.vbank0;
}

static void onImportTilesBase(Console* console, const char* name, const void* buffer, s32 size, uli_tile* base, ImportParams params)
{
    png_buffer png = {(u8*)buffer, size};
    bool error = true;
    s32 bpp = params.bpp ? params.bpp : 4;
    switch (bpp) {
        case 1:
        case 2:
        case 4:
            break;
        default:
            // not real!
            goto exit;
    }

    png_img img = png_read(png, NULL);

    if(img.data) SCOPE(free(img.data))
    {
        const uli_palette* pal = getPalette(console, params.bank, params.vbank);

        s32 bpp_scale = 1;
        switch (bpp) {
            case 1:
                bpp_scale = 4;
                break;
            case 2:
                bpp_scale = 2;
                break;
            default:
                break;
        }
        u32 color1, color2, color3, color4, color;

        for(s32 j = 0, y = params.y, h = y + (params.h ? params.h : img.height); y < h; ++y, ++j)
            for(s32 i = 0, x = params.x, w = x + ((params.w ? params.w : img.width) / bpp_scale); x < w; ++x, i += bpp_scale)
                if(x >= 0 && x < ULI_SPRITESHEET_SIZE && y >= 0 && y < ULI_SPRITESHEET_SIZE)
                    switch (bpp) {
                        case 4:
                            setSpritePixel(base, x, y, uli_nearest_color(pal->colors,
                               (uli_rgb*)(img.pixels + i + j * img.width), ULI_PALETTE_SIZE));
                            break;
                        case 2:
                            color1 = uli_nearest_color(pal->colors, (uli_rgb*)(img.pixels + i + j * img.width), 4);
                            color2 = uli_nearest_color(pal->colors, (uli_rgb*)(img.pixels + i + 1 + j * img.width), 4);
                            // adding them together caused issues with squashing??? no idea why this isn't the case
                            // for bpp 1
                            color = (color2 << 2) | color1;
                            setSpritePixel(base, x, y, color);
                            break;
                        case 1:
                            color1 = uli_nearest_color(pal->colors, (uli_rgb*)(img.pixels + i + j * img.width), 2);
                            color2 = uli_nearest_color(pal->colors, (uli_rgb*)(img.pixels + i + 1 + j * img.width), 2);
                            color3 = uli_nearest_color(pal->colors, (uli_rgb*)(img.pixels + i + 2 + j * img.width), 2);
                            color4 = uli_nearest_color(pal->colors, (uli_rgb*)(img.pixels + i + 3 + j * img.width), 2);
                            color = (color4 << 3) | (color3 << 2) | (color2 << 1) | color1;
                            setSpritePixel(base, x, y, color);
                            break;
                    }

        error = false;
    }

exit:
    onFileImported(console, name, !error);
}

static void onImport_tiles(Console* console, const char* name, const void* buffer, s32 size, ImportParams params)
{
    onImportTilesBase(console, name, buffer, size, getBank(console, params.bank)->tiles.data, params);
}

static void onImport_binary(Console* console, const char* name, const void* buffer, s32 size, ImportParams params)
{
    bool ok = name && buffer && size <= ULI_BINARY_SIZE;

    if(ok)
    {
        uli_binary* binary = &console->uli->cart.binary;
        binary->size = size;
        memcpy(binary->data, buffer, size);
    }

    onFileImported(console, name, ok);
}

static void onImport_sprites(Console* console, const char* name, const void* buffer, s32 size, ImportParams params)
{
    onImportTilesBase(console, name, buffer, size, getBank(console, params.bank)->sprites.data, params);
}

static void onImport_map(Console* console, const char* name, const void* buffer, s32 size, ImportParams params)
{
    bool ok = name && buffer && size <= sizeof(uli_map);

    if(ok)
    {
        enum {Size = sizeof(uli_map)};

        uli_map* map = &getBank(console, params.bank)->map;
        memset(map, 0, Size);
        memcpy(map, buffer, MIN(size, Size));
    }

    onFileImported(console, name, ok);
}

static void onImport_code(Console* console, const char* name, const void* buffer, s32 size, ImportParams params)
{
    uli_mem* uli = console->uli;
    bool error = false;

    if(name && buffer && size <= sizeof(uli_code))
    {
        enum {Size = sizeof(uli_code)};

        memset(uli->cart.code.data, 0, Size);
        memcpy(uli->cart.code.data, buffer, MIN(size, Size));

        studioRomLoaded(console->studio);
    }
    else error = true;

    onFileImported(console, name, !error);
}

static void onImport_screen(Console* console, const char* name, const void* buffer, s32 size, ImportParams params)
{
    png_buffer png = {(u8*)buffer, size};
    bool error = true;

    png_img img = png_read(png, NULL);

    if(img.data) SCOPE(free(img.data))
    {
        if(img.width == ULI78_WIDTH && img.height == ULI78_HEIGHT)
        {
            uli_bank* bank = getBank(console, params.bank);
            const uli_palette* pal = getPalette(console, params.bank, params.vbank);

            s32 i = 0;
            for(const png_rgba *pix = img.pixels, *end = pix + (ULI78_WIDTH * ULI78_HEIGHT); pix < end; pix++)
                uli_tool_poke4(bank->screen.data, i++, uli_nearest_color(pal->colors, (uli_rgb*)pix, ULI_PALETTE_SIZE));

            error = false;
        }
    }

    onFileImported(console, name, !error);
}

static bool findTile(uli_tile* tiles, const uli_tile* tile, s32* index)
{
    for (s32 i = 0; i < ULI_BANK_SPRITES; i++)
    {
        if (memcmp(&tiles[i], tile, sizeof(uli_tile)) == 0)
        {
            *index = i;
            return true;
        }
    }

    return false;
}

static s32 addTile(uli_tile* tiles, const uli_tile* tile)
{
    for (s32 i = 1; i < ULI_BANK_SPRITES; i++)
    {
        // Check if tile is empty (all pixels are color 0)
        bool empty = true;
        for(s32 p = 0; p < sizeof(uli_tile); p++)
        {
            if(((u8*)tiles)[i * sizeof(uli_tile) + p] != 0)
            {
                empty = false;
                break;
            }
        }

        if (empty)
        {
            memcpy(&tiles[i], tile, sizeof(uli_tile));
            return i;
        }
    }

    return -1; // No space left
}

static void onImport_mapimg(Console* console, const char* name, const void* buffer, s32 size, ImportParams params)
{
    png_buffer png = {(u8*)buffer, size};
    bool error = true;

    png_img img = png_read(png, NULL);

    if (img.data) SCOPE(free(img.data))
    {
        if (img.width == ULI_MAP_WIDTH * ULI_SPRITESIZE && img.height == ULI_MAP_HEIGHT * ULI_SPRITESIZE)
        {
            uli_bank* bank = getBank(console, params.bank);
            const uli_palette* pal = getPalette(console, params.bank, params.vbank);

            for (s32 my = 0; my < ULI_MAP_HEIGHT; my++)
            {
                for (s32 mx = 0; mx < ULI_MAP_WIDTH; mx++)
                {
                    uli_tile tile = {0};
                    for (s32 y = 0; y < ULI_SPRITESIZE; y++)
                        for (s32 x = 0; x < ULI_SPRITESIZE; x++)
                            setSpritePixel(&tile, x, y, uli_nearest_color(pal->colors, (uli_rgb*)(img.pixels + (mx * ULI_SPRITESIZE + x) + (my * ULI_SPRITESIZE + y) * img.width), ULI_PALETTE_SIZE));

                    s32 tile_index = 0;
                    if (!findTile(bank->tiles.data, &tile, &tile_index))
                    {
                        tile_index = addTile(bank->tiles.data, &tile);
                    }
                    
                    if(tile_index >= 0)
                        bank->map.data[mx + my * ULI_MAP_WIDTH] = (u8)tile_index;
                }
            }
            error = false;
        }
    }
    onFileImported(console, name, !error);
}

static void onImportCommand(Console* console)
{
    bool error = true;

    if(console->desc->count > 1)
    {
        ImportParams params = {0};

        for(const struct Param* it = console->desc->params, *end = it + console->desc->count; it < end; ++it)
        {
#define     IMPORT_KEYS_DEF(name) if(it->val && strcmp(it->key, #name) == 0) params.name = atoi(it->val);
            IMPORT_KEYS_LIST(IMPORT_KEYS_DEF)
#undef      IMPORT_KEYS_DEF
        }

        const char* filename = console->desc->params[1].key;
        s32 size = 0;
        const void* data = uli_fs_load(console->fs, filename, &size);

        if(data) SCOPE(free((void*)data))
        {
            static const struct Handler
            {
                const char* section;
                void (*handler)(Console*, const char*, const void*, s32, ImportParams);
            } Handlers[] =
            {
#define         IMPORT_CMD_DEF(name) {#name, onImport_##name},
                IMPORT_CMD_LIST(IMPORT_CMD_DEF)
#undef          IMPORT_CMD_DEF
            };

            const char* section = console->desc->params[0].key;
            FOR(const struct Handler*, ptr, Handlers)
                if(strcmp(section, ptr->section) == 0)
                {
                    ptr->handler(console, filename, data, size, params);
                    error = false;
                    break;
                }
        }
        else
        {
            char msg[ULINAME_MAX];
            sprintf(msg, "\nerror, %s file not loaded", filename);
            printError(console, msg);
            commandDone(console);
            return;
        }
    }

    if(error)
    {
        printError(console, "\nerror: invalid parameters.");
        printUsage(console, console->desc->command);

        commandDone(console);
    }
}

static void onFileExported(Console* console, const char* filename, bool result)
{
    if(result)
    {
        printLine(console);
        printBack(console, filename);
        printBack(console, " exported :)");
    }
    else
    {
        char buf[ULINAME_MAX];
        sprintf(buf, "\nerror: %s not exported :(", filename);
        printError(console, buf);
    }

    commandDone(console);
}

typedef struct
{
#define EXPORT_KEYS_DEF(key) s32 key;
    EXPORT_KEYS_LIST(EXPORT_KEYS_DEF)
#undef EXPORT_KEYS_DEF
} ExportParams;

static void exportSprites(Console* console, const char* filename, uli_tile* base, ExportParams params)
{
    uli_mem* uli = console->uli;
    const uli_cartridge* cart = &uli->cart;

    png_img img = {ULI_SPRITESHEET_SIZE, ULI_SPRITESHEET_SIZE, malloc(ULI_SPRITESHEET_SIZE * ULI_SPRITESHEET_SIZE * sizeof(png_rgba))};

    SCOPE(free(img.data))
    {
        const uli_palette* pal = getPalette(console, params.bank, params.vbank);

        for(s32 i = 0; i < ULI_SPRITESHEET_SIZE * ULI_SPRITESHEET_SIZE; i++)
            img.values[i] = uli_rgba(&pal->colors[getSpritePixel(base, i % ULI_SPRITESHEET_SIZE, i / ULI_SPRITESHEET_SIZE)]);

        png_buffer png = png_write(img, (png_buffer){NULL, 0});

        SCOPE(free(png.data))
        {
            onFileExported(console, filename, uli_fs_save(console->fs, filename, png.data, png.size, true));
        }
    }
}

static void* embedCart(Console* console, u8* app, s32* size)
{
    uli_mem* uli = console->uli;
    u8* data = NULL;
    void* cart = newCart();

    SCOPE(free(cart))
    {
        s32 cartSize = uli_cart_save(&uli->cart, cart);

        s32 zipSize = sizeof(uli_cartridge);
        u8* zipData = (u8*)malloc(zipSize);

        SCOPE(free(zipData))
        {
            if((zipSize = uli_tool_zip(zipData, zipSize, cart, cartSize)))
            {
                s32 appSize = *size;

                EmbedHeader header =
                {
                    .appSize = appSize,
                    .cartSize = zipSize,
                };

                memcpy(header.sig, CART_SIG, STRLEN(CART_SIG));

                s32 finalSize = appSize + sizeof header + header.cartSize;
                data = malloc(finalSize);

                if (data)
                {
                    memcpy(data, app, appSize);
                    memcpy(data + appSize, &header, sizeof header);
                    memcpy(data + appSize + sizeof header, zipData, header.cartSize);

                    *size = finalSize;
                }
            }
        }
    }

    return data;
}

static void* embedCartLocal(Console* console, u8* app, s32* size)
{
    uli_mem* uli = console->uli;
    u8* data = NULL;
    void* cart = newCart();

    SCOPE(free(cart))
    {
        s32 cartSize = uli_cart_save(&uli->cart, cart);

        s32 zipSize = sizeof(uli_cartridge);
        u8* zipData = (u8*)malloc(zipSize);

        SCOPE(free(zipData))
        {
            if((zipSize = uli_tool_zip(zipData, zipSize, cart, cartSize)))
            {
                s32 appSize = *size;

                EmbedHeader header =
                {
                    .appSize = appSize,
                    .cartSize = zipSize,
                };

                memcpy(header.sig, CART_SIG, STRLEN(CART_SIG));

                s32 finalSize = appSize + header.cartSize + sizeof header;
                data = malloc(finalSize);

                if (data)
                {
                    memcpy(data, app, appSize);
                    memcpy(data + appSize, zipData, header.cartSize);
                    memcpy(data + appSize + header.cartSize, &header, sizeof header);

                    *size = finalSize;
                }
            }
        }
    }

    return data;
}

typedef struct
{
    Console* console;
    char filename[ULINAME_MAX];
} GameExportData;

static void onExportGet(const net_get_data* data)
{
    GameExportData* exportData = (GameExportData*)data->calldata;
    Console* console = exportData->console;

    switch(data->type)
    {
    case net_get_progress:
        {
            console->cursor.pos.x = 0;
            printf("\r");
            printBack(console, "GET ");
            printFront(console, data->url);

            char buf[8];
            sprintf(buf, " [%i%%]", data->progress.size * 100 / data->progress.total);
            printBack(console, buf);
        }
        break;
    case net_get_error:
        printError(console, "file downloading error :(");
        commandDone(console);
        free(exportData);
        break;
    default:
        break;
    }
}

static void onNativeExportGet(const net_get_data* data)
{
    switch(data->type)
    {
    case net_get_done:
        {
            GameExportData* exportData = (GameExportData*)data->calldata;
            Console* console = exportData->console;

            uli_mem* uli = console->uli;

            char filename[ULINAME_MAX];
            strcpy(filename, exportData->filename);
            free(exportData);

            s32 size = data->done.size;

            printLine(console);

            const char* path = uli_fs_path(console->fs, filename);
            void* buf = NULL;

            onFileExported(console, filename, (buf = embedCart(console, data->done.data, &size)) && fs_write(path, buf, size));
            chmod(path, DEFAULT_CHMOD);

            if (buf)
                free(buf);
        }
        break;
    default:
        onExportGet(data);
    }
}

static void exportGame(Console* console, const char* name, const char* system, net_get_callback callback, ExportParams params)
{
    uli_mem* uli = console->uli;
    printLine(console);
    GameExportData data = {console};
    strcpy(data.filename, name);

    char url[ULINAME_MAX] = "/export/" DEF2STR(ULI_VERSION_MAJOR) "." DEF2STR(ULI_VERSION_MINOR) ULI_VERSION_STATUS "/";
    strcat(url, system);

#if defined(ULI78_PRO)
    if (params.alone)
        strcat(url, uli_get_script(console->uli)->name);
#endif

    uli_net_get(console->net, url, callback, MOVE(data));
}

static inline void exportNativeGame(Console* console, const char* name, const char* system, ExportParams params)
{
    exportGame(console, name, system, onNativeExportGet, params);
}

static void exportLocalNativeGame(Console* console, const char* name, const char* system, ExportParams params)
{
    char playerPath[ULINAME_MAX];
    strcpy(playerPath, fs_appfolder());
    strcat(playerPath, "localplayer.exe.dat");

    s32 playerSize = 0;
    u8* playerBuf = fs_read(playerPath, &playerSize);

    if(playerBuf) SCOPE(free(playerBuf))
    {
        if(playerSize > 0)
        {
            const char* path = uli_fs_path(console->fs, name);
            void* finalExe = NULL;

            onFileExported(console, name, (finalExe = embedCartLocal(console, playerBuf, &playerSize)) && fs_write(path, finalExe, playerSize));
            chmod(path, DEFAULT_CHMOD);

            if (finalExe)
                free(finalExe);
        }
        else
            printError(console, "failed to load player template");
    }
    else
    {
        printError(console, "\nerror: player template 'player.exe.dat' not found");
    }

    commandDone(console);
}

static void onHtmlExportGet(const net_get_data* data)
{
    switch(data->type)
    {
    case net_get_done:
        {
            GameExportData* exportData = (GameExportData*)data->calldata;
            Console* console = exportData->console;

            uli_mem* uli = console->uli;

            char filename[ULINAME_MAX];
            strcpy(filename, exportData->filename);
            free(exportData);

            const char* zipPath = uli_fs_path(console->fs, filename);
            bool errorOccurred = !fs_write(zipPath, data->done.data, data->done.size);

            if(!errorOccurred)
            {
                struct zip_t *zip = zip_open(zipPath, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');

                if(zip) SCOPE(zip_close(zip))
                {
                    void* cart = newCart();

                    SCOPE(free(cart))
                    {
                        s32 cartSize = uli_cart_save(&uli->cart, cart);

                        if(cartSize)
                        {
                            zip_entry_open(zip, "cart.uli");
                            zip_entry_write(zip, cart, cartSize);
                            zip_entry_close(zip);
                        }
                        else errorOccurred = true;
                    }
                }
                else errorOccurred = true;
            }

            onFileExported(console, filename, !errorOccurred);
        }
        break;
    default:
        onExportGet(data);
    }
}

static const char* getFilename(const char* filename, const char* ext)
{
    if(strcmp(filename + strlen(filename) - strlen(ext), ext) == 0)
        return filename;

    static char Name[ULINAME_MAX];
    strcpy(Name, filename);
    strcat(Name, ext);

    return Name;
}

static void onExport_win(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportNativeGame(console, getFilename(filename, ".exe"), param, params);
}

static void onLocalExport_win(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportLocalNativeGame(console, getFilename(filename, ".exe"), param, params);
}

static void onExport_winxp(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportNativeGame(console, getFilename(filename, ".exe"), param, params);
}

static void onExport_linux(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportNativeGame(console, filename, param, params);
}

static void onExport_rpi(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportNativeGame(console, filename, param, params);
}

static void onExport_mac(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportNativeGame(console, filename, param, params);
}

static void onExport_html(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportGame(console, getFilename(filename, ".zip"), param, onHtmlExportGet, params);
}

static void onExport_tiles(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportSprites(console, getFilename(filename, PngExt), getBank(console, params.bank)->tiles.data, params);
}

static void onExport_binary(Console* console, const char* param, const char* path, ExportParams params)
{
    const char* filename = getFilename(path, ".binary");

    uli_binary *binary = &console->uli->cart.binary;
    // TODO: do we need this buffer at all, could we just handle `binary.data` directly to `uli_fs_save`?
    void* buffer = malloc(binary->size);

    SCOPE(free(buffer))
    {
        memcpy(buffer, binary->data, binary->size);

        onFileExported(console, filename, uli_fs_save(console->fs, filename, buffer, binary->size, true));
    }
}

static void onExport_sprites(Console* console, const char* param, const char* filename, ExportParams params)
{
    exportSprites(console, getFilename(filename, PngExt), getBank(console, params.bank)->sprites.data, params);
}

static void onExport_map(Console* console, const char* param, const char* path, ExportParams params)
{
    enum{Size = sizeof(uli_map)};
    const char* filename = getFilename(path, ".map");

    void* buffer = malloc(Size);

    SCOPE(free(buffer))
    {
        uli_map* map = &getBank(console, params.bank)->map;
        memcpy(buffer, map->data, Size);

        onFileExported(console, filename, uli_fs_save(console->fs, filename, buffer, Size, true));
    }
}

static void onExport_mapimg(Console* console, const char* param, const char* path, ExportParams params)
{
    const char* filename = getFilename(path, ".png");

    enum{Width = ULI_MAP_WIDTH * ULI_SPRITESIZE, Height = ULI_MAP_HEIGHT * ULI_SPRITESIZE};

    png_img img = {Width, Height, malloc(Width * Height * sizeof(png_rgba))};

    SCOPE(free(img.data))
    {
        {
            uli_mem* uli = console->uli;

            uli_api_sync(uli, -1, params.bank, false);

            for(s32 r = 0; r < ULI_MAP_ROWS; r++)
                for(s32 c = 0; c < ULI_MAP_COLS; c++)
                {
                    uli_api_map(uli, c * ULI_MAP_SCREEN_WIDTH, r * ULI_MAP_SCREEN_HEIGHT,
                        ULI_MAP_SCREEN_WIDTH, ULI_MAP_SCREEN_HEIGHT, 0, 0, NULL, 0, 1, NULL, NULL);

                    uli_core_blit(uli);

                    for(s32 j = 0; j < ULI78_HEIGHT; j++)
                        for(s32 i = 0; i < ULI78_WIDTH; i++)
                            img.values[(i + c * ULI78_WIDTH) + (j + r * ULI78_HEIGHT) * Width] =
                                uli->product.screen[(i + ULI78_MARGIN_LEFT) + (j + ULI78_MARGIN_TOP) * ULI78_FULLWIDTH];
                }
        }

        png_buffer png = png_write(img, (png_buffer){NULL, 0});

        SCOPE(free(png.data))
        {
            onFileExported(console, filename, uli_fs_save(console->fs, filename, png.data, png.size, true));
        }
    }
}

static void onExport_sfx(Console* console, const char* param, const char* name, ExportParams params)
{
    const char* filename = getFilename(name, ".wav");
    bool error = true;

    if(params.id >= 0 && params.id < SFX_COUNT)
        error = studioExportSfx(console->studio, params.id, filename) == NULL;

    onFileExported(console, filename, !error);
}

static void onExport_music(Console* console, const char* type, const char* name, ExportParams params)
{
    const char* filename = getFilename(name, ".wav");
    bool error = true;

    if(params.id >= 0 && params.id < MUSIC_TRACKS)
        error = studioExportMusic(console->studio, params.id, params.bank, filename) == NULL;

    onFileExported(console, filename, !error);
}

static void onExport_screen(Console* console, const char* param, const char* name, ExportParams params)
{
    const char* filename = getFilename(name, ".png");

    uli_mem* uli = console->uli;
    const uli_cartridge* cart = &uli->cart;

    png_img img = {ULI78_WIDTH, ULI78_HEIGHT, malloc(ULI78_WIDTH * ULI78_HEIGHT * sizeof(png_rgba))};

    SCOPE(free(img.data))
    {
        const uli_palette* pal = getPalette(console, params.bank, params.vbank);

        uli_bank* bank = getBank(console, params.bank);
        for(s32 i = 0; i < ULI78_WIDTH * ULI78_HEIGHT; i++)
            img.values[i] = uli_rgba(&pal->colors[uli_tool_peek4(bank->screen.data, i)]);

        png_buffer png = png_write(img, (png_buffer){NULL, 0});

        SCOPE(free(png.data))
        {
            onFileExported(console, filename, uli_fs_save(console->fs, filename, png.data, png.size, true));
        }
    }
}

static void onExport_help(Console* console, const char* param, const char* name, ExportParams params);

static void onExportCommand(Console* console)
{
    if(console->desc->count > 1)
    {
        ExportParams params = {0};

        for(const struct Param* it = console->desc->params, *end = it + console->desc->count; it < end; ++it)
        {
#define     EXPORT_KEYS_DEF(name) if(it->val && strcmp(it->key, #name) == 0) params.name = atoi(it->val);
            EXPORT_KEYS_LIST(EXPORT_KEYS_DEF)
#undef      EXPORT_KEYS_DEF
        }

        const char* filename = console->desc->params[1].key;

        static const struct Handler
        {
            const char* type;
            void(*handler)(Console*, const char*, const char*, ExportParams);
        } Handlers[] =
        {
#define     EXPORT_CMD_DEF(name) {#name, onExport_##name},
            EXPORT_CMD_LIST(EXPORT_CMD_DEF)
#undef      EXPORT_CMD_DEF
        };

        const char* type = console->desc->params[0].key;

        FOR(const struct Handler*, ptr, Handlers)
            if(strcmp(type, ptr->type) == 0)
            {
                ptr->handler(console, type, filename, params);
                return;
            }
    }

    {
        printError(console, "\nerror: invalid parameters.");
        printUsage(console, console->desc->command);
        commandDone(console);
    }
}

static void drawShadowText(uli_mem* uli, const char* text, s32 x, s32 y, uli_color color, s32 scale)
{
    uli_api_print(uli, text, x, y + scale, uli_color_black, false, scale, false);
    uli_api_print(uli, text, x, y, color, false, scale, false);
}

const char* readMetatag(const char* code, const char* tag, const char* comment);

static CartSaveResult saveCartName(Console* console, const char* name)
{
    uli_mem* uli = console->uli;

    bool success = false;

    if(name && strlen(name))
    {
        u8* buffer = (u8*)malloc(sizeof(uli_cartridge) * 3);

        if(buffer)
        {
            if(strcmp(name, CONFIG_ULI_PATH) == 0)
            {
                console->config->save(console->config);
                studioRomSaved(console->studio);
                free(buffer);
                return CART_SAVE_OK;
            }
            else
            {
                s32 size = 0;

                if(uli_tool_has_ext(name, PngExt))
                {
                    png_buffer cover;

                    {
                        enum{CoverWidth = 256};

                        static const u8 Cartridge[] =
                        {
                            #include "../build/assets/cart.png.dat"
                            //#include "cart.png.dat"
                        };

                        png_buffer template = {(u8*)Cartridge, sizeof Cartridge};
                        png_img img = png_read(template, NULL);

                        // draw screen
                        {
                            enum{PaddingLeft = 8, PaddingTop = 8};

                            const uli_bank* bank = &uli->cart.bank0;
                            const uli_rgb* pal = bank->palette.vbank0.colors;
                            const u8* screen = bank->screen.data;
                            u32* ptr = img.values + PaddingTop * CoverWidth + PaddingLeft;

                            for(s32 i = 0; i < ULI78_WIDTH * ULI78_HEIGHT; i++)
                                ptr[i / ULI78_WIDTH * CoverWidth + i % ULI78_WIDTH] = uli_rgba(pal + uli_tool_peek4(screen, i));
                        }

                        // draw title/author/desc
                        {
                            enum{Width = 224, Height = 40, PaddingTop = 162, PaddingLeft = 16, Scale = 2, Row = ULI_FONT_HEIGHT * 2 * Scale};

                            uli_api_cls(uli, uli_color_dark_grey);

                            const char* comment = uli_get_script(uli)->singleComment;

                            const char* title = uli_tool_metatag(uli->cart.code.data, "title", comment);
                            if(*title)
                            {
                                drawShadowText(uli, title, 0, 0, uli_color_white, Scale);
                            }

                            const char* author = uli_tool_metatag(uli->cart.code.data, "author", comment);
                            if(*author)
                            {
                                char buf[ULINAME_MAX];
                                snprintf(buf, sizeof buf, "by %s", author);
                                drawShadowText(uli, buf, 0, Row, uli_color_grey, Scale);
                            }

                            u32* ptr = img.values + PaddingTop * CoverWidth + PaddingLeft;
                            const u8* screen = uli->ram->vram.screen.data;
							const uli_rgb Sweetie16[] = {
								{0x1a, 0x1c, 0x2c}, {0x5d, 0x27, 0x5d}, {0xb1, 0x3e, 0x53}, {0xef, 0x7d, 0x57},
								{0xff, 0xcd, 0x75}, {0xa7, 0xf0, 0x70}, {0x38, 0xb7, 0x64}, {0x25, 0x71, 0x79},
								{0x29, 0x36, 0x6f}, {0x3b, 0x5d, 0xc9}, {0x41, 0xa6, 0xf6}, {0x73, 0xef, 0xf7},
								{0xf4, 0xf4, 0xf4}, {0x94, 0xb0, 0xc2}, {0x56, 0x6c, 0x86}, {0x33, 0x3c, 0x57}
							};
							const uli_rgb* pal = Sweetie16;

                            for(s32 y = 0; y < Height; y++)
                                for(s32 x = 0; x < Width; x++)
                                    ptr[CoverWidth * y + x] = uli_rgba(pal + uli_tool_peek4(screen, y * ULI78_WIDTH + x));
                        }

                        cover = png_write(img, (png_buffer){NULL, 0});

                        free(img.data);
                    }

                    png_buffer zip = png_create(sizeof(uli_cartridge));

                    {
                        png_buffer cart = png_create(sizeof(uli_cartridge));
                        cart.size = uli_cart_save(&uli->cart, cart.data);
                        zip.size = uli_tool_zip(zip.data, zip.size, cart.data, cart.size);
                        free(cart.data);
                    }

                    png_buffer result = png_encode(cover, zip);
                    free(zip.data);
                    free(cover.data);

                    buffer = result.data;
                    size = result.size;
                }
#if defined(ULI78_PRO)
                else if(project_ext(name))
                {
                    size = uli_project_save(name, buffer, &uli->cart);
                }
#endif
                else
                {
                    name = getCartName(name);
                    size = uli_cart_save(&uli->cart, buffer);
                }

                if(size && uli_fs_save(console->fs, name, buffer, size, true))
                {
                    setCartName(console, name, uli_fs_path(console->fs, name));
                    success = true;
                    studioRomSaved(console->studio);
                }
            }

            free(buffer);
        }
    }
    else if (strlen(console->rom.name))
    {
        return saveCartName(console, console->rom.name);
    }
    else return CART_SAVE_MISSING_NAME;

    return success ? CART_SAVE_OK : CART_SAVE_ERROR;
}

static CartSaveResult saveCart(Console* console)
{
    return saveCartName(console, NULL);
}

static void onSaveCommandConfirmed(Console* console)
{
    CartSaveResult rom = saveCartName(console, console->desc->count ? console->desc->params->key : NULL);

    if(rom == CART_SAVE_OK)
    {
        printBack(console, "\ncart ");
        printFront(console, console->rom.name);
        printBack(console, " saved!\n");
    }
    else if(rom == CART_SAVE_MISSING_NAME)
        printBack(console, "\ncart name is missing\n");
    else
        printBack(console, "\ncart saving error");

    commandDone(console);
}

static void onSaveCommand(Console* console)
{
    const char* param = console->desc->count ? console->desc->params->key : NULL;

    if(param && strlen(param) &&
        (uli_fs_exists(console->fs, param) ||
            uli_fs_exists(console->fs, getCartName(param))))
    {
        static const char* Rows[] =
        {
            "WARNING!",
            "The cart already exists",
            "Do you want to overwrite it?",
        };

        confirmCommand(console, Rows, COUNT_OF(Rows), onSaveCommandConfirmed);
    }
    else
    {
        onSaveCommandConfirmed(console);
    }
}

static void onRunCommand(Console* console)
{
    commandDone(console);

    runGame(console->studio);
}

static void onResumeCommand(Console* console)
{
    if(console->desc->count)
    {
        const char* param = console->desc->params->key;

        if(strcmp(param, "reload") == 0)
        {
            const uli_script* script_config = uli_get_script(console->uli);
            if (script_config->eval)
            {
                script_config->eval(console->uli, console->uli->cart.code.data);
            }
            else
            {
                printError(console, "eval not implemented for the script");
            }
        }
    }
    commandDone(console);

    resumeGame(console->studio);
}

static void onEvalCommand(Console* console)
{
    printLine(console);

    const uli_script* script_config = uli_get_script(console->uli);

    if (script_config->eval)
    {
        if(console->desc->count)
            script_config->eval(console->uli,
                                console->desc->src+strlen(console->desc->command));
        else printError(console, "nothing to eval");
    }
    else
    {
        printError(console, "'eval' not implemented for the script");
    }

    commandDone(console);
}

static void onDelCommandConfirmed(Console* console)
{
    if(console->desc->count)
    {
        if (uli_fs_ispubdir(console->fs))
        {
            printError(console, "\naccess denied");
        }
        else
        {
            const char* param = console->desc->params->key;
            if(uli_fs_isdir(console->fs, param))
            {
                printBack(console, uli_fs_deldir(console->fs, param)
                    ? "\ndir not deleted"
                    : "\ndir successfully deleted");
            }
            else
            {
                printBack(console, uli_fs_delfile(console->fs, param)
                    ? "\nfile not deleted"
                    : "\nfile successfully deleted");
            }
        }
    }
    else printBack(console, "\nname is missing");

    commandDone(console);
}

static void onDelCommand(Console* console)
{
    static const char* Rows[] =
    {
        "WARNING!",
        "Do you really want to delete file?",
    };

    confirmCommand(console, Rows, COUNT_OF(Rows), onDelCommandConfirmed);
}

#if defined(CAN_ADDGET_FILE)

static void onAddFile(Console* console, const char* name, const u8* buffer, s32 size)
{
    if(name)
    {
        const char* path = uli_fs_path(console->fs, name);

        if(!fs_exists(path))
        {
            if(fs_write(path, buffer, size))
            {
                printLine(console);
                printFront(console, name);
                printBack(console, " successfully added :)");
            }
            else printError(console, "\nerror: file not added :(");
        }
        else
        {
            printError(console, "\nerror: ");
            printError(console, name);
            printError(console, " already exists :(");
        }
    }

    commandDone(console);
}

static void onAddCommand(Console* console)
{
    void* data = NULL;

    EM_ASM_
    ({
        Module.showAddPopup(function(filename, rom)
        {
            if(filename == null || rom == null)
            {
                dynCall('viiii', $0, [$1, 0, 0, 0]);
            }
            else
            {
                var filePtr = _malloc(filename.length + 1);
                stringToUTF8(filename, filePtr, filename.length + 1);

                var dataPtr = _malloc(rom.length);
                writeArrayToMemory(rom, dataPtr);

                dynCall('viiii', $0, [$1, filePtr, dataPtr, rom.length]);

                _free(filePtr);
                _free(dataPtr);
            }
        });
    }, onAddFile, console);
}

static void onGetCommand(Console* console)
{
    if(console->desc->count)
    {
        const char* name = console->desc->params->key;
        const char* path = uli_fs_path(console->fs, name);

        if(fs_exists(path))
        {
            s32 size = 0;
            void* buffer = fs_read(path, &size);

            EM_ASM_
            ({
                var name = UTF8ToString($0);
                var blob = new Blob([HEAPU8.subarray($1, $1 + $2)], {type: "application/octet-stream"});

                Module.saveAs(blob, name);
            }, name, buffer, size);
        }
        else
        {
            printError(console, "\nerror: ");
            printError(console, name);
            printError(console, " doesn't exist :(");
        }
    }
    else printBack(console, "\nusage: get <file>");

    commandDone(console);
}

#endif

// Declare this here to resolve a cyclic dependency with COMMANDS_LIST.
static void tabCompleteHelp(TabCompleteData* data);

static const char HelpUsage[] = "help [<text>"
#define HELP_CMD_DEF(name) "|" #name
    HELP_CMD_LIST(HELP_CMD_DEF)
#undef  HELP_CMD_DEF
    "]";

#define SECTION_DEF(NAME, ...)  "|" #NAME
#define EXPORT_CMD_DEF(name)    #name "|"
#define EXPORT_KEYS_DEF(name)   #name "=0 "
#define IMPORT_CMD_DEF(name)    #name "|"
#define IMPORT_KEYS_DEF(key)    #key"=0 "

#if defined(CAN_ADDGET_FILE)
#define ADDGET_FILE(macro)                                                              \
    macro("add",                                                                        \
        NULL,                                                                           \
        "Upload file to the browser local storage.",                                    \
        NULL,                                                                           \
        onAddCommand,                                                                   \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("get",                                                                        \
        NULL,                                                                           \
        "Download file from the browser local storage.",                                \
        "get <file>",                                                                   \
        onGetCommand,                                                                   \
        tabCompleteFiles,                                                               \
        NULL)                                                                           \

#else
#define ADDGET_FILE(macro)
#endif


#define LOCALEXPORT_CMD_LIST(macro) \
    macro(win)

#define LOCALEXPORT_KEYS_LIST(macro) \
    macro(bank)                 \
    macro(vbank)                \
    macro(id)                   \
    ALONE_KEY(macro)

static void onLocalExportCommand(Console* console)
{
    if(console->desc->count > 1)
    {
        ExportParams params = {0};

        for(const struct Param* it = console->desc->params, *end = it + console->desc->count; it < end; ++it)
        {
#define     LOCALEXPORT_KEYS_DEF(name) if(it->val && strcmp(it->key, #name) == 0) params.name = atoi(it->val);
            LOCALEXPORT_KEYS_LIST(LOCALEXPORT_KEYS_DEF)
#undef      LOCALEXPORT_KEYS_DEF
        }

        const char* filename = console->desc->params[1].key;
        const char* type = console->desc->params[0].key;

        if(strcmp(type, "win") == 0)
        {
            onLocalExport_win(console, type, filename, params);
            return;
        }
    }

    printError(console, "\nerror: invalid parameters.");
    printUsage(console, console->desc->command);
    commandDone(console);
}

// macro(name, alt, help, usage, handler, tab-complete for first param, for second param)
#define COMMANDS_LIST(macro)                                                            \
    macro("help",                                                                       \
        NULL,                                                                           \
        "Show help info about commands/api/...",                                        \
        HelpUsage,                                                                      \
        onHelpCommand,                                                                  \
        tabCompleteHelp,                                                                \
        NULL)                                                                           \
                                                                                        \
    macro("exit",                                                                       \
        "quit",                                                                         \
        "Exit the application (Hotkey: CTRL+Q).",                                       \
        NULL,                                                                           \
        onExitCommand,                                                                  \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("edit",                                                                       \
        NULL,                                                                           \
        "Open cart editors (Hotkey: ESC or F1).",                                       \
        NULL,                                                                           \
        onEditCommand,                                                                  \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("new",                                                                        \
        NULL,                                                                           \
        "Creates a new `Hello World` cartridge.",                                       \
        "new <$LANG_NAMES_PIPE$>",                                                      \
        onNewCommand,                                                                   \
        tabCompleteLanguages,                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("load",                                                                       \
        NULL,                                                                           \
        "Load cartridge from the local filesystem"                                      \
        "(there's no need to type the .uli extension).\n"                               \
        "You can also load just the section (sprites, map, screen etc)"                 \
        "from another cart.",                                                           \
        "load <cart> [code" ULI_SYNC_LIST(SECTION_DEF) "]",                             \
        onLoadCommand,                                                                  \
        tabCompleteFiles,                                                               \
        NULL)                                                                           \
                                                                                        \
    macro("save",                                                                       \
        NULL,                                                                           \
        "Save cartridge to the local filesystem (Hotkey: CTRL+S), use $LANG_EXTENSIONS$"\
        "cart extension to save it in text format (PRO feature).\n"                     \
        "Use .png file extension to save it as a png cart.",                            \
        "save <cart>",                                                                  \
        onSaveCommand,                                                                  \
        tabCompleteFiles,                                                               \
        NULL)                                                                           \
                                                                                        \
    macro("run",                                                                        \
        NULL,                                                                           \
        "Run current cart / project (Hotkey: CTRL+R).",                                 \
        NULL,                                                                           \
        onRunCommand,                                                                   \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("resume",                                                                     \
        NULL,                                                                           \
        "Resume last run cart / project. Reload game code\n"                            \
        "first if given reload as an argument.",                                        \
        "resume [reload]",                                                              \
        onResumeCommand,                                                                \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("eval",                                                                       \
        "=",                                                                            \
        "Run provided code within the console, "                                        \
        "useful for debugging and testing.\n"                                           \
        "\nTips\n"                                                                      \
        "- Use trace() to log the results. Eg: eval trace(2+2)\n"                       \
        "- The virtual machine should be launched first by "                            \
        "running a cart; otherwise it will output an empty string.",                    \
        NULL,                                                                           \
        onEvalCommand,                                                                  \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("dir",                                                                        \
        "ls",                                                                           \
        "Show list of local files.",                                                    \
        NULL,                                                                           \
        onDirCommand,                                                                   \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("cd",                                                                         \
        NULL,                                                                           \
        "Change directory.",                                                            \
        "\ncd <path>\ncd /\ncd ..",                                                     \
        onChangeDirectory,                                                              \
        tabCompleteDirs,                                                                \
        NULL)                                                                           \
                                                                                        \
    macro("mkdir",                                                                      \
        NULL,                                                                           \
        "Make a directory.",                                                            \
        "mkdir <name>",                                                                 \
        onMakeDirectory,                                                                \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("folder",                                                                     \
        NULL,                                                                           \
        "Open working directory in OS.",                                                \
        NULL,                                                                           \
        onFolderCommand,                                                                \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("export",                                                                     \
        NULL,                                                                           \
        "Export cart to HTML,\n"                                                        \
        "native build (win linux rpi mac),\n"                                           \
        "export sprites/map/... as a .png image "                                       \
        "or export sfx and music to .wav files.",                                       \
        "\nexport [" EXPORT_CMD_LIST(EXPORT_CMD_DEF) "] "                            \
        "<file> [" EXPORT_KEYS_LIST(EXPORT_KEYS_DEF) "]" ,                           \
        onExportCommand,                                                                \
        tabCompleteExport,                                                              \
        tabCompleteFiles)                                                               \
                                                                                        \
    macro("localexport",                                                                \
        "lexport",                                                                      \
        "Export cart to a native Windows executable completely offline.",               \
        "\nlocalexport [win] <file>",                                                   \
        onLocalExportCommand,                                                           \
        NULL,                                                                           \
        tabCompleteFiles)                                                               \
                                                                                        \
    macro("import",                                                                     \
        NULL,                                                                           \
        "Import code/sprites/map/... from an external file.\n"                          \
        "While importing images, colors are merged to the "                             \
        "closest color of the palette.",                                                \
        "\nimport [" IMPORT_CMD_LIST(IMPORT_CMD_DEF) "] "                            \
        "<file> [" IMPORT_KEYS_LIST(IMPORT_KEYS_DEF) "]",                            \
        onImportCommand,                                                                \
        tabCompleteImport,                                                              \
        tabCompleteFiles)                                                               \
                                                                                        \
    macro("del",                                                                        \
        "rm",                                                                           \
        "Delete from the filesystem.",                                                  \
        "del <file|folder>",                                                            \
        onDelCommand,                                                                   \
        tabCompleteFilesAndDirs,                                                        \
        NULL)                                                                           \
                                                                                        \
    macro("cls",                                                                        \
        "clear",                                                                        \
        "Clear console screen.",                                                        \
        NULL,                                                                           \
        onClsCommand,                                                                   \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("demo",                                                                       \
        NULL,                                                                           \
        "Install demo carts to the current directory.",                                 \
        NULL,                                                                           \
        onInstallDemosCommand,                                                          \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("config",                                                                     \
        NULL,                                                                           \
        "Edit system configuration cartridge.\n"                                        \
        "Use `reset` param to reset current configuration.\n"                           \
        "Use `default` to edit default cart template.",                                 \
        "config [reset|default]",                                                       \
        onConfigCommand,                                                                \
        tabCompleteConfig,                                                              \
        NULL)                                                                           \
                                                                                        \
    macro("surf",                                                                       \
        NULL,                                                                           \
        "Open carts browser.",                                                          \
        NULL,                                                                           \
        onSurfCommand,                                                                  \
        NULL,                                                                           \
        NULL)                                                                           \
                                                                                        \
    macro("menu",                                                                       \
        NULL,                                                                           \
        "Show menu where you can setup video, sound and input options.",                \
        NULL,                                                                           \
        onGameMenuCommand,                                                              \
        NULL,                                                                           \
        NULL)                                                                           \
    ADDGET_FILE(macro)

static struct Command
{
    const char* name;
    const char* alt;
    const char* help;
    const char* usage;
    void(*handler)(Console*);
    void(*tabComplete1)(TabCompleteData*);
    void(*tabComplete2)(TabCompleteData*);

} Commands[] =
{
#define COMMANDS_DEF(name, alt, help, usage, handler, tabComplete1, tabComplete2) \
    {name, alt, help, usage, handler, tabComplete1, tabComplete2},
    COMMANDS_LIST(COMMANDS_DEF)
#undef COMMANDS_DEF
};

#undef SECTION_DEF
#undef EXPORT_CMD_DEF
#undef EXPORT_KEYS_DEF
#undef IMPORT_CMD_DEF
#undef IMPORT_KEYS_DEF

typedef struct Command Command;

#define API_LIST(macro)         \
    ULI_CALLBACK_LIST(macro)    \
    ULI_API_LIST(macro)

static struct ApiItem {const char* name; const char* def; const char* help;} Api[] =
{
#define ULI_API_DEF(name, def, help, ...) {#name, def, help},
    API_LIST(ULI_API_DEF)
#undef ULI_API_DEF
};

typedef struct ApiItem ApiItem;

static void tabCompleteHelp(TabCompleteData* data)
{
#define HELP_CMD_DEF(name) addTabCompleteOption(data, #name);
    HELP_CMD_LIST(HELP_CMD_DEF)
#undef  HELP_CMD_DEF

    for(s32 i = 0; i < COUNT_OF(Commands); i++)
    {
        addTabCompleteOption(data, Commands[i].name);
    }

#define ULI_API_DEF(name, def, help, ...) addTabCompleteOption(data, #name);
    API_LIST(ULI_API_DEF)
#undef ULI_API_DEF

    finishTabComplete(data);
}


static s32 createRamTable(char* buf)
{
    char* ptr = buf;
    ptr += sprintf(ptr, "\n+-----------------------------------+"
                        "\n|           96KB RAM LAYOUT         |"
                        "\n+-------+-------------------+-------+"
                        "\n| ADDR  | INFO              | BYTES |"
                        "\n+-------+-------------------+-------+");

    static const struct Row {s32 addr; const char* info;} Rows[] =
    {
        {0,                                         "<VRAM>"},
        {offsetof(uli_ram, tiles),                  "TILES"},
        {offsetof(uli_ram, sprites),                "SPRITES"},
        {offsetof(uli_ram, map),                    "MAP"},
        {offsetof(uli_ram, input.gamepads),         "GAMEPADS"},
        {offsetof(uli_ram, input.mouse),            "MOUSE"},
        {offsetof(uli_ram, input.keyboard),         "KEYBOARD"},
        {offsetof(uli_ram, sfxpos),                 "SFX STATE"},
        {offsetof(uli_ram, registers),              "SOUND REGISTERS"},
        {offsetof(uli_ram, sfx.waveforms),          "WAVEFORMS"},
        {offsetof(uli_ram, sfx.samples),            "SFX"},
        {offsetof(uli_ram, music.patterns.data),    "MUSIC PATTERNS"},
        {offsetof(uli_ram, music.tracks.data),      "MUSIC TRACKS"},
        {offsetof(uli_ram, music_state),            "MUSIC STATE"},
        {offsetof(uli_ram, stereo),                 "STEREO VOLUME"},
        {offsetof(uli_ram, persistent),             "PERSISTENT MEMORY"},
        {offsetof(uli_ram, flags),                  "SPRITE FLAGS"},
        {offsetof(uli_ram, font.regular),           "FONT"},
        {offsetof(uli_ram, font.regular.params),    "FONT PARAMS"},
        {offsetof(uli_ram, font.alt),               "ALT FONT"},
        {offsetof(uli_ram, font.alt.params),        "ALT FONT PARAMS"},
        {offsetof(uli_ram, mapping),                "BUTTONS MAPPING"},
        {offsetof(uli_ram, pcm),                    "PCM SAMPLES"},
        {offsetof(uli_ram, free),                   "** RESERVED **"},
        {ULI_RAM_SIZE,                              ""},
    };

    for(const struct Row* row = Rows, *end = row + COUNT_OF(Rows) - 1; row < end; row++)
        ptr += sprintf(ptr, "\n| %05X | %-17s | %-5i |", row->addr, row->info, (row + 1)->addr - row->addr);

    ptr += sprintf(ptr, "\n+-------+-------------------+-------+\n");

    return strlen(buf);
}

static s32 createVRamTable(char* buf)
{
    char* ptr = buf;
    ptr += sprintf(ptr, "\n+-----------------------------------+"
                        "\n|          16KB VRAM LAYOUT         |"
                        "\n+-------+-------------------+-------+"
                        "\n| ADDR  | INFO              | BYTES |"
                        "\n+-------+-------------------+-------+");

    static const struct Row {s32 addr; const char* info;} Rows[] =
    {
        {offsetof(uli_ram, vram.screen),        "SCREEN"},
        {offsetof(uli_ram, vram.palette),       "PALETTE"},
        {offsetof(uli_ram, vram.mapping),       "PALETTE MAP"},
        {offsetof(uli_ram, vram.vars),          "BORDER COLOR"},
        {offsetof(uli_ram, vram.vars.offset),   "SCREEN OFFSET"},
        {offsetof(uli_ram, vram.vars.cursor),   "MOUSE CURSOR"},
        {offsetof(uli_ram, vram.blit),          "BLIT SEGMENT"},
        {offsetof(uli_ram, vram.reserved),      "... (reserved) "},
        {ULI_VRAM_SIZE,                         ""},
    };

    for(const struct Row* row = Rows, *end = row + COUNT_OF(Rows) - 1; row < end; row++)
        ptr += sprintf(ptr, "\n| %05X | %-17s | %-5i |", row->addr, row->info, (row + 1)->addr - row->addr);

    ptr += sprintf(ptr, "\n+-------+-------------------+-------+\n");

    return strlen(buf);
}

static s32 createKeysTable(char* buf)
{
    char* ptr = buf;
    ptr += sprintf(ptr, "\n+----+------------+ +----+------------+"
                        "\n|CODE|    KEY     | |CODE|    KEY     |"
                        "\n+----+------------+ +----+------------+");

    static const struct Row {s32 code; const char* key;} Rows[] =
    {
        {1,  "A"},
        {2,  "B"},
        {3,  "C"},
        {4,  "D"},
        {5,  "E"},
        {6,  "F"},
        {7,  "G"},
        {8,  "H"},
        {9,  "I"},
        {10, "J"},
        {11, "K"},
        {12, "L"},
        {13, "M"},
        {14, "N"},
        {15, "O"},
        {16, "P"},
        {17, "Q"},
        {18, "R"},
        {19, "S"},
        {20, "T"},
        {21, "U"},
        {22, "V"},
        {23, "W"},
        {24, "X"},
        {25, "Y"},
        {26, "Z"},
        {27, "0"},
        {28, "1"},
        {29, "2"},
        {30, "3"},
        {31, "4"},
        {32, "5"},
        {33, "6"},
        {34, "7"},
        {35, "8"},
        {36, "9"},
        {37, "MINUS"},
        {38, "EQUALS"},
        {39, "LEFTBRACKET"},
        {40, "RIGHTBRACKT"},
        {41, "BACKSLASH"},
        {42, "SEMICOLON"},
        {43, "APOSTROPHE"},
        {44, "GRAVE"},
        {45, "COMMA"},
        {46, "PERIOD"},
        {47, "SLASH"},
        {48, "SPACE"},
        {49, "TAB"},
        {50, "RETURN"},
        {51, "BACKSPACE"},
        {52, "DELETE"},
        {53, "INSERT"},
        {54, "PAGEUP"},
        {55, "PAGEDOWN"},
        {56, "HOME"},
        {57, "END"},
        {58, "UP"},
        {59, "DOWN"},
        {60, "LEFT"},
        {61, "RIGHT"},
        {62, "CAPSLOCK"},
        {63, "CTRL"},
        {64, "SHIFT"},
        {65, "ALT"},
        {66, "ESC"},
        {67, "F1"},
        {68, "F2"},
        {69, "F3"},
        {70, "F4"},
        {71, "F5"},
        {72, "F6"},
        {73, "F7"},
        {74, "F8"},
        {75, "F9"},
        {76, "F10"},
        {77, "F11"},
        {78, "F12"},
        {79, "NUM0"},
        {80, "NUM1"},
        {81, "NUM2"},
        {82, "NUM3"},
        {83, "NUM4"},
        {84, "NUM5"},
        {85, "NUM6"},
        {86, "NUM7"},
        {87, "NUM8"},
        {88, "NUM9"},
        {89, "NUMPLUS"},
        {90, "NUMMINUS"},
        {91, "NUMMULTIPLY"},
        {92, "NUMDIVIDE"},
        {93, "NUMENTER"},
        {94, "NUMPERIOD"},
    };

    for(const struct Row *row = Rows, *alt = row + COUNT_OF(Rows) / 2, *end = alt; row != end; ++row, ++alt)
    {
        ptr += sprintf(ptr, "\n| %2d | %-11s| | %2d | %-11s|", row->code, row->key, alt->code, alt->key);
    }

    ptr += sprintf(ptr, "\n+----+------------+ +----+------------+\n");

    return strlen(buf);
}

static s32 createButtonsTable(char* buf)
{
    char* ptr = buf;
    ptr += sprintf(ptr, "\n+--------+----+----+----+----+"
                        "\n| ACTION | P1 | P2 | P3 | P4 |"
                        "\n+--------+----+----+----+----+");

    static const struct Row {const char* action;} Rows[] =
    {
        {"UP"},
        {"DOWN"},
        {"LEFT"},
        {"RIGHT"},
        {"A"},
        {"B"},
        {"X"},
        {"Y"},
        {"START(V)"},
        {"SELECT(C)"},
        {"L1/LB(W)"},
        {"R1/RB(E)"},
        {"L2/LT(Q)"},
        {"R2/RT(R)"},
        {"GUIDE(G)"},
    };

    int id = 0;
    for(const struct Row* row = Rows, *end = row + COUNT_OF(Rows); row < end; row++) {
        ptr += sprintf(ptr, "\n| %6s | %2d | %2d | %2d | %2d |", row->action, id, id + ULI_BUTTONS, id + 2 * ULI_BUTTONS, id + 3 * ULI_BUTTONS);
        id++;
    }

    ptr += sprintf(ptr, "\n+--------+----+----+----+----+\n");

    return strlen(buf);
}

static void onExport_help(Console* console, const char* param, const char* name, ExportParams params)
{
    const char* filename = getFilename(name, ".md");

    char* buf = malloc(ULI_BANK_SIZE), *ptr = buf;

    SCOPE(free(buf))
    {
        ptr += sprintf(ptr, "# " ULI_NAME_FULL "\n" ULI_VERSION"\n" ULI_COPYRIGHT"\n");
        ptr += sprintf(ptr, "\n## Welcome\n%s\n", WelcomeText);
        ptr += sprintf(ptr, "\n## Specification\n```\n");

        FOR(const struct SpecRow*, row, SpecText1)
            ptr += sprintf(ptr, "%-10s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n```\n");
        ptr += createRamTable(ptr);
        ptr += sprintf(ptr, "```\n```");
        ptr += createVRamTable(ptr);
        ptr += sprintf(ptr, "```\n\n## Console commands\n");

        FOR(const Command*, cmd, Commands)
            ptr += sprintf(ptr, "\n### %s\n%s\nusage: `%s`\n",
                cmd->name, cmd->help, cmd->usage ? cmd->usage : cmd->name);

        ptr += sprintf(ptr, "\n## API functions\n");

        FOR(const ApiItem*, api, Api)
            ptr += sprintf(ptr, "\n### %s\n`%s`\n%s\n", api->name, api->def, api->help);

        ptr += sprintf(ptr, "\n## Button IDs\n");
        ptr += sprintf(ptr, "```");
        ptr += createButtonsTable(ptr);
        ptr += sprintf(ptr, "```\n");

        ptr += sprintf(ptr, "\n## Key IDs\n");
        ptr += sprintf(ptr, "```");
        ptr += createKeysTable(ptr);
        ptr += sprintf(ptr, "```\n");

        ptr += sprintf(ptr, "\n## Startup options\n```\n");
        FOR(const struct StartupOption*, opt, StartupOptions)
            ptr += sprintf(ptr, "--%-14s %s\n", opt->name, opt->help);

        ptr += sprintf(ptr, "```\n\n## Hotkeys\n");

        ptr += sprintf(ptr, "\n### General:\n```\n");
        FOR(const struct HotkeysRowGeneral*, row, HotkeysTextGeneral)
            ptr += sprintf(ptr, "%-20s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n\n### Navigation:\n```\n");
        FOR(const struct HotkeysRowNavigation*, row, HotkeysTextNavigation)
            ptr += sprintf(ptr, "%-20s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n\n### Code Editor:\n```\n");
        FOR(const struct HotkeysRowCodeEditor*, row, HotkeysTextCodeEditor)
            ptr += sprintf(ptr, "%-20s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n\n### Sprite Editor:\n```\n");
        FOR(const struct HotkeysRowSpriteEditor*, row, HotkeysTextSpriteEditor)
            ptr += sprintf(ptr, "%-20s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n\n### Map Editor:\n```\n");
        FOR(const struct HotkeysRowMapEditor*, row, HotkeysTextMapEditor)
            ptr += sprintf(ptr, "%-20s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n\n### SFX Editor:\n```\n");
        FOR(const struct HotkeysRowSFXEditor*, row, HotkeysTextSFXEditor)
            ptr += sprintf(ptr, "%-20s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n\n### Music Editor:\n```\n");
        FOR(const struct HotkeysRowMusicEditor*, row, HotkeysTextMusicEditor)
            ptr += sprintf(ptr, "%-20s%s\n", row->section, row->info);

        ptr += sprintf(ptr, "```\n\n%s\n\n%s", TermsText, LicenseText);

        char* helpReplaced = replaceHelpTokens(buf);

        SCOPE(free(helpReplaced))
        {
            onFileExported(console, filename, uli_fs_save(console->fs, filename, helpReplaced, strlen(helpReplaced), true));
        }
    }
}

TabCompleteData newTabCompleteData(Console* console, char* incompleteWord) {
    TabCompleteData data = { console, .incompleteWord = incompleteWord };
    data.options = malloc(CONSOLE_BUFFER_SCREEN);
    data.commonPrefix = malloc(CONSOLE_BUFFER_SCREEN);
    data.options[0] = '\0';
    data.commonPrefix[0] = '\0';

    return data;
}

static void processConsoleTab(Console* console)
{
    char* input = console->input.text;
    char* param = strchr(input, ' ');

    if(param)
    {
        // Tab-complete command's parameters.
        param++;
        char* secondParam = strchr(param, ' ');
        if (secondParam)
            secondParam++;

        for(s32 i = 0; i < COUNT_OF(Commands); i++)
        {
            s32 commandLen = param-input-1;
            bool commandMatches = (strlen(Commands[i].name) == commandLen &&
                                       strncmp(Commands[i].name, input, commandLen) == 0) ||
                                  (Commands[i].alt &&
                                      strlen(Commands[i].alt) == commandLen &&
                                      strncmp(Commands[i].alt, input, commandLen) == 0);

            if (commandMatches)
            {
                if (secondParam) {
                    if (Commands[i].tabComplete2) {
                        TabCompleteData data = newTabCompleteData(console, secondParam);
                        Commands[i].tabComplete2(&data);
                    }
                } else {
                    if (Commands[i].tabComplete1) {
                        TabCompleteData data = newTabCompleteData(console, param);
                        Commands[i].tabComplete1(&data);
                    }
                }
            }
        }
    }
    else
    {
        // Tab-complete commands.
        TabCompleteData data = newTabCompleteData(console, input);
        for(s32 i = 0; i < COUNT_OF(Commands); i++)
        {
            addTabCompleteOption(&data, Commands[i].name);
            if (Commands[i].alt)
                addTabCompleteOption(&data, Commands[i].alt);
        }
        finishTabComplete(&data);
    }
    scrollConsole(console);
}

static void toUpperStr(char* str)
{
    while(*str)
    {
        *str = toupper(*str);
        str++;
    }
}

static bool printUsage(Console* console, const char* command)
{
    FOR(const Command*, cmd, Commands)
    {
        if(strcmp(command, cmd->name) == 0)
        {
            consolePrint(console, "\n---=== COMMAND ===---\n", uli_color_green);
            char* helpReplaced = replaceHelpTokens(cmd->help);
            printBack(console, helpReplaced);
            free(helpReplaced);

            if(cmd->usage)
            {
                printFront(console, "\n\nusage: ");
                char* usageReplaced = replaceHelpTokens(cmd->usage);
                printBack(console, usageReplaced);
                free(usageReplaced);
            }

            printLine(console);
            return true;
        }
    }

    return false;
}

static bool printApi(Console* console, const char* param)
{
    FOR(const ApiItem*, api, Api)
    {
        if(strcmp(param, api->name) == 0)
        {
            printLine(console);
            consolePrint(console, "---=== API ===---\n", uli_color_blue);
            consolePrint(console, api->def, uli_color_light_blue);
            printFront(console, "\n\n");
            printBack(console, api->help);
            printLine(console);
            return true;
        }
    }

    return false;
}

#define STRBUF_SIZE(name, ...) + STRLEN(#name) + STRLEN(Sep)

static void onHelp_api(Console* console)
{
    consolePrint(console, "\nAPI functions:\n", uli_color_blue);
    {
        const char Sep[] = " ";

        // calc buf size on compile time
        char buf[API_LIST(STRBUF_SIZE) + 1] = {[0] = 0};

        FOR(const ApiItem*, api, Api)
            strcat(buf, api->name), strcat(buf, Sep);

        printBack(console, buf);
    }
}

static void onHelp_commands(Console* console)
{
    consolePrint(console, "\nConsole commands:\n", uli_color_green);
    {
        const char Sep[] = " ";

        // calc buf size on compile time
        char buf[COMMANDS_LIST(STRBUF_SIZE) + 1] = {[0] = 0};

        FOR(const Command*, cmd, Commands)
            strcat(buf, cmd->name), strcat(buf, Sep);

        printBack(console, buf);
    }
}

#undef STRBUF_SIZE

static void printTable(Console* console, const char* text)
{
#ifndef BAREMETALPI
    printf("%s", text);
#endif

    for(const char* textPointer = text, *endText = textPointer + strlen(text); textPointer != endText;)
    {
        char symbol = *textPointer++;

        scrollConsole(console);

        if(symbol == '\n')
            nextLine(console);
        else
        {
            u8 color = 0;

            switch(symbol)
            {
            case '+':
            case '|':
            case '-':
                color = uli_color_dark_grey;
                break;
            default:
                color = CONSOLE_FRONT_TEXT_COLOR;
            }

            setSymbol(console, symbol, color, cursorOffset(console));

            console->cursor.pos.x++;

            if(console->cursor.pos.x >= CONSOLE_BUFFER_WIDTH)
                nextLine(console);
        }
    }
}

static void onHelp_ram(Console* console)
{
    char buf[2048];
    createRamTable(buf);
    printTable(console, buf);
}

static void onHelp_vram(Console* console)
{
    char buf[1024];
    createVRamTable(buf);
    printTable(console, buf);
}

static void onHelp_keys(Console* console)
{
    char buf[4096];
    createKeysTable(buf);
    printTable(console, buf);
}

static void onHelp_buttons(Console* console)
{
    char buf[1024];
    createButtonsTable(buf);
    printTable(console, buf);
}

static void onHelp_version(Console* console)
{
    consolePrint(console, "\n"ULI_VERSION, CONSOLE_BACK_TEXT_COLOR);
}

static void onHelp_spec(Console* console)
{
    printLine(console);

    char buf[ULINAME_MAX];

    FOR(const struct SpecRow*, row, SpecText1)
    {
#define OFFSET 8
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }
}

static void onHelp_hotkeys(Console* console)
{
    printLine(console);

    char buf[ULINAME_MAX];

    printFront(console, "\nGeneral:\n");
    FOR(const struct HotkeysRowGeneral*, row, HotkeysTextGeneral)
    {
#define OFFSET 14
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }

    printFront(console, "\nNavigation:\n");
    FOR(const struct HotkeysRowNavigation*, row, HotkeysTextNavigation)
    {
#define OFFSET 17
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }

    printFront(console, "\nCode Editor:\n");
    FOR(const struct HotkeysRowCodeEditor*, row, HotkeysTextCodeEditor)
    {
#define OFFSET 19
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }

    printFront(console, "\nSprite Editor:\n");
    FOR(const struct HotkeysRowSpriteEditor*, row, HotkeysTextSpriteEditor)
    {
#define OFFSET 9
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }

    printFront(console, "\nMap Editor:\n");
    FOR(const struct HotkeysRowMapEditor*, row, HotkeysTextMapEditor)
    {
#define OFFSET 11
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }

    printFront(console, "\nSFX Editor:\n");
    FOR(const struct HotkeysRowSFXEditor*, row, HotkeysTextSFXEditor)
    {
#define OFFSET 14
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }

    printFront(console, "\nMusic Editor:\n");
    FOR(const struct HotkeysRowMusicEditor*, row, HotkeysTextMusicEditor)
    {
#define OFFSET 14
        char* rowReplaced = replaceHelpTokens(row->info);
        sprintf(buf, "%-" DEF2STR(OFFSET) "s%s\n", row->section, rowReplaced);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET);
        free(rowReplaced);
#undef  OFFSET
    }


}

static void onHelp_welcome(Console* console)
{
    printLine(console);
    printBack(console, WelcomeText);
}

static void onHelp_startup(Console* console)
{
    char buf[ULINAME_MAX];
    printFront(console, "\nStartup options:\n");
    FOR(const struct StartupOption*, opt, StartupOptions)
    {
#define OFFSET 12
#define PREFIX "--"
        sprintf(buf, PREFIX "%-" DEF2STR(OFFSET) "s%s\n", opt->name, opt->help);
        consolePrintOffset(console, buf, uli_color_grey, OFFSET + STRLEN(PREFIX));
#undef  PREFIX
#undef  OFFSET
    }
}

static void onHelp_terms(Console* console)
{
    printLine(console);
    printBack(console, TermsText);
}

static void onHelp_license(Console* console)
{
    printLine(console);
    printBack(console, LicenseText);
}

static void onHelpCommand(Console* console)
{
    if(console->desc->count)
    {
        const char* param = console->desc->params->key;
        bool foundTopic = false;

        if(printUsage(console, param)) {
            foundTopic = true;
        }
        if(printApi(console, param)) {
            foundTopic = true;
        }

        static const struct Handler {const char* cmd; void(*handler)(Console*);} Handlers[] =
        {
#define         HELP_CMD_DEF(name) {#name, onHelp_##name},
                HELP_CMD_LIST(HELP_CMD_DEF)
#undef          HELP_CMD_DEF
        };

        FOR(const struct Handler*, ptr, Handlers)
            if(strcmp(ptr->cmd, param) == 0)
            {
                foundTopic = true;
                ptr->handler(console);
                break;
            }

        if (!foundTopic) {
            printError(console, "\nunknown topic: ");
            printError(console, param);
        }
    }
    else
    {
        printFront(console, "\n\nusage: ");
        printBack(console, HelpUsage);

        printBack(console, "\n\ntype ");
        printFront(console, "help commands");
        printBack(console, " to show commands");

        printBack(console, "\n\npress ");
        printFront(console, "ESC");
        printBack(console, " to switch editor/console\n");
    }

    commandDone(console);
}

static CommandDesc parseCommand(const char* input)
{
    CommandDesc desc = {.src = strdup(input),
                        .command = strdup(input)};

    char* token = strtok(desc.command, " ");

    while((token = strtok(NULL, " ")))
    {
        desc.params = realloc(desc.params, ++desc.count * sizeof *desc.params);
        desc.params[desc.count - 1].key = token;
    }

    for(struct Param* it = desc.params, *end = it + desc.count; it < end; it++)
    {
        if (strcmp(it->key, "=") == 0) continue;

        it->key = strtok(it->key, "=");
        it->val = strtok(NULL, "=");
    }

    return desc;
}

static void processCommand(Console* console, const char* text)
{
    console->active = false;

    *console->desc = parseCommand(text);

    if (console->desc->command)
    {
        const char* command = console->desc->command;

        FOR(const Command*, cmd, Commands)
            if(casecmp(console->desc->command, cmd->name) == 0 ||
                (cmd->alt && casecmp(console->desc->command, cmd->alt) == 0))
            {
                cmd->handler(console);
                command = NULL;
                break;
            }

        if(command)
        {
            printLine(console);
            printError(console, "unknown command: ");
            printError(console, command);
            commandDone(console);
        }
    }
    else commandDone(console);
}

static void fillHistory(Console* console)
{
    if(console->history.size)
    {
        console->input.pos = 0;
        memset(console->input.text, '\0', strlen(console->input.text));

        const char* item = console->history.items[console->history.index];
        strcpy(console->input.text, item);
        memset(console->color + getInputOffset(console), CONSOLE_INPUT_COLOR, strlen(item));
        processConsoleEnd(console);
    }
}

static void onHistoryUp(Console* console)
{
    fillHistory(console);

    if(console->history.index > 0)
        console->history.index--;
}

static void onHistoryDown(Console* console)
{
    if(console->history.index < console->history.size - 1)
    {
        console->history.index++;
        fillHistory(console);
    }
    else
    {
        memset(console->input.text, '\0', strlen(console->input.text));
        processConsoleEnd(console);
    }
}

static void appendHistory(Console* console, const char* value)
{
    if(console->history.size)
        if(strcmp(console->history.items[console->history.index = console->history.size - 1], value) == 0)
            return;

    console->history.index = console->history.size++;
    console->history.items = realloc(console->history.items, sizeof(char*) * console->history.size);
    console->history.items[console->history.index] = strdup(value);
}

static void processConsoleCommand(Console* console)
{
    size_t commandSize = strlen(console->input.text);

    if(commandSize)
    {
        printf("%s", console->input.text);
        appendHistory(console, console->input.text);
        processCommand(console, console->input.text);
    }
    else commandDone(console);
}

static void error(Console* console, const char* info)
{
    consolePrint(console, info ? info : "unknown error", CONSOLE_ERROR_TEXT_COLOR);
    commandDone(console);
}

static void trace(Console* console, const char* text, u8 color)
{
    consolePrint(console, text, color);
    commandDone(console);
}

static void setScroll(Console* console, s32 val)
{
    if(console->scroll.pos != val)
    {
        console->scroll.pos = MIN(CLAMP(val, 0, console->cursor.pos.y), CONSOLE_BUFFER_ROWS - CONSOLE_BUFFER_HEIGHT);
    }
}

static void onHttpVersionGet(const net_get_data* data)
{
    Console* console = (Console*)data->calldata;

    switch(data->type)
    {
    case net_get_done:
        {
            if(json_parse((char*)data->done.data, data->done.size))
            {
                s32 major = json_int("major", 0);
                s32 minor = json_int("minor", 0);
                s32 patch = json_int("patch", 0);

                if((major > ULI_VERSION_MAJOR)
                    || (major == ULI_VERSION_MAJOR && minor > ULI_VERSION_MINOR)
                    || (major == ULI_VERSION_MAJOR && minor == ULI_VERSION_MINOR && patch > ULI_VERSION_REVISION))
                {
                    char msg[ULINAME_MAX];
                    sprintf(msg, " new version %i.%i.%i available", major, minor, patch);

                    enum{Offset = (2 * STUDIO_TEXT_BUFFER_WIDTH)};

                    memset(console->text + Offset, ' ', STUDIO_TEXT_BUFFER_WIDTH);
                    strcpy(console->text + Offset, msg);
                    memset(console->color + Offset, uli_color_red, strlen(msg));
                }
            }
        }
        break;
    default:
        break;
    }
}

static char* getSelectionText(Console* console)
{
    const char* start = console->select.start;
    const char* end = console->select.end;

    if (start > end)
        SWAP(start, end, const char*);

    s32 size = end - start;
    if (size)
    {
        size += size / CONSOLE_BUFFER_WIDTH + 1;
        char* clipboard = malloc(size);
        memset(clipboard, 0, size);
        char* dst = clipboard;

        s32 index = (start - console->text) % CONSOLE_BUFFER_WIDTH;

        for (const char* ptr = start; ptr < end; ptr++, index++)
        {
            if (index && (index % CONSOLE_BUFFER_WIDTH) == 0)
                *dst++ = '\n';

            if (*ptr)
                *dst++ = *ptr;
        }

        return clipboard;
    }

    return NULL;
}

static void copyToClipboard(Console* console)
{
    char* text = getSelectionText(console);

    if (text)
    {
        uli_sys_clipboard_set(text);
        free(text);
        clearSelection(console);
    }
}

static void copyFromClipboard(Console* console)
{
    if(uli_sys_clipboard_has())
    {
        const char* clipboard = uli_sys_clipboard_get();

        if(clipboard)
        {
            char* text = strdup(clipboard);

            char* dst = text;
            for(const char* src = clipboard; *src; src++)
                if(isprint(*src))
                    *dst++ = *src;

            insertInputText(console, text);
            free(text);

            uli_sys_clipboard_free(clipboard);
        }
    }
}

static void processMouse(Console* console)
{
    uli_mem* uli = console->uli;
    // process scroll
    {
        uli78_input* input = &console->uli->ram->input;

        if(input->mouse.scrolly)
        {
            enum{Scroll = 3};
            s32 delta = input->mouse.scrolly > 0 ? -Scroll : Scroll;
            setScroll(console, console->scroll.pos + delta);
        }
    }

    uli_rect rect = {0, 0, ULI78_WIDTH, ULI78_HEIGHT};

    if(checkMousePos(console->studio, &rect))
        setCursor(console->studio, uli_cursor_ibeam);

#if defined(__ULI_ANDROID__)

    if(checkMouseDown(console->studio, &rect, uli_mouse_left))
    {
        setCursor(console->studio, uli_cursor_hand);

        if(console->scroll.active)
        {
            setScroll(console, (console->scroll.start - uli_api_mouse(uli).y) / STUDIO_TEXT_HEIGHT);
        }
        else
        {
            console->scroll.active = true;
            console->scroll.start = uli_api_mouse(uli).y + console->scroll.pos * STUDIO_TEXT_HEIGHT;
        }
    }
    else console->scroll.active = false;

#else

    if(checkMouseDown(console->studio, &rect, uli_mouse_left))
    {
        uli_point m = uli_api_mouse(uli);

        console->select.end = console->text
            + m.x / STUDIO_TEXT_WIDTH
            + (m.y / STUDIO_TEXT_HEIGHT + console->scroll.pos) * CONSOLE_BUFFER_WIDTH;

        if(!console->select.active)
        {
            console->select.active = true;
            console->select.start = console->select.end;
        }
    }
    else console->select.active = false;

#endif

    if(checkMouseClick(console->studio, &rect, uli_mouse_middle))
    {
        char* text = getSelectionText(console);

        if (text)
        {
            insertInputText(console, text);
            uli_sys_clipboard_set(text);
            free(text);
        }
        else
            copyFromClipboard(console);
    }
}

static void processConsolePgUp(Console* console)
{
    setScroll(console, console->scroll.pos - STUDIO_TEXT_BUFFER_HEIGHT/2);
}

static void processConsolePgDown(Console* console)
{
    setScroll(console, console->scroll.pos + STUDIO_TEXT_BUFFER_HEIGHT/2);
}

static inline bool isalnum_(char c) {return isalnum(c) || c == '_';}

static s32 leftWordPos(Console* console)
{
    const char* start = console->input.text;
    const char* pos = console->input.text + console->input.pos - 1;

    if(pos > start)
    {
        if(isalnum_(*pos)) while(pos > start && isalnum_(*(pos-1))) pos--;
        else while(pos > start && !isalnum_(*(pos-1))) pos--;
        return pos - console->input.text;
    }

    return console->input.pos;
}

static s32 rightWordPos(Console* console)
{
    const char* end = console->input.text + strlen(console->input.text);
    const char* pos = console->input.text + console->input.pos;

    if(pos < end)
    {
        if(isalnum_(*pos)) while(pos < end && isalnum_(*pos)) pos++;
        else while(pos < end && !isalnum_(*pos)) pos++;
        return pos - console->input.text;
    }

    return console->input.pos;
}

static void leftWord(Console* console)
{
    console->input.pos = leftWordPos(console);
}

static void rightWord(Console* console)
{
    console->input.pos = rightWordPos(console);
}

static void deleteWord(Console* console)
{
    s32 pos = rightWordPos(console);
    deleteText(console, console->input.pos, pos);
}

static void backspaceWord(Console* console)
{
    s32 pos = leftWordPos(console);
    deleteText(console, pos, console->input.pos);
    console->input.pos = pos;
}

static void processKeyboard(Console* console)
{
    uli_mem* uli = console->uli;

    if(!console->active)
        return;

    if(uli->ram->input.keyboard.data != 0)
    {
        switch(getClipboardEvent(console->studio))
        {
        case ULI_CLIPBOARD_COPY: copyToClipboard(console); break;
        case ULI_CLIPBOARD_PASTE: copyFromClipboard(console); scrollConsole(console); break;
        default: break;
        }

        console->cursor.delay = CONSOLE_CURSOR_DELAY;

        bool ctrl = uli_api_key(uli, uli_key_ctrl);
        bool alt = uli_api_key(uli, uli_key_alt);

        if (ctrl || alt)
        {
            if (ctrl)
            {
#if defined(__ULI_LINUX__)
                uli_keycode clearKey = uli_key_l;
#else
                uli_keycode clearKey = uli_key_k;
#endif

                if (keyWasPressed(console->studio, uli_key_a))      processConsoleHome(console);
                else if (keyWasPressed(console->studio, uli_key_e)) processConsoleEnd(console);
                else if (keyWasPressed(console->studio, clearKey))
                {
                    onClsCommand(console);
                    return;
                }
            }

            if (keyWasPressed(console->studio, uli_key_left))           leftWord(console);
            else if(keyWasPressed(console->studio, uli_key_right))      rightWord(console);
            else if(keyWasPressed(console->studio, uli_key_delete))     deleteWord(console);
            else if(keyWasPressed(console->studio, uli_key_backspace))  backspaceWord(console);
        }
        else
        {
            if(keyWasPressed(console->studio, uli_key_up)) {
			    onHistoryUp(console);
			    scrollConsole(console);
            }
            else if(keyWasPressed(console->studio, uli_key_down)) onHistoryDown(console);
            else if(keyWasPressed(console->studio, uli_key_left))
            {
                if(console->input.pos > 0)
                    console->input.pos--;
            }
            else if(keyWasPressed(console->studio, uli_key_right))
            {
                console->input.pos++;
                size_t len = strlen(console->input.text);
                if(console->input.pos > len)
                    console->input.pos = len;
            }
            else if(enterWasPressed(console->studio))                    processConsoleCommand(console);
            else if(keyWasPressed(console->studio, uli_key_backspace))   processConsoleBackspace(console);
            else if(keyWasPressed(console->studio, uli_key_delete))      processConsoleDel(console);
            else if(keyWasPressed(console->studio, uli_key_home))        processConsoleHome(console);
            else if(keyWasPressed(console->studio, uli_key_end))         processConsoleEnd(console);
            else if(keyWasPressed(console->studio, uli_key_tab))         processConsoleTab(console);
            else if(keyWasPressed(console->studio, uli_key_pageup))      processConsolePgUp(console);
            else if(keyWasPressed(console->studio, uli_key_pagedown))    processConsolePgDown(console);
        }
    }

    char sym = getKeyboardText(console->studio);

    if(sym)
    {
        insertInputText(console, (char[]){sym, '\0'});
        scrollConsole(console);

        console->cursor.delay = CONSOLE_CURSOR_DELAY;
    }

}

static void processGamepad(Console* console)
{
    uli_mem* uli = console->uli;

    if(!console->active)
        return;

    if(uli->ram->input.keyboard.data == 0 && uli_api_btnp(uli, 6, -1, -1))
    {
        gotoSurf(console->studio);
    }
}

static void tick(Console* console)
{
    uli_mem* uli = console->uli;

    processMouse(console);
    processKeyboard(console);
    processGamepad(console);

    Start* start = getStartScreen(console->studio);

    if(console->tickCounter == 0)
    {
        if(!start->embed)
        {
            loadDemo(console, uli_get_script(uli));

            if(!console->args.cli)
            {
                printBack(console, "\n hello! type ");
                printFront(console, "help");
                printBack(console, " for help\n");

                if(getConfig(console->studio)->checkNewVersion)
                    uli_net_get(console->net, "/json?fn=version", onHttpVersionGet, console);
            }

            commandDone(console);
        }
        else printBack(console, "\n loading cart...");
    }

    uli_api_cls(uli, ULI_COLOR_BG);
    drawConsoleText(console);

    if(start->embed)
    {
        if(console->tickCounter >= (u32)(console->args.skip ? 1 : ULI78_FRAMERATE))
        {
            runGame(console->studio);

            start->embed = false;
            studioRomLoaded(console->studio);

            printLine(console);
            commandDone(console);
            console->active = true;

            return;
        }
    }
    else
    {
        if(console->cursor.delay)
            console->cursor.delay--;

        console->tickCounter++;

        if (getStudioMode(console->studio) != ULI_CONSOLE_MODE) return;

        drawCursor(console);

        if(console->active)
        {
            if(console->commands.current < console->commands.count)
            {
                const char* command = console->commands.items[console->commands.current];
                if(!console->args.cli)
                    printFront(console, command);

                processCommand(console, command);

                console->commands.current++;
            }
            else if(getConfig(console->studio)->cli)
                exitStudio(console->studio);
        }
    }

    console->tickCounter++;
}

static inline bool isslash(char c)
{
    return c == '/' || c == '\\';
}

static bool cmdLoadCart(Console* console, const char* path)
{
    bool done = false;

    s32 size = 0;
    void* data = fs_read(path, &size);

    if(data)
    {
        const char* cartName = NULL;

        {
            const char* ptr = path + strlen(path);
            while(ptr > path && !isslash(*ptr))--ptr;
            cartName = ptr + isslash(*ptr);
        }

        setCartName(console, cartName, path);
        uli_mem* uli = console->uli;

        if(uli_tool_has_ext(cartName, PngExt))
        {
            uli_cartridge* cart = loadPngCart((png_buffer){data, size});

            if(cart)
            {
                memcpy(&uli->cart, cart, sizeof(uli_cartridge));
                free(cart);
                done = true;
            }
        }
        else if(uli_tool_has_ext(cartName, CART_EXT))
        {
            uli_cart_load(&uli->cart, data, size);
            done = true;
        }
#if defined(ULI78_PRO)
        else if(project_ext(cartName))
        {
            if(uli_project_load(cartName, data, size, &uli->cart))
                done = true;
        }
#endif

        free(data);
    }

    if(done)
        studioRomLoaded(console->studio);

    return done;
}

void forceAutoSave(Console* console, const char* cart_name)
{
    char namepath[ULINAME_MAX];
    strcpy(namepath, "/downloads/");
    strcat(namepath, cart_name);
    CartSaveResult rom = saveCartName(console, namepath);

    if(rom == CART_SAVE_OK)
    {
        printBack(console, "\ncart ");
        printFront(console, console->rom.name);
        printBack(console, " autosaved!\n");
    }
    else if(rom == CART_SAVE_MISSING_NAME)
        printBack(console, "\nautosave name is missing\n");
    else
        printBack(console, "\ncart autosave error");

    commandDone(console);
}

static int cmdcmp(const void* a, const void* b)
{
    return strcmp(((const Command*)a)->name, ((const Command*)b)->name);
}

static int apicmp(const void* a, const void* b)
{
    return strcmp(((const ApiItem*)a)->name, ((const ApiItem*)b)->name);
}

void initConsole(Console* console, Studio* studio, uli_fs* fs, uli_net* net, Config* config, StartArgs args)
{
    if(!console->text)  console->text = malloc(CONSOLE_BUFFER_SIZE);
    if(!console->color) console->color = malloc(CONSOLE_BUFFER_SIZE);
    if(!console->desc)  console->desc = malloc(sizeof(CommandDesc));

    *console = (Console)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .config = config,
        .loadByHash = loadByHash,
        .load = loadExternal,
        .loadCart = cmdLoadCart,
        .updateProject = updateProject,
        .error = error,
        .trace = trace,
        .tick = tick,
        .save = saveCart,
        .done = commandDone,
        .cursor = {.pos.x = 1, .pos.y = 3, .delay = 0},
        .input = console->text,
        .tickCounter = 0,
        .active = false,
        .text = console->text,
        .color = console->color,
        .fs = fs,
        .net = net,
        .args = args,
        .desc = console->desc,
    };

    // parse --cmd param
    {
        char* command = args.cmd;
        while(command)
        {
            console->commands.items = realloc(console->commands.items, sizeof(char*) * (console->commands.count + 1));
            console->commands.items[console->commands.count++] = command;

            static const char Sep[] = " & ";
            command = strstr(command, Sep);

            if(command)
            {
                *command = '\0';
                command += STRLEN(Sep);
            }
        }
    }

    qsort(Commands, COUNT_OF(Commands), sizeof Commands[0], cmdcmp);
    qsort(Api, COUNT_OF(Api), sizeof Api[0], apicmp);

    memset(console->text, 0, CONSOLE_BUFFER_SIZE);
    memset(console->color, ULI_COLOR_BG, CONSOLE_BUFFER_SIZE);
    memset(console->desc, 0, sizeof(CommandDesc));

    Start* start = getStartScreen(console->studio);

    if(!console->args.cli)
    {
        memcpy(console->text, start->text, STUDIO_TEXT_BUFFER_SIZE);
        memcpy(console->color, start->color, STUDIO_TEXT_BUFFER_SIZE);

        printLine(console);
        for(const char* ptr = console->text, *end = ptr + STUDIO_TEXT_BUFFER_SIZE;
            ptr < end; ptr += CONSOLE_BUFFER_WIDTH)
            if(*ptr)
                puts(ptr);
    }

    if (args.cart)
    {
        if (!cmdLoadCart(console, args.cart))
        {
            printf("error: cart `%s` not loaded\n", args.cart);
            exit(1);
        }
        else
            getStartScreen(console->studio)->embed = true;
    }

    console->active = !start->embed;
}

void freeConsole(Console* console)
{
    free(console->text);
    free(console->color);

    if(console->history.items)
    {
        for(char **ptr = console->history.items, **end = ptr + console->history.size; ptr < end; ptr++)
            free(*ptr);

        free(console->history.items);
    }

    FREE(console->commands.items);
    free(console->desc);
    free(console);
}
