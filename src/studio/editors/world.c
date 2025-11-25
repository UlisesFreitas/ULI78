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

#include "world.h"
#include "map.h"

#define PREVIEW_SIZE (ULI78_WIDTH * ULI78_HEIGHT * ULI_PALETTE_BPP / BITS_IN_BYTE)

static void drawGrid(World* world)
{
    uli_mem* uli = world->uli;
    Map* map = world->map;
    u8 color = uli_color_grey;

    for(s32 c = 0; c < ULI78_WIDTH; c += ULI_MAP_SCREEN_WIDTH)
        uli_api_line(world->uli, c, 0, c, ULI78_HEIGHT, color);

    for(s32 r = 0; r < ULI78_HEIGHT; r += ULI_MAP_SCREEN_HEIGHT)
        uli_api_line(world->uli, 0, r, ULI78_WIDTH, r, color);

    uli_api_rectb(world->uli, 0, 0, ULI78_WIDTH, ULI78_HEIGHT, color);

    uli_rect rect = {0, 0, ULI78_WIDTH, ULI78_HEIGHT};

    if(checkMousePos(world->studio, &rect))
    {
        setCursor(world->studio, uli_cursor_hand);

        s32 mx = uli_api_mouse(uli).x;
        s32 my = uli_api_mouse(uli).y;

        if(checkMouseDown(world->studio, &rect, uli_mouse_left))
        {
            map->scroll.x = (mx - ULI_MAP_SCREEN_WIDTH/2) * ULI_SPRITESIZE;
            map->scroll.y = (my - ULI_MAP_SCREEN_HEIGHT/2) * ULI_SPRITESIZE;
            if(map->scroll.x < 0)
                map->scroll.x += ULI_MAP_WIDTH * ULI_SPRITESIZE;
            if(map->scroll.y < 0)
                map->scroll.y += ULI_MAP_HEIGHT * ULI_SPRITESIZE;
        }

        if(checkMouseClick(world->studio, &rect, uli_mouse_left))
            setStudioMode(world->studio, ULI_MAP_MODE);
    }

    s32 x = map->scroll.x / ULI_SPRITESIZE;
    s32 y = map->scroll.y / ULI_SPRITESIZE;

    uli_api_rectb(world->uli, x, y, ULI_MAP_SCREEN_WIDTH+1, ULI_MAP_SCREEN_HEIGHT+1, uli_color_red);

    if(x >= ULI_MAP_WIDTH - ULI_MAP_SCREEN_WIDTH)
        uli_api_rectb(world->uli, x - ULI_MAP_WIDTH, y, ULI_MAP_SCREEN_WIDTH+1, ULI_MAP_SCREEN_HEIGHT+1, uli_color_red);

    if(y >= ULI_MAP_HEIGHT - ULI_MAP_SCREEN_HEIGHT)
        uli_api_rectb(world->uli, x, y - ULI_MAP_HEIGHT, ULI_MAP_SCREEN_WIDTH+1, ULI_MAP_SCREEN_HEIGHT+1, uli_color_red);

    if(x >= ULI_MAP_WIDTH - ULI_MAP_SCREEN_WIDTH && y >= ULI_MAP_HEIGHT - ULI_MAP_SCREEN_HEIGHT)
        uli_api_rectb(world->uli, x - ULI_MAP_WIDTH, y - ULI_MAP_HEIGHT, ULI_MAP_SCREEN_WIDTH+1, ULI_MAP_SCREEN_HEIGHT+1, uli_color_red);
}

static void tick(World* world)
{
    uli_mem* uli = world->uli;

    // process scroll
    if(uli->ram->input.mouse.scrolly > 0)
    {
        setStudioMode(world->studio, ULI_MAP_MODE);
        return;
    }

    if(keyWasPressed(world->studio, uli_key_tab)) setStudioMode(world->studio, ULI_MAP_MODE);

    memcpy(&uli->ram->vram, world->preview, PREVIEW_SIZE);

    VBANK(uli, 1)
    {
        uli_api_cls(uli, uli->ram->vram.vars.clear = uli_color_black);
        memcpy(uli->ram->vram.palette.data, getConfig(world->studio)->cart->bank0.palette.vbank0.data, sizeof(uli_palette));
        drawGrid(world);
    }
}

static void scanline(uli_mem* uli, s32 row, void* data)
{
    World* world = data;
    if(row == 0)
        memcpy(&uli->ram->vram.palette, getBankPalette(world->studio, false), sizeof(uli_palette));
}

void initWorld(World* world, Studio* studio, Map* map)
{
    if(!world->preview)
        world->preview = malloc(PREVIEW_SIZE);

    *world = (World)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .map = map,
        .tick = tick,
        .preview = world->preview,
        .scanline = scanline,
    };

    memset(world->preview, 0, PREVIEW_SIZE);
    s32 colors[ULI_PALETTE_SIZE];

    for(s32 i = 0; i < ULI78_WIDTH * ULI78_HEIGHT; i++)
    {
        u8 index = getBankMap(world->studio)->data[i];

        if(index)
        {
            memset(colors, 0, sizeof colors);

            uli_tile* tile = &getBankTiles(world->studio)->data[index];

            for(s32 p = 0; p < ULI_SPRITESIZE * ULI_SPRITESIZE; p++)
            {
                u8 color = uli_tool_peek4(tile, p);

                if(color)
                    colors[color]++;
            }

            s32 max = 0;

            for(s32 c = 0; c < COUNT_OF(colors); c++)
                if(colors[c] > colors[max]) max = c;

            uli_tool_poke4(world->preview, i, max);
        }
    }
}

void freeWorld(World* world)
{
    free(world->preview);
    free(world);
}