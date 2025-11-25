// MIT License

// Copyright (c) 2017 Vadim Grigoruk @uli78 // grigoruk@gmail.com
// Copyright (c) 2020-2021 Julia Nelz <julia ~at~ nelz.pl>

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
#include <ctype.h>

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/error.h>
#include <mruby/throw.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/variable.h>
#include <mruby/value.h>
#include <mruby/string.h>

extern bool parse_note(const char* noteStr, s32* note, s32* octave);

typedef struct {
    struct mrb_state* mrb;
    struct mrbc_context* mrb_cxt;
} mrbVm;

static uli_core* CurrentMachine = NULL;
static inline uli_core* getMRubyMachine(mrb_state* mrb)
{
    return CurrentMachine;
}

static mrb_value mrb_peek(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    mrb_int address;
    mrb_int bits = BITS_IN_BYTE;
    mrb_get_args(mrb, "i|i", &address, &bits);

    return mrb_fixnum_value(core->api.peek(uli, address, bits));
}

static mrb_value mrb_poke(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int address, value;
    mrb_int bits = BITS_IN_BYTE;
    mrb_get_args(mrb, "ii|i", &address, &value, &bits);

    core->api.poke(uli, address, value, bits);

    return mrb_nil_value();
}

static mrb_value mrb_peek1(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int address;
    mrb_get_args(mrb, "i", &address);

    return mrb_fixnum_value(core->api.peek1(uli, address));
}

static mrb_value mrb_poke1(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int address, value;
    mrb_get_args(mrb, "ii", &address, &value);

    core->api.poke1(uli, address, value);

    return mrb_nil_value();
}

static mrb_value mrb_peek2(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int address;
    mrb_get_args(mrb, "i", &address);

    return mrb_fixnum_value(core->api.peek2(uli, address));
}

static mrb_value mrb_poke2(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int address, value;
    mrb_get_args(mrb, "ii", &address, &value);

    core->api.poke2(uli, address, value);

    return mrb_nil_value();
}

static mrb_value mrb_peek4(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int address;
    mrb_get_args(mrb, "i", &address);

    return mrb_fixnum_value(core->api.peek4(uli, address));
}

static mrb_value mrb_poke4(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int address, value;
    mrb_get_args(mrb, "ii", &address, &value);

    core->api.poke4(uli, address, value);

    return mrb_nil_value();
}

