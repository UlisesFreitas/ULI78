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
#include <stdio.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdblob.h>
#include <ctype.h>

extern bool parse_note(const char* noteStr, s32* note, s32* octave);

static const char TicCore[] = "_ULI78";

static float getSquirrelFloat(HSQUIRRELVM vm, s32 index)
{
    SQFloat f = 0.0;
    sq_getfloat(vm, index, &f);
    return f;
}

// !TODO: get rid of this wrap
static s32 getSquirrelNumber(HSQUIRRELVM vm, s32 index)
{
    SQInteger i;
    if (SQ_SUCCEEDED(sq_getinteger(vm, index, &i)))
        return (s32)i;

    return (s32)getSquirrelFloat(vm, index);
}

static void registerSquirrelFunction(uli_core* core, SQFUNCTION func, const char *name)
{
    sq_pushroottable(core->currentVM);
    sq_pushstring(core->currentVM, name, -1);
    sq_newclosure(core->currentVM, func, 0);
    sq_newslot(core->currentVM, -3, SQTrue);
    sq_poptop(core->currentVM); // remove root table.
}

static uli_core* getSquirrelCore(HSQUIRRELVM vm)
{
#if USE_FOREIGN_POINTER
    return (uli_core*)sq_getforeignpointer(vm);
#else
    sq_pushregistrytable(vm);
    sq_pushstring(vm, TicCore, -1);
    if (SQ_FAILED(sq_get(vm, -2)))
    {
        fprintf(stderr, "FATAL ERROR: TicCore not found!\n");
        abort();
    }
    SQUserPointer ptr;
    if (SQ_FAILED(sq_getuserpointer(vm, -1, &ptr)))
    {
        fprintf(stderr, "FATAL ERROR: Cannot get user pointer for TicCore!\n");
        abort();
    }
    uli_core* core = (uli_core*)ptr;
    sq_pop(vm, 2); // user pointer and registry table.
    return core;
#endif
}

void squirrel_compilerError(HSQUIRRELVM vm, const SQChar* desc, const SQChar* source,
                             SQInteger line, SQInteger column)
{
    uli_core* core = getSquirrelCore(vm);
    char buffer[1024];
    snprintf(buffer, 1023, "%.40s line %.6d column %.6d: %s\n", source, (int)line, (int)column, desc);

    if (core->data)
        core->data->error(core->data->data, buffer);
}

static SQInteger squirrel_errorHandler(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);

    SQStackInfos si;
    SQInteger level = 0;
    while (SQ_SUCCEEDED(sq_stackinfos(vm, level, &si)))
    {
        char buffer[100];
        snprintf(buffer, 99, "%.40s %.40s %.6d\n", si.funcname, si.source, (int)si.line);

        if (core->data)
            core->data->error(core->data->data, buffer);
        ++level;
    }

    return 0;
}


static SQInteger squirrel_peek(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;
    SQInteger top = sq_gettop(vm);

    // check number of args
    if (top < 2)
        return sq_throwerror(vm, "invalid parameters, peek(address)");

    s32 address = getSquirrelNumber(vm, 2);
    s32 bits = BITS_IN_BYTE;

    if(top == 3)
        bits = getSquirrelNumber(vm, 3);

    sq_pushinteger(vm, core->api.peek(uli, address, bits));
    return 1;
}

static SQInteger squirrel_poke(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;
    SQInteger top = sq_gettop(vm);

    if (top < 3)
        return sq_throwerror( vm, "invalid parameters, poke(address,value)" );

    s32 address = getSquirrelNumber(vm, 2);
    u8 value = getSquirrelNumber(vm, 3);
    s32 bits = BITS_IN_BYTE;

    if(top == 4)
        bits = getSquirrelNumber(vm, 4);

    core->api.poke(uli, address, value, bits);

    return 0;
}

static SQInteger squirrel_peek1(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    // check number of args
    if (sq_gettop(vm) != 2)
        return sq_throwerror(vm, "invalid parameters, peek4(address)");
    s32 address = getSquirrelNumber(vm, 2);

    sq_pushinteger(vm, core->api.peek1(uli, address));
    return 1;
}

static SQInteger squirrel_poke1(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    if (sq_gettop(vm) != 3)
        return sq_throwerror( vm, "invalid parameters, poke4(address,value)" );

    s32 address = getSquirrelNumber(vm, 2);
    u8 value = getSquirrelNumber(vm, 3);

    core->api.poke1(uli, address, value);

    return 0;
}

static SQInteger squirrel_peek2(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    // check number of args
    if (sq_gettop(vm) != 2)
        return sq_throwerror(vm, "invalid parameters, peek2(address)");
    s32 address = getSquirrelNumber(vm, 2);

    sq_pushinteger(vm, core->api.peek2(uli, address));
    return 1;
}

static SQInteger squirrel_poke2(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    if (sq_gettop(vm) != 3)
        return sq_throwerror( vm, "invalid parameters, poke2(address,value)" );

    s32 address = getSquirrelNumber(vm, 2);
    u8 value = getSquirrelNumber(vm, 3);

    core->api.poke2(uli, address, value);

    return 0;
}

static SQInteger squirrel_peek4(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    // check number of args
    if (sq_gettop(vm) != 2)
        return sq_throwerror(vm, "invalid parameters, peek4(address)");
    s32 address = getSquirrelNumber(vm, 2);

    sq_pushinteger(vm, core->api.peek4(uli, address));
    return 1;
}

static SQInteger squirrel_poke4(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    if (sq_gettop(vm) != 3)
        return sq_throwerror( vm, "invalid parameters, poke4(address,value)" );

    s32 address = getSquirrelNumber(vm, 2);
    u8 value = getSquirrelNumber(vm, 3);

    core->api.poke4(uli, address, value);

    return 0;
}

