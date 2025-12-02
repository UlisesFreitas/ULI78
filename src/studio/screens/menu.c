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

#include "menu.h"
#include "studio/studio.h"
#include "studio/fs.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#define NUM_STARS 256
static struct
{
    float x, y, z;
} Stars[NUM_STARS];

static bool StarsInitialized = false;
static s32 CurrentTicks = 0;

#define ANIM_STATES(macro)  \
    macro(idle)             \
    macro(start)            \
    macro(up)               \
    macro(down)             \
    macro(close)            \
    macro(back)

#define ANIM_MOVIE(name)    Movie name;
#define ANIM_FREE(name)     FREE(menu->anim.name.items);

struct Menu
{
    Studio* studio;
    uli_mem* uli;

    s32 ticks;

    MenuItem* items;
    s32 count;
    s32 pos;
    s32 backPos;

    void* data;
    MenuItemHandler back;

    struct
    {
        s32 pos;
        s32 top;
        s32 bottom;
        s32 cursor;
        s32 offset;

        Movie* movie;

        ANIM_STATES(ANIM_MOVIE)

    } anim;

    struct
    {
        s32 item;
        s32 option;
    } maxwidth;
};

#define BG_ANIM_COLOR uli_color_black

enum
{
    Up, Down, Left, Right, A, B, X, Y,
    Start, Select, L1, R1, L2, R2, Guide
};

enum{TextMargin = 2, ItemHeight = ULI_FONT_HEIGHT + TextMargin * 2};
enum{Hold = KEYBOARD_HOLD, Period = ItemHeight};

static void emptyDone(void* data) {}

static void menuUpDone(void* data)
{
    Menu *menu = data;
    menu->pos = (menu->pos + (menu->count - 1)) % menu->count;
    if (strcmp("", menu->items[menu->pos].label) == 0) return menuUpDone(data);
    menu->anim.pos = 0;
    menu->anim.movie = resetMovie(&menu->anim.idle);
}

static void menuDownDone(void* data)
{
    Menu *menu = data;
    menu->pos = (menu->pos + (menu->count + 1)) % menu->count;
    if (strcmp("", menu->items[menu->pos].label) == 0) return menuDownDone(data);
    menu->anim.pos = 0;
    menu->anim.movie = resetMovie(&menu->anim.idle);
}

static void startDone(void* data)
{
    Menu *menu = data;
    menu->anim.movie = resetMovie(&menu->anim.idle);
}

static void closeDone(void* data)
{
    Menu *menu = data;
    menu->anim.movie = resetMovie(&menu->anim.idle);
    menu->items[menu->pos].handler(menu->data, menu->pos);
}

static void backDone(void* data)
{
    Menu *menu = data;
    menu->anim.movie = resetMovie(&menu->anim.idle);
    s32 pos = menu->backPos;
    menu->back(menu->data, 0);
    menu->pos = pos;
}

static void printShadow(uli_mem* uli, const char* text, s32 x, s32 y, uli_color color)
{
    uli_api_print(uli, text, x, y + 1, uli_color_black, true, 1, false);
    uli_api_print(uli, text, x, y, color, true, 1, false);
}

static inline bool animIdle(Menu* menu)
{
    return menu->anim.movie == &menu->anim.idle;
}

static void drawTopBar(Menu* menu, s32 x, s32 y)
{
    uli_mem* uli = menu->uli;

    y += menu->anim.top;

    uli_api_rect(uli, x, y, ULI78_WIDTH, ItemHeight, uli_color_grey);
    uli_api_rect(uli, x, y + ItemHeight, ULI78_WIDTH, 1, uli_color_black);

    static const char Text[] = ULI_NAME_FULL;
    printShadow(uli, Text, (ULI78_WIDTH - STRLEN(Text) * ULI_FONT_WIDTH) / 2, y + TextMargin, uli_color_white);
}

static void drawBottomBar(Menu* menu, s32 x, s32 y)
{
    uli_mem* uli = menu->uli;

    y += menu->anim.bottom;

    uli_api_rect(uli, x, y, ULI78_WIDTH, ItemHeight, uli_color_grey);
    uli_api_rect(uli, x, y - 1, ULI78_WIDTH, 1, uli_color_black);

    const char* help = menu->items[menu->pos].help;
    if(help)
    {
        if(menu->ticks % ULI78_FRAMERATE < ULI78_FRAMERATE / 2)
            printShadow(uli, help, x + (ULI78_WIDTH - strlen(help) * ULI_FONT_WIDTH) / 2 + menu->anim.offset,
                y + TextMargin, uli_color_white);
    }
    else
    {
        static const char Text[] = ULI_COPYRIGHT;
        printShadow(uli, Text, (ULI78_WIDTH - STRLEN(Text) * ULI_FONT_WIDTH) / 2, y + TextMargin, uli_color_white);
    }
}

