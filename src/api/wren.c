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

#include "core/core.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "tools.h"
#include "wren.h"

extern bool parse_note(const char* noteStr, s32* note, s32* octave);

static WrenHandle* game_class       = NULL;
static WrenHandle* new_handle       = NULL;
static WrenHandle* update_handle    = NULL;
static WrenHandle* boot_handle      = NULL;
static WrenHandle* scanline_handle  = NULL;
static WrenHandle* border_handle    = NULL;
static WrenHandle* menu_handle      = NULL;
static WrenHandle* overline_handle  = NULL;

static bool loaded = false;

static char const* uli_wren_api = "\n\
class ULI {\n\
    foreign static btn()\n\
    foreign static btn(id)\n\
    foreign static btnp(id)\n\
    foreign static btnp(id, hold, period)\n\
    foreign static key(id)\n\
    foreign static keyp(id)\n\
    foreign static keyp(id, hold, period)\n\
    foreign static mouse()\n\
    foreign static font(text)\n\
    foreign static font(text, x, y)\n\
    foreign static font(text, x, y, alpha_color)\n\
    foreign static font(text, x, y, alpha_color, w, h)\n\
    foreign static font(text, x, y, alpha_color, w, h, fixed)\n\
    foreign static font(text, x, y, alpha_color, w, h, fixed, scale)\n\
    foreign static spr(id)\n\
    foreign static spr(id, x, y)\n\
    foreign static spr(id, x, y, alpha_color)\n\
    foreign static spr(id, x, y, alpha_color, scale)\n\
    foreign static spr(id, x, y, alpha_color, scale, flip)\n\
    foreign static spr(id, x, y, alpha_color, scale, flip, rotate)\n\
    foreign static spr(id, x, y, alpha_color, scale, flip, rotate, cell_width, cell_height)\n\
    foreign static map()\n\
    foreign static map(cell_x, cell_y)\n\
    foreign static map(cell_x, cell_y, cell_w, cell_h)\n\
    foreign static map(cell_x, cell_y, cell_w, cell_h, x, y)\n\
    foreign static map(cell_x, cell_y, cell_w, cell_h, x, y, alpha_color)\n\
    foreign static map(cell_x, cell_y, cell_w, cell_h, x, y, alpha_color, scale)\n\
    foreign static mset(cell_x, cell_y)\n\
    foreign static mset(cell_x, cell_y, index)\n\
    foreign static mget(cell_x, cell_y)\n\
    "

#if defined(BUILD_DEPRECATED)
    "\
    foreign static textri(x1, y1, x2, y2, x3, y3, u1, v1, u2, v2, u3, v3)\n\
    foreign static textri(x1, y1, x2, y2, x3, y3, u1, v1, u2, v2, u3, v3, src)\n\
    foreign static textri(x1, y1, x2, y2, x3, y3, u1, v1, u2, v2, u3, v3, src, alpha_color)\n\
    "
#endif

    "\
    foreign static ttri(x1, y1, x2, y2, x3, y3, u1, v1, u2, v2, u3, v3)\n\
    foreign static ttri(x1, y1, x2, y2, x3, y3, u1, v1, u2, v2, u3, v3, src)\n\
    foreign static ttri(x1, y1, x2, y2, x3, y3, u1, v1, u2, v2, u3, v3, src, alpha_color)\n\
    foreign static ttri_depth()\n\
    foreign static ttri_depth(z1, z2, z3)\n\
    foreign static pix(x, y)\n\
    foreign static pix(x, y, color)\n\
    foreign static line(x0, y0, x1, y1, color)\n\
    foreign static circ(x, y, radius, color)\n\
    foreign static circb(x, y, radius, color)\n\
    foreign static elli(x, y, a, b, color)\n\
    foreign static ellib(x, y, a, b, color)\n\
    foreign static paint(x, y, color)\n\
    foreign static paint(x, y, color, bordercolor)\n\
    foreign static rect(x, y, w, h, color)\n\
    foreign static rectb(x, y, w, h, color)\n\
    foreign static tri(x1, y1, x2, y2, x3, y3, color)\n\
    foreign static trib(x1, y1, x2, y2, x3, y3, color)\n\
    foreign static cls()\n\
    foreign static cls(color)\n\
    foreign static clip()\n\
    foreign static clip(x, y, w, h)\n\
    foreign static peek(addr)\n\
    foreign static poke(addr, val)\n\
    foreign static peek(addr, bits)\n\
    foreign static poke(addr, val, bits)\n\
    foreign static peek1(addr)\n\
    foreign static poke1(addr, val)\n\
    foreign static peek2(addr)\n\
    foreign static poke2(addr, val)\n\
    foreign static peek4(addr)\n\
    foreign static poke4(addr, val)\n\
    foreign static memcpy(dst, src, size)\n\
    foreign static memset(dst, src, size)\n\
    foreign static pmem(index)\n\
    foreign static pmem(index, val)\n\
    foreign static sfx(id)\n\
    foreign static sfx(id, note)\n\
    foreign static sfx(id, note, duration)\n\
    foreign static sfx(id, note, duration, channel)\n\
    foreign static sfx(id, note, duration, channel, volume)\n\
    foreign static sfx(id, note, duration, channel, volume, speed)\n\
    foreign static music()\n\
    foreign static music(track)\n\
    foreign static music(track, frame)\n\
    foreign static music(track, frame, row)\n\
    foreign static music(track, frame, row, loop)\n\
    foreign static music(track, frame, row, loop, sustain)\n\
    foreign static music(track, frame, row, loop, sustain, tempo)\n\
    foreign static music(track, frame, row, loop, sustain, tempo, speed)\n\
    foreign static time()\n\
    foreign static tstamp()\n\
    foreign static vbank()\n\
    foreign static vbank(bank)\n\
    foreign static sync()\n\
    foreign static sync(mask)\n\
    foreign static sync(mask, bank)\n\
    foreign static sync(mask, bank, tocart)\n\
    foreign static reset()\n\
    foreign static exit()\n\
    foreign static fft(start_freq, end_freq)\n\
    foreign static ffts(start_freq, end_freq)\n\
    foreign static map_width__\n\
    foreign static map_height__\n\
    foreign static spritesize__\n\
    foreign static print__(v, x, y, color, fixed, scale, alt)\n\
    foreign static trace__(msg, color)\n\
    foreign static spr__(id, x, y, alpha_color, scale, flip, rotate)\n\
    foreign static fget(index, flag)\n\
    foreign static fset(index, flag, val)\n\
    foreign static mgeti__(index)\n\
    static print(v) { ULI.print__(v.toString, 0, 0, 15, false, 1, false) }\n\
    static print(v,x,y) { ULI.print__(v.toString, x, y, 15, false, 1, false) }\n\
    static print(v,x,y,color) { ULI.print__(v.toString, x, y, color, false, 1, false) }\n\
    static print(v,x,y,color,fixed) { ULI.print__(v.toString, x, y, color, fixed, 1, false) }\n\
    static print(v,x,y,color,fixed,scale) { ULI.print__(v.toString, x, y, color, fixed, scale, false) }\n\
    static print(v,x,y,color,fixed,scale,alt) { ULI.print__(v.toString, x, y, color, fixed, scale, alt) }\n\
    static trace(v) { ULI.trace__(v.toString, 15) }\n\
    static trace(v,color) { ULI.trace__(v.toString, color) }\n\
    static map(cell_x, cell_y, cell_w, cell_h, x, y, alpha_color, scale, remap) {\n\
        var map_w = ULI.map_width__\n\
        var map_h = ULI.map_height__\n\
        var size = ULI.spritesize__ * scale\n\
        var jj = y\n\
        var ii = x\n\
        var flip = 0\n\
        var rotate = 0\n\
        for (j in cell_y...cell_y+cell_h) {\n\
            ii = x\n\
            for (i in cell_x...cell_x+cell_w) {\n\
                var mi = i\n\
                var mj = j\n\
                while(mi < 0) mi = mi + map_w\n\
                while(mj < 0) mj = mj + map_h\n\
                while(mi >= map_w) mi = mi - map_w\n\
                while(mj >= map_h) mj = mj - map_h\n\
                var index = mi + mj * map_w\n\
                var tile_index = ULI.mgeti__(index)\n\
                var ret = remap.call(tile_index, mi, mj)\n\
                if (ret.type == List) {\n\
                    tile_index = ret[0]\n\
                    flip = ret[1]\n\
                    rotate = ret[2]\n\
                } else if (ret.type == Num) {\n\
                    tile_index = ret\n\
                }\n\
                ULI.spr__(tile_index, ii, jj, alpha_color, scale, flip, rotate)\n\
                ii = ii + size\n\
            }\n\
            jj = jj + size\n\
        }\n\
    }\n\
}\n";
}\n\
" ULI_FN "(){}\n\
" BOOT_FN "(){}\n\
" SCN_FN "(row){}\n\
" BDR_FN "(row){}\n\
" MENU_FN "(index){}\n\
" OVR_FN "(){}\n\
";