static SQInteger squirrel_cls(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.cls(uli, top == 2 ? getSquirrelNumber(vm, 2) : 0);

    return 0;
}

static SQInteger squirrel_pix(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top >= 3)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        if(top >= 4)
        {
            s32 color = getSquirrelNumber(vm, 4);
            core->api.pix(uli, x, y, color, false);
        }
        else
        {
            sq_pushinteger(vm, core->api.pix(uli, x, y, 0, true));
            return 1;
        }

    }
    else return sq_throwerror(vm, "invalid parameters, pix(x y [color])\n");

    return 0;
}

static SQInteger squirrel_line(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 6)
    {
        float x0 = getSquirrelFloat(vm, 2);
        float y0 = getSquirrelFloat(vm, 3);
        float x1 = getSquirrelFloat(vm, 4);
        float y1 = getSquirrelFloat(vm, 5);
        s32 color = getSquirrelNumber(vm, 6);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.line(uli, x0, y0, x1, y1, color);
    }
    else return sq_throwerror(vm, "invalid parameters, line(x0,y0,x1,y1,color)\n");

    return 0;
}

static SQInteger squirrel_rect(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 6)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 w = getSquirrelNumber(vm, 4);
        s32 h = getSquirrelNumber(vm, 5);
        s32 color = getSquirrelNumber(vm, 6);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.rect(uli, x, y, w, h, color);
    }
    else return sq_throwerror(vm, "invalid parameters, rect(x,y,w,h,color)\n");

    return 0;
}

static SQInteger squirrel_rectb(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 6)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 w = getSquirrelNumber(vm, 4);
        s32 h = getSquirrelNumber(vm, 5);
        s32 color = getSquirrelNumber(vm, 6);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.rectb(uli, x, y, w, h, color);
    }
    else return sq_throwerror(vm, "invalid parameters, rectb(x,y,w,h,color)\n");

    return 0;
}

static SQInteger squirrel_circ(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 5)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 radius = getSquirrelNumber(vm, 4);
        s32 color = getSquirrelNumber(vm, 5);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.circ(uli, x, y, radius, color);
    }
    else return sq_throwerror(vm, "invalid parameters, circ(x,y,radius,color)\n");

    return 0;
}

static SQInteger squirrel_circb(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 5)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 radius = getSquirrelNumber(vm, 4);
        s32 color = getSquirrelNumber(vm, 5);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.circb(uli, x, y, radius, color);
    }
    else return sq_throwerror(vm, "invalid parameters, circb(x,y,radius,color)\n");

    return 0;
}

static SQInteger squirrel_elli(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 6)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 a = getSquirrelNumber(vm, 4);
        s32 b = getSquirrelNumber(vm, 5);
        s32 color = getSquirrelNumber(vm, 6);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.elli(uli, x, y, a, b, color);
    }
    else return sq_throwerror(vm, "invalid parameters, elli(x,y,a,b,color)\n");

    return 0;
}

static SQInteger squirrel_ellib(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 6)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 a = getSquirrelNumber(vm, 4);
        s32 b = getSquirrelNumber(vm, 5);
        s32 color = getSquirrelNumber(vm, 6);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.ellib(uli, x, y, a, b, color);
    }
    else return sq_throwerror(vm, "invalid parameters, ellib(x,y,a,b,color)\n");

    return 0;
}

static SQInteger squirrel_paint(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top >= 4 && top <= 5)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 color = getSquirrelNumber(vm, 4);
        s32 bordercolor = top >= 5 ? getSquirrelNumber(vm, 5) : -1;

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.paint(uli, x, y, color, bordercolor);
    }
    else return sq_throwerror(vm, "invalid parameters, paint(x,y,color,[bordercolor=-1])\n");

    return 0;
}

static SQInteger squirrel_tri(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 8)
    {
        float pt[6];

        for(s32 i = 0; i < COUNT_OF(pt); i++)
            pt[i] = getSquirrelFloat(vm, i + 2);

        s32 color = getSquirrelNumber(vm, 8);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.tri(uli, pt[0], pt[1], pt[2], pt[3], pt[4], pt[5], color);
    }
    else return sq_throwerror(vm, "invalid parameters, tri(x1,y1,x2,y2,x3,y3,color)\n");

    return 0;
}

static SQInteger squirrel_trib(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 8)
    {
        float pt[6];

        for(s32 i = 0; i < COUNT_OF(pt); i++)
            pt[i] = getSquirrelFloat(vm, i + 2);

        s32 color = getSquirrelNumber(vm, 8);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.trib(uli, pt[0], pt[1], pt[2], pt[3], pt[4], pt[5], color);
    }
    else return sq_throwerror(vm, "invalid parameters, trib(x1,y1,x2,y2,x3,y3,color)\n");

    return 0;
}