static mrb_value mrb_cls(mrb_state* mrb, mrb_value self)
{
    mrb_int color = 0;
    mrb_get_args(mrb, "|i", &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.cls(uli, color);

    return mrb_nil_value();
}

static mrb_value mrb_pix(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, color;
    mrb_int argc = mrb_get_args(mrb, "ii|i", &x, &y, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    if(argc == 3)
    {
        core->api.pix(uli, x, y, color, false);
        return mrb_nil_value();
    }
    else
    {
        return mrb_fixnum_value(core->api.pix(uli, x, y, 0, true));
    }
}

static mrb_value mrb_line(mrb_state* mrb, mrb_value self)
{
    mrb_float x0, y0, x1, y1;
    mrb_int color;
    mrb_get_args(mrb, "ffffi", &x0, &y0, &x1, &y1, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.line(uli, x0, y0, x1, y1, color);

    return mrb_nil_value();
}

static mrb_value mrb_rect(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, w, h, color;
    mrb_get_args(mrb, "iiiii", &x, &y, &w, &h, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.rect(uli, x, y, w, h, color);

    return mrb_nil_value();
}

static mrb_value mrb_rectb(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, w, h, color;
    mrb_get_args(mrb, "iiiii", &x, &y, &w, &h, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.rectb(uli, x, y, w, h, color);

    return mrb_nil_value();
}

static mrb_value mrb_circ(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, radius, color;
    mrb_get_args(mrb, "iiii", &x, &y, &radius, &color);

    if(radius >= 0) {
        uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

        core->api.circ(uli, x, y, radius, color);
    } else {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "radius must be greater than or equal 0");
    }

    return mrb_nil_value();
}

static mrb_value mrb_circb(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, radius, color;
    mrb_get_args(mrb, "iiii", &x, &y, &radius, &color);

    if(radius >= 0) {
        uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

        core->api.circb(uli, x, y, radius, color);
    } else {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "radius must be greater than or equal 0");
    }

    return mrb_nil_value();
}

static mrb_value mrb_elli(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, a, b, color;
    mrb_get_args(mrb, "iiiii", &x, &y, &a, &b, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.elli(uli, x, y, a, b, color);

    return mrb_nil_value();
}

static mrb_value mrb_ellib(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, a, b, color;
    mrb_get_args(mrb, "iiiii", &x, &y, &a, &b, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.ellib(uli, x, y, a, b, color);

    return mrb_nil_value();
}

static mrb_value mrb_paint(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, color;
    mrb_int bordercolor = -1;
    mrb_get_args(mrb, "iii|i", &x, &y, &color, &bordercolor);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.paint(uli, x, y, color, bordercolor);

    return mrb_nil_value();
}

static mrb_value mrb_tri(mrb_state* mrb, mrb_value self)
{
    mrb_float x1, y1, x2, y2, x3, y3;
    mrb_int color;
    mrb_get_args(mrb, "ffffffi", &x1, &y1, &x2, &y2, &x3, &y3, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.tri(uli, x1, y1, x2, y2, x3, y3, color);

    return mrb_nil_value();
}

static mrb_value mrb_trib(mrb_state* mrb, mrb_value self)
{
    mrb_float x1, y1, x2, y2, x3, y3;
    mrb_int color;
    mrb_get_args(mrb, "ffffffi", &x1, &y1, &x2, &y2, &x3, &y3, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.trib(uli, x1, y1, x2, y2, x3, y3, color);

    return mrb_nil_value();
}

static mrb_value mrb_ttri(mrb_state* mrb, mrb_value self)
{
    mrb_value chroma = mrb_fixnum_value(0xff);
    mrb_int src = uli_tiles_texture;

    mrb_float x1, y1, x2, y2, x3, y3, u1, v1, u2, v2, u3, v3, z1, z2, z3;
    mrb_int argc = mrb_get_args(mrb, "ffffffffffff|iofff",
            &x1, &y1, &x2, &y2, &x3, &y3,
            &u1, &v1, &u2, &v2, &u3, &v3,
            &src, &chroma, &z1, &z2, &z3);

    mrb_int count;
    u8 *chromas;
    if (mrb_array_p(chroma))
    {
        count = ARY_LEN(RARRAY(chroma));
        chromas = malloc(count * sizeof(u8));

        for (mrb_int i = 0; i < count; ++i)
        {
            chromas[i] = mrb_integer(mrb_ary_entry(chroma, i));
        }
    }
    else
    {
        count = 1;
        chromas = malloc(sizeof(u8));
        chromas[0] = mrb_integer(chroma);
    }

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.ttri(uli,
                        x1, y1, x2, y2, x3, y3,
                        u1, v1, u2, v2, u3, v3,
                        src, chromas, count, z1, z2, z3, argc == 17);

    free(chromas);

    return mrb_nil_value();
}


static mrb_value mrb_clip(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, w, h;
    mrb_int argc = mrb_get_args(mrb, "|iiii", &x, &y, &w, &h);

    if(argc == 0)
    {
        uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

        core->api.clip(uli, 0, 0, ULI78_WIDTH, ULI78_HEIGHT);
    }
    else if(argc == 4)
    {
        uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

        core->api.clip((uli_mem*)getMRubyMachine(mrb), x, y, w, h);
    }
    else mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid parameters, use clip(x,y,w,h) or clip()");

    return mrb_nil_value();
}

static mrb_value mrb_btnp(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int index, hold, period;
    mrb_int argc = mrb_get_args(mrb, "|iii", &index, &hold, &period);

    if (argc == 0)
    {
        return mrb_fixnum_value(core->api.btnp(uli, -1, -1, -1));
    }
    else if(argc == 1)
    {
        return mrb_bool_value(core->api.btnp(uli, index & 0x1f, -1, -1) != 0);
    }
    else if (argc == 3)
    {
        return mrb_bool_value(core->api.btnp(uli, index & 0x1f, hold, period) != 0);
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid params, btnp [ id [ hold period ] ]");
        return mrb_nil_value();
    }
}

static mrb_value mrb_btn(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int index, hold, period;
    mrb_int argc = mrb_get_args(mrb, "|i", &index, &hold, &period);

    if (argc == 0)
    {
        return mrb_bool_value(core->api.btn(uli, -1) != 0);
    }
    else if (argc == 1)
    {
        return mrb_bool_value(core->api.btn(uli, index & 0x1f) != 0);
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid params, btn [ id ]\n");
        return mrb_nil_value();
    }
}

static mrb_value mrb_spr(mrb_state* mrb, mrb_value self)
{
    mrb_int index, x, y;
    mrb_int w = 1, h = 1, scale = 1;
    mrb_int flip = uli_no_flip, rotate = uli_no_rotate;
    mrb_value colors_obj;
    static u8 colors[ULI_PALETTE_SIZE];
    mrb_int count = 0;

    mrb_int argc = mrb_get_args(mrb, "iii|oiiiii", &index, &x, &y, &colors_obj, &scale, &flip, &rotate, &w, &h);

    if(mrb_array_p(colors_obj))
    {
        for(; count < ULI_PALETTE_SIZE && count < ARY_LEN(RARRAY(colors_obj)); count++) // HACK
            colors[count] = (u8) mrb_int(mrb, mrb_ary_entry(colors_obj, count));
        count++;
    }
    else if(mrb_fixnum_p(colors_obj))
    {
        colors[0] = mrb_int(mrb, colors_obj);
        count = 1;
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "color must be either an array or a palette index");
        return mrb_nil_value();
    }

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.spr(uli, index, x, y, w, h, colors, count, scale, flip, rotate);

    return mrb_nil_value();
}

static mrb_value mrb_mget(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y;
    mrb_get_args(mrb, "ii", &x, &y);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    u8 value = core->api.mget(uli, x, y);
    return mrb_fixnum_value(value);
}

static mrb_value mrb_mset(mrb_state* mrb, mrb_value self)
{
    mrb_int x, y, val;
    mrb_get_args(mrb, "iii", &x, &y, &val);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.mset(uli, x, y, val);

    return mrb_nil_value();
}

static mrb_value mrb_tstamp(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    return mrb_fixnum_value(core->api.tstamp(uli));
}

static mrb_value mrb_vbank(mrb_state* mrb, mrb_value self)
{
    uli_core *core = getMRubyMachine(mrb);
    uli_mem *uli = (uli_mem*)core;

    s32 prev = core->state.vbank.id;

    mrb_int vbank;
    mrb_int argc = mrb_get_args(mrb, "|i", &vbank);

    if (argc >= 1)
    {
        core->api.vbank(uli, vbank);
    }

    return mrb_fixnum_value(prev);
}

static mrb_value mrb_fget(mrb_state* mrb, mrb_value self)
{
    mrb_int index, flag;
    mrb_get_args(mrb, "ii", &index, &flag);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    return mrb_bool_value(core->api.fget(uli, index, flag));
}

static mrb_value mrb_fset(mrb_state* mrb, mrb_value self)
{
    mrb_int index, flag;
    mrb_bool value;
    mrb_get_args(mrb, "iib", &index, &flag, &value);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.fset(uli, index, flag, value);

    return mrb_nil_value();
}

static mrb_value mrb_fft(mrb_state* mrb, mrb_value self)
{
    mrb_int start_freq, end_freq = -1;
    mrb_int argc = mrb_get_args(mrb, "i|i", &start_freq, &end_freq);

    uli_core* core = getMRubyMachine(mrb);
    uli_mem* uli = (uli_mem*)core;

    if (argc == 0)
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid params, fft [ start_freq end_freq ]\n");
        return mrb_nil_value();
    }
    else
    {
        return mrb_float_value(mrb, core->api.fft(uli, start_freq, end_freq));
    }
}

static mrb_value mrb_ffts(mrb_state* mrb, mrb_value self)
{
    mrb_int start_freq, end_freq = -1;
    mrb_int argc = mrb_get_args(mrb, "i|i", &start_freq, &end_freq);

    uli_core* core = getMRubyMachine(mrb);
    uli_mem* uli = (uli_mem*)core;

    if (argc == 0)
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid params, ffts [ start_freq end_freq ]\n");
        return mrb_nil_value();
    }
    else
    {
        return mrb_float_value(mrb, core->api.ffts(uli, start_freq, end_freq));
    }
}

typedef struct
{
    mrb_state* mrb;
    mrb_value block;
} RemapData;

static void remapCallback(void* data, s32 x, s32 y, RemapResult* result)
{
    RemapData* remap = (RemapData*)data;
    mrb_state* mrb = remap->mrb;

    mrb_value vals[] = {
        mrb_fixnum_value(result->index),
        mrb_fixnum_value(x),
        mrb_fixnum_value(y)
    };
    mrb_value out = mrb_yield_argv(mrb, remap->block, 3, vals);

    if (mrb_array_p(out))
    {
        mrb_int len = ARY_LEN(RARRAY(out));
        if (len > 3) len = 3;
        switch (len)
        {
        case 3:
            result->rotate = mrb_int(mrb, mrb_ary_entry(out, 2));
        case 2:
            result->flip = mrb_int(mrb, mrb_ary_entry(out, 1));
        case 1:
            result->index = mrb_int(mrb, mrb_ary_entry(out, 0));
        }
    }
    else
    {
        result->index = mrb_int(mrb, out);
    }
}

static mrb_value mrb_map(mrb_state* mrb, mrb_value self)
{
    mrb_int x = 0;
    mrb_int y = 0;
    mrb_int w = ULI_MAP_SCREEN_WIDTH;
    mrb_int h = ULI_MAP_SCREEN_HEIGHT;
    mrb_int sx = 0;
    mrb_int sy = 0;
    mrb_value chroma;
    mrb_int scale = 1;
    mrb_value block;

    mrb_int argc = mrb_get_args(mrb, "|iiiiiioi&", &x, &y, &w, &h, &sx, &sy, &chroma, &scale, &block);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    mrb_int count;
    u8 *chromas;

    if (mrb_array_p(chroma))
    {
        count = ARY_LEN(RARRAY(chroma));
        chromas = malloc(count * sizeof(u8));

        for (mrb_int i = 0; i < count; ++i)
        {
            chromas[i] = mrb_integer(mrb_ary_entry(chroma, i));
        }
    }
    else
    {
        count = 1;
        chromas = malloc(sizeof(u8));
        chromas[0] = mrb_integer(chroma);
    }

    if (mrb_proc_p(block))
    {
        RemapData data = { mrb, block };
        core->api.map(uli, x, y, w, h, sx, sy, chromas, count, scale, remapCallback, &data);
    }
    else
    {
        core->api.map(uli, x, y, w, h, sx, sy, chromas, count, scale, NULL, NULL);
    }

    free(chromas);

    return mrb_nil_value();
}

static mrb_value mrb_music(mrb_state* mrb, mrb_value self)
{
    mrb_int track = 0;
    mrb_int frame = -1;
    mrb_int row = -1;
    bool loop = true;
    bool sustain = false;
    mrb_int tempo = -1;
    mrb_int speed = -1;

    mrb_int argc = mrb_get_args(mrb, "|iiibbii", &track, &frame, &row, &loop, &sustain, &tempo, &speed);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->api.music(uli, -1, 0, 0, false, false, -1, -1);

    if(track >= 0)
    {
        if(track > MUSIC_TRACKS - 1)
            mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid music track index");
        else
            core->api.music(uli, track, frame, row, loop, sustain, tempo, speed);
    }

    return mrb_nil_value();
}

static mrb_value mrb_sfx(mrb_state* mrb, mrb_value self)
{
    mrb_int index;

    mrb_value note_obj;
    s32 note = -1;
    mrb_int octave = -1;
    mrb_int duration = -1;
    mrb_int channel = 0;
    mrb_value volume = mrb_int_value(mrb, MAX_VOLUME);
    mrb_int volumes[ULI78_SAMPLE_CHANNELS];
    mrb_int speed = SFX_DEF_SPEED;

    mrb_int argc = mrb_get_args(mrb, "i|oiio!i", &index, &note_obj, &duration, &channel, &volume, &speed);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    if (mrb_array_p(volume))
    {
        for (mrb_int i = 0; i < ULI78_SAMPLE_CHANNELS; ++i)
        {
            volumes[i] = mrb_integer(mrb_ary_entry(volume, i));
        }
    }
    else if (mrb_fixnum_p(volume))
    {
        for (size_t ch = 0; ch < ULI78_SAMPLE_CHANNELS; ++ch)
        {
            volumes[ch] = mrb_integer(volume);
        }
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "volume must be an integer or a array of integers per channel");
        return mrb_nil_value();
    }

    if(index < SFX_COUNT)
    {
        if (index >= 0)
        {
            uli_sample* effect = uli->ram->sfx.samples.data + index;

            note = effect->note;
            octave = effect->octave;
            speed = effect->speed;
        }

        if (argc >= 2)
        {
            if (mrb_fixnum_p(note_obj))
            {
                mrb_int id = mrb_integer(note_obj);
                note = id % NOTES;
                octave = id / NOTES;
            }
            else if (mrb_string_p(note_obj))
            {
                const char* noteStr = mrb_str_to_cstr(mrb, mrb_funcall(mrb, note_obj, "to_s", 0));

                s32 octave32;
                if (!parse_note(noteStr, &note, &octave32))
                {
                    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid note, should be like C#4");
                    return mrb_nil_value();
                }
                octave = octave32;
            }
            else
            {
                mrb_raise(mrb, E_ARGUMENT_ERROR, "note must be either an integer number or a string like \"C#4\"");
                return mrb_nil_value();
            }
        }

        if (channel >= 0 && channel < ULI_SOUND_CHANNELS)
        {
            core->api.sfx(uli, index, note, octave, duration, channel, volumes[0] & 0xf, volumes[1] & 0xf, speed);
        }
        else mrb_raise(mrb, E_ARGUMENT_ERROR, "unknown channel");
    }
    else mrb_raise(mrb, E_ARGUMENT_ERROR, "unknown sfx index");

    return mrb_nil_value();
}

static mrb_value mrb_sync(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    mrb_int mask = 0;
    mrb_int bank = 0;
    mrb_bool toCart = false;

    mrb_int argc = mrb_get_args(mrb, "|iib", &mask, &bank, &toCart);

    if(bank >= 0 && bank < ULI_BANKS)
        core->api.sync(uli, mask, bank, toCart);
    else
        mrb_raise(mrb, E_ARGUMENT_ERROR, "sync() error, invalid bank");

    return mrb_nil_value();
}

static mrb_value mrb_reset(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    core->state.initialized = false;

    return mrb_nil_value();
}

static mrb_value mrb_key(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    mrb_int key;
    mrb_int argc = mrb_get_args(mrb, "|i", &key);

    if (argc == 0)
        return mrb_bool_value(core->api.key(uli, uli_key_unknown));
    else
    {
        if(key < uli_key_escape)
            return mrb_bool_value(core->api.key(uli, key));
        else
        {
            mrb_raise(mrb, E_ARGUMENT_ERROR, "unknown keyboard code");
            return mrb_nil_value();
        }
    }
}

static mrb_value mrb_keyp(mrb_state* mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;


    mrb_int key, hold, period;
    mrb_int argc = mrb_get_args(mrb, "|iii", &key, &hold, &period);

    if (argc == 0)
    {
        return mrb_fixnum_value(core->api.keyp(uli, -1, -1, -1));
    }
    else if (key >= uli_key_escape)
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "unknown keyboard code");
    }
    else if(argc == 1)
    {
        return mrb_bool_value(core->api.keyp(uli, key, -1, -1));
    }
    else if (argc == 3)
    {
        return mrb_bool_value(core->api.keyp(uli, key, hold, period));
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid params, btnp [ id [ hold period ] ]");
        return mrb_nil_value();
    }
}

static mrb_value mrb_memcpy(mrb_state* mrb, mrb_value self)
{
    mrb_int dest, src, size;
    mrb_int argc = mrb_get_args(mrb, "iii", &dest, &src, &size);

    s32 bound = sizeof(uli_ram) - size;

    if(size >= 0 && size <= sizeof(uli_ram) && dest >= 0 && src >= 0 && dest <= bound && src <= bound)
    {
        uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;
        core->api.memcpy(uli, dest, src, size);
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "uli address not in range!");
    }

    return mrb_nil_value();
}

