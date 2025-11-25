// MIT License

// Copyright (c) 2020 Vadim Grigoruk @uli78 // grigoruk@gmail.com

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

#include "fftdata.h"
#include "../ext/fft.h"

#include "api.h"
#include "core.h"
#include "tilesheet.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <time.h>

#include "uli_assert.h"

#ifdef _3DS
#include <3ds.h>
#endif

#include "blip_buf.h"

static_assert(ULI_BANK_BITS == 3,                   "uli_bank_bits");
static_assert(sizeof(uli_map) < 1024 * 32,          "uli_map");
static_assert(sizeof(uli_rgb) == 3,                 "uli_rgb");
static_assert(sizeof(uli_palette) == 48,            "uli_palette");
static_assert(sizeof(((uli_vram *)0)->vars) == 4,   "uli_vram vars");
static_assert(sizeof(uli_vram) == ULI_VRAM_SIZE,    "uli_vram");
static_assert(sizeof(uli_ram) == ULI_RAM_SIZE,      "uli_ram");

u8 uli_api_peek(uli_mem* memory, s32 address, s32 bits)
{
    if (address < 0)
        return 0;

    const u8* ram = (u8*)memory->ram;
    enum{RamBits = sizeof(uli_ram) * BITS_IN_BYTE};

    switch(bits)
    {
    case 1: if(address < RamBits / 1) return uli_tool_peek1(ram, address);
    case 2: if(address < RamBits / 2) return uli_tool_peek2(ram, address);
    case 4: if(address < RamBits / 4) return uli_tool_peek4(ram, address);
    case 8: if(address < RamBits / 8) return ram[address];
    }

    return 0;
}

void uli_api_poke(uli_mem* memory, s32 address, u8 value, s32 bits)
{
    if (address < 0)
        return;

    uli_core* core = (uli_core*)memory;
    u8* ram = (u8*)memory->ram;
    enum{RamBits = sizeof(uli_ram) * BITS_IN_BYTE};

    switch(bits)
    {
    case 1: if(address < RamBits / 1) uli_tool_poke1(ram, address, value); break;
    case 2: if(address < RamBits / 2) uli_tool_poke2(ram, address, value); break;
    case 4: if(address < RamBits / 4) uli_tool_poke4(ram, address, value); break;
    case 8: if(address < RamBits / 8) ram[address] = value; break;
    }
}

u8 uli_api_peek4(uli_mem* memory, s32 address)
{
    return uli_api_peek(memory, address, 4);
}

u8 uli_api_peek1(uli_mem* memory, s32 address)
{
    return uli_api_peek(memory, address, 1);
}

void uli_api_poke1(uli_mem* memory, s32 address, u8 value)
{
    uli_api_poke(memory, address, value, 1);
}

u8 uli_api_peek2(uli_mem* memory, s32 address)
{
    return uli_api_peek(memory, address, 2);
}

void uli_api_poke2(uli_mem* memory, s32 address, u8 value)
{
    uli_api_poke(memory, address, value, 2);
}

void uli_api_poke4(uli_mem* memory, s32 address, u8 value)
{
    uli_api_poke(memory, address, value, 4);
}

void uli_api_memcpy(uli_mem* memory, s32 dst, s32 src, s32 size)
{
    uli_core* core = (uli_core*)memory;
    s32 bound = sizeof(uli_ram) - size;

    if (size >= 0
        && size <= sizeof(uli_ram)
        && dst >= 0
        && src >= 0
        && dst <= bound
        && src <= bound)
    {
        u8* base = (u8*)memory->ram;
        memmove(base + dst, base + src, size);
    }
}

void uli_api_memset(uli_mem* memory, s32 dst, u8 val, s32 size)
{
    uli_core* core = (uli_core*)memory;
    s32 bound = sizeof(uli_ram) - size;

    if (size >= 0
        && size <= sizeof(uli_ram)
        && dst >= 0
        && dst <= bound)
    {
        u8* base = (u8*)memory->ram;
        memset(base + dst, val, size);
    }
}