static SQInteger squirrel_ttri(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if (top >= 13)
    {
        float pt[12];

        for (s32 i = 0; i < COUNT_OF(pt); i++)
            pt[i] = getSquirrelFloat(vm, i + 2);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;
        static u8 colors[ULI_PALETTE_SIZE];
        s32 count = 0;
        uli_texture_src src = uli_tiles_texture;

        //  check for texture source
        if (top >= 14)
        {
            src = getSquirrelNumber(vm, 14);
        }
        //  check for chroma
        if(OT_ARRAY == sq_gettype(vm, 15))
        {
            for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
            {
                sq_pushinteger(vm, (SQInteger)i);
                sq_rawget(vm, 15);
                if(sq_gettype(vm, -1) & (OT_FLOAT|OT_INTEGER))
                {
                    colors[i-1] = getSquirrelNumber(vm, -1);
                    count++;
                    sq_poptop(vm);
                }
                else
                {
                    sq_poptop(vm);
                    break;
                }
            }
        }
        else
        {
            colors[0] = getSquirrelNumber(vm, 15);
            count = 1;
        }

        float z[3];
        bool depth = false;

        if (top == 18)
        {
            for (s32 i = 0; i < COUNT_OF(pt); i++)
                pt[i] = getSquirrelFloat(vm, i + 16);

            depth = true;
        }

        core->api.ttri(uli, pt[0], pt[1],   //  xy 1
                            pt[2], pt[3],   //  xy 2
                            pt[4], pt[5],   //  xy 3
                            pt[6], pt[7],   //  uv 1
                            pt[8], pt[9],   //  uv 2
                            pt[10], pt[11], //  uv 3
                            src,            // texture source
                            colors, count,  // chroma
                            z[0], z[1], z[2], depth); // depth
    }
    else return sq_throwerror(vm, "invalid parameters, ttri(x1,y1,x2,y2,x3,y3,u1,v1,u2,v2,u3,v3,[texsrc=0],[chroma=off],[z1=0],[z2=0],[z3=0])\n");
    return 0;
}


static SQInteger squirrel_clip(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 1)
    {
        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.clip(uli, 0, 0, ULI78_WIDTH, ULI78_HEIGHT);
    }
    else if(top == 5)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        s32 w = getSquirrelNumber(vm, 4);
        s32 h = getSquirrelNumber(vm, 5);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.clip((uli_mem*)getSquirrelCore(vm), x, y, w, h);
    }
    else return sq_throwerror(vm, "invalid parameters, use clip(x,y,w,h) or clip()\n");

    return 0;
}

static SQInteger squirrel_btnp(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);
    uli_mem* uli = (uli_mem*)core;

    SQInteger top = sq_gettop(vm);

    if (top == 1)
    {
        sq_pushinteger(vm, core->api.btnp(uli, -1, -1, -1));
    }
    else if(top == 2)
    {
        s32 index = getSquirrelNumber(vm, 2) & 0x1f;

        sq_pushbool(vm, (core->api.btnp(uli, index, -1, -1) ? SQTrue : SQFalse));
    }
    else if (top == 4)
    {
        s32 index = getSquirrelNumber(vm, 2) & 0x1f;
        u32 hold = getSquirrelNumber(vm, 3);
        u32 period = getSquirrelNumber(vm, 4);

        sq_pushbool(vm, (core->api.btnp(uli, index, hold, period) ? SQTrue : SQFalse));
    }
    else
    {
        return sq_throwerror(vm, "invalid params, btnp [ id [ hold period ] ]\n");
    }

    return 1;
}

static SQInteger squirrel_btn(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);
    uli_mem* uli = (uli_mem*)core;

    SQInteger top = sq_gettop(vm);

    if (top == 1)
    {
        sq_pushinteger(vm, core->api.btn(uli, -1));
    }
    else if (top == 2)
    {
        bool pressed = core->api.btn(uli, getSquirrelNumber(vm, 2) & 0x1f);
        sq_pushbool(vm, pressed ? SQTrue : SQFalse);
    }
    else
    {
        return sq_throwerror(vm, "invalid params, btn [ id ]\n");
    }

    return 1;
}

static SQInteger squirrel_spr(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

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

    if(top >= 2)
    {
        index = getSquirrelNumber(vm, 2);

        if(top >= 4)
        {
            x = getSquirrelNumber(vm, 3);
            y = getSquirrelNumber(vm, 4);

            if(top >= 5)
            {
                if(OT_ARRAY == sq_gettype(vm, 5))
                {
                    for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
                    {
                        sq_pushinteger(vm, (SQInteger)i);
                        sq_rawget(vm, 5);
                        if(sq_gettype(vm, -1) & (OT_FLOAT|OT_INTEGER))
                        {
                            colors[i-1] = getSquirrelNumber(vm, -1);
                            count++;
                            sq_poptop(vm);
                        }
                        else
                        {
                            sq_poptop(vm);
                            break;
                        }
                    }
                }
                else
                {
                    colors[0] = getSquirrelNumber(vm, 5);
                    count = 1;
                }

                if(top >= 6)
                {
                    scale = getSquirrelNumber(vm, 6);

                    if(top >= 7)
                    {
                        flip = getSquirrelNumber(vm, 7);

                        if(top >= 8)
                        {
                            rotate = getSquirrelNumber(vm, 8);

                            if(top >= 10)
                            {
                                w = getSquirrelNumber(vm, 9);
                                h = getSquirrelNumber(vm, 10);
                            }
                        }
                    }
                }
            }
        }
    }

    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.spr(uli, index, x, y, w, h, colors, count, scale, flip, rotate);

    return 0;
}

static SQInteger squirrel_mget(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 3)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        u8 value = core->api.mget(uli, x, y);
        sq_pushinteger(vm, value);
        return 1;
    }
    else return sq_throwerror(vm, "invalid params, mget(x,y)\n");

    return 0;
}

static SQInteger squirrel_mset(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 4)
    {
        s32 x = getSquirrelNumber(vm, 2);
        s32 y = getSquirrelNumber(vm, 3);
        u8 val = getSquirrelNumber(vm, 4);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.mset(uli, x, y, val);
    }
    else return sq_throwerror(vm, "invalid params, mget(x,y)\n");

    return 0;
}

typedef struct
{
    HSQUIRRELVM vm;
    HSQOBJECT reg;
} RemapData;

