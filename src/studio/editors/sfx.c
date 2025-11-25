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

#include "sfx.h"
#include "ext/history.h"

#define DEFAULT_CHANNEL 0
#define NOTES 12
#define SECOND_OCTAVE_KEYBOARD_INDEX 16
#define SECOND_OCTAVE_KEYBOARD_SHIFT 5

enum
{
    SFX_WAVE_PANEL,
    SFX_VOLUME_PANEL,
    SFX_CHORD_PANEL,
    SFX_PITCH_PANEL,
};

static uli_sample* getEffect(Sfx* sfx)
{
    return sfx->src->samples.data + sfx->index;
}

static uli_waveform* getWaveformById(Sfx* sfx, s32 i)
{
    return &sfx->src->waveforms.items[i];
}

static void drawPanelBorder(uli_mem* uli, s32 x, s32 y, s32 w, s32 h, uli_color color)
{
    uli_api_rect(uli, x, y, w, h, color);

    uli_api_rect(uli, x, y-1, w, 1, uli_color_dark_grey);
    uli_api_rect(uli, x-1, y, 1, h, uli_color_dark_grey);
    uli_api_rect(uli, x, y+h, w, 1, uli_color_light_grey);
    uli_api_rect(uli, x+w, y, 1, h, uli_color_light_grey);
}

static s32 hold(Sfx* sfx, s32 value)
{
    uli_mem* uli = sfx->uli;

    if(uli_api_key(uli, uli_key_ctrl) ||
        uli_api_key(uli, uli_key_shift))
    {
        if(sfx->holdValue < 0)
            sfx->holdValue = value;

        return sfx->holdValue;
    }

    return value;
}

static inline void unhold(Sfx* sfx)
{
    sfx->holdValue = -1;
}

static void drawCanvasLeds(Sfx* sfx, s32 x, s32 y, s32 canvasTab)
{
    uli_mem* uli = sfx->uli;

    enum
    {
        Cols = SFX_TICKS, Rows = 16,
        Gap = 1, LedWidth = 3 + Gap, LedHeight = 1 + Gap,
        Width = LedWidth * Cols + Gap,
        Height = LedHeight * Rows + Gap
    };

    uli_api_rect(uli, x, y, Width, Height, uli_color_dark_grey);

    for(s32 i = 0; i < Height; i += LedHeight)
        uli_api_rect(uli, x, y + i, Width, Gap, uli_color_black);

    for(s32 i = 0; i < Width; i += LedWidth)
        uli_api_rect(uli, x + i, y, Gap, Height, uli_color_black);

    {
        const uli_sfx_pos* pos = &uli->ram->sfxpos[DEFAULT_CHANNEL];
        s32 tickIndex = *(pos->data + canvasTab);

        if(tickIndex >= 0)
            uli_api_rect(uli, x + tickIndex * LedWidth, y, LedWidth + 1, Height, uli_color_white);
    }

    uli_rect rect = {x, y, Width - Gap, Height - Gap};

    uli_sample* effect = getEffect(sfx);
    uli_rect border = {-1};

    if(checkMousePos(sfx->studio, &rect))
    {
        setCursor(sfx->studio, uli_cursor_hand);

        s32 mx = uli_api_mouse(uli).x - x;
        s32 my = uli_api_mouse(uli).y - y;
        mx /= LedWidth;
        s32 vy = my /= LedHeight;
        border = (uli_rect){x + mx * LedWidth + Gap, y + my * LedHeight + Gap, LedWidth - Gap, LedHeight - Gap};

        switch(canvasTab)
        {
        case SFX_VOLUME_PANEL: vy = MAX_VOLUME - my; break;
        case SFX_WAVE_PANEL:   sfx->hoverWave = vy = my = Rows - my - 1; break;
        case SFX_CHORD_PANEL:  vy = my = Rows - my - 1; break;
        case SFX_PITCH_PANEL:  vy = my = Rows / 2 - my - 1; break;
        default: break;
        }

        SHOW_TOOLTIP(sfx->studio, "[x=%02i y=%02i]", mx, vy);

        if(checkMouseDown(sfx->studio, &rect, uli_mouse_left))
        {
            my = hold(sfx, my);

            switch(canvasTab)
            {
            case SFX_WAVE_PANEL:   effect->data[mx].wave = my; break;
            case SFX_VOLUME_PANEL: effect->data[mx].volume = my; break;
            case SFX_CHORD_PANEL:  effect->data[mx].chord = my; break;
            case SFX_PITCH_PANEL:  effect->data[mx].pitch = my; break;
            default: break;
            }

            history_add(sfx->history);
        }
        else unhold(sfx);
    }

    for(s32 i = 0; i < Cols; i++)
    {
        switch(canvasTab)
        {
        case SFX_WAVE_PANEL:
            for(s32 j = 1, start = Height - LedHeight, value = effect->data[i].wave + 1; j <= value; j++, start -= LedHeight)
                uli_api_rect(uli, x + i * LedWidth + Gap, y + start, LedWidth-Gap, LedHeight-Gap, j == value ? uli_color_red : uli_color_orange);
            break;

        case SFX_VOLUME_PANEL:
            for(s32 j = 1, start = Height - LedHeight, value = Rows - effect->data[i].volume; j <= value; j++, start -= LedHeight)
                uli_api_rect(uli, x + i * LedWidth + Gap, y + start, LedWidth-Gap, LedHeight-Gap, j == value ? uli_color_blue : uli_color_light_blue);
            break;

        case SFX_CHORD_PANEL:
            for(s32 j = 1, start = Height - LedHeight, value = effect->data[i].chord + 1; j <= value; j++, start -= LedHeight)
                uli_api_rect(uli, x + i * LedWidth + Gap, y + start, LedWidth-Gap, LedHeight-Gap, j == value ? uli_color_green : uli_color_light_green);
            break;

        case SFX_PITCH_PANEL:
            for(s32 value = effect->data[i].pitch, j = MIN(0, value); j <= MAX(0, value); j++)
                uli_api_rect(uli, x + i * LedWidth + Gap, y + (Height / 2 - (j + 1) * LedHeight + Gap),
                    LedWidth-Gap, LedHeight-Gap, j == value ? uli_color_orange : uli_color_yellow);
            break;
        }
    }

    {
        uli_sound_loop* loop = effect->loops + canvasTab;
        if(loop->size > 0)
            for(s32 r = 0; r < Rows; r++)
            {
                uli_api_rect(uli, x + loop->start * LedWidth + 2, y + Gap + r * LedHeight, 1, 1, uli_color_white);
                uli_api_rect(uli, x + (loop->start + loop->size-1) * LedWidth + 2, y + Gap + r * LedHeight, 1, 1, uli_color_white);
            }
    }

    if(border.x >= 0)
        uli_api_rectb(uli, border.x, border.y, border.w, border.h, uli_color_white);
}