static inline void wrenError(WrenVM* vm, const char* msg)
{
    wrenEnsureSlots(vm, 1);
    wrenSetSlotString(vm, 0, msg);
    wrenAbortFiber(vm, 0);
}

static inline s32 getWrenNumber(WrenVM* vm, s32 index)
{
    return (s32)wrenGetSlotDouble(vm, index);
}

static inline bool isNumber(WrenVM* vm, s32 index)
{
    return wrenGetSlotType(vm, index) == WREN_TYPE_NUM;
}

static inline bool isString(WrenVM* vm, s32 index)
{
    return wrenGetSlotType(vm, index) == WREN_TYPE_STRING;
}

static inline bool isList(WrenVM* vm, s32 index)
{
    return wrenGetSlotType(vm, index) == WREN_TYPE_LIST;
}

static void closeWren(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;
    if(core->currentVM)
    {
        // release handles
        if (loaded)
        {
            wrenReleaseHandle(core->currentVM, new_handle);
            wrenReleaseHandle(core->currentVM, update_handle);
            wrenReleaseHandle(core->currentVM, boot_handle);
            wrenReleaseHandle(core->currentVM, scanline_handle);
            wrenReleaseHandle(core->currentVM, border_handle);
            wrenReleaseHandle(core->currentVM, menu_handle);
            wrenReleaseHandle(core->currentVM, overline_handle);
            if (game_class != NULL)
            {
                wrenReleaseHandle(core->currentVM, game_class);
            }
        }

        wrenFreeVM(core->currentVM);
        core->currentVM = NULL;

    }
    loaded = false;
}

static uli_core* getWrenCore(WrenVM* vm)
{
    uli_core* core = wrenGetUserData(vm);

    return core;
}

static void wren_map_width(WrenVM* vm)
{
    wrenSetSlotDouble(vm, 0, ULI_MAP_WIDTH);
}

static void wren_map_height(WrenVM* vm)
{
    wrenSetSlotDouble(vm, 0, ULI_MAP_HEIGHT);
}

static void wren_mgeti(WrenVM* vm)
{
    s32 index = getWrenNumber(vm, 1);

    if(index < 0 || index >= ULI_MAP_WIDTH * ULI_MAP_HEIGHT)
    {
        wrenSetSlotDouble(vm, 0, 0);
        return;
    }

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;
    wrenSetSlotDouble(vm, 0, *(uli->ram->map.data + index));
}

static void wren_spritesize(WrenVM* vm)
{
    wrenSetSlotDouble(vm, 0, ULI_SPRITESIZE);
}

static void wren_btn(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);
    uli_mem* uli = (uli_mem*)core;

    s32 top = wrenGetSlotCount(vm);

    if (top == 1)
    {
        wrenSetSlotDouble(vm, 0, core->api.btn(uli, -1));
    }
    else if (top == 2)
    {
        bool pressed = core->api.btn(uli, getWrenNumber(vm, 1) & 0x1f);
        wrenSetSlotBool(vm, 0, pressed);
    }

}

static void wren_btnp(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);
    uli_mem* uli = (uli_mem*)core;

    s32 top = wrenGetSlotCount(vm);

    if (top == 1)
    {
        wrenSetSlotBool(vm, 0, core->api.btnp(uli, -1, -1, -1));
    }
    else if(top == 2)
    {
        s32 index = getWrenNumber(vm, 1) & 0xf;

        wrenSetSlotBool(vm, 0, core->api.btnp(uli, index, -1, -1));
    }
    else if (top == 4)
    {
        s32 index = getWrenNumber(vm, 1) & 0xf;
        u32 hold = getWrenNumber(vm, 2);
        u32 period = getWrenNumber(vm, 3);

        wrenSetSlotBool(vm, 0, core->api.btnp(uli, index, hold, period));
    }
}

static void wren_key(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);
    uli_mem* uli = &core->memory;

    s32 top = wrenGetSlotCount(vm);

    if (top == 1)
    {
        wrenSetSlotBool(vm, 0, core->api.key(uli, uli_key_unknown));
    }
    else if (top == 2)
    {
        uli_key key = getWrenNumber(vm, 1);

        if(key < uli_keys_count)
            wrenSetSlotBool(vm, 0, core->api.key(uli, key));
        else
        {
            wrenError(vm, "unknown keyboard code\n");
            return;
        }
    }
}

static void wren_keyp(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);
    uli_mem* uli = &core->memory;

    s32 top = wrenGetSlotCount(vm);

    if (top == 1)
    {
        wrenSetSlotBool(vm, 0, core->api.keyp(uli, uli_key_unknown, -1, -1));
    }
    else
    {
        uli_key key = getWrenNumber(vm, 1);

        if(key >= uli_keys_count)
        {
            wrenError(vm, "unknown keyboard code\n");
        }
        else
        {
            if(top == 2)
            {
                wrenSetSlotBool(vm, 0, core->api.keyp(uli, key, -1, -1));
            }
            else if(top == 4)
            {
                u32 hold = getWrenNumber(vm, 2);
                u32 period = getWrenNumber(vm, 3);

                wrenSetSlotBool(vm, 0, core->api.keyp(uli, key, hold, period));
            }
        }
    }
}