void uli_api_trace(uli_mem* memory, const char* text, u8 color)
{
    uli_core* core = (uli_core*)memory;
    core->data->trace(core->data->data, text ? text : "nil", color);
}

u32 uli_api_pmem(uli_mem* uli, s32 index, u32 value, bool set)
{
    u32 old = uli->ram->persistent.data[index];

    if (set)
        uli->ram->persistent.data[index] = value;

    return old;
}

void uli_api_exit(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;
    core->data->exit(core->data->data);
}

static inline void sync(void* dst, void* src, s32 size, bool rev)
{
    if(rev)
        SWAP(dst, src, void*);

    memcpy(dst, src, size);
}

static inline uli_vram* vbank0(uli_core* core)
{
    return core->state.vbank.id ? &core->state.vbank.mem : &core->memory.ram->vram;
}

static inline uli_vram* vbank1(uli_core* core)
{
    return core->state.vbank.id ? &core->memory.ram->vram : &core->state.vbank.mem;
}

void uli_api_sync(uli_mem* uli, u32 mask, s32 bank, bool toCart)
{
    uli_core* core = (uli_core*)uli;

    static const struct { s32 bank; s32 ram; s32 size; u8 mask; } Sections[] =
    {
#define ULI_SYNC_DEF(CART, RAM, ...) { offsetof(uli_bank, CART), offsetof(uli_ram, RAM), sizeof(uli_##CART), uli_sync_##CART },
        ULI_SYNC_LIST(ULI_SYNC_DEF)
#undef  ULI_SYNC_DEF
    };

    enum { Count = COUNT_OF(Sections), Mask = (1 << Count) - 1 };

    if (mask == 0) mask = Mask;

    mask &= ~core->state.synced & Mask;

    assert(bank >= 0 && bank < ULI_BANKS);

    for (s32 i = 0; i < Count; i++)
    {
        u32 sectionMask = Sections[i].mask;
        if(mask & sectionMask)
        {
            uli_bank* bankPtr = &uli->cart.banks[bank];
            s32 size = Sections[i].size;

            if(sectionMask == uli_sync_palette)
            {
                // palette syncing is a special case where we copy both vbank0 and vbank1 palettes
                sync(vbank0(core)->palette.data, bankPtr->palette.vbank0.data, size, toCart);

                if(!EMPTY(bankPtr->palette.vbank1.data))
                    sync(vbank1(core)->palette.data, bankPtr->palette.vbank1.data, size, toCart);
            }
            else
            {
                sync(uli->ram->data + Sections[i].ram, (u8*)bankPtr + Sections[i].bank, size, toCart);
            }
        }
    }

    core->state.synced |= mask;
}

double uli_api_time(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;
    return (double)(core->data->counter(core->data->data) - core->data->start) * 1000.0 / core->data->freq(core->data->data);
}

s32 uli_api_tstamp(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;
    return (s32)time(NULL);
}

static void updateSaveid(uli_mem* memory)
{
    memset(memory->saveid, 0, sizeof memory->saveid);
    const char* saveid = uli_tool_metatag(memory->cart.code.data, "saveid", NULL);
    if (*saveid)
    {
        strncpy(memory->saveid, saveid, ULI_SAVEID_SIZE - 1);
    }
}

static void soundClear(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;

    for (s32 i = 0; i < ULI_SOUND_CHANNELS; i++)
    {
        static const uli_channel_data EmptyChannel =
        {
            .tick = -1,
            .pos = NULL,
            .index = -1,
            .note = 0,
            .volume = {0, 0},
            .speed = 0,
            .duration = -1,
        };

        memcpy(&core->state.music.channels[i], &EmptyChannel, sizeof EmptyChannel);
        memcpy(&core->state.sfx.channels[i], &EmptyChannel, sizeof EmptyChannel);

        memset(core->state.sfx.channels[i].pos = &memory->ram->sfxpos[i], -1, sizeof(uli_sfx_pos));
        memset(core->state.music.channels[i].pos = &core->state.music.sfxpos[i], -1, sizeof(uli_sfx_pos));
    }

    memset(&memory->ram->registers, 0, sizeof memory->ram->registers);
    memset(&memory->ram->pcm, 0, sizeof memory->ram->pcm);
    memset(memory->product.samples.buffer, 0, memory->product.samples.count * ULI78_SAMPLESIZE);

    uli_api_music(memory, -1, 0, 0, false, false, -1, -1);
}