static void drawVolumeStereo(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    enum {Width = ULI_ALTFONT_WIDTH-1, Height = ULI_FONT_HEIGHT};

    uli_sample* effect = getEffect(sfx);

    {
        uli_rect rect = {x, y, Width, Height};

        bool hover = false;
        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;

            showTooltip(sfx->studio, "left stereo");

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
                effect->stereo_left = ~effect->stereo_left;
        }

        uli_api_print(uli, "L", rect.x, rect.y, effect->stereo_left ? hover ? uli_color_grey : uli_color_dark_grey : uli_color_light_green, true, 1, true);
    }

    {
        uli_rect rect = {x + 4, y, Width, Height};

        bool hover = false;
        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;

            showTooltip(sfx->studio, "right stereo");

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
                effect->stereo_right = ~effect->stereo_right;
        }

        uli_api_print(uli, "R", rect.x, rect.y, effect->stereo_right ? hover ? uli_color_grey : uli_color_dark_grey : uli_color_light_green, true, 1, true);
    }
}

static void drawArppeggioSwitch(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    static const char Label[] = "DOWN";

    enum {Width = (sizeof Label - 1) * ULI_ALTFONT_WIDTH - 1, Height = ULI_FONT_HEIGHT};

    uli_sample* effect = getEffect(sfx);

    {
        uli_rect rect = {x, y, Width, Height};

        bool hover = false;
        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;

            showTooltip(sfx->studio, "up/down arpeggio");

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
                effect->reverse = ~effect->reverse;
        }

        uli_api_print(uli, Label, rect.x, rect.y, effect->reverse ? uli_color_light_green : hover ? uli_color_grey : uli_color_dark_grey, true, 1, true);
    }
}

static void drawPitchSwitch(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    static const char Label[] = "x16";

    enum {Width = (sizeof Label - 1) * ULI_ALTFONT_WIDTH - 1, Height = ULI_FONT_HEIGHT};

    uli_sample* effect = getEffect(sfx);

    {
        uli_rect rect = {x, y, Width, Height};

        bool hover = false;
        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;

            showTooltip(sfx->studio, "x16 pitch");

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
                effect->pitch16x = ~effect->pitch16x;
        }

        uli_api_print(uli, Label, rect.x, rect.y, effect->pitch16x ? uli_color_light_green : hover ? uli_color_grey : uli_color_dark_grey, true, 1, true);
    }
}