static void wren_mouse(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);

    const uli78_mouse* mouse = &core->memory.ram->input.mouse;

    wrenEnsureSlots(vm, 6);
    wrenSetSlotNewList(vm, 0);

    {
        uli_point pos = core->api.mouse((uli_mem*)core);

        wrenSetSlotDouble(vm, 1, pos.x);
        wrenInsertInList(vm, 0, 0, 1);

        wrenSetSlotDouble(vm, 1, pos.y);
        wrenInsertInList(vm, 0, 1, 1);
    }

    wrenSetSlotBool(vm, 1, mouse->left ? true : false);
    wrenInsertInList(vm, 0, 2, 1);
    wrenSetSlotBool(vm, 1, mouse->middle ? true : false);
    wrenInsertInList(vm, 0, 3, 1);
    wrenSetSlotBool(vm, 1, mouse->right ? true : false);
    wrenInsertInList(vm, 0, 4, 1);
    wrenSetSlotDouble(vm, 1, mouse->scrollx);
    wrenInsertInList(vm, 0, 5, 1);
    wrenSetSlotDouble(vm, 1, mouse->scrolly);
    wrenInsertInList(vm, 0, 6, 1);
}

static void wren_print(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    const char* text = wrenGetSlotString(vm, 1);

    s32 x = getWrenNumber(vm, 2);
    s32 y = getWrenNumber(vm, 3);

    s32 color = getWrenNumber(vm, 4) % ULI_PALETTE_SIZE;

    bool fixed = wrenGetSlotBool(vm, 5);

    s32 scale = getWrenNumber(vm, 6);

    if(scale == 0)
    {
        wrenSetSlotDouble(vm, 0, 0);
        return;
    }

    bool alt = wrenGetSlotBool(vm, 7);

    s32 size = core->api.print(uli, text, x, y, color, fixed, scale, alt);

    wrenSetSlotDouble(vm, 0, size);
}

static void wren_font(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;
    s32 top = wrenGetSlotCount(vm);

    if(top > 1)
    {
        const char* text = NULL;
        if (isString(vm, 1))
        {
            text = wrenGetSlotString(vm, 1);
        }

        s32 x = 0;
        s32 y = 0;
        s32 width = ULI_SPRITESIZE;
        s32 height = ULI_SPRITESIZE;
        u8 chromakey = 0;
        bool fixed = false;
        s32 scale = 1;
        bool alt = false;

        if(top > 3)
        {
            x = getWrenNumber(vm, 2);
            y = getWrenNumber(vm, 3);

            if(top > 4)
            {
                chromakey = getWrenNumber(vm, 4);

                if(top > 6)
                {
                    width = getWrenNumber(vm, 5);
                    height = getWrenNumber(vm, 6);

                    if(top > 7)
                    {
                        fixed = wrenGetSlotBool(vm, 7);

                        if(top > 8)
                        {
                            scale = getWrenNumber(vm, 8);

                            if(top > 9)
                            {
                                alt = wrenGetSlotBool(vm, 9);
                            }
                        }
                    }
                }
            }
        }

        if(scale == 0)
        {
            wrenSetSlotDouble(vm, 0, 0);
            return;
        }

        s32 size = core->api.font(uli, text ? text : "null", x, y, &chromakey, 1, width, height, fixed, scale, alt);
        wrenSetSlotDouble(vm, 0, size);
    }
}

static void wren_trace(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    const char* text = wrenGetSlotString(vm, 1);
    u8 color = (u8)getWrenNumber(vm, 2);

    core->api.trace(uli, text, color);
}

