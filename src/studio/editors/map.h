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

#include "studio/studio.h"
#include "tilesheet.h"

typedef struct Map Map;

struct Map
{
    Studio* studio;
    uli_mem* uli;

    uli_map* src;

    s32 tickCounter;

    enum
    {
        MAP_DRAW_MODE = 0,
        MAP_DRAG_MODE,
        MAP_SELECT_MODE,
        MAP_FILL_MODE,
    } mode;

    struct
    {
        bool grid;
        bool draw;
        uli_point start;
    } canvas;

    struct
    {
        bool keep;
        uli_rect rect;
        uli_point start;
        bool drag;

        uli_blit blit;
    } sheet;

    struct
    {
        s32 x;
        s32 y;

        uli_point start;

        bool active;
        bool gesture;

    } scroll;

    struct
    {
        uli_rect rect;
        uli_point start;
        bool drag;
    } select;

    u8* paste;

    struct History* history;

    struct
    {
        struct
        {
            s32 sheet;
            s32 bank;
            s32 page;
        } pos;

        Movie* movie;

        Movie idle;
        Movie show;
        Movie hide;
        Movie bank;
        Movie page;

    } anim;

    void (*tick)(Map*);
    void (*event)(Map*, StudioEvent);
    void (*scanline)(uli_mem* uli, s32 row, void* data);
};

void initMap(Map*, Studio* studio, uli_map* src);
void freeMap(Map* map);