static void drawVolWaveSelector(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    typedef struct {const char* label; s32 panel; uli_rect rect; const char* tip;} Item;
    static const Item Items[] =
    {
        {"WAV", SFX_WAVE_PANEL, {ULI_ALTFONT_WIDTH * 3, 0, ULI_ALTFONT_WIDTH * 3, ULI_FONT_HEIGHT}, "wave data"},
        {"VOL", SFX_VOLUME_PANEL, {0, 0, ULI_ALTFONT_WIDTH * 3, ULI_FONT_HEIGHT}, "volume data"},
    };

    for(s32 i = 0; i < COUNT_OF(Items); i++)
    {
        const Item* item = &Items[i];

        uli_rect rect = {x + item->rect.x, y + item->rect.y, item->rect.w, item->rect.h};

        bool hover = false;

        if(checkMousePos(sfx->studio, &rect))
        {
            showTooltip(sfx->studio, item->tip);

            setCursor(sfx->studio, uli_cursor_hand);

            hover = true;

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
                sfx->volwave = item->panel;
        }

        uli_api_print(uli, item->label, x + item->rect.x, y + item->rect.y, item->panel == sfx->volwave ? uli_color_light_green : hover ? uli_color_grey : uli_color_dark_grey, true, 1, true);
    }
}

static void drawCanvas(Sfx* sfx, s32 x, s32 y, s32 canvasTab)
{
    uli_mem* uli = sfx->uli;

    enum
    {
        Width = 147, Height = 33
    };

    drawPanelBorder(uli, x, y, Width, Height, uli_color_black);

    static const char* Labels[] = {"", "", "ARPEGG", "PITCH"};
    uli_api_print(uli, Labels[canvasTab], x + 2, y + 2, uli_color_dark_grey, true, 1, true);

    switch(canvasTab)
    {
    case SFX_WAVE_PANEL:
        drawVolWaveSelector(sfx, x + 2, y + 2);
        break;
    case SFX_VOLUME_PANEL:
        drawVolWaveSelector(sfx, x + 2, y + 2);
        drawVolumeStereo(sfx, x + 2, y + 9);
        break;
    case SFX_CHORD_PANEL:
        drawArppeggioSwitch(sfx, x + 2, y + 9);
        break;
    case SFX_PITCH_PANEL:
        drawPitchSwitch(sfx, x + 2, y + 9);
        break;
    default:
        break;
    }

    uli_api_print(uli, "LOOP:", x + 2, y + 20, uli_color_dark_grey, true, 1, true);

    enum
    {
        ArrowWidth = 3, ArrowHeight = 5
    };

    uli_sample* effect = getEffect(sfx);
    static const char SetLoopPosLabel[] = "set loop start";
    static const char SetLoopSizeLabel[] = "set loop size";

    {
        uli_rect rect = {x + 2, y + 27, ArrowWidth, ArrowHeight};
        bool hover = false;

        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;

            showTooltip(sfx->studio, SetLoopPosLabel);

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
            {
                effect->loops[canvasTab].start--;
                history_add(sfx->history);
            }
        }

        drawBitIcon(sfx->studio, uli_icon_left, rect.x - 2, rect.y - 1, hover ? uli_color_grey : uli_color_dark_grey);
    }

    {
        uli_rect rect = {x + 10, y + 27, ArrowWidth, ArrowHeight};
        bool hover = false;

        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;

            showTooltip(sfx->studio, SetLoopPosLabel);

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
            {
                effect->loops[canvasTab].start++;
                history_add(sfx->history);
            }
        }

        drawBitIcon(sfx->studio, uli_icon_right, rect.x - 2, rect.y - 1, hover ? uli_color_grey : uli_color_dark_grey);
    }

    {
        char buf[] = "0";
        sprintf(buf, "%X", effect->loops[canvasTab].start);
        uli_api_print(uli, buf, x + 6, y + 27, uli_color_grey, true, 1, true);
    }

    {
        uli_rect rect = {x + 14, y + 27, ArrowWidth, ArrowHeight};
        bool hover = false;

        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;
            showTooltip(sfx->studio, SetLoopSizeLabel);

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
            {
                effect->loops[canvasTab].size--;
                history_add(sfx->history);
            }
        }

        drawBitIcon(sfx->studio, uli_icon_left, rect.x - 2, rect.y - 1, hover ? uli_color_grey : uli_color_dark_grey);
    }

    {
        uli_rect rect = {x + 22, y + 27, ArrowWidth, ArrowHeight};
        bool hover = false;

        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);
            hover = true;
            showTooltip(sfx->studio, SetLoopSizeLabel);

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
            {
                effect->loops[canvasTab].size++;
                history_add(sfx->history);
            }
        }

        drawBitIcon(sfx->studio, uli_icon_right, rect.x - 2, rect.y - 1, hover ? uli_color_grey : uli_color_dark_grey);
    }

    {
        char buf[] = "0";
        sprintf(buf, "%X", effect->loops[canvasTab].size);
        uli_api_print(uli, buf, x + 18, y + 27, uli_color_grey, true, 1, true);
    }

    drawCanvasLeds(sfx, x + 26, y, canvasTab);
}