static void wren_spr(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    s32 index = 0;
    s32 x = 0;
    s32 y = 0;
    s32 w = 1;
    s32 h = 1;
    s32 scale = 1;
    uli_flip flip = uli_no_flip;
    uli_rotate rotate = uli_no_rotate;
    static u8 colors[ULI_PALETTE_SIZE];
    s32 count = 0;

    if(top > 1)
    {
        index = getWrenNumber(vm, 1);

        if(top > 3)
        {
            x = getWrenNumber(vm, 2);
            y = getWrenNumber(vm, 3);

            if(top > 4)
            {
                if(isList(vm, 4))
                {
                    wrenEnsureSlots(vm, top+1);
                    s32 list_count = wrenGetListCount(vm, 4);
                    for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
                    {
                        wrenGetListElement(vm, 4, i, top);
                        if(i < list_count && isNumber(vm, top))
                        {
                            colors[i] = getWrenNumber(vm, top);
                            count++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    colors[0] = getWrenNumber(vm, 4);
                    count = 1;
                }

                if(top > 5)
                {
                    scale = getWrenNumber(vm, 5);

                    if(top > 6)
                    {
                        flip = getWrenNumber(vm, 6);

                        if(top > 7)
                        {
                            rotate = getWrenNumber(vm, 7);

                            if(top > 9)
                            {
                                w = getWrenNumber(vm, 8);
                                h = getWrenNumber(vm, 9);
                            }
                        }
                    }
                }
            }
        }
    }

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.spr(uli, index, x, y, w, h, colors, count, scale, flip, rotate);
}

static void wren_spr_internal(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    s32 index = getWrenNumber(vm, 1);
    s32 x = getWrenNumber(vm, 2);
    s32 y = getWrenNumber(vm, 3);

    static u8 colors[ULI_PALETTE_SIZE];
    s32 count = 0;

    if(isList(vm, 4))
    {
        wrenEnsureSlots(vm, top+1);
        s32 list_count = wrenGetListCount(vm, 4);
        for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
        {
            wrenGetListElement(vm, 4, i, top);
            if(i < list_count && isNumber(vm, top))
            {
                colors[i] = getWrenNumber(vm, top);
                count++;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        colors[0] = getWrenNumber(vm, 4);
        count = 1;
    }

    s32 scale = getWrenNumber(vm, 5);
    s32 flip = getWrenNumber(vm, 6);
    s32 rotate = getWrenNumber(vm, 7);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.spr(uli, index, x, y, 1, 1, colors, count, scale, flip, rotate);
}

static void wren_map(WrenVM* vm)
{
    s32 x = 0;
    s32 y = 0;
    s32 w = ULI_MAP_SCREEN_WIDTH;
    s32 h = ULI_MAP_SCREEN_HEIGHT;
    s32 sx = 0;
    s32 sy = 0;
    s32 scale = 1;
    static u8 colors[ULI_PALETTE_SIZE];
    s32 count = 0;

    s32 top = wrenGetSlotCount(vm);

    if(top > 2)
    {
        x = getWrenNumber(vm, 1);
        y = getWrenNumber(vm, 2);

        if(top > 4)
        {
            w = getWrenNumber(vm, 3);
            h = getWrenNumber(vm, 4);

            if(top > 6)
            {
                sx = getWrenNumber(vm, 5);
                sy = getWrenNumber(vm, 6);

                if(top > 7)
                {
                    if(isList(vm, 7))
                    {
                        wrenEnsureSlots(vm, top+1);
                        s32 list_count = wrenGetListCount(vm, 7);
                        for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
                        {
                            wrenGetListElement(vm, 7, i, top);
                            if(i < list_count && isNumber(vm, top))
                            {
                                colors[i] = getWrenNumber(vm, top);
                                count++;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        colors[0] = getWrenNumber(vm, 7);
                        count = 1;
                    }

                    if(top > 8)
                    {
                        scale = getWrenNumber(vm, 8);
                    }
                }
            }
        }
    }

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.map(uli, x, y, w, h, sx, sy, colors, count, scale, NULL, NULL);
}

static void wren_mset(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    u8 value = getWrenNumber(vm, 3);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.mset(uli, x, y, value);
}

static void wren_mget(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    u8 value = core->api.mget(uli, x, y);
    wrenSetSlotDouble(vm, 0, value);
}

static struct
{
    float z[3];
    bool on;
} depth = {0};

static void wren_ttri_depth(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    depth.on = false;

    if (top == 4)
    {
        for (s32 i = 0; i < COUNT_OF(depth.z); i++)
            depth.z[i] = (float)wrenGetSlotDouble(vm, i + 1);

        depth.on = true;
    }
}

static void wren_ttri(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    float pt[12];

    for (s32 i = 0; i < COUNT_OF(pt); i++)
    {
        pt[i] = (float)wrenGetSlotDouble(vm, i + 1);
    }

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;
    static u8 colors[ULI_PALETTE_SIZE];
    s32 count = 0;
    uli_texture_src src = uli_tiles_texture;

    //  check for texture source
    if (top > 13)
    {
        src = getWrenNumber(vm, 13);
    }

    //  check for chroma
    if(isList(vm, 14))
    {
        wrenEnsureSlots(vm, top+1);
        s32 list_count = wrenGetListCount(vm, 14);
        for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
        {
            wrenGetListElement(vm, 14, i, top);
            if(i < list_count && isNumber(vm, top))
            {
                colors[i] = getWrenNumber(vm, top);
                count++;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        colors[0] = getWrenNumber(vm, 14);
        count = 1;
    }

    core->api.ttri(uli,
        pt[0], pt[1],   //  xy 1
        pt[2], pt[3],   //  xy 2
        pt[4], pt[5],   //  xy 3
        pt[6], pt[7],   //  uv 1
        pt[8], pt[9],   //  uv 2
        pt[10], pt[11], //  uv 3
        src,            // texture source
        colors, count,  // chroma
        depth.z[0], depth.z[1], depth.z[2], depth.on); // depth
}

#if defined(BUILD_DEPRECATED)

static void wren_textri(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    float pt[12];

    for (s32 i = 0; i < COUNT_OF(pt); i++)
    {
        pt[i] = (float)wrenGetSlotDouble(vm, i + 1);
    }

    uli_core* core = getWrenCore(vm);
    uli_mem* uli = (uli_mem*)core;
    static u8 colors[ULI_PALETTE_SIZE];
    s32 count = 0;
    uli_texture_src src = uli_tiles_texture;

    //  check for texture source
    if (top > 13)
    {
        src = getWrenNumber(vm, 13);
    }

    //  check for chroma
    if(isList(vm, 14))
    {
        wrenEnsureSlots(vm, top+1);
        s32 list_count = wrenGetListCount(vm, 14);
        for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
        {
            wrenGetListElement(vm, 14, i, top);
            if(i < list_count && isNumber(vm, top))
            {
                colors[i] = getWrenNumber(vm, top);
                count++;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        colors[0] = getWrenNumber(vm, 14);
        count = 1;
    }

    core->api.textri(uli,
        pt[0], pt[1],   //  xy 1
        pt[2], pt[3],   //  xy 2
        pt[4], pt[5],   //  xy 3
        pt[6], pt[7],   //  uv 1
        pt[8], pt[9],   //  uv 2
        pt[10], pt[11], //  uv 3
        src,            // texture source
        colors, count);
}

#endif

static void wren_pix(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    if(top > 3)
    {
        s32 color = getWrenNumber(vm, 3);
        core->api.pix(uli, x, y, color, false);
    }
    else
    {
        wrenSetSlotDouble(vm, 0, core->api.pix(uli, x, y, 0, true));
    }
}

static void wren_line(WrenVM* vm)
{
    float x0 = (float)wrenGetSlotDouble(vm, 1);
    float y0 = (float)wrenGetSlotDouble(vm, 2);
    float x1 = (float)wrenGetSlotDouble(vm, 3);
    float y1 = (float)wrenGetSlotDouble(vm, 4);
    s32 color = getWrenNumber(vm, 5);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.line(uli, x0, y0, x1, y1, color);
}

static void wren_circ(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    s32 radius = getWrenNumber(vm, 3);
    s32 color = getWrenNumber(vm, 4);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.circ(uli, x, y, radius, color);
}

static void wren_circb(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    s32 radius = getWrenNumber(vm, 3);
    s32 color = getWrenNumber(vm, 4);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.circb(uli, x, y, radius, color);
}

static void wren_elli(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    s32 a = getWrenNumber(vm, 3);
    s32 b = getWrenNumber(vm, 4);
    s32 color = getWrenNumber(vm, 5);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.elli(uli, x, y, a, b, color);
}

static void wren_ellib(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    s32 a = getWrenNumber(vm, 3);
    s32 b = getWrenNumber(vm, 4);
    s32 color = getWrenNumber(vm, 5);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.ellib(uli, x, y, a, b, color);
}

static void wren_paint(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    s32 color = getWrenNumber(vm, 3);
    s32 bordercolor = top > 4 ? getWrenNumber(vm, 4) : -1;

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.paint(uli, x, y, color, bordercolor);
}

static void wren_rect(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    s32 w = getWrenNumber(vm, 3);
    s32 h = getWrenNumber(vm, 4);
    s32 color = getWrenNumber(vm, 5);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.rect(uli, x, y, w, h, color);
}

static void wren_rectb(WrenVM* vm)
{
    s32 x = getWrenNumber(vm, 1);
    s32 y = getWrenNumber(vm, 2);
    s32 w = getWrenNumber(vm, 3);
    s32 h = getWrenNumber(vm, 4);
    s32 color = getWrenNumber(vm, 5);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.rectb(uli, x, y, w, h, color);
}

static void wren_tri(WrenVM* vm)
{
    float pt[6];

    for(s32 i = 0; i < COUNT_OF(pt); i++)
    {
        pt[i] = (float)wrenGetSlotDouble(vm, i + 1);
    }

    s32 color = getWrenNumber(vm, 7);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.tri(uli, pt[0], pt[1], pt[2], pt[3], pt[4], pt[5], color);
}

static void wren_trib(WrenVM* vm)
{
    float pt[6];

    for(s32 i = 0; i < COUNT_OF(pt); i++)
    {
        pt[i] = (float)wrenGetSlotDouble(vm, i + 1);
    }

    s32 color = getWrenNumber(vm, 7);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.trib(uli, pt[0], pt[1], pt[2], pt[3], pt[4], pt[5], color);
}

static void wren_cls(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.cls(uli, top == 1 ? 0 : getWrenNumber(vm, 1));
}

static void wren_clip(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    if(top == 1)
    {
        core->api.clip(uli, 0, 0, ULI78_WIDTH, ULI78_HEIGHT);
    }
    else
    {
        s32 x = getWrenNumber(vm, 1);
        s32 y = getWrenNumber(vm, 2);
        s32 w = getWrenNumber(vm, 3);
        s32 h = getWrenNumber(vm, 4);

        core->api.clip(uli, x, y, w, h);
    }
}

static void wren_peek(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);
    s32 bits = BITS_IN_BYTE;

    if(wrenGetSlotCount(vm) > 2)
        bits = getWrenNumber(vm, 2);

    wrenSetSlotDouble(vm, 0, core->api.peek(uli, address, bits));
}

static void wren_poke(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);
    u8 value = getWrenNumber(vm, 2) & 0xff;
    s32 bits = BITS_IN_BYTE;
    if(wrenGetSlotCount(vm) > 3)
        bits = getWrenNumber(vm, 3);

    core->api.poke(uli, address, value, bits);
}

static void wren_peek1(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);

    wrenSetSlotDouble(vm, 0, core->api.peek1(uli, address));
}

static void wren_poke1(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);
    u8 value = getWrenNumber(vm, 2);

    core->api.poke1(uli, address, value);
}

static void wren_peek2(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);

    wrenSetSlotDouble(vm, 0, core->api.peek2(uli, address));
}

static void wren_poke2(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);
    u8 value = getWrenNumber(vm, 2);

    core->api.poke2(uli, address, value);
}

static void wren_peek4(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);

    wrenSetSlotDouble(vm, 0, core->api.peek4(uli, address));
}

static void wren_poke4(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 address = getWrenNumber(vm, 1);
    u8 value = getWrenNumber(vm, 2);

    core->api.poke4(uli, address, value);
}

static void wren_memcpy(WrenVM* vm)
{
    s32 dest = getWrenNumber(vm, 1);
    s32 src = getWrenNumber(vm, 2);
    s32 size = getWrenNumber(vm, 3);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.memcpy(uli, dest, src, size);
}

static void wren_memset(WrenVM* vm)
{
    s32 dest = getWrenNumber(vm, 1);
    u8 value = getWrenNumber(vm, 2);
    s32 size = getWrenNumber(vm, 3);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.memset(uli, dest, value, size);
}

static void wren_pmem(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    u32 index = getWrenNumber(vm, 1);

    if(index < ULI_PERSISTENT_SIZE)
    {
        u32 val = core->api.pmem(uli, index, 0, false);

        if(top > 2)
        {
            core->api.pmem(uli, index, getWrenNumber(vm, 2), true);
        }

        wrenSetSlotDouble(vm, 0, val);
    }
    else wrenError(vm, "invalid persistent uli index\n");
}

static void wren_sfx(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 index = getWrenNumber(vm, 1);

    if(index < SFX_COUNT)
    {

        s32 note = -1;
        s32 octave = -1;
        s32 duration = -1;
        s32 channel = 0;
        s32 volumes[ULI78_SAMPLE_CHANNELS] = {MAX_VOLUME, MAX_VOLUME};
        s32 speed = SFX_DEF_SPEED;

        if (index >= 0)
        {
            uli_sample* effect = uli->ram->sfx.samples.data + index;

            note = effect->note;
            octave = effect->octave;
            speed = effect->speed;
        }

        if(top > 2)
        {
            if(isNumber(vm, 2))
            {
                s32 id = getWrenNumber(vm, 2);
                note = id % NOTES;
                octave = id / NOTES;
            }
            else if(isString(vm, 2))
            {
                const char* noteStr = wrenGetSlotString(vm, 2);

                if(!parse_note(noteStr, &note, &octave))
                {
                    wrenError(vm, "invalid note, should be like C#4\n");
                    return;
                }
            }

            if(top > 3)
            {
                duration = getWrenNumber(vm, 3);

                if(top > 4)
                {
                    channel = getWrenNumber(vm, 4);

                    if(top > 5)
                    {
                        if(isList(vm, 5) && wrenGetListCount(vm, 5) == COUNT_OF(volumes))
                        {
                            for(s32 i = 0; i < COUNT_OF(volumes); i++)
                            {
                                wrenGetListElement(vm, 5, i, top);
                                if(isNumber(vm, top))
                                    volumes[i] = getWrenNumber(vm, top);
                            }
                        }
                        else volumes[0] = volumes[1] = getWrenNumber(vm, 5);

                        if(top > 6)
                        {
                            speed = getWrenNumber(vm, 6);
                        }
                    }
                }
            }
        }

        if (channel >= 0 && channel < ULI_SOUND_CHANNELS)
        {
            core->api.sfx(uli, index, note, octave, duration, channel, volumes[0] & 0xf, volumes[1] & 0xf, speed);
        }
        else wrenError(vm, "unknown channel\n");
    }
    else wrenError(vm, "unknown sfx index\n");
}

static void wren_music(WrenVM* vm)
{
    s32 top = wrenGetSlotCount(vm);

    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 track = -1;
    s32 frame = -1;
    s32 row = -1;
    bool loop = true;
    bool sustain = false;
    s32 tempo = -1;
    s32 speed = -1;

    if(top > 1)
    {
        track = getWrenNumber(vm, 1);

        if(track > MUSIC_TRACKS - 1)
        {
            wrenError(vm, "invalid music track index");
            return;
        }

        if(top > 2)
        {
            frame = getWrenNumber(vm, 2);

            if(top > 3)
            {
                row = getWrenNumber(vm, 3);

                if(top > 4)
                {
                    loop = wrenGetSlotBool(vm, 4);

                    if(top > 5)
                    {
                        sustain = wrenGetSlotBool(vm, 5);

                        if (top > 6)
                        {
                            tempo = getWrenNumber(vm, 6);

                            if (top > 7)
                            {
                                speed = getWrenNumber(vm, 7);
                            }
                        }
                    }
                }
            }
        }
    }

    core->api.music(uli, track, frame, row, loop, sustain, tempo, speed);
}

static void wren_time(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    wrenSetSlotDouble(vm, 0, core->api.time(uli));
}

static void wren_tstamp(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    wrenSetSlotDouble(vm, 0, core->api.tstamp(uli));
}

static void wren_vbank(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);
    uli_mem* uli = (uli_mem*)core;

    s32 prev = core->state.vbank.id;

    if(wrenGetSlotCount(vm) == 2)
        core->api.vbank(uli, getWrenNumber(vm, 1));

    wrenSetSlotDouble(vm, 0, prev);
}

static void wren_sync(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    bool toCart = false;
    u32 mask = 0;
    s32 bank = 0;

    s32 top = wrenGetSlotCount(vm);

    if(top > 1)
    {
        mask = getWrenNumber(vm, 1);

        if(top > 2)
        {
            bank = getWrenNumber(vm, 2);

            if(top > 3)
            {
                toCart = wrenGetSlotBool(vm, 3);
            }
        }
    }

    if(bank >= 0 && bank < ULI_BANKS)
        core->api.sync(uli, mask, bank, toCart);
    else wrenError(vm, "sync() error, invalid bank");
}

static void wren_reset(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);

    core->state.initialized = false;
}

static void wren_exit(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;
    core->api.exit(uli);
}

static void wren_fget(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;

    s32 top = wrenGetSlotCount(vm);

    if(top > 1)
    {
        u32 index = getWrenNumber(vm, 1);

        if(top > 2)
        {
            u32 flag = getWrenNumber(vm, 2);
            wrenSetSlotBool(vm, 0, core->api.fget(uli, index, flag));
            return;
        }
    }

    wrenError(vm, "invalid params, fget(sprite,flag)\n");
}

static void wren_fset(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm); uli_mem* uli = (uli_mem*)core;
    s32 top = wrenGetSlotCount(vm);

    if(top > 1)
    {
        u32 index = getWrenNumber(vm, 1);

        if(top > 2)
        {
            u32 flag = getWrenNumber(vm, 2);

            if(top > 3)
            {
                bool value = wrenGetSlotBool(vm, 3);
                core->api.fset(uli, index, flag, value);
                return;
            }
        }
    }

    wrenError(vm, "invalid params, fset(sprite,flag,value)\n");
}