static void updateOption(MenuOption* option, s32 val, void* data)
{
    option->pos = (option->pos + option->count + val) % option->count;
    option->set(data, option->pos);
    option->pos = option->get(data);
}

static void onMenuItem(Menu* menu, const MenuItem* item)
{
    playSystemSfx(menu->studio, 2);
    menu->anim.movie = resetMovie(item->back ? &menu->anim.back : &menu->anim.close);
}

static void drawOptionArrow(Menu* menu, MenuOption* option, s32 x, s32 y, s32 icon, s32 delta)
{
    uli_rect left = {x - 1, y, ULI_FONT_WIDTH, ULI_FONT_HEIGHT};
    bool down = false;
    bool over = false;
    if(checkMousePos(menu->studio, &left))
    {
        over = true;
        setCursor(menu->studio, uli_cursor_hand);
        down = checkMouseDown(menu->studio, &left, uli_mouse_left);

        if(checkMouseClick(menu->studio, &left, uli_mouse_left))
        {
            playSystemSfx(menu->studio, 2);
            updateOption(option, delta, menu->data);
        }
    }

    if(down)
    {
        drawBitIcon(menu->studio, icon, left.x, left.y, uli_color_white);
    }
    else
    {
        drawBitIcon(menu->studio, icon, left.x, left.y, uli_color_black);
        drawBitIcon(menu->studio, icon, left.x, left.y - 1, over ? uli_color_white : uli_color_light_grey);
    }
}