static mrb_value mrb_memset(mrb_state* mrb, mrb_value self)
{
    mrb_int dest, value, size;
    mrb_int argc = mrb_get_args(mrb, "iii", &dest, &value, &size);

    s32 bound = sizeof(uli_ram) - size;

    if(size >= 0 && size <= sizeof(uli_ram) && dest >= 0 && dest <= bound)
    {
        uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

        core->api.memset(uli, dest, value, size);
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "uli address not in range!");
    }

    return mrb_nil_value();
}

static char* mrb_value_to_cstr(mrb_state* mrb, mrb_value value)
{
    mrb_value str = mrb_funcall(mrb, value, "to_s", 0);
    return mrb_str_to_cstr(mrb, str);
}

static mrb_value mrb_font(mrb_state* mrb, mrb_value self)
{
    mrb_value text_obj;
    mrb_int x = 0;
    mrb_int y = 0;
    mrb_int width = ULI_SPRITESIZE;
    mrb_int height = ULI_SPRITESIZE;
    mrb_int chromakey = 0;
    mrb_bool fixed = false;
    mrb_int scale = 1;
    mrb_bool alt = false;
    mrb_get_args(mrb, "S|iiiiibib",
            &text_obj, &x, &y, &chromakey,
            &width, &height, &fixed, &scale, &alt);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    if(scale == 0)
        return mrb_fixnum_value(0);

    const char* text = mrb_value_to_cstr(mrb, text_obj);

    s32 size = core->api.font(uli, text, x, y, (u8*)&chromakey, 1, width, height, fixed, scale, alt);
    return mrb_fixnum_value(size);
}