static void wren_fft(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);
    uli_mem* uli = (uli_mem*)core;
    s32 top = wrenGetSlotCount(vm);

    if (top > 1)
    {
        double start_freq = getWrenNumber(vm, 1);
        double end_freq = -1;

        if (top > 2)
            end_freq = getWrenNumber(vm, 2);

        wrenSetSlotDouble(vm, 0, core->api.fft(uli, start_freq, end_freq));
        return;
    }

    wrenError(vm, "invalid params, fft(start_freq, end_freq)\n");
}

static void wren_ffts(WrenVM* vm)
{
    uli_core* core = getWrenCore(vm);
    uli_mem* uli = (uli_mem*)core;
    s32 top = wrenGetSlotCount(vm);

    if (top > 1)
    {
        double start_freq = getWrenNumber(vm, 1);
        double end_freq = -1;

        if (top > 2)
            end_freq = getWrenNumber(vm, 2);

        wrenSetSlotDouble(vm, 0, core->api.ffts(uli, start_freq, end_freq));
        return;
    }

    wrenError(vm, "invalid params, ffts(start_freq, end_freq)\n");
}

static WrenForeignMethodFn foreignTicMethods(const char* signature)
{
    if (strcmp(signature, "static ULI.btn()"                    ) == 0) return wren_btn;
    if (strcmp(signature, "static ULI.btn(_)"                   ) == 0) return wren_btn;
    if (strcmp(signature, "static ULI.btnp(_)"                  ) == 0) return wren_btnp;
    if (strcmp(signature, "static ULI.btnp(_,_,_)"              ) == 0) return wren_btnp;
    if (strcmp(signature, "static ULI.key(_)"                   ) == 0) return wren_key;
    if (strcmp(signature, "static ULI.keyp(_)"                  ) == 0) return wren_keyp;
    if (strcmp(signature, "static ULI.keyp(_,_,_)"              ) == 0) return wren_keyp;
    if (strcmp(signature, "static ULI.mouse()"                  ) == 0) return wren_mouse;

    if (strcmp(signature, "static ULI.font(_)"                  ) == 0) return wren_font;
    if (strcmp(signature, "static ULI.font(_,_,_)"              ) == 0) return wren_font;
    if (strcmp(signature, "static ULI.font(_,_,_,_)"            ) == 0) return wren_font;
    if (strcmp(signature, "static ULI.font(_,_,_,_,_,_)"        ) == 0) return wren_font;
    if (strcmp(signature, "static ULI.font(_,_,_,_,_,_,_)"      ) == 0) return wren_font;
    if (strcmp(signature, "static ULI.font(_,_,_,_,_,_,_,_)"    ) == 0) return wren_font;

    if (strcmp(signature, "static ULI.spr(_)"                   ) == 0) return wren_spr;
    if (strcmp(signature, "static ULI.spr(_,_,_)"               ) == 0) return wren_spr;
    if (strcmp(signature, "static ULI.spr(_,_,_,_)"             ) == 0) return wren_spr;
    if (strcmp(signature, "static ULI.spr(_,_,_,_,_)"           ) == 0) return wren_spr;
    if (strcmp(signature, "static ULI.spr(_,_,_,_,_,_)"         ) == 0) return wren_spr;
    if (strcmp(signature, "static ULI.spr(_,_,_,_,_,_,_)"       ) == 0) return wren_spr;
    if (strcmp(signature, "static ULI.spr(_,_,_,_,_,_,_,_,_)"   ) == 0) return wren_spr;

    if (strcmp(signature, "static ULI.map()"                    ) == 0) return wren_map;
    if (strcmp(signature, "static ULI.map(_,_)"                 ) == 0) return wren_map;
    if (strcmp(signature, "static ULI.map(_,_,_,_)"             ) == 0) return wren_map;
    if (strcmp(signature, "static ULI.map(_,_,_,_,_,_)"         ) == 0) return wren_map;
    if (strcmp(signature, "static ULI.map(_,_,_,_,_,_,_)"       ) == 0) return wren_map;
    if (strcmp(signature, "static ULI.map(_,_,_,_,_,_,_,_)"     ) == 0) return wren_map;

    if (strcmp(signature, "static ULI.mset(_,_)"                ) == 0) return wren_mset;
    if (strcmp(signature, "static ULI.mset(_,_,_)"              ) == 0) return wren_mset;
    if (strcmp(signature, "static ULI.mget(_,_)"                ) == 0) return wren_mget;

#if defined(BUILD_DEPRECATED)
    if (strcmp(signature, "static ULI.textri(_,_,_,_,_,_,_,_,_,_,_,_)"      ) == 0) return wren_textri;
    if (strcmp(signature, "static ULI.textri(_,_,_,_,_,_,_,_,_,_,_,_,_)"    ) == 0) return wren_textri;
    if (strcmp(signature, "static ULI.textri(_,_,_,_,_,_,_,_,_,_,_,_,_,_)"  ) == 0) return wren_textri;
#endif

    if (strcmp(signature, "static ULI.ttri(_,_,_,_,_,_,_,_,_,_,_,_)"        ) == 0) return wren_ttri;
    if (strcmp(signature, "static ULI.ttri(_,_,_,_,_,_,_,_,_,_,_,_,_)"      ) == 0) return wren_ttri;
    if (strcmp(signature, "static ULI.ttri(_,_,_,_,_,_,_,_,_,_,_,_,_,_)"    ) == 0) return wren_ttri;
    if (strcmp(signature, "static ULI.ttri_depth()"             ) == 0) return wren_ttri_depth;
    if (strcmp(signature, "static ULI.ttri_depth(_,_,_)"        ) == 0) return wren_ttri_depth;

    if (strcmp(signature, "static ULI.pix(_,_)"                 ) == 0) return wren_pix;
    if (strcmp(signature, "static ULI.pix(_,_,_)"               ) == 0) return wren_pix;
    if (strcmp(signature, "static ULI.line(_,_,_,_,_)"          ) == 0) return wren_line;
    if (strcmp(signature, "static ULI.circ(_,_,_,_)"            ) == 0) return wren_circ;
    if (strcmp(signature, "static ULI.circb(_,_,_,_)"           ) == 0) return wren_circb;
    if (strcmp(signature, "static ULI.elli(_,_,_,_,_)"          ) == 0) return wren_elli;
    if (strcmp(signature, "static ULI.ellib(_,_,_,_,_)"         ) == 0) return wren_ellib;
    if (strcmp(signature, "static ULI.paint(_,_,_)"             ) == 0) return wren_paint;
    if (strcmp(signature, "static ULI.paint(_,_,_,_)"           ) == 0) return wren_paint;
    if (strcmp(signature, "static ULI.rect(_,_,_,_,_)"          ) == 0) return wren_rect;
    if (strcmp(signature, "static ULI.rectb(_,_,_,_,_)"         ) == 0) return wren_rectb;
    if (strcmp(signature, "static ULI.tri(_,_,_,_,_,_,_)"       ) == 0) return wren_tri;
    if (strcmp(signature, "static ULI.trib(_,_,_,_,_,_,_)"      ) == 0) return wren_trib;

    if (strcmp(signature, "static ULI.cls()"                    ) == 0) return wren_cls;
    if (strcmp(signature, "static ULI.cls(_)"                   ) == 0) return wren_cls;
    if (strcmp(signature, "static ULI.clip()"                   ) == 0) return wren_clip;
    if (strcmp(signature, "static ULI.clip(_,_,_,_)"            ) == 0) return wren_clip;

    if (strcmp(signature, "static ULI.peek(_)"                  ) == 0) return wren_peek;
    if (strcmp(signature, "static ULI.poke(_,_)"                ) == 0) return wren_poke;
    if (strcmp(signature, "static ULI.peek(_,_)"                ) == 0) return wren_peek;
    if (strcmp(signature, "static ULI.poke(_,_,_)"              ) == 0) return wren_poke;
    if (strcmp(signature, "static ULI.peek1(_)"                 ) == 0) return wren_peek1;
    if (strcmp(signature, "static ULI.poke1(_,_)"               ) == 0) return wren_poke1;
    if (strcmp(signature, "static ULI.peek2(_)"                 ) == 0) return wren_peek2;
    if (strcmp(signature, "static ULI.poke2(_,_)"               ) == 0) return wren_poke2;
    if (strcmp(signature, "static ULI.peek4(_)"                 ) == 0) return wren_peek4;
    if (strcmp(signature, "static ULI.poke4(_,_)"               ) == 0) return wren_poke4;
    if (strcmp(signature, "static ULI.memcpy(_,_,_)"            ) == 0) return wren_memcpy;
    if (strcmp(signature, "static ULI.memset(_,_,_)"            ) == 0) return wren_memset;
    if (strcmp(signature, "static ULI.pmem(_)"                  ) == 0) return wren_pmem;
    if (strcmp(signature, "static ULI.pmem(_,_)"                ) == 0) return wren_pmem;

    if (strcmp(signature, "static ULI.sfx(_)"                   ) == 0) return wren_sfx;
    if (strcmp(signature, "static ULI.sfx(_,_)"                 ) == 0) return wren_sfx;
    if (strcmp(signature, "static ULI.sfx(_,_,_)"               ) == 0) return wren_sfx;
    if (strcmp(signature, "static ULI.sfx(_,_,_,_)"             ) == 0) return wren_sfx;
    if (strcmp(signature, "static ULI.sfx(_,_,_,_,_)"           ) == 0) return wren_sfx;
    if (strcmp(signature, "static ULI.sfx(_,_,_,_,_,_)"         ) == 0) return wren_sfx;
    if (strcmp(signature, "static ULI.music()"                  ) == 0) return wren_music;
    if (strcmp(signature, "static ULI.music(_)"                 ) == 0) return wren_music;
    if (strcmp(signature, "static ULI.music(_,_)"               ) == 0) return wren_music;
    if (strcmp(signature, "static ULI.music(_,_,_)"             ) == 0) return wren_music;
    if (strcmp(signature, "static ULI.music(_,_,_,_)"           ) == 0) return wren_music;
    if (strcmp(signature, "static ULI.music(_,_,_,_,_)"         ) == 0) return wren_music;
    if (strcmp(signature, "static ULI.music(_,_,_,_,_,_)"       ) == 0) return wren_music;
    if (strcmp(signature, "static ULI.music(_,_,_,_,_,_,_)"     ) == 0) return wren_music;

    if (strcmp(signature, "static ULI.time()"                   ) == 0) return wren_time;
    if (strcmp(signature, "static ULI.tstamp()"                 ) == 0) return wren_tstamp;
    if (strcmp(signature, "static ULI.vbank()"                  ) == 0) return wren_vbank;
    if (strcmp(signature, "static ULI.vbank(_)"                 ) == 0) return wren_vbank;
    if (strcmp(signature, "static ULI.sync()"                   ) == 0) return wren_sync;
    if (strcmp(signature, "static ULI.sync(_)"                  ) == 0) return wren_sync;
    if (strcmp(signature, "static ULI.sync(_,_)"                ) == 0) return wren_sync;
    if (strcmp(signature, "static ULI.sync(_,_,_)"              ) == 0) return wren_sync;
    if (strcmp(signature, "static ULI.reset()"                  ) == 0) return wren_reset;
    if (strcmp(signature, "static ULI.exit()"                   ) == 0) return wren_exit;
    if (strcmp(signature, "static ULI.fget(_,_)"                ) == 0) return wren_fget;
    if (strcmp(signature, "static ULI.fset(_,_,_)"              ) == 0) return wren_fset;

    if (strcmp(signature, "static ULI.fft(_,_)"                 ) == 0) return wren_fft;
    if (strcmp(signature, "static ULI.ffts(_,_)"                ) == 0) return wren_ffts;

    // internal functions
    if (strcmp(signature, "static ULI.map_width__"              ) == 0) return wren_map_width;
    if (strcmp(signature, "static ULI.map_height__"             ) == 0) return wren_map_height;
    if (strcmp(signature, "static ULI.spritesize__"             ) == 0) return wren_spritesize;
    if (strcmp(signature, "static ULI.print__(_,_,_,_,_,_,_)"   ) == 0) return wren_print;
    if (strcmp(signature, "static ULI.trace__(_,_)"             ) == 0) return wren_trace;
    if (strcmp(signature, "static ULI.spr__(_,_,_,_,_,_,_)"     ) == 0) return wren_spr_internal;
    if (strcmp(signature, "static ULI.mgeti__(_)"               ) == 0) return wren_mgeti;

    return NULL;
}