static void playSound(Sfx* sfx)
{
    if(sfx->play.active)
    {
        uli_sample* effect = getEffect(sfx);

        if(sfx->play.note != effect->note || sfx->play.tick == 0)
        {
            sfx->play.note = effect->note;

            sfx_stop(sfx->uli, DEFAULT_CHANNEL);
            uli_api_sfx(sfx->uli, sfx->index, effect->note, effect->octave + effect->temp, -1, DEFAULT_CHANNEL, MAX_VOLUME, MAX_VOLUME, SFX_DEF_SPEED);
        }
    }
    else
    {
        sfx->play.note = -1;
        sfx_stop(sfx->uli, DEFAULT_CHANNEL);
    }
}

static void undo(Sfx* sfx)
{
    history_undo(sfx->history);
}

static void redo(Sfx* sfx)
{
    history_redo(sfx->history);
}

static void copyToClipboard(Sfx* sfx)
{
    uli_sample* effect = getEffect(sfx);
    toClipboard(effect, sizeof(uli_sample), true);
}

static void resetSfx(Sfx* sfx)
{
    uli_sample* effect = getEffect(sfx);
    memset(effect, 0, sizeof(uli_sample));

    history_add(sfx->history);
}

static void cutToClipboard(Sfx* sfx)
{
    copyToClipboard(sfx);
    resetSfx(sfx);
}

static void copyFromClipboard(Sfx* sfx)
{
    uli_sample* effect = getEffect(sfx);

    if(fromClipboard(effect, sizeof(uli_sample), true, false, true))
        history_add(sfx->history);
}

static inline bool keyWasPressedOnce(uli_mem* uli, s32 key)
{
    return uli_api_keyp(uli, key, -1, -1);
}

static void processKeyboard(Sfx* sfx)
{
    uli_mem* uli = sfx->uli;

    if(uli->ram->input.keyboard.data == 0) return;

    bool ctrl = uli_api_key(uli, uli_key_ctrl);
    bool shift = uli_api_key(uli, uli_key_shift);

    s32 keyboardButton = -1;

    static const s32 Keycodes[] =
    {
        uli_key_z,
        uli_key_s,
        uli_key_x,
        uli_key_d,
        uli_key_c,
        uli_key_v,
        uli_key_g,
        uli_key_b,
        uli_key_h,
        uli_key_n,
        uli_key_j,
        uli_key_m,
        uli_key_comma,
        uli_key_l,
        uli_key_period,
        uli_key_semicolon,
        uli_key_slash,

        // octave +1
        uli_key_q,
        uli_key_2,
        uli_key_w,
        uli_key_3,
        uli_key_e,
        uli_key_r,
        uli_key_5,
        uli_key_t,
        uli_key_6,
        uli_key_y,
        uli_key_7,
        uli_key_u,

        // extra keys
        uli_key_i,
        uli_key_9,
        uli_key_o,
        uli_key_0,
        uli_key_p,
    };

    if(uli_api_key(uli, uli_key_alt))
        return;

    uli_sample* effect = getEffect(sfx);

    if(ctrl || shift) { }
    else
    {
        u8 octaveShift = 0;
        for(int i = 0; i < COUNT_OF(Keycodes); i++) {
            if(uli_api_key(uli, Keycodes[i])) {
                if (sfx->play.tick == 0 && !keyWasPressedOnce(uli, Keycodes[i]))
                    continue;

                keyboardButton = (i > SECOND_OCTAVE_KEYBOARD_INDEX
                    ? i - SECOND_OCTAVE_KEYBOARD_SHIFT
                    : i) % NOTES;

                octaveShift = (i - keyboardButton) / NOTES;
            }
        }

        if (effect->temp != octaveShift)
            sfx->play.tick = 0;

        effect->temp = octaveShift;
    }

    if(keyboardButton >= 0)
    {
        if (keyboardButton != sfx->play.note)
            sfx->play.tick = 0;

        effect->note = keyboardButton;
        sfx->play.active = true;
    }

    if(uli_api_key(uli, uli_key_space))
        sfx->play.active = true;

    if(keyWasPressedOnce(uli, uli_key_z) && shift)
        effect->octave--;
    else if(keyWasPressedOnce(uli, uli_key_x) && shift)
        effect->octave++;
}