static void resetVbank(uli_mem* memory)
{
    ZEROMEM(memory->ram->vram.vars);

    static const u8 DefaultMapping[] = { 0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe };
    memcpy(memory->ram->vram.mapping, DefaultMapping, sizeof DefaultMapping);
    memory->ram->vram.palette = memory->cart.bank0.palette.vbank0;
    memory->ram->vram.blit.segment = ULI_DEFAULT_BLIT_MODE;
}

static void font2ram(uli_mem* memory)
{
  memory->ram->font = (uli_font) {
        .regular =
        {
            .data =
            {
                #include "font.inl"
            },
	    {
	      {
		.width = ULI_FONT_WIDTH,
		.height = ULI_FONT_HEIGHT,
	      }
	    }
        },

        .alt =
        {
            .data =
            {
                #include "altfont.inl"
            },
	    {
	      {
		.width = ULI_ALTFONT_WIDTH,
		.height = ULI_FONT_HEIGHT,
	      }
	    }
        },
  };
}

void uli_api_reset(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;

    // keyboard state is critical and must be preserved across API resets.
    // Often `uli_api_reset` is called to effect transitions between modes
    // yet we still need to know when the key WAS pressed after the
    // transition - to prevent it from counting as a second keypress.
    //
    // So why presev `now` not `previous`?  this is most often called in
    // the middle of a tick... so we preserve now, which during `tick_end`
    // is copied to previous. This duplicates the prior behavior of
    // `ram.input.keyboard` (which existing outside `state`).
    u32 kb_now = core->state.keyboard.now.data;
    u32 gp_now = core->state.gamepads.now.data;
    ZEROMEM(core->state);
    core->state.keyboard.now.data = kb_now;
    core->state.gamepads.now.data = gp_now;
    uli_api_clip(memory, 0, 0, ULI78_WIDTH, ULI78_HEIGHT);

    resetVbank(memory);

    VBANK(memory, 1)
    {
        resetVbank(memory);
    }

    memory->ram->vram.vars.cursor.sprite = uli_cursor_arrow;
    memory->ram->vram.vars.cursor.system = true;
    memory->ram->input.mouse.relative = 0;

    soundClear(memory);
    updateSaveid(memory);
    font2ram(memory);
}

static void cart2ram(uli_mem* memory)
{
    font2ram(memory);

    enum
    {
#define     ULI_SYNC_DEF(NAME, _, INDEX) sync_##NAME = INDEX,
            ULI_SYNC_LIST(ULI_SYNC_DEF)
#undef      ULI_SYNC_DEF
            count,
            all = (1 << count) - 1,
            noscreen = BITCLEAR(all, sync_screen)
    };

    // don't sync empty screen
    uli_api_sync(memory, EMPTY(memory->cart.bank0.screen.data) ? noscreen : all, 0, false);
}

static void uli_close_current_vm(uli_core* core)
{
    // close previous VM if any
    if(core->currentVM)
    {
        // printf("Closing VM of %s, %d\n", core->currentScript->name, core->currentVM);
        core->currentScript->close( (uli_mem*)core );
        core->currentVM = NULL;
    }
    if (core->memory.ram == NULL) {
        core->memory.ram = core->memory.base_ram;
    }
}

static bool uli_init_vm(uli_core* core, const char* code, const uli_script* config)
{
    uli_close_current_vm(core);
    // set current script config and init
    core->currentScript = config;

    bool done = config->init((uli_mem*)core, code);
    if(!done)
    {
        // if it couldn't init, make sure the VM is not left dirty by the implementation
        core->currentVM = NULL;
    }

    return done;
}