static void drawMenu(Menu* menu, s32 x, s32 y)
{
    if (getStudioMode(menu->studio) != ULI_MENU_MODE)
        return;

    uli_mem* uli = menu->uli;

    if(animIdle(menu))
    {
        if(uli_api_btnp(menu->uli, Up, Hold, Period)
            || uli_api_keyp(uli, uli_key_up, Hold, Period))
        {
            playSystemSfx(menu->studio, 2);
            menu->anim.movie = resetMovie(&menu->anim.up);
        }

        if(uli_api_btnp(menu->uli, Down, Hold, Period)
            || uli_api_keyp(uli, uli_key_down, Hold, Period))
        {
            playSystemSfx(menu->studio, 2);
            menu->anim.movie = resetMovie(&menu->anim.down);
        }

        MenuItem* item = &menu->items[menu->pos];
        MenuOption* option = item->option;
        if(option)
        {
            if(uli_api_btnp(menu->uli, Left, Hold, Period)
                || uli_api_keyp(uli, uli_key_left, Hold, Period))
            {
                playSystemSfx(menu->studio, 2);
                updateOption(option, -1, menu->data);
            }

            if(uli_api_btnp(menu->uli, Right, Hold, Period)
                || uli_api_keyp(uli, uli_key_right, Hold, Period))
            {
                playSystemSfx(menu->studio, 2);
                updateOption(option, +1, menu->data);
            }
        }

        if(uli_api_btnp(menu->uli, A, -1, -1) || ticEnterWasPressed(uli, -1, -1))
        {
            if(option)
            {
                playSystemSfx(menu->studio, 2);
                updateOption(option, +1, menu->data);
            }
            else if(menu->items[menu->pos].handler)
                onMenuItem(menu, item);
        }

        if((uli_api_btnp(menu->uli, B, -1, -1)
            || uli_api_keyp(uli, uli_key_backspace, Hold, Period))
                && menu->back)
        {
            playSystemSfx(menu->studio, 2);
            menu->anim.movie = resetMovie(&menu->anim.back);
        }
    }

    s32 i = 0;
    for(const MenuItem *it = menu->items, *end = it + menu->count; it != end; ++it, ++i)
    {
        s32 width = it->option ? menu->maxwidth.item + menu->maxwidth.option + 3 * ULI_FONT_WIDTH : it->width;

        uli_rect rect = {x + (ULI78_WIDTH - width) / 2 + menu->anim.offset,
            y + TextMargin + ItemHeight * (i - menu->pos) - menu->anim.pos, it->width, ULI_FONT_HEIGHT};

        if (it->hotkey != uli_key_unknown && uli_api_keyp(uli, it->hotkey, Hold, Period))
        {
            // hotkeys not supported on options for simplicity
            if(it->option == NULL && it->handler)
                onMenuItem(menu, it);

            menu->pos = it - menu->items; // set pos so that close will call this handler
        }

        bool down = false;
        if(animIdle(menu) && checkMousePos(menu->studio, &rect) && it->handler)
        {
            setCursor(menu->studio, uli_cursor_hand);

            if(checkMouseDown(menu->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(menu->studio, &rect, uli_mouse_left))
            {
                if(it->handler)
                {
                    menu->pos = i;
                    onMenuItem(menu, it);
                }
            }
        }

        if(down)
            uli_api_print(uli, it->label, rect.x, rect.y + 1, uli_color_white, true, 1, false);
        else
            printShadow(uli, it->label, rect.x, rect.y, uli_color_white);

        if(it->option)
        {
            drawOptionArrow(menu, it->option, rect.x + menu->maxwidth.item + ULI_FONT_WIDTH, rect.y, uli_icon_left, -1);
            drawOptionArrow(menu, it->option,
                rect.x + menu->maxwidth.item + it->option->width + 2 * ULI_FONT_WIDTH, rect.y, uli_icon_right, +1);

            printShadow(uli, it->option->values[it->option->pos],
                rect.x + menu->maxwidth.item + 2 * ULI_FONT_WIDTH, rect.y, uli_color_yellow);
        }
    }
}

// BG animation based on DevEd code
void studio_menu_anim(uli_mem* uli, s32 ticks)
{
    CurrentTicks = ticks;
    if(!StarsInitialized)
    {
        srand(0);
        for(int i = 0; i < NUM_STARS; ++i)
        {
            Stars[i].x = (rand() % (ULI78_WIDTH * 2)) - ULI78_WIDTH;
            Stars[i].y = (rand() % (ULI78_HEIGHT * 2)) - ULI78_HEIGHT;
            Stars[i].z = rand() % ULI78_WIDTH;
        }
        StarsInitialized = true;
    }

    uli_api_cls(uli, uli_color_black);

    for(int i = 0; i < NUM_STARS; ++i)
    {
        Stars[i].z -= 0.75f;
        if(Stars[i].z < 1)
        {
            Stars[i].x = (rand() % (ULI78_WIDTH * 2)) - ULI78_WIDTH;
            Stars[i].y = (rand() % (ULI78_HEIGHT * 2)) - ULI78_HEIGHT;
            Stars[i].z = ULI78_WIDTH;
        }

        float k = 128.0f / Stars[i].z;
        s32 sx = Stars[i].x * k + ULI78_WIDTH / 2;
        s32 sy = Stars[i].y * k + ULI78_HEIGHT / 2;

        if(sx >= 0 && sx < ULI78_WIDTH && sy >= 0 && sy < ULI78_HEIGHT)
        {
            float size = (1.0f - Stars[i].z / ULI78_WIDTH) * 2;
            if (size > 0.75)
                uli_api_circb(uli, sx, sy, (s32)size, BG_ANIM_COLOR);
            
            uli_api_pix(uli, sx, sy, uli_color_white, false);
        }
    }
}

void studio_menu_anim_scanline(uli_mem* uli, s32 row, void* data)
{
    s32 val = 127 + sin(CurrentTicks / 30.0f) * 127;
    uli_rgb* dst = uli->ram->vram.palette.colors + BG_ANIM_COLOR;
    *dst = (uli_rgb){(u8)(val/4), (u8)(val/2), (u8)val};
}

static void drawCursor(Menu* menu, s32 x, s32 y)
{
    uli_mem* uli = menu->uli;

    uli_api_rect(uli, x, y - (menu->anim.cursor - ItemHeight) / 2, ULI78_WIDTH, menu->anim.cursor, uli_color_red);
}

Menu* studio_menu_create(Studio* studio)
{
    Menu* menu = malloc(sizeof(Menu));
    *menu = (Menu)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .anim =
        {
            .idle = {.done = emptyDone,},

            .start = MOVIE_DEF(10, startDone,
            {
                {-10, 0, 10, &menu->anim.top, AnimLinear},
                {10, 0, 10, &menu->anim.bottom, AnimLinear},
                {0, 10, 10, &menu->anim.cursor, AnimLinear},
                {-ULI78_WIDTH, 0, 10, &menu->anim.offset, AnimLinear},
            }),

            .up = MOVIE_DEF(9, menuUpDone, {{0, -9, 9, &menu->anim.pos, AnimEaseIn}}),
            .down = MOVIE_DEF(9, menuDownDone, {{0, 9, 9, &menu->anim.pos, AnimEaseIn}}),

            .close = MOVIE_DEF(10, closeDone,
            {
                {0, -10, 10, &menu->anim.top, AnimLinear},
                {0, 10, 10, &menu->anim.bottom, AnimLinear},
                {10, 0, 10, &menu->anim.cursor, AnimLinear},
                {0, ULI78_WIDTH, 10, &menu->anim.offset, AnimLinear},
            }),

            .back = MOVIE_DEF(10, backDone,
            {
                {0, -10, 10, &menu->anim.top, AnimLinear},
                {0, 10, 10, &menu->anim.bottom, AnimLinear},
                {10, 0, 10, &menu->anim.cursor, AnimLinear},
                {0, ULI78_WIDTH, 10, &menu->anim.offset, AnimLinear},
            }),
        },
    };

    return menu;
}