static mrb_value mrb_print(mrb_state* mrb, mrb_value self)
{
    mrb_value text_obj;
    mrb_int x = 0;
    mrb_int y = 0;
    mrb_int color = ULI_PALETTE_SIZE-1;
    mrb_bool fixed = false;
    mrb_int scale = 1;
    mrb_bool alt = false;
    mrb_get_args(mrb, "S|iiibib",
            &text_obj, &x, &y, &color,
            &fixed, &scale, &alt);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    if(scale == 0)
        return mrb_fixnum_value(0);

    const char* text = mrb_str_to_cstr(mrb, text_obj);

    s32 size = core->api.print(uli, text, x, y, color, fixed, scale, alt);
    return mrb_fixnum_value(size);
}

static mrb_value mrb_trace(mrb_state *mrb, mrb_value self)
{
    mrb_value text_obj;
    mrb_int color = ULI_DEFAULT_COLOR;
    mrb_get_args(mrb, "o|i", &text_obj, &color);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    const char* text = mrb_value_to_cstr(mrb, text_obj);
    core->data->trace(core->data->data, text, color);

    return mrb_nil_value();
}

static mrb_value mrb_pmem(mrb_state *mrb, mrb_value self)
{
    mrb_int index, value;
    mrb_int argc = mrb_get_args(mrb, "i|i", &index, &value);

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    if(index < ULI_PERSISTENT_SIZE)
    {
        u32 val = core->api.pmem((uli_mem*)core, index, 0, false);

        if(argc == 2)
        {
            u32 val = core->api.pmem((uli_mem*)core, index, value, true);
        }

        return mrb_fixnum_value(val);
    }
    else
    {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid persistent uli index");

        return mrb_nil_value();
    }
}

