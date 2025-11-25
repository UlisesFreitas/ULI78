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
#include "tilesheet.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#define TRANSPARENT_COLOR 255

typedef void(*PixelFunc)(uli_mem* memory, s32 x, s32 y, u8 color);

static uli_tilesheet getTileSheetFromSegment(uli_mem* memory, u8 segment)
{
    u8* src;
    switch (segment) {
    case 0:
    case 1:
        src = (u8*)&memory->ram->font; break;
    default:
        src = (u8*)&memory->ram->tiles.data; break;
    }

    return uli_tilesheet_get(segment, src);
}

static u8* getPalette(uli_mem* uli, u8* colors, u8 count)
{
    static u8 mapping[ULI_PALETTE_SIZE];
    for (s32 i = 0; i < ULI_PALETTE_SIZE; i++) mapping[i] = uli_tool_peek4(uli->ram->vram.mapping, i);
    for (s32 i = 0; i < count; i++) {
        if (colors[i] < ULI_PALETTE_SIZE)
        {
            mapping[colors[i]] = TRANSPARENT_COLOR;
        }
    }
    return mapping;
}

static inline u8 mapColor(uli_mem* uli, u8 color)
{
    return uli_tool_peek4(uli->ram->vram.mapping, color & 0xf);
}

static inline void setPixel(uli_core* core, s32 x, s32 y, u8 color)
{
    const uli_vram* vram = &core->memory.ram->vram;

    if (x < core->state.clip.l || y < core->state.clip.t || x >= core->state.clip.r || y >= core->state.clip.b) return;

    uli_api_poke4((uli_mem*)core, y * ULI78_WIDTH + x, color);
}

static inline void setPixelFast(uli_core* core, s32 x, s32 y, u8 color)
{
    // does not do any CLIP checking, the caller needs to do that first
    uli_api_poke4((uli_mem*)core, y * ULI78_WIDTH + x, color);
}

static inline u8 getPixel(uli_core* core, s32 x, s32 y)
{
    return x < 0 || y < 0 || x >= ULI78_WIDTH || y >= ULI78_HEIGHT
        ? 0
        : uli_api_peek4((uli_mem*)core, y * ULI78_WIDTH + x);
}

#define EARLY_CLIP(x, y, width, height) \
    ( \
        (((y)+(height)-1) < core->state.clip.t) \
        || (((x)+(width)-1) < core->state.clip.l) \
        || ((y) >= core->state.clip.b) \
        || ((x) >= core->state.clip.r) \
    )

static void drawHLine(uli_core* core, s32 x, s32 y, s32 width, u8 color)
{
    const uli_vram* vram = &core->memory.ram->vram;

    if (y < core->state.clip.t || core->state.clip.b <= y) return;

    s32 xl = MAX(x, core->state.clip.l);
    s32 xr = MIN(x + width, core->state.clip.r);
    s32 start = y * ULI78_WIDTH;

    for(s32 i = start + xl, end = start + xr; i < end; ++i)
        uli_api_poke4((uli_mem*)core, i, color);
}

static void drawVLine(uli_core* core, s32 x, s32 y, s32 height, u8 color)
{
    const uli_vram* vram = &core->memory.ram->vram;

    if (x < core->state.clip.l || core->state.clip.r <= x) return;

    s32 yl = y < 0 ? 0 : y;
    s32 yr = y + height >= ULI78_HEIGHT ? ULI78_HEIGHT : y + height;

    for (s32 i = yl; i < yr; ++i)
        setPixel(core, x, i, color);
}

static void drawRect(uli_core* core, s32 x, s32 y, s32 width, s32 height, u8 color)
{
    for (s32 i = y; i < y + height; ++i)
        drawHLine(core, x, i, width, color);
}

static void drawRectBorder(uli_core* core, s32 x, s32 y, s32 width, s32 height, u8 color)
{
    drawHLine(core, x, y, width, color);
    drawHLine(core, x, y + height - 1, width, color);

    drawVLine(core, x, y, height, color);
    drawVLine(core, x + width - 1, y, height, color);
}

#define DRAW_TILE_BODY(X, Y) do {\
    for(s32 py=sy; py < ey; py++, y++) \
    { \
        s32 xx = x; \
        for(s32 px=sx; px < ex; px++, xx++) \
        { \
            u8 color = mapping[uli_tilesheet_gettilepix(tile, (X), (Y))];\
            if(color != TRANSPARENT_COLOR) setPixelFast(core, xx, y, color); \
        } \
    } \
    } while(0)

#define REVERT(X) (ULI_SPRITESIZE - 1 - (X))