static void processEnvelopesKeyboard(Sfx* sfx)
{
    uli_mem* uli = sfx->uli;
    bool ctrl = uli_api_key(uli, uli_key_ctrl);

    switch(getClipboardEvent(sfx->studio))
    {
    case ULI_CLIPBOARD_CUT: cutToClipboard(sfx); break;
    case ULI_CLIPBOARD_COPY: copyToClipboard(sfx); break;
    case ULI_CLIPBOARD_PASTE: copyFromClipboard(sfx); break;
    default: break;
    }

    if(ctrl)
    {
        if(keyWasPressed(sfx->studio, uli_key_z))        undo(sfx);
        else if(keyWasPressed(sfx->studio, uli_key_y))   redo(sfx);
    }

    else if(keyWasPressed(sfx->studio, uli_key_left))    sfx->index--;
    else if(keyWasPressed(sfx->studio, uli_key_right))   sfx->index++;
    else if(keyWasPressed(sfx->studio, uli_key_delete))  resetSfx(sfx);
}

static uli_waveform* getWave(Sfx* sfx)
{
    uli_sample* effect = getEffect(sfx);
    return getWaveformById(sfx, effect->data[0].wave);
}

static void copyWave(Sfx* sfx)
{
    toClipboard(getWave(sfx), sizeof(uli_waveform), true);
}

static void cutWave(Sfx* sfx)
{
    copyWave(sfx);

    memset(getWave(sfx), 0, sizeof(uli_waveform));
    history_add(sfx->waveHistory);
}

static void pasteWave(Sfx* sfx)
{
    if(fromClipboard(getWave(sfx), sizeof(uli_waveform), true, false, true))
        history_add(sfx->waveHistory);
}

static void undoWave(Sfx* sfx)
{
    history_undo(sfx->waveHistory);
}

static void redoWave(Sfx* sfx)
{
    history_redo(sfx->waveHistory);
}

static void drawWavesBar(Sfx* sfx, s32 x, s32 y)
{
    static struct Button
    {
        u8 icon;
        const char* tip;
        void(*handler)(Sfx*);
    } Buttons[] =
    {
        {
            uli_icon_cut,
            "CUT WAVE",
            cutWave,
        },
        {
            uli_icon_copy,
            "COPY WAVE",
            copyWave,
        },
        {
            uli_icon_paste,
            "PASTE WAVE",
            pasteWave,
        },
        {
            uli_icon_undo,
            "UNDO WAVE",
            undoWave,
        },
        {
            uli_icon_redo,
            "REDO WAVE",
            redoWave,
        },
    };

    enum {Size = 7};

    FOR(const struct Button*, it, Buttons)
    {
        uli_rect rect = {x, y, Size, Size};

        bool over = false;
        s32 push = 0;
        if(checkMousePos(sfx->studio, &rect))
        {
            over = true;
            setCursor(sfx->studio, uli_cursor_hand);

            showTooltip(sfx->studio, it->tip);

            if(checkMouseDown(sfx->studio, &rect, uli_mouse_left))
                push = 1;

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
                it->handler(sfx);
        }

        if(over)
            drawBitIcon(sfx->studio, it->icon, rect.x, rect.y + 1, uli_color_black);

        drawBitIcon(sfx->studio, it->icon, rect.x, rect.y + push,
            over ? uli_color_white : uli_color_dark_grey);

        y += Size;
    }
}

static void drawWaves(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    enum{Width = 10, Height = 6, MarginRight = 6, MarginBottom = 4, Cols = 4, Rows = 4, Scale = 4};

    for(s32 i = 0; i < WAVES_COUNT; i++)
    {
        s32 xi = i % Cols;
        s32 yi = i / Cols;

        uli_rect rect = {x + xi * (Width + MarginRight), y + yi * (Height + MarginBottom), Width, Height};
        uli_sample* effect = getEffect(sfx);
        bool hover = false;

        if(checkMousePos(sfx->studio, &rect))
        {
            setCursor(sfx->studio, uli_cursor_hand);

            hover = true;

            SHOW_TOOLTIP(sfx->studio, "select wave #%02i", i);

            if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
            {
                for(s32 c = 0; c < SFX_TICKS; c++)
                    effect->data[c].wave = i;

                history_add(sfx->history);
            }
        }

        bool sel = i == effect->data[0].wave;

        const uli_sfx_pos* pos = &uli->ram->sfxpos[DEFAULT_CHANNEL];
        bool active = *pos->data < 0 ? sfx->hoverWave == i : i == effect->data[*pos->data].wave;

        drawPanelBorder(uli, rect.x, rect.y, rect.w, rect.h, active ? uli_color_orange : sel ? uli_color_light_green : uli_color_black);

        // draw tiny wave previews
        {
            uli_waveform* wave = getWaveformById(sfx, i);

            for(s32 i = 0; i < WAVE_VALUES/Scale; i++)
            {
                s32 value = uli_tool_peek4(wave->data, i*Scale)/Scale;
                uli_api_pix(uli, rect.x + i+1, rect.y + Height - value - 2,
                    active ? uli_color_red : sel ? uli_color_dark_green : hover ? uli_color_light_grey : uli_color_white, false);
            }

            // draw flare
            if(sel || active)
            {
                uli_api_rect(uli, rect.x + rect.w - 2, rect.y, 2, 1, uli_color_white);
                uli_api_pix(uli, rect.x + rect.w - 1, rect.y + 1, uli_color_white, false);
            }
        }
    }
}