static mrb_value mrb_time(mrb_state *mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;
    return mrb_float_value(mrb, core->api.time(uli));
}

static mrb_value mrb_exit(mrb_state *mrb, mrb_value self)
{
    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;
    core->data->exit(core->data->data);

    return mrb_nil_value();
}

static mrb_value mrb_mouse(mrb_state *mrb, mrb_value self)
{
    mrb_value sym_x = mrb_symbol_value(mrb_intern_cstr(mrb, "x"));
    mrb_value sym_y = mrb_symbol_value(mrb_intern_cstr(mrb, "y"));
    mrb_value sym_left = mrb_symbol_value(mrb_intern_cstr(mrb, "left"));
    mrb_value sym_middle = mrb_symbol_value(mrb_intern_cstr(mrb, "middle"));
    mrb_value sym_right = mrb_symbol_value(mrb_intern_cstr(mrb, "right"));
    mrb_value sym_scrollx = mrb_symbol_value(mrb_intern_cstr(mrb, "scrollx"));
    mrb_value sym_scrolly = mrb_symbol_value(mrb_intern_cstr(mrb, "scrolly"));

    uli_core* core = getMRubyMachine(mrb); uli_mem* uli = (uli_mem*)core;

    uli_point pos = core->api.mouse(uli);
    const uli78_mouse* mouse = &uli->ram->input.mouse;

    mrb_value hash = mrb_hash_new(mrb);

    mrb_hash_set(mrb, hash, sym_x, mrb_fixnum_value(pos.x));
    mrb_hash_set(mrb, hash, sym_y, mrb_fixnum_value(pos.y));
    mrb_hash_set(mrb, hash, sym_left, mrb_bool_value(mouse->left));
    mrb_hash_set(mrb, hash, sym_middle, mrb_bool_value(mouse->middle));
    mrb_hash_set(mrb, hash, sym_right, mrb_bool_value(mouse->right));
    mrb_hash_set(mrb, hash, sym_scrollx, mrb_fixnum_value(mouse->scrollx));
    mrb_hash_set(mrb, hash, sym_scrolly, mrb_fixnum_value(mouse->scrolly));

    return hash;
}