static void drawTile(uli_core* core, uli_tileptr* tile, s32 x, s32 y, u8* colors, s32 count, s32 scale, uli_flip flip, uli_rotate rotate)
{
    const uli_vram* vram = &core->memory.ram->vram;
    u8* mapping = getPalette(&core->memory, colors, count);

    rotate &= 3;
    u32 orientation = flip & 3;

    if (rotate == uli_90_rotate) orientation ^= 1;
    else if (rotate == uli_180_rotate) orientation ^= 3;
    else if (rotate == uli_270_rotate) orientation ^= 2;
    if (rotate == uli_90_rotate || rotate == uli_270_rotate) orientation |= 4;

    if (scale == 1) {
        // the most common path
        s32 sx, sy, ex, ey;
        sx = core->state.clip.l - x; if (sx < 0) sx = 0;
        sy = core->state.clip.t - y; if (sy < 0) sy = 0;
        ex = core->state.clip.r - x; if (ex > ULI_SPRITESIZE) ex = ULI_SPRITESIZE;
        ey = core->state.clip.b - y; if (ey > ULI_SPRITESIZE) ey = ULI_SPRITESIZE;
        y += sy;
        x += sx;
        switch (orientation) {
        case 4: DRAW_TILE_BODY(py, px); break;
        case 6: DRAW_TILE_BODY(REVERT(py), px); break;
        case 5: DRAW_TILE_BODY(py, REVERT(px)); break;
        case 7: DRAW_TILE_BODY(REVERT(py), REVERT(px)); break;
        case 0: DRAW_TILE_BODY(px, py); break;
        case 2: DRAW_TILE_BODY(px, REVERT(py)); break;
        case 1: DRAW_TILE_BODY(REVERT(px), py); break;
        case 3: DRAW_TILE_BODY(REVERT(px), REVERT(py)); break;
        }
        return;
    }

    if (EARLY_CLIP(x, y, ULI_SPRITESIZE * scale, ULI_SPRITESIZE * scale)) return;

    for (s32 py = 0; py < ULI_SPRITESIZE; py++, y += scale)
    {
        s32 xx = x;
        for (s32 px = 0; px < ULI_SPRITESIZE; px++, xx += scale)
        {
            s32 ix = orientation & 1 ? ULI_SPRITESIZE - px - 1 : px;
            s32 iy = orientation & 2 ? ULI_SPRITESIZE - py - 1 : py;
            if (orientation & 4) {
                s32 tmp = ix; ix = iy; iy = tmp;
            }
            u8 color = mapping[uli_tilesheet_gettilepix(tile, ix, iy)];
            if (color != TRANSPARENT_COLOR) drawRect(core, xx, y, scale, scale, color);
        }
    }
}

#undef DRAW_TILE_BODY
#undef REVERT

static void drawSprite(uli_core* core, s32 index, s32 x, s32 y, s32 w, s32 h, u8* colors, s32 count, s32 scale, uli_flip flip, uli_rotate rotate)
{
    const uli_vram* vram = &core->memory.ram->vram;

    if (index < 0)
        return;

    rotate &= 3;
    flip &= 3;

    uli_tilesheet sheet = getTileSheetFromSegment(&core->memory, core->memory.ram->vram.blit.segment);
    if (w == 1 && h == 1) {
        uli_tileptr tile = uli_tilesheet_gettile(&sheet, index, false);
        drawTile(core, &tile, x, y, colors, count, scale, flip, rotate);
    }
    else
    {
        s32 step = ULI_SPRITESIZE * scale;
        s32 cols = sheet.segment->sheet_width;

        const uli_flip vert_horz_flip = uli_horz_flip | uli_vert_flip;

        if (EARLY_CLIP(x, y, w * step, h * step)) return;

        for (s32 i = 0; i < w; i++)
        {
            for (s32 j = 0; j < h; j++)
            {
                s32 mx = i;
                s32 my = j;

                if (flip == uli_horz_flip || flip == vert_horz_flip) mx = w - 1 - i;
                if (flip == uli_vert_flip || flip == vert_horz_flip) my = h - 1 - j;

                if (rotate == uli_180_rotate)
                {
                    mx = w - 1 - mx;
                    my = h - 1 - my;
                }
                else if (rotate == uli_90_rotate)
                {
                    if (flip == uli_no_flip || flip == vert_horz_flip) my = h - 1 - my;
                    else mx = w - 1 - mx;
                }
                else if (rotate == uli_270_rotate)
                {
                    if (flip == uli_no_flip || flip == vert_horz_flip) mx = w - 1 - mx;
                    else my = h - 1 - my;
                }

                enum { Cols = ULI_SPRITESHEET_SIZE / ULI_SPRITESIZE };


                uli_tileptr tile = uli_tilesheet_gettile(&sheet, index + mx + my * cols, false);
                if (rotate == 0 || rotate == 2)
                    drawTile(core, &tile, x + i * step, y + j * step, colors, count, scale, flip, rotate);
                else
                    drawTile(core, &tile, x + j * step, y + i * step, colors, count, scale, flip, rotate);
            }
        }
    }
}