static void remapCallback(void* data, s32 x, s32 y, RemapResult* result)
{
    RemapData* remap = (RemapData*)data;
    HSQUIRRELVM vm = remap->vm;

    SQInteger top = sq_gettop(vm);

    sq_pushobject(vm, remap->reg);
    sq_pushroottable(vm);
    sq_pushinteger(vm, result->index);
    sq_pushinteger(vm, x);
    sq_pushinteger(vm, y);
    //lua_pcall(lua, 3, 3, 0);

    if (SQ_SUCCEEDED(sq_call(vm, 4, SQTrue, SQTrue)))
    {
        sq_pushinteger(vm, 0);
        if (SQ_SUCCEEDED(sq_get(vm, -2)))
        {
            result->index = getSquirrelNumber(vm, -1);
            sq_poptop(vm);
            sq_pushinteger(vm, 1);
            if (SQ_SUCCEEDED(sq_get(vm, -2)))
            {
                result->flip = getSquirrelNumber(vm, -1);
                sq_poptop(vm);
                sq_pushinteger(vm, 2);
                if (SQ_SUCCEEDED(sq_get(vm, -2)))
                {
                    result->rotate = getSquirrelNumber(vm, -1);
                    sq_poptop(vm);
                }
            }
        }
    }

    sq_settop(vm, top);
}