static void closeMRuby(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;

    if(core->currentVM)
    {
        mrbVm *currentVM = (mrbVm*)core->currentVM;
        mrbc_context_free(currentVM->mrb, currentVM->mrb_cxt);
        mrb_close(currentVM->mrb);
        currentVM->mrb_cxt = NULL;
        currentVM->mrb = NULL;

        free(currentVM);
        CurrentMachine = core->currentVM = NULL;
    }
}

static mrb_bool catcherr(uli_core* core)
{
    mrb_state* mrb = ((mrbVm*)core->currentVM)->mrb;
    if (mrb->exc)
    {
        mrb_value ex = mrb_obj_value(mrb->exc);
        mrb_value bt = mrb_exc_backtrace(mrb, ex);
        if (!mrb_array_p(bt))
            bt = mrb_get_backtrace(mrb);
        mrb_value insp = mrb_inspect(mrb, ex);
        mrb_ary_unshift(mrb, bt, insp);
        mrb_value msg = mrb_ary_join(mrb, bt, mrb_str_new_cstr(mrb, "\n"));
        char* cmsg = mrb_str_to_cstr(mrb, msg);
        core->data->error(core->data->data, cmsg);
        mrb->exc = NULL;

        return false;
    }

    return true;
}

static bool initMRuby(uli_mem* uli, const char* code)
{
    uli_core* core = (uli_core*)uli;

    closeMRuby(uli);

    CurrentMachine = core;

    core->currentVM = malloc(sizeof(mrbVm));
    mrbVm *currentVM = (mrbVm*)core->currentVM;

    mrb_state* mrb = currentVM->mrb = mrb_open();
    mrbc_context* mrb_cxt = currentVM->mrb_cxt = mrbc_context_new(mrb);
    mrb_cxt->capture_errors = 1;
    mrbc_filename(mrb, mrb_cxt, "user code");

#define API_FUNC_DEF(name, _, __, nparam, nrequired, callback, ...) {mrb_ ## name, nrequired, (nparam - nrequired), callback, #name},
    static const struct{mrb_func_t func; s32 nrequired; s32 noptional; bool block; const char* name;} ApiItems[] = {ULI_API_LIST(API_FUNC_DEF)};
#undef API_FUNC_DEF

    for (s32 i = 0; i < COUNT_OF(ApiItems); ++i)
    {
        mrb_aspec args = MRB_ARGS_NONE();
        if (ApiItems[i].nrequired > 0)
            args |= MRB_ARGS_REQ(ApiItems[i].nrequired);
        if (ApiItems[i].noptional > 0)
            args |= MRB_ARGS_OPT(ApiItems[i].noptional);
        if (ApiItems[i].block)
            args |= MRB_ARGS_BLOCK();

        mrb_define_method(mrb, mrb->kernel_module, ApiItems[i].name, ApiItems[i].func, args);
    }

    mrb_load_string_cxt(mrb, code, mrb_cxt);
    return catcherr(core);
}