s32 uli_api_vbank(uli_mem* uli, s32 bank)
{
    uli_core* core = (uli_core*)uli;

    s32 prev = core->state.vbank.id;

    switch(bank)
    {
    case 0:
    case 1:
        if(core->state.vbank.id != bank)
        {
            SWAP(uli->ram->vram, core->state.vbank.mem, uli_vram);
            core->state.vbank.id = bank;
        }
    }

    return prev;
}

void uli_core_tick(uli_mem* uli, uli_tick_data* data)
{
    uli_core* core = (uli_core*)uli;

    core->data = data;

    if (fftEnabled)
    {
        FFT_GetFFT(fftData);
    }
    if (!core->state.initialized)
    {
        const char* code = uli->cart.code.data;

        bool done = false;
        const uli_script* config = uli_get_script(uli);

        if (config && strlen(code))
        {
            cart2ram(uli);

            core->state.synced = 0;
            uli->input.data = 0;

            if(strcmp(uli_tool_metatag(code, "input", config->singleComment), "mouse") == 0)
                uli->input.mouse = 1;
            else if(strcmp(uli_tool_metatag(code, "input", config->singleComment), "gamepad") == 0)
                uli->input.gamepad = 1;
            else if(strcmp(uli_tool_metatag(code, "input", config->singleComment), "keyboard") == 0)
                uli->input.keyboard = 1;
            else uli->input.data = -1;  // default is all enabled

            data->start = data->counter(core->data->data);

            if (config->useBinarySection)
                code = uli->cart.binary.data;

            done = uli_init_vm(core, code, config);
        }
        else
        {
            core->data->error(core->data->data, "the code is empty");
        }

        if (done)
        {
            config->boot(uli);
            core->state.tick = config->tick;
            core->state.callback = config->callback;
            core->state.initialized = true;
        }
        else return;
    }

    core->state.tick(uli);
}

void uli_core_pause(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;

    memcpy(&core->pause.state, &core->state, sizeof(uli_core_state_data));
    memcpy(&core->pause.ram, memory->ram, sizeof(uli_ram));
    core->pause.input = memory->input.data;

    if (core->data)
    {
        core->pause.time.start = core->data->start;
        core->pause.time.paused = core->data->counter(core->data->data);
    }
}

void uli_core_resume(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;

    if (core->data)
    {
        memcpy(&core->state, &core->pause.state, sizeof(uli_core_state_data));
        memcpy(memory->ram, &core->pause.ram, sizeof(uli_ram));
        core->data->start = core->pause.time.start + core->data->counter(core->data->data) - core->pause.time.paused;
        memory->input.data = core->pause.input;
    }
    else
    {
        uli_api_reset(memory);
    }
}

void uli_core_close(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;

    core->state.initialized = false;

    uli_close_current_vm(core);

    blip_delete(core->blip.left);
    blip_delete(core->blip.right);

#ifdef _3DS
    linearFree(memory->product.screen);
#else
    free(memory->product.screen);
#endif
    free(memory->product.samples.buffer);
    free(core);
}

void uli_core_tick_start(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;
    uli_core_sound_tick_start(memory);
    uli_core_tick_io(memory);

    // SECURITY: preserve the system keyboard/game controller input state
    // (and restore it post-tick, see below) to prevent user cartridges
    // from being able to corrupt and take control of the inputs in
    // nefarious ways.
    //
    // Related: https://github.com/uli78/ULI-78/issues/1785
    core->state.keyboard.now.data = core->memory.ram->input.keyboard.data;
    core->state.gamepads.now.data = core->memory.ram->input.gamepads.data;

    core->state.synced = 0;
}