static WrenForeignMethodFn bindForeignMethod(
    WrenVM* vm, const char* module, const char* className,
    bool isStatic, const char* signature)
{
    if (strcmp(module, "main") != 0) return NULL;

    // For convenience, concatenate all of the method qualifiers into a single signature string.
    char fullName[256];
    fullName[0] = '\0';
    if (isStatic)
    {
        strcat(fullName, "static ");
    }

    strcat(fullName, className);
    strcat(fullName, ".");
    strcat(fullName, signature);

    return foreignTicMethods(fullName);
}

static void initAPI(uli_core* core)
{
    wrenSetUserData(core->currentVM, core);

    if (wrenInterpret(core->currentVM, "main", uli_wren_api) != WREN_RESULT_SUCCESS)
    {
        core->data->error(core->data->data, "can't load ULI wren api");
    }
}

static void reportError(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message)
{
    uli_core* core = getWrenCore(vm);

    char buffer[1024];

    if (module)
    {
        snprintf(buffer, sizeof buffer, "\"%s\", %d ,\"%s\"",module, line, message);
    } else {
        snprintf(buffer, sizeof buffer, "%d, \"%s\"",line, message);
    }

    core->data->error(core->data->data, buffer);
}

static void writeFn(WrenVM* vm, const char* text)
{
    uli_core* core = getWrenCore(vm);
    u8 color = uli_color_dark_blue;
    core->data->trace(core->data->data, text ? text : "null", color);
}