#undef MOVIE_DEF

void studio_menu_tick(Menu* menu)
{
    uli_mem* uli = menu->uli;

    processAnim(menu->anim.movie, menu);

    // process scroll
    if(animIdle(menu))
    {
        uli78_input* input = &uli->ram->input;

        if(input->mouse.scrolly)
        {
            if(uli_api_key(uli, uli_key_ctrl) || uli_api_key(uli, uli_key_shift))
            {
                MenuOption* option = menu->items[menu->pos].option;
                if(option)
                    updateOption(option, input->mouse.scrolly < 0 ? -1 : +1, menu->data);
            }
            else
            {
                s32 pos = menu->pos + (input->mouse.scrolly < 0 ? +1 : -1);
                menu->pos = CLAMP(pos, 0, menu->count - 1);
            }
        }
    }

    if(getStudioMode(menu->studio) != ULI_MENU_MODE)
        return;

    studio_menu_anim(uli, menu->ticks);

    VBANK(uli, 1)
    {
        uli_api_cls(uli, uli->ram->vram.vars.clear = uli_color_blue);
        memcpy(uli->ram->vram.palette.data, getConfig(menu->studio)->cart->bank0.palette.vbank0.data, sizeof(uli_palette));

        drawCursor(menu, 0, (ULI78_HEIGHT - ItemHeight) / 2);
        drawMenu(menu, 0, (ULI78_HEIGHT - ItemHeight) / 2);
        drawTopBar(menu, 0, 0);
        drawBottomBar(menu, 0, ULI78_HEIGHT - ItemHeight);
    }

    menu->ticks++;
}

void studio_menu_init(Menu* menu, const MenuItem* items, s32 rows, s32 pos, s32 backPos, MenuItemHandler back, void* data)
{
    const s32 size = sizeof menu->items[0] * rows;

    *menu = (Menu)
    {
        .studio = menu->studio,
        .uli = menu->uli,
        .anim = menu->anim,
        .items = realloc(menu->items, size),
        .data = data,
        .count = rows,
        .pos = pos,
        .backPos = backPos,
        .back = back,
    };

    memcpy(menu->items, items, size);
    for(MenuItem *it = menu->items, *end = it + menu->count; it != end; ++it)
    {
        it->width = strlen(it->label) * ULI_FONT_WIDTH;

        if(it->option)
        {
            if(menu->maxwidth.item < it->width)
                menu->maxwidth.item = it->width;

            for(const char **opt = it->option->values, **end = opt + it->option->count; opt != end; ++opt)
            {
                s32 len = strlen(*opt) * ULI_FONT_WIDTH;

                if(it->option->width < len)
                    it->option->width = len;

                if(menu->maxwidth.option < len)
                    menu->maxwidth.option = len;
            }

            it->option->pos = it->option->get(menu->data);
        }
    }

    menu->anim.movie = resetMovie(&menu->anim.start);
}

bool studio_menu_back(Menu* menu)
{
    if(menu->back)
    {
        playSystemSfx(menu->studio, 2);
        menu->anim.movie = resetMovie(&menu->anim.back);
    }

    return menu->back != NULL;
}

void studio_menu_free(Menu* menu)
{
    ANIM_STATES(ANIM_FREE);

    FREE(menu->items);
    free(menu);
}