static void evalMRuby(uli_mem* uli, const char* code) {
    uli_core* core = (uli_core*)uli;

    mrbVm* vm=(mrbVm*)core->currentVM;
    if(!vm || !vm->mrb)
        return;

    mrb_state* mrb = vm->mrb;

    if (((mrbVm*)core->currentVM)->mrb_cxt)
    {
        mrbc_context_free(mrb, ((mrbVm*)core->currentVM)->mrb_cxt);
    }

    mrbc_context* mrb_cxt = ((mrbVm*)core->currentVM)->mrb_cxt = mrbc_context_new(mrb);
    mrbc_filename(mrb, mrb_cxt, "eval");
    mrb_load_string_cxt(mrb, code, mrb_cxt);
    catcherr(core);
}

static void callMRubyTick(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;
    const char* TicFunc = ULI_FN;

    mrb_state* mrb = ((mrbVm*)core->currentVM)->mrb;

    if(mrb)
    {
        if (mrb_respond_to(mrb, mrb_top_self(mrb), mrb_intern_cstr(mrb, TicFunc)))
        {
            mrb_funcall(mrb, mrb_top_self(mrb), TicFunc, 0);
            catcherr(core);
        }
        else
        {
            core->data->error(core->data->data, "'def ULI...' isn't found :(");
        }
    }
}

