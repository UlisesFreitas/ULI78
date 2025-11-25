//
// kernel.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include "customscreen.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <uli78.h>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/input/mouse.h>
#include "utils.h"
#include <limits.h>
#include <studio.h>
#include "keycodes.h"
#include "gamepads.h"
#include "syscore.h"

static const char* KN = "ULI78"; // kernel name

// currently pressed keys and modifiers
static unsigned char keyboardRawKeys[6];
static char keyboardModifiers;

// mouse status
static int mousex;
static int mousey;
static unsigned mousebuttons;

// gamepad status
static struct TGamePadState gamepad;

static CSpinLock keyspinlock;

extern "C" {

/*
unsigned int sleep(unsigned int seconds)
{
    mTimer.SimpleMsDelay (seconds*1000);
}
*/

static struct
{

    Studio* studio;
    uli78_input input;

    struct
    {
        bool state[uli_keys_count];
    } keyboard;

    char* clipboard;

} platform;

void uli_sys_clipboard_set(const char* text)
{
    if(platform.clipboard)
    {
        free(platform.clipboard);
        platform.clipboard = NULL;
    }

    platform.clipboard = strdup(text);
}

bool uli_sys_clipboard_has()
{
    return platform.clipboard != NULL;
}

char* uli_sys_clipboard_get()
{
    return platform.clipboard ? strdup(platform.clipboard) : NULL;
}

void uli_sys_clipboard_free(const char* text)
{
    free((void*)text);
}

u64 uli_sys_counter_get()
{
    return CTimer::Get()->GetTicks();
}

u64 uli_sys_freq_get()
{
    return HZ;
}

void uli_sys_fullscreen_set(bool value)
{
}

bool uli_sys_fullscreen_get()
{
}

void uli_sys_message(const char* title, const char* message)
{
}

void uli_sys_title(const char* title)
{
}

void uli_sys_open_path(const char* path) {}
void uli_sys_open_url(const char* url) {}

void uli_sys_preseed()
{
#if defined(__ULI_MACOSX__)
    srandom(time(NULL));
    random();
#else
    srand(time(NULL));
    rand();
#endif
}

void uli_sys_update_config()
{

}

void uli_sys_default_mapping(uli_mapping* mapping)
{
    *mapping = (uli_mapping)
    {
        uli_key_up,
        uli_key_down,
        uli_key_left,
        uli_key_right,

        uli_key_z, // a
        uli_key_x, // b
        uli_key_a, // x
        uli_key_s, // y
    };
}

bool uli_sys_keyboard_text(char* text)
{
    return false;
}

void screenCopy(CScreenDevice* screen, const u32* ts)
{
    u32 pitch = screen->GetPitch();
    u32* buf = screen->GetBuffer();
    for (int y = 0; y < ULI78_HEIGHT; y++)
    {
        const u32 *line = ts + ((y+ULI78_OFFSET_TOP)*(ULI78_FULLWIDTH) + ULI78_OFFSET_LEFT);
        memcpy(buf + (pitch * y), line, ULI78_WIDTH * 4);
    }
}


} //extern C

void mouseStatusHandler (unsigned nButtons, int nDisplacementX, int nDisplacementY, int nWheelMove)
{
    keyspinlock.Acquire();

    mousex += nDisplacementX;
    mousey += nDisplacementY;

    if (mousex < 0) mousex = 0;
    if (mousex > ULI78_WIDTH*MOUSE_SENS) mousex = ULI78_WIDTH*MOUSE_SENS;
    if (mousey < 0) mousey = 0;
    if (mousey > ULI78_HEIGHT*MOUSE_SENS) mousey = ULI78_HEIGHT*MOUSE_SENS;

    mousebuttons = nButtons;
    keyspinlock.Release();
}

void gamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState)
{

    keyspinlock.Acquire();
    // Just copy buttons and axes.
    gamepad.buttons = pState -> buttons;
    gamepad.naxes = pState -> naxes;
    for (int i = 0; i< pState->naxes; i++)
    {
        gamepad.axes[i].value = pState -> axes[i].value;
    }
    keyspinlock.Release();
}