static SQInteger squirrel_map(HSQUIRRELVM vm)
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

    SQInteger top = sq_gettop(vm);

    if(top >= 3)
    {
        x = getSquirrelNumber(vm, 2);
        y = getSquirrelNumber(vm, 3);

        if(top >= 5)
        {
            w = getSquirrelNumber(vm, 4);
            h = getSquirrelNumber(vm, 5);

            if(top >= 7)
            {
                sx = getSquirrelNumber(vm, 6);
                sy = getSquirrelNumber(vm, 7);

                if(top >= 8)
                {
                    if(OT_ARRAY == sq_gettype(vm, 8))
                    {
                        for(s32 i = 0; i < ULI_PALETTE_SIZE; i++)
                        {
                            sq_pushinteger(vm, (SQInteger)i);
                            sq_rawget(vm, 8);
                            if(sq_gettype(vm, -1) & (OT_FLOAT|OT_INTEGER))
                            {
                                colors[i-1] = getSquirrelNumber(vm, -1);
                                count++;
                                sq_poptop(vm);
                            }
                            else
                            {
                                sq_poptop(vm);
                                break;
                            }
                        }
                    }
                    else
                    {
                        colors[0] = getSquirrelNumber(vm, 8);
                        count = 1;
                    }

                    if(top >= 9)
                    {
                        scale = getSquirrelNumber(vm, 9);

                        if(top >= 10)
                        {
                            SQObjectType type = sq_gettype(vm, 10);
                            if (type & (OT_CLOSURE|OT_NATIVECLOSURE|OT_INSTANCE))
                            {
                                RemapData data = {vm};
                                sq_resetobject(&data.reg);
                                sq_getstackobj(vm, 10, &data.reg);
                                sq_addref(vm, &data.reg);

                                uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

                                core->api.map(uli, x, y, w, h, sx, sy, colors, count, scale, remapCallback, &data);

                                //luaL_unref(lua, LUA_REGISTRYINDEX, data.reg);
                                sq_release(vm, &data.reg);

                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    core->api.map((uli_mem*)getSquirrelCore(vm), x, y, w, h, sx, sy, colors, count, scale, NULL, NULL);

    return 0;
}

static SQInteger squirrel_music(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    if(top == 1) core->api.music(uli, -1, 0, 0, false, false, -1, -1);
    else if(top >= 2)
    {
        core->api.music(uli, -1, 0, 0, false, false, -1, -1);

        s32 track = getSquirrelNumber(vm, 2);

        if(track > MUSIC_TRACKS - 1)
            return sq_throwerror(vm, "invalid music track index\n");

        s32 frame = -1;
        s32 row = -1;
        bool loop = true;
        bool sustain = false;
        s32 tempo = -1;
        s32 speed = -1;

        if(top >= 3)
        {
            frame = getSquirrelNumber(vm, 3);

            if(top >= 4)
            {
                row = getSquirrelNumber(vm, 4);

                if(top >= 5)
                {
                    SQBool b = SQFalse;
                    sq_getbool(vm, 5, &b);
                    loop = (b != SQFalse);
                    if(top >= 6)
                    {
                        SQBool b = SQFalse;
                        sq_getbool(vm, 6, &b);
                        sustain = (b != SQFalse);

                        if (top >= 7)
                        {
                            tempo = getSquirrelNumber(vm, 7);

                            if (top >= 8)
                            {
                                speed = getSquirrelNumber(vm, 8);
                            }
                        }
                    }
                }
            }
        }

        core->api.music(uli, track, frame, row, loop, sustain, tempo, speed);
    }
    else return sq_throwerror(vm, "invalid params, use music(track)\n");

    return 0;
}

static SQInteger squirrel_sfx(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top >= 2)
    {
        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        s32 note = -1;
        s32 octave = -1;
        s32 duration = -1;
        s32 channel = 0;
        s32 volumes[ULI78_SAMPLE_CHANNELS] = {MAX_VOLUME, MAX_VOLUME};
        s32 speed = SFX_DEF_SPEED;

        s32 index = getSquirrelNumber(vm, 2);

        if(index < SFX_COUNT)
        {
            if (index >= 0)
            {
                uli_sample* effect = uli->ram->sfx.samples.data + index;

                note = effect->note;
                octave = effect->octave;
                speed = effect->speed;
            }

            if(top >= 3)
            {
                if(sq_gettype(vm, 3) & (OT_INTEGER|OT_FLOAT))
                {
                    s32 id = getSquirrelNumber(vm, 3);
                    note = id % NOTES;
                    octave = id / NOTES;
                }
                else if(sq_gettype(vm, 3) == OT_STRING)
                {
                    const SQChar* str;
                    sq_getstring(vm, 3, &str);
                    const char* noteStr = (const char*)str;

                    if(!parse_note(noteStr, &note, &octave))
                    {
                        return sq_throwerror(vm, "invalid note, should be like C#4\n");
                    }
                }

                if(top >= 4)
                {
                    duration = getSquirrelNumber(vm, 4);

                    if(top >= 5)
                    {
                        channel = getSquirrelNumber(vm, 5);

                        if(top >= 6)
                        {
                            if(OT_ARRAY == sq_gettype(vm, 6))
                            {
                                for(s32 i = 0; i < COUNT_OF(volumes); i++)
                                {
                                    sq_pushinteger(vm, (SQInteger)i);
                                    sq_rawget(vm, 6);
                                    if(sq_gettype(vm, -1) & (OT_FLOAT|OT_INTEGER))
                                        volumes[i] = getSquirrelNumber(vm, -1);
                                    sq_poptop(vm);
                                }
                            }
                            else volumes[0] = volumes[1] = getSquirrelNumber(vm, 6);

                            if(top >= 7)
                            {
                                speed = getSquirrelNumber(vm, 7);
                            }
                        }
                    }
                }
            }

            if (channel >= 0 && channel < ULI_SOUND_CHANNELS)
            {
                core->api.sfx(uli, index, note, octave, duration, channel, volumes[0] & 0xf, volumes[1] & 0xf, speed);
            }
            else return sq_throwerror(vm, "unknown channel\n");
        }
        else return sq_throwerror(vm, "unknown sfx index\n");
    }
    else return sq_throwerror(vm, "invalid sfx params\n");

    return 0;
}

static SQInteger squirrel_vbank(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);
    uli_mem* uli = (uli_mem*)core;

    s32 prev = core->state.vbank.id;

    if(sq_gettop(vm) == 2)
        core->api.vbank(uli, getSquirrelNumber(vm, 2));

    sq_pushinteger(vm, prev);
    return 1;
}

static SQInteger squirrel_sync(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    bool toCart = false;
    u32 mask = 0;
    s32 bank = 0;

    if(sq_gettop(vm) >= 2)
    {
        mask = getSquirrelNumber(vm, 2);

        if(sq_gettop(vm) >= 3)
        {
            bank = getSquirrelNumber(vm, 3);

            if(sq_gettop(vm) >= 4)
            {
                SQBool b = SQFalse;
                sq_getbool(vm, 4, &b);
                toCart = (b != SQFalse);
            }
        }
    }

    if(bank >= 0 && bank < ULI_BANKS)
        core->api.sync(uli, mask, bank, toCart);
    else
        return sq_throwerror(vm, "sync() error, invalid bank");

    return 0;
}

static SQInteger squirrel_reset(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);

    core->state.initialized = false;

    return 0;
}

static SQInteger squirrel_key(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);
    uli_mem* uli = &core->memory;

    SQInteger top = sq_gettop(vm);

    if (top == 1)
    {
        sq_pushbool(vm, core->api.key(uli, uli_key_unknown) ? SQTrue : SQFalse);
    }
    else if (top == 2)
    {
        uli_key key = getSquirrelNumber(vm, 2);

        if(key < uli_keys_count)
            sq_pushbool(vm, core->api.key(uli, key) ? SQTrue : SQFalse);
        else
        {
            return sq_throwerror(vm, "unknown keyboard code\n");
        }
    }
    else
    {
        return sq_throwerror(vm, "invalid params, key [code]\n");
    }

    return 1;
}

static SQInteger squirrel_keyp(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);
    uli_mem* uli = &core->memory;

    SQInteger top = sq_gettop(vm);

    if (top == 1)
    {
        sq_pushbool(vm, core->api.keyp(uli, uli_key_unknown, -1, -1) ? SQTrue : SQFalse);
    }
    else
    {
        uli_key key = getSquirrelNumber(vm, 2);

        if(key >= uli_keys_count)
        {
            return sq_throwerror(vm, "unknown keyboard code\n");
        }
        else
        {
            if(top == 2)
            {
                sq_pushbool(vm, core->api.keyp(uli, key, -1, -1) ? SQTrue : SQFalse);
            }
            else if(top == 4)
            {
                u32 hold = getSquirrelNumber(vm, 3);
                u32 period = getSquirrelNumber(vm, 4);

                sq_pushbool(vm, core->api.keyp(uli, key, hold, period) ? SQTrue : SQFalse);
            }
            else
            {
                return sq_throwerror(vm, "invalid params, keyp [ code [ hold period ] ]\n");
            }
        }
    }

    return 1;
}

static SQInteger squirrel_memcpy(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 4)
    {
        s32 dest = getSquirrelNumber(vm, 2);
        s32 src = getSquirrelNumber(vm, 3);
        s32 size = getSquirrelNumber(vm, 4);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.memcpy(uli, dest, src, size);
        return 0;
    }

    return sq_throwerror(vm, "invalid params, memcpy(dest,src,size)\n");
}

static SQInteger squirrel_memset(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top == 4)
    {
        s32 dest = getSquirrelNumber(vm, 2);
        u8 value = getSquirrelNumber(vm, 3);
        s32 size = getSquirrelNumber(vm, 4);

        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        core->api.memset(uli, dest, value, size);
        return 0;
    }

    return sq_throwerror(vm, "invalid params, memset(dest,val,size)\n");
}

// NB we leave the string on the stack so that the char* pointer remains valid.
static const char* printString(HSQUIRRELVM vm, s32 index)
{
    const SQChar* text = "";
    if (SQ_SUCCEEDED(sq_tostring(vm, index)))
    {
        sq_getstring(vm, -1, &text);
    }

    return (const char*)text;
}