static void drawMap(uli_core* core, const uli_map* src, s32 x, s32 y, s32 width, s32 height, s32 sx, s32 sy, u8* colors, s32 count, s32 scale, RemapFunc remap, void* data)
{
    const s32 size = ULI_SPRITESIZE * scale;

    uli_tilesheet sheet = getTileSheetFromSegment(&core->memory, core->memory.ram->vram.blit.segment);

    for (s32 j = y, jj = sy; j < y + height; j++, jj += size)
        for (s32 i = x, ii = sx; i < x + width; i++, ii += size)
        {
            s32 mi = uli_modulo(i, ULI_MAP_WIDTH);
            s32 mj = uli_modulo(j, ULI_MAP_HEIGHT);

            s32 index = mi + mj * ULI_MAP_WIDTH;
            RemapResult retile = { *(src->data + index), uli_no_flip, uli_no_rotate };

            if (remap)
                remap(data, mi, mj, &retile);

            uli_tileptr tile = uli_tilesheet_gettile(&sheet, retile.index, true);
            drawTile(core, &tile, ii, jj, colors, count, scale, retile.flip, retile.rotate);
        }
}

static s32 drawChar(uli_core* core, uli_tileptr* font_char, s32 x, s32 y, s32 scale, bool fixed, u8* mapping)
{
    const uli_vram* vram = &core->memory.ram->vram;

    enum { Size = ULI_SPRITESIZE };

    s32 j = 0, start = 0, end = Size;

    if (!fixed) {
        for (s32 i = 0; i < Size; i++) {
            for (j = 0; j < Size; j++)
                if (mapping[uli_tilesheet_gettilepix(font_char, i, j)] != TRANSPARENT_COLOR) break;
            if (j < Size) break; else start++;
        }
        for (s32 i = Size - 1; i >= start; i--) {
            for (j = 0; j < Size; j++)
                if (mapping[uli_tilesheet_gettilepix(font_char, i, j)] != TRANSPARENT_COLOR) break;
            if (j < Size) break; else end--;
        }
    }
    s32 width = end - start;

    if (EARLY_CLIP(x, y, Size * scale, Size * scale)) return width;

    s32 colStart = start, colStep = 1, rowStart = 0, rowStep = 1;

    for (s32 i = 0, col = colStart, xs = x; i < width; i++, col += colStep, xs += scale)
    {
        for (s32 j = 0, row = rowStart, ys = y; j < Size; j++, row += rowStep, ys += scale)
        {
            u8 color = uli_tilesheet_gettilepix(font_char, col, row);
            if (mapping[color] != TRANSPARENT_COLOR)
                drawRect(core, xs, ys, scale, scale, mapping[color]);
        }
    }
    return width;
}

static s32 drawText(uli_core* core, uli_tilesheet* font_face, const char* text, s32 x, s32 y, s32 width, s32 height, bool fixed, u8* mapping, s32 scale, bool alt)
{
    s32 pos = x;
    s32 MAX = x;
    char sym = 0;

    while ((sym = *text++))
    {
        if (sym == '\n')
        {
            if (pos > MAX)
                MAX = pos;

            pos = x;
            y += height * scale;
        }
        else {
            uli_tileptr font_char = uli_tilesheet_gettile(font_face, alt * ULI_FONT_CHARS + sym, true);
            s32 size = drawChar(core, &font_char, pos, y, scale, fixed, mapping);
            pos += ((!fixed && size) ? size + 1 : width) * scale;
        }
    }

    return pos > MAX ? pos - x : MAX - x;
}

void uli_api_clip(uli_mem* memory, s32 x, s32 y, s32 width, s32 height)
{
    uli_core* core = (uli_core*)memory;
    uli_vram* vram = &memory->ram->vram;

    core->state.clip.l = x;
    core->state.clip.t = y;
    core->state.clip.r = x + width;
    core->state.clip.b = y + height;

    if (core->state.clip.l < 0) core->state.clip.l = 0;
    if (core->state.clip.t < 0) core->state.clip.t = 0;
    if (core->state.clip.r > ULI78_WIDTH) core->state.clip.r = ULI78_WIDTH;
    if (core->state.clip.b > ULI78_HEIGHT) core->state.clip.b = ULI78_HEIGHT;
}

void uli_api_rect(uli_mem* memory, s32 x, s32 y, s32 width, s32 height, u8 color)
{
    uli_core* core = (uli_core*)memory;

    drawRect(core, x, y, width, height, mapColor(memory, color));
}

static double ZBuffer[ULI78_WIDTH * ULI78_HEIGHT];

