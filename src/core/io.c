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

#include "api.h"
#include "core.h"

#include "uli_assert.h"

static_assert(sizeof(uli78_gamepad) == 2, "uli78_gamepad");
static_assert(sizeof(uli78_gamepads) == 8, "uli78_gamepads");
static_assert(sizeof(uli78_mouse) == 4, "uli78_mouse");
static_assert(sizeof(uli78_keyboard) == 4, "uli78_keyboard");
static_assert(sizeof(uli78_input) == 16, "uli78_input");

static bool isKeyPressed(const uli78_keyboard* input, uli_key key)
{
    for (s32 i = 0; i < ULI78_KEY_BUFFER; i++)
        if (input->keys[i] == key)
            return true;

    return false;
}

u32 uli_api_btnp(uli_mem* uli, s32 index, s32 hold, s32 period)
{
    uli_core* core = (uli_core*)uli;

    if (index < 0)
    {
        return (~core->state.gamepads.previous.data) & core->memory.ram->input.gamepads.data;
    }
    else if (hold < 0 || period < 0)
    {
        return ((~core->state.gamepads.previous.data) & core->memory.ram->input.gamepads.data) & (1 << index);
    }

    uli78_gamepads previous;

    previous.data = core->state.gamepads.holds[index] >= (u32)hold
        ? period && core->state.gamepads.holds[index] % period ? core->state.gamepads.previous.data : 0
        : core->state.gamepads.previous.data;

    return ((~previous.data) & core->memory.ram->input.gamepads.data) & (1 << index);
}

u32 uli_api_btn(uli_mem* uli, s32 index)
{
    uli_core* core = (uli_core*)uli;

    if (index < 0)
    {
        return core->memory.ram->input.gamepads.data;
    }
    else
    {
        return core->memory.ram->input.gamepads.data & (1 << index);
    }
}

bool uli_api_key(uli_mem* uli, uli_key key)
{
    return key > uli_key_unknown
        ? isKeyPressed(&uli->ram->input.keyboard, key)
        : uli->ram->input.keyboard.data;
}

bool uli_api_keyp(uli_mem* uli, uli_key key, s32 hold, s32 period)
{
    uli_core* core = (uli_core*)uli;

    if (key > uli_key_unknown)
    {
        bool prevDown = hold >= 0 && period >= 0 && core->state.keyboard.holds[key] >= (u32)hold
            ? period && core->state.keyboard.holds[key] % period
            ? isKeyPressed(&core->state.keyboard.previous, key)
            : false
            : isKeyPressed(&core->state.keyboard.previous, key);

        bool down = isKeyPressed(&uli->ram->input.keyboard, key);

        return !prevDown && down;
    }

    for (s32 i = 0; i < ULI78_KEY_BUFFER; i++)
    {
        uli_key key = uli->ram->input.keyboard.keys[i];

        if (key)
        {
            bool wasPressed = false;

            for (s32 p = 0; p < ULI78_KEY_BUFFER; p++)
            {
                if (core->state.keyboard.previous.keys[p] == key)
                {
                    wasPressed = true;
                    break;
                }
            }

            if (!wasPressed)
                return true;
        }
    }

    return false;
}

uli_point uli_api_mouse(uli_mem* memory)
{
    return memory->ram->input.mouse.relative
        ? (uli_point){memory->ram->input.mouse.rx, memory->ram->input.mouse.ry}
        : (uli_point){memory->ram->input.mouse.x - ULI78_OFFSET_LEFT, memory->ram->input.mouse.y - ULI78_OFFSET_TOP};
}

void uli_core_tick_io(uli_mem* uli)
{
    uli_core* core = (uli_core*)uli;

    // process gamepads mapping
    u8* keycodes = uli->ram->mapping.data;
    for(s32 i = 0; i < sizeof(uli_mapping); ++i)
        if(keycodes[i] && uli_api_key(uli, keycodes[i]))
            uli->ram->input.gamepads.data |= 1 << i;

    // process gamepad
    for (s32 i = 0; i < COUNT_OF(core->state.gamepads.holds); i++)
    {
        u32 mask = 1 << i;
        u32 prevDown = core->state.gamepads.previous.data & mask;
        u32 down = uli->ram->input.gamepads.data & mask;

        u32* hold = &core->state.gamepads.holds[i];
        if (prevDown && prevDown == down) (*hold)++;
        else *hold = 0;
    }

    // process keyboard
    for (s32 i = 0; i < uli_keys_count; i++)
    {
        bool prevDown = isKeyPressed(&core->state.keyboard.previous, i);
        bool down = isKeyPressed(&uli->ram->input.keyboard, i);

        u32* hold = &core->state.keyboard.holds[i];

        if (prevDown && down) (*hold)++;
        else *hold = 0;
    }
}