static void callMRubyBoot(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;
    const char* BootFunc = BOOT_FN;

    mrb_state* mrb = ((mrbVm*)core->currentVM)->mrb;

    if(mrb)
    {
        if (mrb_respond_to(mrb, mrb_top_self(mrb), mrb_intern_cstr(mrb, BootFunc)))
        {
            mrb_funcall(mrb, mrb_top_self(mrb), BootFunc, 0);
            catcherr(core);
        }
    }
}

static void callMRubyIntCallback(uli_mem* uli, s32 value, void* data, const char* name)
{
    uli_core* core = (uli_core*)uli;
    mrb_state* mrb = ((mrbVm*)core->currentVM)->mrb;

    if (mrb && mrb_respond_to(mrb, mrb_top_self(mrb), mrb_intern_cstr(mrb, name)))
    {
        mrb_funcall(mrb, mrb_top_self(mrb), name, 1, mrb_fixnum_value(value));
        catcherr(core);
    }
}

static void callMRubyScanline(uli_mem* uli, s32 row, void* data)
{
    callMRubyIntCallback(uli, row, data, SCN_FN);

    callMRubyIntCallback(uli, row, data, "scanline");
}

static void callMRubyBorder(uli_mem* uli, s32 row, void* data)
{
    callMRubyIntCallback(uli, row, data, BDR_FN);
}

static void callMRubyMenu(uli_mem* uli, s32 index, void* data)
{
    callMRubyIntCallback(uli, index, data, MENU_FN);
}

/**
 * External Resources
 * ==================
 *
 * [Outdated official documentation regarding syntax](https://ruby-doc.org/docs/ruby-doc-bundle/Manual/man-1.4/syntax.html#resword)
 * [Ruby for dummies reserved word listing](https://www.dummies.com/programming/ruby/rubys-reserved-words/)
 */
static const char* const MRubyKeywords [] =
{
    "BEGIN", "class", "ensure", "nil", "self", "when",
    "END", "def", "false", "not", "super", "while",
    "alias", "defined", "for", "or", "then", "yield",
    "and", "do", "if", "redo", "true",
    "begin", "else", "in", "rescue", "undef",
    "break", "elsif", "module", "retry", "unless",
    "case", "end", "next", "return", "until",
    "__FILE__", "__LINE__"
};

static inline bool isalnum_(char c) {return isalnum(c) || c == '_' || c == '?' || c == '=' || c == '!';}

static const uli_outline_item* getMRubyOutline(const char* code, s32* size)
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
        static const char FuncString[] = "def ";

        ptr = strstr(ptr, FuncString);

        if(ptr)
        {
            ptr += sizeof FuncString - 1;

            const char* start = ptr;
            const char* end = start;

            while(*ptr)
            {
                char c = *ptr;

                if(isalnum_(c));
                else if(c == '(' || c == ' ' || c == '\n')
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

static const u8 DemoRom[] =
{
    #include "../build/assets/rubydemo.uli.dat"
};

static const u8 MarkRom[] =
{
    #include "../build/assets/rubymark.uli.dat"
};

ULI_EXPORT const uli_script EXPORT_SCRIPT(Ruby) =
{
    .id                 = 11,
    .name               = "ruby",
    .fileExtension      = ".rb",
    .projectComment     = "#",
    .init               = initMRuby,
    .close              = closeMRuby,
    .tick               = callMRubyTick,
    .boot               = callMRubyBoot,

    .callback           =
    {
        .scanline       = callMRubyScanline,
        .border         = callMRubyBorder,
        .menu           = callMRubyMenu,
    },

    .getOutline         = getMRubyOutline,
    .eval               = evalMRuby,

    .blockCommentStart  = "=begin",
    .blockCommentEnd    = "=end",
    .blockCommentStart2 = NULL,
    .blockCommentEnd2   = NULL,
    .singleComment      = "#",
    .blockStringStart   = NULL,
    .blockStringEnd     = NULL,
    .blockEnd           = "end",

    .keywords           = MRubyKeywords,
    .keywordsCount      = COUNT_OF(MRubyKeywords),

    .demo = {DemoRom, sizeof DemoRom},
    .mark = {MarkRom, sizeof MarkRom, "rubymark.uli"},
};