static bool initWren(uli_mem* uli, const char* code)
{
    uli_core* core = (uli_core*)uli;
    closeWren(uli);

    WrenConfiguration config;
    wrenInitConfiguration(&config);

    config.bindForeignMethodFn = bindForeignMethod;

    config.errorFn = reportError;
    config.writeFn = writeFn;

    WrenVM* vm = core->currentVM = wrenNewVM(&config);

    initAPI(core);

    if (wrenInterpret(core->currentVM, "main", code) != WREN_RESULT_SUCCESS)
    {
        return false;
    }

    loaded = true;

    // make handles
    wrenEnsureSlots(vm, 1);
    wrenGetVariable(vm, "main", "ULI", 0);
    game_class = wrenGetSlotHandle(vm, 0); // handle from game class

    new_handle = wrenMakeCallHandle(vm, "new()");
    update_handle = wrenMakeCallHandle(vm, ULI_FN "()");
    boot_handle = wrenMakeCallHandle(vm, BOOT_FN "()");
    scanline_handle = wrenMakeCallHandle(vm, SCN_FN "(_)");
    border_handle = wrenMakeCallHandle(vm, BDR_FN "(_)");
    menu_handle = wrenMakeCallHandle(vm, MENU_FN "(_)");
    overline_handle = wrenMakeCallHandle(vm, OVR_FN "()");

    // create game class
    if (game_class)
    {
        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, game_class);
        wrenCall(vm, new_handle);
        wrenReleaseHandle(core->currentVM, game_class); // release game class handle
        game_class = NULL;
        if (wrenGetSlotCount(vm) == 0)
        {
            core->data->error(core->data->data, "Error in game class :(");
            return false;
        }
        game_class = wrenGetSlotHandle(vm, 0); // handle from game object
    } else {
        core->data->error(core->data->data, "'ULI class' isn't found :(");
        return false;
    }

    return true;
}