static SQInteger squirrel_font(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;
    SQInteger top = sq_gettop(vm);

    if(top >= 2)
    {
        const char* text = printString(vm, 2);
        s32 x = 0;
        s32 y = 0;
        s32 width = ULI_SPRITESIZE;
        s32 height = ULI_SPRITESIZE;
        u8 chromakey = 0;
        bool fixed = false;
                bool alt = false;
        s32 scale = 1;

        if(top >= 4)
        {
            x = getSquirrelNumber(vm, 3);
            y = getSquirrelNumber(vm, 4);

            if(top >= 5)
            {
                chromakey = getSquirrelNumber(vm, 5);

                if(top >= 7)
                {
                    width = getSquirrelNumber(vm, 6);
                    height = getSquirrelNumber(vm, 7);

                    if(top >= 8)
                    {
                        SQBool b = SQFalse;
                        sq_getbool(vm, 8, &b);
                        fixed = (b != SQFalse);

                        if(top >= 9)
                        {
                            scale = getSquirrelNumber(vm, 9);

                                                        if (top >= 10)
                                                        {
                                                            SQBool b = SQFalse;
                                                            sq_getbool(vm, 10, &b);
                                                            alt = (b != SQFalse);
                                                        }

                        }
                    }
                }
            }
        }

        if(scale == 0)
        {
            sq_pushinteger(vm, 0);
            return 1;
        }

        s32 size = core->api.font(uli, text, x, y, &chromakey, 1, width, height, fixed, scale, alt);

        sq_pushinteger(vm, size);
        return 1;
    }

    return 0;
}

static SQInteger squirrel_print(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);

    if(top >= 2)
    {
        uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

        s32 x = 0;
        s32 y = 0;
        s32 color = ULI_DEFAULT_COLOR;
        bool fixed = false;
                bool alt = false;
        s32 scale = 1;

        const char* text = printString(vm, 2);

        if(top >= 4)
        {
            x = getSquirrelNumber(vm, 3);
            y = getSquirrelNumber(vm, 4);

            if(top >= 5)
            {
                color = getSquirrelNumber(vm, 5) % ULI_PALETTE_SIZE;

                if(top >= 6)
                {
                    SQBool b = SQFalse;
                    sq_getbool(vm, 6, &b);
                    fixed = (b != SQFalse);

                    if(top >= 7)
                    {
                        scale = getSquirrelNumber(vm, 7);

                                                if (top >= 8)
                                                {
                                                    SQBool b = SQFalse;
                                                    sq_getbool(vm, 8, &b);
                                                    alt = (b != SQFalse);
                                                }
                    }
                }
            }
        }

        if(scale == 0)
        {
            sq_pushinteger(vm, 0);
            return 1;
        }

        s32 size = core->api.print(uli, text ? text : "nil", x, y, color, fixed, scale, alt);

        sq_pushinteger(vm, size);

        return 1;
    }

    return 0;
}

static SQInteger squirrel_trace(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    if(top >= 2)
    {
        const char* text = printString(vm, 2);
        u8 color = ULI_DEFAULT_COLOR;

        if(top >= 3)
        {
            color = getSquirrelNumber(vm, 3);
        }

        core->api.trace(uli, text, color);
    }

    return 0;
}

static SQInteger squirrel_pmem(HSQUIRRELVM vm)
{
    SQInteger top = sq_gettop(vm);
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    if(top >= 2)
    {
        u32 index = getSquirrelNumber(vm, 2);

        if(index < ULI_PERSISTENT_SIZE)
        {
            u32 val = core->api.pmem(uli, index, 0, false);

            if(top >= 3)
            {
                SQInteger i = 0;
                sq_getinteger(vm, 3, &i);
                core->api.pmem(uli, index, (u32)i, true);
            }

            sq_pushinteger(vm, val);

            return 1;
        }
        return sq_throwerror(vm, "invalid persistent uli index\n");
    }
    else return sq_throwerror(vm, "invalid params, pmem(index [val]) -> val\n");

    return 0;
}

static SQInteger squirrel_time(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    sq_pushfloat(vm, (SQFloat)(core->api.time(uli)));

    return 1;
}

static SQInteger squirrel_tstamp(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    sq_pushinteger(vm, core->api.tstamp(uli));

    return 1;
}

static SQInteger squirrel_exit(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;
    core->api.exit(uli);

    return 0;
}

static SQInteger squirrel_mouse(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);

    const uli78_mouse* mouse = &core->memory.ram->input.mouse;

    sq_newarray(vm, 0);

    {
        uli_point pos = core->api.mouse((uli_mem*)core);

        sq_pushinteger(vm, pos.x);
        sq_arrayappend(vm, -2);
        sq_pushinteger(vm, pos.y);
        sq_arrayappend(vm, -2);
    }

    sq_pushbool(vm, mouse->left ? SQTrue : SQFalse);
    sq_arrayappend(vm, -2);
    sq_pushbool(vm, mouse->middle ? SQTrue : SQFalse);
    sq_arrayappend(vm, -2);
    sq_pushbool(vm, mouse->right ? SQTrue : SQFalse);
    sq_arrayappend(vm, -2);
    sq_pushinteger(vm, mouse->scrollx);
    sq_arrayappend(vm, -2);
    sq_pushinteger(vm, mouse->scrolly);
    sq_arrayappend(vm, -2);

    return 1;
}

static SQInteger squirrel_fget(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    SQInteger top = sq_gettop(vm);

    if(top >= 2)
    {
        u32 index = getSquirrelNumber(vm, 2);

        if(top >= 3)
        {
            u32 flag = getSquirrelNumber(vm, 3);
            sq_pushbool(vm, core->api.fget(uli, index, flag));
            return 1;
        }
    }

    sq_throwerror(vm, "invalid params, fget(index, flag) -> val\n");

    return 0;
}