static void drawWavePanel(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    enum {Width = 73, Height = 83, Round = 2};

    typedef struct {s32 x; s32 y; s32 x1; s32 y1; uli_color color;} Edge;
    static const Edge Edges[] =
    {
        {Width, Round, Width, Height - Round, uli_color_dark_grey},
        {Round, Height, Width - Round, Height, uli_color_dark_grey},
        {Width - Round, Height, Width, Height - Round, uli_color_dark_grey},
        {Width - Round, 0, Width, Round, uli_color_dark_grey},
        {0, Height - Round, Round, Height, uli_color_dark_grey},
        {Round, 0, Width - Round, 0, uli_color_light_grey},
        {0, Round, 0, Height - Round, uli_color_light_grey},
        {0, Round, Round, 0, uli_color_white},
    };

    FOR(const Edge*, edge, Edges)
        uli_api_line(uli, x + edge->x, y + edge->y, x + edge->x1, y + edge->y1, edge->color);

    // draw current wave shape
    {
        enum {Scale = 2, MaxValue = WAVE_MAX_VALUE};

        uli_rect rect = {x + 5, y + 5, 64, 32};
        uli_sample* effect = getEffect(sfx);
        uli_waveform* wave = getWaveformById(sfx, effect->data[0].wave);

        drawPanelBorder(uli, rect.x - 1, rect.y - 1, rect.w + 2, rect.h + 2, uli_color_light_green);

        if(sfx->play.active)
        {
            for(s32 i = 0; i < WAVE_VALUES; i++)
            {
                s32 amp = calcWaveAnimation(uli, i + sfx->play.tick, 0) / WAVE_MAX_VALUE;
                uli_api_rect(uli, rect.x + i*Scale, rect.y + (MaxValue - amp) * Scale, Scale, Scale, uli_color_dark_green);
            }
        }
        else
        {
            if(checkMousePos(sfx->studio, &rect))
            {
                setCursor(sfx->studio, uli_cursor_hand);

                s32 cx = (uli_api_mouse(uli).x - rect.x) / Scale;
                s32 cy = MaxValue - (uli_api_mouse(uli).y - rect.y) / Scale;

                SHOW_TOOLTIP(sfx->studio, "[x=%02i y=%02i]", cx, cy);

                enum {Border = 1};
                uli_api_rectb(uli, rect.x + cx*Scale - Border,
                    rect.y + (MaxValue - cy) * Scale - Border, Scale + Border*2, Scale + Border*2, uli_color_dark_green);

                if(checkMouseDown(sfx->studio, &rect, uli_mouse_left))
                {
                    cy = hold(sfx, cy);

                    if(uli_tool_peek4(wave->data, cx) != cy)
                    {
                        uli_tool_poke4(wave->data, cx, cy);
                        history_add(sfx->waveHistory);
                    }
                }
                else unhold(sfx);
            }

            for(s32 i = 0; i < WAVE_VALUES; i++)
            {
                s32 value = uli_tool_peek4(wave->data, i);
                uli_api_rect(uli, rect.x + i*Scale, rect.y + (MaxValue - value) * Scale, Scale, Scale, uli_color_dark_green);
            }
        }

        // draw flare
        {
            uli_api_rect(uli, rect.x + 59, rect.y + 2, 4, 1, uli_color_white);
            uli_api_rect(uli, rect.x + 62, rect.y + 2, 1, 3, uli_color_white);
        }
    }

    drawWaves(sfx, x + 5, y + 43);
    drawWavesBar(sfx, x + 65, y + 43);
}