static void callWrenTick(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;
    WrenVM* vm = core->currentVM;

    if(vm && game_class)
    {
        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, game_class);
        wrenCall(vm, update_handle);

#if defined(BUILD_DEPRECATED)
        // call OVR() callback for backward compatibility
        if(overline_handle)
        {
            OVR(core)
            {
                wrenEnsureSlots(vm, 1);
                wrenSetSlotHandle(vm, 0, game_class);
                wrenCall(vm, overline_handle);
            }
        }
#endif
    }
}

static void callWrenBoot(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;
    WrenVM* vm = core->currentVM;

    if(vm && game_class)
    {
        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, game_class);
        wrenCall(vm, boot_handle);
    }
}

static void callWrenIntCallback(uli_mem* uli, s32 value, WrenHandle* handle, void* data)
{
    uli_core* core = (uli_core*)uli;
    WrenVM* vm = core->currentVM;

    if(vm && game_class)
    {
        wrenEnsureSlots(vm, 2);
        wrenSetSlotHandle(vm, 0, game_class);
        wrenSetSlotDouble(vm, 1, value);
        wrenCall(vm, handle);
    }
}

static void callWrenScanline(uli_mem* uli, s32 row, void* data)
{
    callWrenIntCallback(uli, row, scanline_handle, data);
}

static void callWrenBorder(uli_mem* uli, s32 row, void* data)
{
    callWrenIntCallback(uli, row, border_handle, data);
}

static void callWrenMenu(uli_mem* uli, s32 index, void* data)
{
    callWrenIntCallback(uli, index, menu_handle, data);
}

static const char* const WrenKeywords [] =
{
    "as", "break", "class", "construct", "continue", "else", "false",
    "for", "foreign", "if", "import", "in", "is", "null", "return",
    "static", "super", "this", "true", "var", "while"
};

static inline bool isalnum_(char c) {return isalnum(c) || c == '_';}

static const uli_outline_item* getWrenOutline(const char* code, s32* size)
{
    enum{Size = sizeof(uli_outline_item)};

    *size = 0;

    static uli_outline_item* items = NULL;

    if(items)
    {
        free(items);
        items = NULL;
    }

    const char* ptr = code;

    while(true)
    {
        static const char ClassString[] = "class ";

        ptr = strstr(ptr, ClassString);

        if(ptr)
        {
            ptr += sizeof ClassString - 1;

            const char* start = ptr;
            const char* end = start;

            while(*ptr)
            {
                char c = *ptr;

                if(isalnum_(c));
                else if(c == ' ' || c == '{')
                {
                    end = ptr;
                    break;
                }
                else break;

                ptr++;
            }

            if(end > start)
            {
                items = realloc(items, (*size + 1) * Size);

                items[*size].pos = start;
                items[*size].size = (s32)(end - start);

                (*size)++;
            }
        }
        else break;
    }

    return items;
}

static void evalWren(uli_mem* uli, const char* code)
{
    uli_core* core = (uli_core*)uli;
    wrenInterpret(core->currentVM, "main", code);
}

static const u8 DemoRom[] =
{
    #include "../build/assets/wrendemo.uli.dat"
};

static const u8 MarkRom[] =
{
    #include "../build/assets/wrenmark.uli.dat"
};

ULI_EXPORT const uli_script EXPORT_SCRIPT(Wren) =
{
    .id                 = 16,
    .name               = "wren",
    .fileExtension      = ".wren",
    .projectComment     = "//",
    {
      .init               = initWren,
      .close              = closeWren,
      .tick               = callWrenTick,
      .boot               = callWrenBoot,

      .callback           =
      {
        .scanline       = callWrenScanline,
        .border         = callWrenBorder,
        .menu           = callWrenMenu,
      },
    },

    .getOutline         = getWrenOutline,
    .eval               = evalWren,

    .blockCommentStart  = "/*",
    .blockCommentEnd    = "*/",
    .blockCommentStart2 = NULL,
    .blockCommentEnd2   = NULL,
    .blockStringStart   = NULL,
    .blockStringEnd     = NULL,
    .singleComment      = "//",
    .blockEnd           = "}",

    .keywords           = WrenKeywords,
    .keywordsCount      = COUNT_OF(WrenKeywords),

    .demo = {DemoRom, sizeof DemoRom},
    .mark = {MarkRom, sizeof MarkRom, "wrenmark.uli"},
};