static SQInteger squirrel_fset(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm); uli_mem* uli = (uli_mem*)core;

    SQInteger top = sq_gettop(vm);

    if(top >= 2)
    {
        u32 index = getSquirrelNumber(vm, 2);

        if(top >= 3)
        {
            u32 flag = getSquirrelNumber(vm, 3);

            if(top >= 4)
            {
                SQBool value = SQFalse;
                sq_getbool(vm, 4, &value);

                core->api.fset(uli, index, flag, value);
                return 0;
            }
        }
    }

    sq_throwerror(vm, "invalid params, fset(index, flag, value)\n");

    return 0;
}

static SQInteger squirrel_fft(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);
    uli_mem* uli = (uli_mem*)core;

    SQInteger top = sq_gettop(vm);

    if (top >= 2)
    {
        double start_freq = getSquirrelNumber(vm, 2);
        double end_freq = -1;

        if (top >= 3)
        {
            end_freq = getSquirrelNumber(vm, 3);
        }

        sq_pushfloat(vm, (SQFloat)(core->api.fft(uli, start_freq, end_freq)));
        return 1;
    }

    sq_throwerror(vm, "invalid params, fft(start_freq, end_freq)\n");

    return 0;
}

static SQInteger squirrel_ffts(HSQUIRRELVM vm)
{
    uli_core* core = getSquirrelCore(vm);
    uli_mem* uli = (uli_mem*)core;

    SQInteger top = sq_gettop(vm);

    if (top >= 2)
    {
        double start_freq = getSquirrelNumber(vm, 2);
        double end_freq = -1;

        if (top >= 3)
        {
            end_freq = getSquirrelNumber(vm, 3);
        }

        sq_pushfloat(vm, (SQFloat)(core->api.ffts(uli, start_freq, end_freq)));
        return 1;
    }

    sq_throwerror(vm, "invalid params, ffts(start_freq, end_freq)\n");

    return 0;
}

static SQInteger squirrel_dofile(HSQUIRRELVM vm)
{
    return sq_throwerror(vm, "unknown method: \"dofile\"\n");
}

static SQInteger squirrel_loadfile(HSQUIRRELVM vm)
{
    return sq_throwerror(vm, "unknown method: \"loadfile\"\n");
}

static void squirrel_open_builtins(HSQUIRRELVM vm)
{
    sq_pushroottable(vm);
    sqstd_register_mathlib(vm);
    sqstd_register_stringlib(vm);
    sqstd_register_bloblib(vm);
    sq_poptop(vm);
}

static void initAPI(uli_core* core)
{
    HSQUIRRELVM vm = core->currentVM;

    sq_setcompilererrorhandler(vm, squirrel_compilerError);

    sq_pushregistrytable(vm);
    sq_pushstring(vm, TicCore, -1);
    sq_pushuserpointer(core->currentVM, core);
    sq_newslot(vm, -3, SQTrue);
    sq_poptop(vm);

#if USE_FOREIGN_POINTER
    sq_setforeignptr(vm, core);
#endif

#define API_FUNC_DEF(name, ...) {squirrel_ ## name, #name},
    static const struct{SQFUNCTION func; const char* name;} ApiItems[] = {ULI_API_LIST(API_FUNC_DEF)};
#undef API_FUNC_DEF

    for (s32 i = 0; i < COUNT_OF(ApiItems); i++)
        registerSquirrelFunction(core, ApiItems[i].func, ApiItems[i].name);

    registerSquirrelFunction(core, squirrel_dofile, "dofile");
    registerSquirrelFunction(core, squirrel_loadfile, "loadfile");

    sq_enabledebuginfo(vm, SQTrue);

}

static void closeSquirrel(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;

    if(core->currentVM)
    {
        sq_close(core->currentVM);
        core->currentVM = NULL;
    }
}

static bool initSquirrel(uli_mem* uli, const char* code)
{
    uli_core* core = (uli_core*)uli;

    closeSquirrel(uli);

    HSQUIRRELVM vm = core->currentVM = sq_open(100);
    squirrel_open_builtins(vm);

    sq_newclosure(vm, squirrel_errorHandler, 0);
    sq_seterrorhandler(vm);

    initAPI(core);

    {
        HSQUIRRELVM vm = core->currentVM;

        sq_settop(vm, 0);

        if((SQ_FAILED(sq_compilebuffer(vm, code, strlen(code), "squirrel", SQTrue))) ||
            (sq_pushroottable(vm), false) ||
            (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue))))
        {
            sq_getlasterror(vm);
            sq_tostring(vm, -1);
            const SQChar* errorString = "unknown error";
            sq_getstring(vm, -1, &errorString);

            if (core->data)
                core->data->error(core->data->data, errorString);

            sq_pop(vm, 2); // error and error string

            return false;
        }
    }

    return true;
}

static void errorReport(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;

    HSQUIRRELVM vm = core->currentVM;

    sq_getlasterror(vm);
    sq_tostring(vm, -1);
    const SQChar* errorString = "unknown error";
    sq_getstring(vm, -1, &errorString);

    if (core->data)
        core->data->error(core->data->data, errorString);
    sq_pop(vm, 3); // remove string, error and root table.
}