void uli_api_cls(uli_mem* uli, u8 color)
{
    uli_core* core = (uli_core*)uli;
    uli_vram* vram = &uli->ram->vram;

    static const struct ClipRect EmptyClip = { 0, 0, ULI78_WIDTH, ULI78_HEIGHT };

    color = mapColor(uli, color);

    if (MEMCMP(core->state.clip, EmptyClip))
    {
        memset(&vram->screen, (color & 0xf) | (color << ULI_PALETTE_BPP), sizeof(uli_screen));
        ZEROMEM(ZBuffer);
    }
    else
    {
        for(s32 y = core->state.clip.t, start = y * ULI78_WIDTH; y < core->state.clip.b; ++y, start += ULI78_WIDTH)
            for(s32 x = core->state.clip.l, pixel = start + x; x < core->state.clip.r; ++x, ++pixel)
            {
                uli_api_poke4(uli, pixel, color);
                ZBuffer[pixel] = 0;
            }
    }
}

s32 uli_api_font(uli_mem* memory, const char* text, s32 x, s32 y, u8* trans_colors, u8 trans_count, s32 w, s32 h, bool fixed, s32 scale, bool alt)
{
    u8* mapping = getPalette(memory, trans_colors, trans_count);

    // Compatibility : flip top and bottom of the spritesheet
    // to preserve uli_api_font's default target
    u8 segment = memory->ram->vram.blit.segment >> 1;
    u8 flipmask = 1; while (segment >>= 1) flipmask <<= 1;

    uli_tilesheet font_face = getTileSheetFromSegment(memory, memory->ram->vram.blit.segment ^ flipmask);
    return drawText((uli_core*)memory, &font_face, text, x, y, w, h, fixed, mapping, scale, alt);
}

s32 uli_api_print(uli_mem* memory, const char* text, s32 x, s32 y, u8 color, bool fixed, s32 scale, bool alt)
{
    u8 mapping[] = { 255, color };
    uli_tilesheet font_face = getTileSheetFromSegment(memory, 1);

    const uli_font_data* font = alt ? &memory->ram->font.alt : &memory->ram->font.regular;
    s32 width = font->width;

    // Compatibility : print uses reduced width for non-fixed space
    if (!fixed) width -= 2;
    return drawText((uli_core*)memory, &font_face, text, x, y, width, font->height, fixed, mapping, scale, alt);
}

void uli_api_spr(uli_mem* memory, s32 index, s32 x, s32 y, s32 w, s32 h, u8* trans_colors, u8 trans_count, s32 scale, uli_flip flip, uli_rotate rotate)
{
    drawSprite((uli_core*)memory, index, x, y, w, h, trans_colors, trans_count, scale, flip, rotate);
}

static inline u8* getFlag(uli_mem* memory, s32 index, u8 flag)
{
    static u8 stub = 0;
    if (index >= ULI_FLAGS || flag >= BITS_IN_BYTE)
        return &stub;

    return memory->ram->flags.data + index;
}

bool uli_api_fget(uli_mem* memory, s32 index, u8 flag)
{
    return *getFlag(memory, index, flag) & (1 << flag);
}

void uli_api_fset(uli_mem* memory, s32 index, u8 flag, bool value)
{
    if (value)
        *getFlag(memory, index, flag) |= (1 << flag);
    else
        *getFlag(memory, index, flag) &= ~(1 << flag);
}

u8 uli_api_pix(uli_mem* memory, s32 x, s32 y, u8 color, bool get)
{
    uli_core* core = (uli_core*)memory;

    if (get) return getPixel(core, x, y);

    setPixel(core, x, y, mapColor(memory, color));
    return 0;
}

void uli_api_rectb(uli_mem* memory, s32 x, s32 y, s32 width, s32 height, u8 color)
{
    uli_core* core = (uli_core*)memory;

    drawRectBorder(core, x, y, width, height, mapColor(memory, color));
}

static struct
{
    s16 Left[ULI78_HEIGHT];
    s16 Right[ULI78_HEIGHT];
} SidesBuffer;

static void initSidesBuffer()
{
    for (s32 i = 0; i < COUNT_OF(SidesBuffer.Left); i++)
        SidesBuffer.Left[i] = ULI78_WIDTH, SidesBuffer.Right[i] = -1;
}

static void setSidePixel(s32 x, s32 y)
{
    if (y >= 0 && y < ULI78_HEIGHT)
    {
        if (x < SidesBuffer.Left[y]) SidesBuffer.Left[y] = x;
        if (x > SidesBuffer.Right[y]) SidesBuffer.Right[y] = x;
    }
}