static void drawPianoOctave(Sfx* sfx, s32 x, s32 y, s32 octave)
{
    uli_mem* uli = sfx->uli;

    enum
    {
        Gap = 1, WhiteShadow = 1,
        WhiteWidth = 3, WhiteHeight = 8, WhiteCount = 7, WhiteWidthGap = WhiteWidth + Gap,
        BlackWidth = 3, BlackHeight = 4, BlackCount = 6,
        BlackOffset = WhiteWidth - (BlackWidth - Gap) / 2,
        Width = WhiteCount * WhiteWidthGap - Gap,
        Height = WhiteHeight
    };

    uli_rect rect = {x, y, Width, Height};

    typedef struct{s32 note; uli_rect rect; bool white;} PianoBtn;
    static const PianoBtn Buttons[] =
    {
        {0, WhiteWidthGap * 0, 0, WhiteWidth, WhiteHeight, true},
        {2, WhiteWidthGap * 1, 0, WhiteWidth, WhiteHeight, true},
        {4, WhiteWidthGap * 2, 0, WhiteWidth, WhiteHeight, true},
        {5, WhiteWidthGap * 3, 0, WhiteWidth, WhiteHeight, true},
        {7, WhiteWidthGap * 4, 0, WhiteWidth, WhiteHeight, true},
        {9, WhiteWidthGap * 5, 0, WhiteWidth, WhiteHeight, true},
        {11, WhiteWidthGap * 6, 0, WhiteWidth, WhiteHeight, true},

        {1, WhiteWidthGap * 0 + BlackOffset, 0, BlackWidth, BlackHeight, false},
        {3, WhiteWidthGap * 1 + BlackOffset, 0, BlackWidth, BlackHeight, false},
        {6, WhiteWidthGap * 3 + BlackOffset, 0, BlackWidth, BlackHeight, false},
        {8, WhiteWidthGap * 4 + BlackOffset, 0, BlackWidth, BlackHeight, false},
        {10, WhiteWidthGap * 5 + BlackOffset, 0, BlackWidth, BlackHeight, false},
    };

    uli_sample* effect = getEffect(sfx);

    s32 hover = -1;

    if(checkMousePos(sfx->studio, &rect))
    {
        for(s32 i = COUNT_OF(Buttons)-1; i >= 0; i--)
        {
            const PianoBtn* btn = Buttons + i;
            uli_rect btnRect = btn->rect;
            btnRect.x += x;
            btnRect.y += y;

            if(checkMousePos(sfx->studio, &btnRect))
            {
                setCursor(sfx->studio, uli_cursor_hand);

                hover = btn->note;

                {
                    static const char* Notes[] = SFX_NOTES;
                    SHOW_TOOLTIP(sfx->studio, "play %s%i note", Notes[btn->note], octave + 1);
                }

                if(checkMouseDown(sfx->studio, &rect, uli_mouse_left))
                {
                    effect->note = btn->note;
                    effect->octave = octave;
                    sfx->play.active = true;

                    history_add(sfx->history);
                }

                break;
            }
        }
    }

    s32 currentOctave = effect->octave + effect->temp;

    bool active = sfx->play.active && currentOctave == octave;

    uli_api_rect(uli, rect.x, rect.y, rect.w, rect.h, uli_color_dark_grey);

    for(s32 i = 0; i < COUNT_OF(Buttons); i++)
    {
        const PianoBtn* btn = Buttons + i;
        const uli_rect* rect = &btn->rect;
        uli_api_rect(uli, x + rect->x, y + rect->y, rect->w, rect->h,
            active && effect->note == btn->note ? uli_color_red :
                btn->white
                    ? hover == btn->note ? uli_color_light_grey : uli_color_white
                    : hover == btn->note ? uli_color_dark_grey : uli_color_black);

        if(btn->white)
            uli_api_rect(uli, x + rect->x, y + (WhiteHeight - WhiteShadow), WhiteWidth, WhiteShadow, uli_color_black);

        // draw current note marker
        if(currentOctave == octave && effect->note == btn->note) {
            uli_api_rect(uli, x + rect->x + 1, y + rect->y + rect->h - 3, 1, 1, uli_color_red);
        }
    }
}

static void drawPiano(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    enum {Width = 29};

    for(s32 i = 0; i < OCTAVES; i++)
    {
        drawPianoOctave(sfx, x + Width*i, y, i);
    }
}

static void drawSpeedPanel(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    enum
    {
        Count = 8, Gap = 1, ColWidth = 1, ColWidthGap = ColWidth + Gap,
        Width = Count * ColWidthGap - Gap, Height = 5,
        MaxSpeed = (1 << SFX_SPEED_BITS) / 2
    };

    uli_rect rect = {x + 13, y, Width, Height};
    uli_sample* effect = getEffect(sfx);
    s32 hover = -1;

    if(checkMousePos(sfx->studio, &rect))
    {
        setCursor(sfx->studio, uli_cursor_hand);

        s32 spd = (uli_api_mouse(uli).x - rect.x) / ColWidthGap;
        hover = spd;

        SHOW_TOOLTIP(sfx->studio, "set speed to %i", spd);

        if(checkMouseDown(sfx->studio, &rect, uli_mouse_left))
        {
            effect->speed = spd - MaxSpeed;
            history_add(sfx->history);
        }
    }

    uli_api_print(uli, "SPD", x, y, uli_color_dark_grey, true, 1, true);

    for(s32 i = 0; i < Count; i++)
        uli_api_rect(uli, rect.x + i * ColWidthGap, rect.y, ColWidth, rect.h, i - MaxSpeed <= effect->speed ? uli_color_light_green : hover == i ? uli_color_grey : uli_color_dark_grey);
}