void uli_core_tick_end(uli_mem* memory)
{
    uli_core* core = (uli_core*)memory;
    uli78_input* input = &core->memory.ram->input;

    core->state.gamepads.previous.data = input->gamepads.data;
    // SECURITY: we do not use `memory.ram.input` here because it is
    // untrustworthy since the cartridge could have modified it to
    // inject artificial keyboard/gamepad events.
    core->state.keyboard.previous.data = core->state.keyboard.now.data;
    core->state.gamepads.previous.data = core->state.gamepads.now.data;

    uli_core_sound_tick_end(memory);
}

// copied from SDL2
static inline void memset4(void* dst, u32 val, u32 dwords)
{
#if defined(__GNUC__) && defined(i386)
    s32 u0, u1, u2;
    __asm__ __volatile__(
        "cld \n\t"
        "rep ; stosl \n\t"
        : "=&D" (u0), "=&a" (u1), "=&c" (u2)
        : "0" (dst), "1" (val), "2" (dwords)
        : "memory"
    );
#else
    u32 _n = (dwords + 3) / 4;
    u32* _p = (u32*)dst;
    u32 _val = (val);
    if (dwords == 0)
        return;
    switch (dwords % 4)
    {
    case 0: do {
        *_p++ = _val;
    case 3:         *_p++ = _val;
    case 2:         *_p++ = _val;
    case 1:         *_p++ = _val;
    } while (--_n);
    }
#endif
}

static inline void updpal(uli_mem* uli, uli_blitpal* pal0, uli_blitpal* pal1)
{
    uli_core* core = (uli_core*)uli;
    *pal0 = uli_tool_palette_blit(&vbank0(core)->palette, core->screen_format);
    *pal1 = uli_tool_palette_blit(&vbank1(core)->palette, core->screen_format);
}

static inline void updbdr(uli_mem* uli, s32 row, u32* ptr, uli_blit_callback clb, uli_blitpal* pal0, uli_blitpal* pal1)
{
    uli_core* core = (uli_core*)uli;

    if(clb.border) clb.border(uli, row, clb.data);

    if(clb.scanline)
    {
        if(row == 0) clb.scanline(uli, 0, clb.data);
        else if(row > ULI78_MARGIN_TOP && row < (ULI78_HEIGHT + ULI78_MARGIN_TOP))
            clb.scanline(uli, row - ULI78_MARGIN_TOP, clb.data);
    }

    if(clb.border || clb.scanline)
        updpal(uli, pal0, pal1);

    memset4(ptr, pal0->data[vbank0(core)->vars.border], ULI78_FULLWIDTH);
}

static inline u32 blitpix(uli_mem* uli, s32 offset0, s32 offset1, const uli_blitpal* pal0, const uli_blitpal* pal1)
{
    uli_core* core = (uli_core*)uli;
    u32 pix = uli_tool_peek4(vbank1(core)->screen.data, offset1);

    return pix != vbank1(core)->vars.clear
        ? pal1->data[pix]
        : pal0->data[uli_tool_peek4(vbank0(core)->screen.data, offset0)];
}