static void drawEllipse(uli_mem* memory, s32 x0, s32 y0, s32 x1, s32 y1, u8 color, PixelFunc pix)
{
    if(x0 > x1 || y0 > y1)
        return;

    s64 a = abs(x1 - x0), b = abs(y1 - y0), b1 = b & 1; /* values of diameter */
    s64 dx = 4 * (1 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
    s64 err = dx + dy + b1 * a * a, e2; /* error of 1.step */

    if (x0 > x1) { x0 = x1; x1 += a; } /* if called with swapped pos32s */
    if (y0 > y1) y0 = y1; /* .. exchange them */
    y0 += (b + 1) / 2; y1 = y0 - b1;   /* starting pixel */
    a *= 8 * a; b1 = 8 * b * b;

    do
    {
        pix(memory, x1, y0, color); /*   I. Quadrant */
        pix(memory, x0, y0, color); /*  II. Quadrant */
        pix(memory, x0, y1, color); /* III. Quadrant */
        pix(memory, x1, y1, color); /*  IV. Quadrant */
        e2 = 2 * err;
        if (e2 <= dy) { y0++; y1--; err += dy += a; }  /* y step */
        if (e2 >= dx || 2 * err > dy) { x0++; x1--; err += dx += b1; } /* x step */
    } while (x0 <= x1);

    while (y0-y1 < b)
    {  /* too early stop of flat ellipses a=1 */
        pix(memory, x0 - 1, y0,    color); /* -> finish tip of ellipse */
        pix(memory, x1 + 1, y0++,  color);
        pix(memory, x0 - 1, y1,    color);
        pix(memory, x1 + 1, y1--,  color);
    }
}

static void setElliPixel(uli_mem* uli, s32 x, s32 y, u8 color)
{
    setPixel((uli_core*)uli, x, y, color);
}

static void setElliSide(uli_mem* uli, s32 x, s32 y, u8 color)
{
    setSidePixel(x, y);
}

static void drawSidesBuffer(uli_mem* memory, s32 y0, s32 y1, u8 color)
{
    uli_vram* vram = &memory->ram->vram;

    uli_core* core = (uli_core*)memory;
    s32 yt = MAX(core->state.clip.t, y0);
    s32 yb = MIN(core->state.clip.b, y1 + 1);
    u8 final_color = mapColor(&core->memory, color);
    for (s32 y = yt; y < yb; y++)
    {
        s32 xl = MAX(SidesBuffer.Left[y], core->state.clip.l);
        s32 xr = MIN(SidesBuffer.Right[y] + 1, core->state.clip.r);
        s32 start = y * ULI78_WIDTH;

        for(s32 i = start + xl, end = start + xr; i < end; ++i)
            uli_api_poke4(memory, i, color);
    }
}

void uli_api_circ(uli_mem* memory, s32 x, s32 y, s32 r, u8 color)
{
    initSidesBuffer();
    drawEllipse(memory, x - r, y - r, x + r, y + r, 0, setElliSide);
    drawSidesBuffer(memory, y - r, y + r + 1, mapColor(memory, color));
}

void uli_api_circb(uli_mem* memory, s32 x, s32 y, s32 r, u8 color)
{
    drawEllipse(memory, x - r, y - r, x + r, y + r, mapColor(memory, color), setElliPixel);
}

void uli_api_elli(uli_mem* memory, s32 x, s32 y, s32 a, s32 b, u8 color)
{
    initSidesBuffer();
    drawEllipse(memory, x - a, y - b, x + a, y + b, 0, setElliSide);
    drawSidesBuffer(memory, y - b, y + b + 1, mapColor(memory, color));
}

void uli_api_ellib(uli_mem* memory, s32 x, s32 y, s32 a, s32 b, u8 color)
{
    drawEllipse(memory, x - a, y - b, x + a, y + b, mapColor(memory, color), setElliPixel);
}

static inline float initLine(float *x0, float *x1, float *y0, float *y1)
{
    if (*y0 > *y1)
    {
        SWAP(*x0, *x1, float);
        SWAP(*y0, *y1, float);
    }

    float t = (*x1 - *x0) / (*y1 - *y0);

    if(*y0 < 0) *x0 -= *y0 * t, *y0 = 0;
    if(*y1 > ULI78_WIDTH) *x1 += (ULI78_WIDTH - *y0) * t, *y1 = ULI78_WIDTH;

    return t;
}

static void drawLine(uli_mem* uli, float x0, float y0, float x1, float y1, u8 color)
{
    if(fabs(x0 - x1) < fabs(y0 - y1))
        for (float t = initLine(&x0, &x1, &y0, &y1); y0 < y1; y0++, x0 += t)
            setPixel((uli_core*)uli, x0, y0, color);
    else
        for (float t = initLine(&y0, &y1, &x0, &x1); x0 < x1; x0++, y0 += t)
            setPixel((uli_core*)uli, x0, y0, color);

    setPixel((uli_core*)uli, x1, y1, color);
}

// Queue frame for floodFill.
// Filled horizontal segment of scanline y for xl <= x <= xr.
// Parent segment was on line y – dy. dy = 1 or –1.
typedef struct
{
    s32 y;
    s32 xl;
    s32 xr;
    s32 dy;
} FillSegment;

#define FILLQUEUESIZE 400
static struct
{
    FillSegment seg[FILLQUEUESIZE];
    size_t ini; // index of empty next in
    size_t outi; // index of next out
} fillQueue;

static inline void fillEnqueue(uli_core* uli, s32 y, s32 xl, s32 xr, s32 dy)
{
    size_t nextini = (fillQueue.ini + 1) % FILLQUEUESIZE;
    if (nextini == fillQueue.outi)
        return; // queue full
    if (y + dy < uli->state.clip.t || y + dy >= uli->state.clip.b)
        return;
    FillSegment* qseg = &fillQueue.seg[fillQueue.ini];
    qseg->y = y;
    qseg->xl = xl;
    qseg->xr = xr;
    qseg->dy = dy;
    fillQueue.ini = nextini;
}

static inline bool fillDequeue(s32* y, s32* xl, s32* xr, s32* dy)
{
    if (fillQueue.ini == fillQueue.outi)
        return false; // queue empty
    FillSegment* qseg = &fillQueue.seg[fillQueue.outi];
    *y = qseg->y + qseg->dy;
    *xl = qseg->xl;
    *xr = qseg->xr;
    *dy = qseg->dy;
    fillQueue.outi = (fillQueue.outi + 1) % FILLQUEUESIZE;
    return true;
}

static inline bool floodFillInside(u8 pix, u8 paint, u8 border, u8 original)
{
    return border == 255 ? pix == original : pix != paint && pix != border;
}

// "A Seed Fill Algorithm", Paul S. Heckbert, Graphics Gems, Andrew Glassner
// https://github.com/erich666/GraphicsGems/blob/master/gems/SeedFill.c
static void floodFill(uli_core* uli, s32 x, s32 y, u8 color, u8 border)
{
    if (x < uli->state.clip.l || y < uli->state.clip.t || x >= uli->state.clip.r || y >= uli->state.clip.b)
        return;
    u8 ov = getPixel(uli, x, y);
    if (ov == color || ov == border)
        return;
    fillQueue.ini = fillQueue.outi = 0;
    fillEnqueue(uli, y, x, x, 1); // needed in some cases
    fillEnqueue(uli, y + 1, x, x, -1); // seed segment
    s32 l, x1, x2, dy;
    while (fillDequeue(&y, &x1, &x2, &dy))
    {
        // segment of scan line y-dy for x1<=x<=x2 was previously filled,
        // now explore adjacent pixels in scan line y
        for (x = x1; x >= uli->state.clip.l && floodFillInside(getPixel(uli, x, y), color, border, ov); x--)
            setPixelFast(uli, x, y, color);
        if (x >= x1)
            goto floodFill_skip;
        l = x + 1;
        if (l < x1)
            fillEnqueue(uli, y, l, x1 - 1, -dy); // check leak left
        x = x1 + 1;
        do {
            for (; x < uli->state.clip.r && floodFillInside(getPixel(uli, x, y), color, border, ov); x++)
                setPixelFast(uli, x, y, color);
            fillEnqueue(uli, y, l, x - 1, dy);
            if (x > x2 + 1)
                fillEnqueue(uli, y, x2 + 1, x - 1, -dy); // check leak right
floodFill_skip:
            for (x++; x <= x2 && !floodFillInside(getPixel(uli, x, y), color, border, ov); x++);
            l = x;
        } while (x <= x2);
    }
}

typedef union
{
    struct
    {
        double x, y;
    };

    double d[2];
} Vec2;

typedef union
{
    struct
    {
        double x, y, z;
    };

    double d[3];
} Vec3;

typedef struct
{
    void* data;
    const Vec2* v[3];
    Vec3 w;
} ShaderAttr;

typedef uli_color(*PixelShader)(const ShaderAttr* a, s32 pixel);

static inline double edgeFn(const Vec2* a, const Vec2* b, const Vec2* c)
{
    return (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
}

static void drawTri(uli_mem* uli, const Vec2* v0, const Vec2* v1, const Vec2* v2, PixelShader shader, void* data)
{
    ShaderAttr a = {data, v0, v1, v2};

    uli_core* core = (uli_core*)uli;
    const struct ClipRect* clip = &core->state.clip;

    uli_point min = {floor(MIN3(a.v[0]->x, a.v[1]->x, a.v[2]->x)), floor(MIN3(a.v[0]->y, a.v[1]->y, a.v[2]->y))};
    uli_point max = {ceil(MAX3(a.v[0]->x, a.v[1]->x, a.v[2]->x)), ceil(MAX3(a.v[0]->y, a.v[1]->y, a.v[2]->y))};

    min.x = MAX(min.x, clip->l);
    min.y = MAX(min.y, clip->t);
    max.x = MIN(max.x, clip->r);
    max.y = MIN(max.y, clip->b);

    if(min.x >= max.x || min.y >= max.y) return;

    double area = edgeFn(a.v[0], a.v[1], a.v[2]);
    if((s32)floor(area) == 0) return;
    if(area < 0.0)
    {
        SWAP(a.v[1], a.v[2], const Vec2*);
        area = -area;
    }

    Vec2 d[3];
    Vec3 s;

    for(s32 i = 0; i != COUNT_OF(s.d); ++i)
    {
        // pixel center
        const double Center = 0.5 - FLT_EPSILON;
        Vec2 p = {min.x + Center, min.y + Center};

        s32 c = (i + 1) % 3, n = (i + 2) % 3;

        d[i].x = (a.v[c]->y - a.v[n]->y) / area;
        d[i].y = (a.v[n]->x - a.v[c]->x) / area;
        s.d[i] = edgeFn(a.v[c], a.v[n], &p) / area;
    }

    for(s32 y = min.y, start = min.y * ULI78_WIDTH + min.x; y < max.y; ++y, start += ULI78_WIDTH)
    {
        for(s32 i = 0; i != COUNT_OF(a.w.d); ++i)
            a.w.d[i] = s.d[i];

        for(s32 x = min.x, pixel = start; x < max.x; ++x, ++pixel)
        {
            if(a.w.x > -DBL_EPSILON && a.w.y > -DBL_EPSILON && a.w.z > -DBL_EPSILON)
            {
                u8 color = shader(&a, pixel);
                if(color != TRANSPARENT_COLOR)
                    uli_api_poke4(uli, pixel, color);
            }

            for(s32 i = 0; i != COUNT_OF(a.w.d); ++i)
                a.w.d[i] += d[i].x;
        }

        for(s32 i = 0; i != COUNT_OF(s.d); ++i)
            s.d[i] += d[i].y;
    }
}

static uli_color triColorShader(const ShaderAttr* a, s32 pixel){return *(u8*)a->data;}

void uli_api_tri(uli_mem* uli, float x1, float y1, float x2, float y2, float x3, float y3, u8 color)
{
    color = mapColor(uli, color);
    drawTri(uli,
        &(Vec2){x1, y1},
        &(Vec2){x2, y2},
        &(Vec2){x3, y3},
        triColorShader, &color);
}

void uli_api_trib(uli_mem* uli, float x1, float y1, float x2, float y2, float x3, float y3, u8 color)
{
    uli_core* core = (uli_core*)uli;

    u8 finalColor = mapColor(uli, color);

    drawLine(uli, x1, y1, x2, y2, finalColor);
    drawLine(uli, x2, y2, x3, y3, finalColor);
    drawLine(uli, x3, y3, x1, y1, finalColor);
}

typedef struct
{
    Vec2 _;
    Vec3 d;
}TexVert;

typedef struct
{
    uli_tilesheet sheet;
    u8* mapping;
    const u8* map;
    const uli_vram* vram;
    bool depth;
} TexData;

static inline bool shaderStart(const ShaderAttr* a, Vec3* vars, s32 pixel)
{
    TexData* data = a->data;

    if(data->depth)
    {
        vars->z = 0;
        for(s32 i = 0; i != COUNT_OF(a->v); ++i)
        {
            const TexVert* t = (TexVert*)a->v[i];
            vars->z += a->w.d[i] * t->d.z;
        }

        if(ZBuffer[pixel] < vars->z);
        else return false;
    }

    vars->x = vars->y = 0;
    for(s32 i = 0; i != COUNT_OF(a->v); ++i)
    {
        const TexVert* t = (TexVert*)a->v[i];
        vars->x += a->w.d[i] * t->d.x;
        vars->y += a->w.d[i] * t->d.y;
    }

    if(data->depth)
        vars->x /= vars->z,
        vars->y /= vars->z;

    return true;
}

static inline uli_color shaderEnd(const ShaderAttr* a, const Vec3* vars, s32 pixel, uli_color color)
{
    TexData* data = a->data;

    if(data->depth && color != TRANSPARENT_COLOR)
        ZBuffer[pixel] = vars->z;

    return color;
}

static uli_color triTexMapShader(const ShaderAttr* a, s32 pixel)
{
    TexData* data = a->data;

    Vec3 vars;
    if(!shaderStart(a, &vars, pixel))
        return TRANSPARENT_COLOR;

    enum { MapWidth = ULI_MAP_WIDTH * ULI_SPRITESIZE, MapHeight = ULI_MAP_HEIGHT * ULI_SPRITESIZE,
        WMask = ULI_SPRITESIZE - 1, HMask = ULI_SPRITESIZE - 1 };

    s32 iu = uli_modulo(floor(vars.x), MapWidth);
    s32 iv = uli_modulo(floor(vars.y), MapHeight);

    u8 idx = data->map[(iv >> 3) * ULI_MAP_WIDTH + (iu >> 3)];
    uli_tileptr tile = uli_tilesheet_gettile(&data->sheet, idx, true);

    return shaderEnd(a, &vars, pixel, data->mapping[uli_tilesheet_gettilepix(&tile, iu & WMask, iv & HMask)]);
}

static uli_color triTexTileShader(const ShaderAttr* a, s32 pixel)
{
    TexData* data = a->data;

    Vec3 vars;
    if(!shaderStart(a, &vars, pixel))
        return TRANSPARENT_COLOR;

    enum { WMask = ULI_SPRITESHEET_SIZE - 1, HMask = ULI_SPRITESHEET_SIZE * ULI_SPRITE_BANKS - 1 };

    return shaderEnd(a, &vars, pixel, data->mapping[uli_tilesheet_getpix(&data->sheet,
                     (s32)floor(vars.x) & WMask, (s32)floor(vars.y) & HMask)]);
}

static uli_color triTexVbankShader(const ShaderAttr* a, s32 pixel)
{
    TexData* data = a->data;

    Vec3 vars;
    if(!shaderStart(a, &vars, pixel))
        return TRANSPARENT_COLOR;

    s32 iu = uli_modulo(floor(vars.x), ULI78_WIDTH);
    s32 iv = uli_modulo(floor(vars.y), ULI78_HEIGHT);

    return shaderEnd(a, &vars, pixel, data->mapping[uli_tool_peek4(data->vram->data, iv * ULI78_WIDTH + iu)]);
}

void uli_api_ttri(uli_mem* uli,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float u1, float v1,
    float u2, float v2,
    float u3, float v3,
    uli_texture_src texsrc, u8* colors, s32 count,
    float z1, float z2, float z3, bool depth)
{
    // do not use depth if user passed z=0.0
    if(z1 < FLT_EPSILON || z2 < FLT_EPSILON || z3 < FLT_EPSILON)
        depth = false;

    TexData texData =
    {
        .sheet = getTileSheetFromSegment(uli, uli->ram->vram.blit.segment),
        .mapping = getPalette(uli, colors, count),
        .map = uli->ram->map.data,
        .vram = &((uli_core*)uli)->state.vbank.mem,
        .depth = depth,
    };

    TexVert t[] =
    {
        {x1, y1, u1, v1, z1},
        {x2, y2, u2, v2, z2},
        {x3, y3, u3, v3, z3},
    };

    if(depth)
        for(s32 i = 0; i != COUNT_OF(t); ++i)
            t[i].d.x /= t[i].d.z,
            t[i].d.y /= t[i].d.z,
            t[i].d.z = 1.0 / t[i].d.z;

    static const PixelShader Shaders[] =
    {
        [uli_tiles_texture] = triTexTileShader,
        [uli_map_texture]   = triTexMapShader,
        [uli_vbank_texture] = triTexVbankShader,
    };

    if(texsrc >= 0 && texsrc < COUNT_OF(Shaders))
        drawTri(uli,
            (const Vec2*)&t[0],
            (const Vec2*)&t[1],
            (const Vec2*)&t[2],
            Shaders[texsrc], &texData);
}

void uli_api_map(uli_mem* memory, s32 x, s32 y, s32 width, s32 height, s32 sx, s32 sy, u8* colors, u8 count, s32 scale, RemapFunc remap, void* data)
{
    drawMap((uli_core*)memory, &memory->ram->map, x, y, width, height, sx, sy, colors, count, scale, remap, data);
}

void uli_api_mset(uli_mem* memory, s32 x, s32 y, u8 value)
{
    if (x < 0 || x >= ULI_MAP_WIDTH || y < 0 || y >= ULI_MAP_HEIGHT) return;

    uli_map* src = &memory->ram->map;
    *(src->data + y * ULI_MAP_WIDTH + x) = value;
}

u8 uli_api_mget(uli_mem* memory, s32 x, s32 y)
{
    if (x < 0 || x >= ULI_MAP_WIDTH || y < 0 || y >= ULI_MAP_HEIGHT) return 0;

    const uli_map* src = &memory->ram->map;
    return *(src->data + y * ULI_MAP_WIDTH + x);
}

void uli_api_line(uli_mem* memory, float x0, float y0, float x1, float y1, u8 color)
{
    drawLine(memory, x0, y0, x1, y1, mapColor(memory, color));
}

void uli_api_paint(uli_mem* memory, s32 x, s32 y, u8 color, u8 bordercolor)
{
    bordercolor = bordercolor == 255 ? 255 : mapColor(memory, bordercolor);
    floodFill((uli_core*)memory, x, y, mapColor(memory, color), bordercolor);
}

#if defined(BUILD_DEPRECATED)
#include "draw_dep.c"
#endif