static void drawSelectorPanel(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    enum
    {
        Size = 3, Gap = 1, SizeGap = Size + Gap,
        GroupGap = 2, Groups = 4, Cols = 4, Rows = SFX_COUNT / (Cols * Groups),
        GroupWidth = Cols * SizeGap - Gap,
        Width = (GroupWidth + GroupGap) * Groups - GroupGap, Height = Rows * SizeGap - Gap
    };

    uli_rect rect = {x, y, Width, Height};
    s32 hover = -1;

    if(checkMousePos(sfx->studio, &rect))
        for(s32 g = 0, i = 0; g < Groups; g++)
            for(s32 r = 0; r < Rows; r++)
                for(s32 c = 0; c < Cols; c++, i++)
                {
                    uli_rect rect = {x + c * SizeGap + g * (GroupWidth + GroupGap), y + r * SizeGap, SizeGap, SizeGap};

                    if(checkMousePos(sfx->studio, &rect))
                    {
                        setCursor(sfx->studio, uli_cursor_hand);
                        hover = i;

                        SHOW_TOOLTIP(sfx->studio, "edit sfx #%02i", hover);

                        if(checkMouseClick(sfx->studio, &rect, uli_mouse_left))
                            sfx->index = i;

                        goto draw;
                    }
                }
draw:

    for(s32 g = 0, i = 0; g < Groups; g++)
        for(s32 r = 0; r < Rows; r++)
            for(s32 c = 0; c < Cols; c++, i++)
            {
                static const u8 EmptyEffect[sizeof(uli_sample)] = {0};
                bool empty = memcmp(sfx->src->samples.data + i, EmptyEffect, sizeof EmptyEffect) == 0;

                uli_api_rect(uli, x + c * SizeGap + g * (GroupWidth + GroupGap), y + r * SizeGap, Size, Size,
                    sfx->index == i ? uli_color_light_green : hover == i ? uli_color_grey : empty ? uli_color_dark_grey : uli_color_light_grey);
            }
}

static void drawSelector(Sfx* sfx, s32 x, s32 y)
{
    uli_mem* uli = sfx->uli;

    enum {Width = 70, Height = 25};

    drawPanelBorder(uli, x, y, Width, Height, uli_color_black);

    {
        char buf[] = "00";
        sprintf(buf, "%02i", sfx->index);
        uli_api_print(uli, buf, x + 20, y + 2, uli_color_light_green, true, 1, true);
        uli_api_print(uli, "IDX", x + 6, y + 2, uli_color_dark_grey, true, 1, true);
    }

    drawSpeedPanel(sfx, x + 40, y + 2);
    drawSelectorPanel(sfx, x + 2, y + 9);
}

static void tick(Sfx* sfx)
{
    uli_mem* uli = sfx->uli;

    sfx->play.active = false;
    sfx->hoverWave = -1;

    processKeyboard(sfx);
    processEnvelopesKeyboard(sfx);

    uli_api_cls(uli, uli_color_grey);

    drawCanvas(sfx, 88, 12, sfx->volwave);
    drawCanvas(sfx, 88, 51, SFX_CHORD_PANEL);
    drawCanvas(sfx, 88, 90, SFX_PITCH_PANEL);

    drawSelector(sfx, 9, 12);
    drawPiano(sfx, 5, 127);
    drawWavePanel(sfx, 7, 41);
    drawToolbar(sfx->studio, uli, true);

    playSound(sfx);

    if(sfx->play.active)
        sfx->play.tick++;
    else
        sfx->play.tick = 0;
}

static void onStudioEvent(Sfx* sfx, StudioEvent event)
{
    switch(event)
    {
    case ULI_TOOLBAR_CUT:   cutToClipboard(sfx); break;
    case ULI_TOOLBAR_COPY:  copyToClipboard(sfx); break;
    case ULI_TOOLBAR_PASTE: copyFromClipboard(sfx); break;
    case ULI_TOOLBAR_UNDO:  undo(sfx); break;
    case ULI_TOOLBAR_REDO:  redo(sfx); break;
    default: break;
    }
}

void initSfx(Sfx* sfx, Studio* studio, uli_sfx* src)
{
    if(sfx->history) history_delete(sfx->history);
    if(sfx->waveHistory) history_delete(sfx->waveHistory);

    *sfx = (Sfx)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .tick = tick,
        .src = src,
        .index = 0,
        .volwave = SFX_VOLUME_PANEL,
        .hoverWave = -1,
        .holdValue = -1,
        .play =
        {
            .note = -1,
            .active = false,
            .tick = 0,
        },

        .history = history_create(&src->samples, sizeof(uli_samples)),
        .waveHistory = history_create(&src->waveforms, sizeof(uli_waveforms)),
        .event = onStudioEvent,
    };
}

void freeSfx(Sfx* sfx)
{
    history_delete(sfx->history);
    history_delete(sfx->waveHistory);
    free(sfx);
}