static void callSquirrelTick(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;

    HSQUIRRELVM vm = core->currentVM;

    if(vm)
    {
        sq_pushroottable(vm);
        sq_pushstring(vm, ULI_FN, -1);

        if (SQ_SUCCEEDED(sq_get(vm, -2)))
        {
            sq_pushroottable(vm);
            if(SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
            {
                errorReport(uli);
                return;
            }

#if defined(BUILD_DEPRECATED)
            // call OVR() callback for backward compatibility
            {
                sq_pushroottable(vm);
                sq_pushstring(vm, OVR_FN, -1);

                if(SQ_SUCCEEDED(sq_get(vm, -2)))
                {
                    OVR(core)
                    {
                        sq_pushroottable(vm);

                        if(SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
                        {
                            errorReport(uli);
                        }
                    }
                }
                else sq_poptop(vm);
            }
#endif
        }
        else
        {
            sq_pop(vm, 1);
            if (core->data)
                core->data->error(core->data->data, "'function ULI()...' isn't found :(");
        }
    }
}

static void callSquirrelBoot(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;

    HSQUIRRELVM vm = core->currentVM;

    if(vm)
    {
        sq_pushroottable(vm);
        sq_pushstring(vm, BOOT_FN, -1);

        if (SQ_SUCCEEDED(sq_get(vm, -2)))
        {
            sq_pushroottable(vm);
            if(SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue)))
            {
                errorReport(uli);
                return;
            }
        }
    }
}

static void callSquirrelIntCallback(uli_mem* uli, s32 value, void* data, const char* name)
{
    uli_core* core = (uli_core*)uli;
    HSQUIRRELVM vm = core->currentVM;

    if (vm)
    {
        sq_pushroottable(vm);
        sq_pushstring(vm, name, -1);
        if (SQ_SUCCEEDED(sq_get(vm, -2)))
        {
            sq_pushroottable(vm);
            sq_pushinteger(vm, value);

            if(SQ_FAILED(sq_call(vm, 2, SQFalse, SQTrue)))
            {
                sq_getlasterror(vm);
                sq_tostring(vm, -1);

                const SQChar* errorString = "unknown error";
                sq_getstring(vm, -1, &errorString);
                if (core->data)
                    core->data->error(core->data->data, errorString);
                sq_pop(vm, 3); // error string, error and root table
            }
        }
        else sq_poptop(vm);
    }
}

static void callSquirrelScanline(uli_mem* uli, s32 row, void* data)
{
    callSquirrelIntCallback(uli, row, data, SCN_FN);

    // try to call old scanline
    callSquirrelIntCallback(uli, row, data, "scanline");
}

static void callSquirrelBorder(uli_mem* uli, s32 row, void* data)
{
    callSquirrelIntCallback(uli, row, data, BDR_FN);
}

static void callSquirrelMenu(uli_mem* uli, s32 index, void* data)
{
    callSquirrelIntCallback(uli, index, data, MENU_FN);
}

static const char* const SquirrelKeywords [] =
{
    "base", "break", "case", "catch", "class", "clone",
    "continue", "const", "default", "delete", "else", "enum",
    "extends", "for", "foreach", "function", "if", "in",
    "local", "null", "resume", "return", "switch", "this",
    "throw", "try", "typeof", "while", "yield", "constructor",
    "instanceof", "true", "false", "static", "__LINE__", "__FILE__"
};

static inline bool isalnum_(char c) {return isalnum(c) || c == '_';}

static const uli_outline_item* getSquirrelOutline(const char* code, s32* size)
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
        static const char FuncString[] = "function ";

        ptr = strstr(ptr, FuncString);

        if(ptr)
        {
            ptr += sizeof FuncString - 1;

            const char* start = ptr;
            const char* end = start;

            while(*ptr)
            {
                char c = *ptr;

                if(isalnum_(c) || c == ':');
                else if(c == '(')
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

void evalSquirrel(uli_mem* uli, const char* code) {
    uli_core* core = (uli_core*)uli;
    HSQUIRRELVM vm = core->currentVM;

    // make sure that the Squirrel interpreter is initialized.
    if (vm == NULL)
    {
        if (!initSquirrel(uli, ""))
            return;
        vm = core->currentVM;
    }

    sq_settop(vm, 0);

    if((SQ_FAILED(sq_compilebuffer(vm, code, strlen(code), "squirrel", SQTrue))) ||
        (sq_pushroottable(vm), false) ||
        (SQ_FAILED(sq_call(vm, 1, SQFalse, SQTrue))))
    {
        sq_getlasterror(vm);
        sq_tostring(vm, -1);
        const SQChar* errorString = "unknown error";
        sq_getstring(vm, -1, &errorString);
        if (core->data)
            core->data->error(core->data->data, errorString);
    }

    sq_settop(vm, 0);
}

static const u8 DemoRom[] =
{
    #include "../build/assets/squirreldemo.uli.dat"
};

static const u8 MarkRom[] =
{
    #include "../build/assets/squirrelmark.uli.dat"
};

ULI_EXPORT const uli_script EXPORT_SCRIPT(Squirrel) =
{
    .id                 = 15,
    .name               = "squirrel",
    .fileExtension      = ".nut",
    .projectComment     = "//",
    {
      .init               = initSquirrel,
      .close              = closeSquirrel,
      .tick               = callSquirrelTick,
      .boot               = callSquirrelBoot,

      .callback           =
      {
        .scanline       = callSquirrelScanline,
        .border         = callSquirrelBorder,
        .menu           = callSquirrelMenu,
      },
    },

    .getOutline         = getSquirrelOutline,
    .eval               = evalSquirrel,

    .blockCommentStart  = "/*",
    .blockCommentEnd    = "*/",
    .blockCommentStart2 = NULL,
    .blockCommentEnd2   = NULL,
    .singleComment      = "//",
    .blockStringStart   = "@\"",
    .blockStringEnd     = "\"",
    .blockEnd           = "}",

    .keywords           = SquirrelKeywords,
    .keywordsCount      = COUNT_OF(SquirrelKeywords),

    .demo = {DemoRom, sizeof DemoRom},
    .mark = {MarkRom, sizeof MarkRom, "squirrelmark.uli"},
};