void uli_core_blit_ex(uli_mem* uli, uli_blit_callback clb)
{
    uli_core* core = (uli_core*)uli;

    uli_blitpal pal0, pal1;
    updpal(uli, &pal0, &pal1);

    s32 row = 0;
    u32* rowPtr = uli->product.screen;

#define UPDBDR() updbdr(uli, row, rowPtr, clb, &pal0, &pal1)

    for(; row != ULI78_MARGIN_TOP; ++row, rowPtr += ULI78_FULLWIDTH)
        UPDBDR();

    for(; row != ULI78_FULLHEIGHT - ULI78_MARGIN_BOTTOM; ++row)
    {
        UPDBDR();
        rowPtr += ULI78_MARGIN_LEFT;

        if(*(u16*)&vbank0(core)->vars.offset == 0 && *(u16*)&vbank1(core)->vars.offset == 0)
        {
            // render line without XY offsets
            for(s32 x = (row - ULI78_MARGIN_TOP) * ULI78_WIDTH, end = x + ULI78_WIDTH; x != end; ++x)
                *rowPtr++ = blitpix(uli, x, x, &pal0, &pal1);
        }
        else
        {
            // render line with XY offsets
            enum{OffsetY = ULI78_HEIGHT - ULI78_MARGIN_TOP};
            s32 start0 = (row + vbank0(core)->vars.offset.y + OffsetY) % ULI78_HEIGHT * ULI78_WIDTH;
            s32 start1 = (row + vbank1(core)->vars.offset.y + OffsetY) % ULI78_HEIGHT * ULI78_WIDTH;
            s32 offsetX0 = vbank0(core)->vars.offset.x;
            s32 offsetX1 = vbank1(core)->vars.offset.x;

            for(s32 x = ULI78_WIDTH; x != 2 * ULI78_WIDTH; ++x)
                *rowPtr++ = blitpix(uli, (x + offsetX0) % ULI78_WIDTH + start0,
                    (x + offsetX1) % ULI78_WIDTH + start1, &pal0, &pal1);
        }

        rowPtr += ULI78_MARGIN_RIGHT;
    }

    for(; row != ULI78_FULLHEIGHT; ++row, rowPtr += ULI78_FULLWIDTH)
        UPDBDR();

#undef  UPDBDR
}

static inline void scanline(uli_mem* memory, s32 row, void* data)
{
    uli_core* core = (uli_core*)memory;

    if (core->state.initialized)
        core->state.callback.scanline(memory, row, data);
}

static inline void border(uli_mem* memory, s32 row, void* data)
{
    uli_core* core = (uli_core*)memory;

    if (core->state.initialized)
        core->state.callback.border(memory, row, data);
}

void uli_core_blit(uli_mem* uli)
{
    uli_core_blit_ex(uli, (uli_blit_callback){scanline, border, NULL});
}

uli_mem* uli_core_create(s32 samplerate, uli78_pixel_color_format format)
{
    uli_core* core = (uli_core*)malloc(sizeof(uli_core));
    memset(core, 0, sizeof(uli_core));

    uli78* product = &core->memory.product;

    core->screen_format = format;
    core->memory.ram = (uli_ram*)malloc(ULI_RAM_SIZE);
    core->memory.base_ram = core->memory.ram;
    core->samplerate = samplerate;

    memset(core->memory.ram, 0, sizeof(uli_ram));
#ifdef _3DS
    // To feed texture data directly to the 3DS GPU, linearly allocated memory is required, which is
    // not guaranteed by malloc.
    // Additionally, allocate ULI78_FULLHEIGHT + 1 lines to minimize glitches in linear scaling mode.
    product->screen = linearAlloc(ULI78_FULLWIDTH * (ULI78_FULLHEIGHT + 1) * sizeof(u32));
#else
    product->screen = malloc(ULI78_FULLWIDTH * ULI78_FULLHEIGHT * sizeof product->screen[0]);
#endif
    product->samples.count = samplerate * ULI78_SAMPLE_CHANNELS / ULI78_FRAMERATE;
    product->samples.buffer = malloc(product->samples.count * ULI78_SAMPLESIZE);

    core->blip.left = blip_new(samplerate / 10);
    core->blip.right = blip_new(samplerate / 10);

    blip_set_rates(core->blip.left, CLOCKRATE, samplerate);
    blip_set_rates(core->blip.right, CLOCKRATE, samplerate);

    {
#define API_FUNC_DEF(name, ...) core->api.name = uli_api_ ## name;
        ULI_API_LIST(API_FUNC_DEF)
#undef  API_FUNC_DEF

#if defined BUILD_DEPRECATED
        void uli_api_textri(uli_mem* uli, float x1, float y1, float x2, float y2, float x3, float y3, float u1, float v1, float u2, float v2, float u3, float v3, bool use_map, u8* colors, s32 count);
        core->api.textri = uli_api_textri;
#endif
    }

    uli_api_reset(&core->memory);

    return &core->memory;
}
