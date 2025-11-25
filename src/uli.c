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

#include "uli78.h"
#include "script.h"
#include "tools.h"
#include "cart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(ULI_MODULE_EXT)
#include <dlfcn.h>
#endif

static void onTrace(void* data, const char* text, u8 color)
{
    uli78* uli = (uli78*)data;

    if(uli->callback.trace)
        uli->callback.trace(text, color);
}

static void onError(void* data, const char* info)
{
    uli78* uli = (uli78*)data;

    if(uli->callback.error)
        uli->callback.error(info);
}

static void onExit(void* data)
{
    uli78* uli = (uli78*)data;

    if(uli->callback.exit)
        uli->callback.exit();
}

uli78* uli78_create(s32 samplerate, uli78_pixel_color_format format)
{
    return &uli_core_create(samplerate, format)->product;
}

ULI78_API void uli78_load(uli78* uli, void* cart, s32 size)
{
    uli_mem* mem = (uli_mem*)uli;

    uli_cart_load(&mem->cart, cart, size);

    const uli_script* script = uli_get_script(mem);
    if(script)
    {
        uli_api_reset(mem);
    }

#if defined(ULI_MODULE_EXT)
    else
    {
        const char* tag = uli_tool_metatag(mem->cart.code.data, "script", NULL);
        char name[128];
        sprintf(name, "%s" ULI_MODULE_EXT, tag);

        void* module = dlopen(name, RTLD_NOW | RTLD_LOCAL);

        if(module)
        {
            const uli_script* config = dlsym(module, DEF2STR(SCRIPT_CONFIG));

            if(config)
            {
                uli_add_script(config);
                uli_api_reset(mem);
            }
            else
            {
                dlclose(module);
            }
        }
    }
#endif
}

ULI78_API void uli78_tick(uli78* uli, uli78_input input, CounterCallback counter, FreqCallback freq)
{
    uli_mem* mem = (uli_mem*)uli;

    mem->ram->input = input;

    uli_tick_data tickData = (uli_tick_data)
    {
        .error = onError,
        .trace = onTrace,
        .exit = onExit,
        .data = uli,
        .start = 0,
        .counter = counter,
        .freq = freq
    };

    uli_core_tick_start(mem);
    uli_core_tick(mem, &tickData);
    uli_core_tick_end(mem);
    uli_core_blit(mem);
}

ULI78_API void uli78_sound(uli78* uli)
{
    uli_mem* mem = (uli_mem*)uli;
    uli_core_synth_sound(mem);
}

ULI78_API void uli78_delete(uli78* uli)
{
    uli_mem* mem = (uli_mem*)uli;
    uli_core_close(mem);
}