void inputToTic()
{
    uli78_input* uli_input = &platform.input;

    // mouse
    if (mousebuttons & 0x01) uli_input->mouse.left = true; else uli_input->mouse.left = false;
    if (mousebuttons & 0x02) uli_input->mouse.right = true; else uli_input->mouse.right = false;
    if (mousebuttons & 0x04) uli_input->mouse.middle = true; else uli_input->mouse.middle = false;
    uli_input->mouse.x = mousex/MOUSE_SENS + ULI78_OFFSET_LEFT;
    uli_input->mouse.y = mousey/MOUSE_SENS + ULI78_OFFSET_TOP;

    // keyboard
    uli_input->gamepads.first.data = 0;
    uli_input->keyboard.data = 0;

    u32 keynum = 0;

    //dbg("MODIF %02x ", keyboardModifiers);

    if(keyboardModifiers & 0x11) uli_input->keyboard.keys[keynum++]= uli_key_ctrl;
    if(keyboardModifiers & 0x22) uli_input->keyboard.keys[keynum++]= uli_key_shift;
    if(keyboardModifiers & 0x44) uli_input->keyboard.keys[keynum++]= uli_key_alt;

    for (unsigned i = 0; i < 6; i++)
    {
        if (keyboardRawKeys[i] != 0)
        {
            // keyboard
            if(keynum<ULI78_KEY_BUFFER){
                uli_keycode tkc = TicKeyboardCodes[keyboardRawKeys[i]];
                if(tkc != uli_key_unknown) uli_input->keyboard.keys[keynum++]= tkc;
            }
            // key to gamepad
            switch(keyboardRawKeys[i])
            {
                case 0x1d: uli_input->gamepads.first.a = true; break;
                case 0x1b: uli_input->gamepads.first.b = true; break;
                case 0x52: uli_input->gamepads.first.up = true; break;
                case 0x51: uli_input->gamepads.first.down = true; break;
                case 0x50: uli_input->gamepads.first.left = true; break;
                case 0x4F: uli_input->gamepads.first.right = true; break;
            }
            //dbg(" %02x ", keyboardRawKeys[i]);
        }
    }
    //dbg("\n");

    if(keynum >= ULI78_KEY_BUFFER) return;

    // gamepads

    if (gamepad.buttons & 0x100) uli_input->gamepads.first.a = true;
    if (gamepad.buttons & 0x200) uli_input->gamepads.first.b = true;
    if (gamepad.buttons & 0x400) uli_input->gamepads.first.x = true;
    if (gamepad.buttons & 0x800) uli_input->gamepads.first.y = true;
    // map ESC to a gamepad button to exit the game
    if (gamepad.buttons & 0x10) uli_input->keyboard.keys[keynum++]= uli_key_escape;

    // TODO use min and max instead of hardcoded range
    if (gamepad.naxes > 0)
    {
        if (gamepad.axes[0].value < 50) uli_input->gamepads.first.left = true;
        if (gamepad.axes[0].value > 200) uli_input->gamepads.first.right = true;

    }
    if (gamepad.naxes > 1)
    {
        if (gamepad.axes[1].value < 50) uli_input->gamepads.first.up = true;
        if (gamepad.axes[1].value > 200) uli_input->gamepads.first.down = true;
    }
/*
        dbg("G:BTN 0x%X", gamepad.buttons);

        if (gamepad.naxes > 0)
        {
                dbg(" A");

                for (int i = 0; i < gamepad.naxes; i++)
                {
                        dbg(" %d", gamepad.axes[i].value);
                }
        }

        if (gamepad.nhats > 0)
        {
                dbg(" H");

                for (int i = 0; i < gamepad.nhats; i++)
                {
                        dbg(" %d", gamepad.hats[i]);
                }
        }

       dbg("\n");
*/

}
void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
    // this gets called with whatever key is currently pressed in RawKeys
    // (plus modifiers).
    keyspinlock.Acquire();
    keyboardModifiers = ucModifiers;
    memcpy(keyboardRawKeys, RawKeys, sizeof(unsigned char)*6);
    keyspinlock.Release();
}

TShutdownMode Run(void)
{
    initializeCore();
    mLogger.Write (KN, LogNotice, "ULI78 Port");

    f_mkdir("uli78");

    dbg("Calling studio init instance..\n");

    if (pKeyboard)
    {
        dbg("With keyboard\n");
        char  arg0[] = "xxkernel";
        char* argv[] = { &arg0[0], NULL };
        int argc = 1;
        platform.studio = studio_create(argc, argv, 44100, ULI78_PIXEL_COLOR_BGRA8888, "uli78", INT32_MAX, uli_layout_qwerty);
    }
    else
    {
        //  if no keyboard, start in surf mode!
        char  arg0[] = "xxkernel";
        char  arg1[] = "--cmd=surf";
        char* argv[] = { &arg0[0], &arg1[0], NULL };
        int argc = 2;
        dbg("Without keyboard\n");
        platform.studio = studio_create(argc, argv, 44100, ULI78_PIXEL_COLOR_BGRA8888, "uli78", INT32_MAX, uli_layout_qwerty);
    }
    dbg("studio_create OK\n");

    if( !platform.studio)
    {
        Die("Could not init studio");
    }

    dbg("Studio init ok..\n");

    if (pKeyboard){
        pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);
    }

    initGamepads(mDeviceNameService, gamePadStatusHandler);

    if (pMouse) {
        pMouse->RegisterStatusHandler (mouseStatusHandler);
    }

    const uli78* product = &studio_mem(platform.studio)->product;
    uli78_input* uli_input = &platform.input;
    uli_input->keyboard.data = 0;

    // sound system
    mSound->AllocateQueue(1000);
    mSound->SetWriteFormat(SoundFormatSigned16);

    if (!mSound->Start())
    {
        Die("Could not start sound.");
    }

    // MAIN LOOP
    while(true)
    {
        keyspinlock.Acquire();
        inputToTic();
        keyspinlock.Release();

        studio_tick(platform.studio, platform.input);
        studio_sound(platform.studio);

        mSound->Write(product->samples.buffer, product->samples.count * ULI78_SAMPLESIZE);

        mScreen.vsync();

        screenCopy(&mScreen, product->screen);

        mScheduler.Yield(); // for sound
    }

    return ShutdownHalt;
}
