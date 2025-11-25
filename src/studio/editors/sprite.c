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

#include "sprite.h"
#include "ext/history.h"

#include <math.h>
#include <ctype.h>

#define CANVAS_SIZE (64)
#define PALETTE_CELL_SIZE 8
#define PALETTE_ROWS 2
#define PALETTE_COLS (ULI_PALETTE_SIZE / PALETTE_ROWS)
#define PALETTE_WIDTH (PALETTE_COLS * PALETTE_CELL_SIZE)
#define PALETTE_HEIGHT (PALETTE_ROWS * PALETTE_CELL_SIZE)
#define BRUSH_SIZES 4

enum
{
    ToolbarH = TOOLBAR_SIZE,
    CanvasX = 24, CanvasY = 20, CanvasW = CANVAS_SIZE, CanvasH = CANVAS_SIZE,
    PaletteX = 24, PaletteY = 112, PaletteW = PALETTE_WIDTH, PaletteH = PALETTE_HEIGHT,
    SheetX = ULI78_WIDTH - ULI_SPRITESHEET_SIZE - 1, SheetY = ToolbarH, SheetW = ULI_SPRITESHEET_SIZE, SheetH = ULI_SPRITESHEET_SIZE,
};

// !TODO: move it to helpers place
static void drawPanelBorder(uli_mem* uli, s32 x, s32 y, s32 w, s32 h)
{
    uli_api_rect(uli, x, y-1, w, 1, uli_color_dark_grey);
    uli_api_rect(uli, x-1, y, 1, h, uli_color_dark_grey);
    uli_api_rect(uli, x, y+h, w, 1, uli_color_light_grey);
    uli_api_rect(uli, x+w, y, 1, h, uli_color_light_grey);
}

static void clearCanvasSelection(Sprite* sprite)
{
    memset(&sprite->select.rect, 0, sizeof(uli_rect));
}

static void initTileSheet(Sprite* sprite)
{
    sprite->blit.page %= sprite->blit.pages;
    sprite->sheet = uli_tilesheet_get((( sprite->blit.pages + sprite->blit.page) << 1) + sprite->blit.bank, (u8*)sprite->src);
}

static void updateIndex(Sprite* sprite)
{
    sprite->index = sprite->y * sprite->blit.pages * ULI_SPRITESHEET_COLS + sprite->x;
    // index has changed, clear selection
    clearCanvasSelection(sprite);
}

static inline bool isIdle(Sprite* sprite)
{
    return sprite->anim.movie == &sprite->anim.idle;
}

static void selectViewportPage(Sprite* sprite, u8 page)
{
    if(isIdle(sprite))
    {
        Anim* anim = sprite->anim.page.items;
        anim->start = (page - sprite->blit.page) * ULI_SPRITESHEET_SIZE;
        sprite->anim.movie = resetMovie(&sprite->anim.page);

        sprite->blit.page = page;
        updateIndex(sprite);
        initTileSheet(sprite);
    }
}

static void leftViewport(Sprite* sprite)
{
    s32 page = sprite->blit.page + sprite->blit.pages - 1;
    selectViewportPage(sprite, page % sprite->blit.pages);
}

static void rightViewport(Sprite* sprite)
{
    s32 page = sprite->blit.page + sprite->blit.pages + 1;
    selectViewportPage(sprite, page % sprite->blit.pages);
}

static s32 getIndexPosX(Sprite* sprite)
{
    return (sprite->x + sprite->blit.page * ULI_SPRITESHEET_COLS) * ULI_SPRITESIZE;
}

static s32 getIndexPosY(Sprite* sprite)
{
    return (sprite->y + sprite->blit.bank * ULI_SPRITESHEET_COLS) * ULI_SPRITESIZE;
}

static void drawSelection(Sprite* sprite, s32 x, s32 y, s32 w, s32 h)
{
    uli_mem* uli = sprite->uli;

    enum{Step = 3};
    u8 color = uli_color_white;

    s32 index = sprite->tickCounter / 10;
    for(s32 i = x; i < (x+w); i++)      { uli_api_pix(uli, i, y, index++ % Step ? color : 0, false);} index++;
    for(s32 i = y; i < (y+h); i++)      { uli_api_pix(uli, x + w-1, i, index++ % Step ? color : 0, false);} index++;
    for(s32 i = (x+w-1); i >= x; i--)   { uli_api_pix(uli, i, y + h-1, index++ % Step ? color : 0, false);} index++;
    for(s32 i = (y+h-1); i >= y; i--)   { uli_api_pix(uli, x, i, index++ % Step ? color : 0, false);}
}

static uli_rect getSpriteRect(Sprite* sprite)
{
    s32 x = getIndexPosX(sprite);
    s32 y = getIndexPosY(sprite);

    return (uli_rect){x, y, sprite->size, sprite->size};
}

static void drawCursorBorder(Sprite* sprite, s32 x, s32 y, s32 w, s32 h)
{
    uli_mem* uli = sprite->uli;

    uli_api_rectb(uli, x, y, w, h, uli_color_black);
    uli_api_rectb(uli, x-1, y-1, w+2, h+2, uli_color_white);
}

static void processPickerCanvasMouse(Sprite* sprite, s32 x, s32 y, s32 sx, s32 sy)
{
    uli_mem* uli = sprite->uli;
    uli_rect rect = {x, y, CANVAS_SIZE, CANVAS_SIZE};
    const s32 Size = CANVAS_SIZE / sprite->size;

    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);

        s32 mx = uli_api_mouse(uli).x - x;
        s32 my = uli_api_mouse(uli).y - y;

        mx -= mx % Size;
        my -= my % Size;

        drawCursorBorder(sprite, x + mx, y + my, Size, Size);

        if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
            sprite->color = uli_tilesheet_getpix(&sprite->sheet, sx + mx / Size, sy + my / Size);

        if(checkMouseDown(sprite->studio, &rect, uli_mouse_right))
            sprite->color2 = uli_tilesheet_getpix(&sprite->sheet, sx + mx / Size, sy + my / Size);
    }
}

static void paintPoint(Sprite* sprite, u8 color, s32 x, s32 y)
{
    s32 pixels = sprite->brushSize;

    for(s32 j = 0; j < pixels; j++)
        for(s32 i = 0; i < pixels; i++)
            uli_tilesheet_setpix(&sprite->sheet, x+i, y+j, color);
}

static void paintLine(Sprite* sprite, u8 color, s32 x1, s32 y1, s32 x2, s32 y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;

    // If infinite slope, draw vertical line
    if (dx == 0)
    {
        s32 step = y2 > y1 ? 1 : -1;
        for (s32 ly = y1; y2 > y1 ? ly <= y2 : ly >= y2; ly += step)
            paintPoint(sprite, color, x1, ly);
    }
    else
    {
        float slope = dy / dx;
        float intercept = y1 - slope * x1;

        // Draw for each point along whichever axis is longer
        if (fabs(dx) >= fabs(dy))
        {
            s32 step = x2 > x1 ? 1 : -1;
            for (s32 lx = x1; x2 > x1 ? lx <= x2 : lx >= x2; lx += step)
                paintPoint(sprite, color, lx, slope * lx + intercept);
        }
        else
        {
            s32 step = y2 > y1 ? 1 : -1;
            for (s32 ly = y1; y2 > y1 ? ly <= y2 : ly >= y2; ly += step)
                paintPoint(sprite, color, (ly - intercept) / slope, ly);
        }
    }
}

static s32 toCanvasCoord(s32 Size, s32 brushSize, s32 canvas_x, s32 x)
{
    s32 offset = (brushSize - Size) / 2;

    x = (x - canvas_x) - offset;
    x -= x % Size;

    if (x < 0) x = 0;
    if (x + brushSize >= CANVAS_SIZE) x = CANVAS_SIZE - brushSize;

    return x;
}

static void processDrawCanvasMouse(Sprite* sprite, s32 x, s32 y, s32 sx, s32 sy)
{
    uli_mem* uli = sprite->uli;
    uli_rect rect = {x, y, CANVAS_SIZE, CANVAS_SIZE};
    const s32 Size = CANVAS_SIZE / sprite->size;

    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);

        s32 brushSize = sprite->brushSize * Size;
        uli_point mouse = uli_api_mouse(uli);

        s32 mx = toCanvasCoord(Size, brushSize, x, mouse.x);
        s32 my = toCanvasCoord(Size, brushSize, y, mouse.y);

        SHOW_TOOLTIP(sprite->studio, "[x=%02i y=%02i]", mx / Size, my / Size);

        drawCursorBorder(sprite, x + mx, y + my, brushSize, brushSize);

        bool left = checkMouseDown(sprite->studio, &rect, uli_mouse_left);
        bool right = checkMouseDown(sprite->studio, &rect, uli_mouse_right);

        if(left || right)
        {
            if(!sprite->draw.start)
            {
                sprite->draw.start = true;
                sprite->draw.last = uli_api_mouse(uli);
            }

            s32 lx = toCanvasCoord(Size, brushSize, x, sprite->draw.last.x);
            s32 ly = toCanvasCoord(Size, brushSize, y, sprite->draw.last.y);

            u8 color = left ? sprite->color : sprite->color2;
            paintLine(sprite, color,
                sx + lx / Size,
                sy + ly / Size,
                sx + mx / Size,
                sy + my / Size
            );

            history_add(sprite->history);

            sprite->draw.last = uli_api_mouse(uli);
        }
        else
        {
            sprite->draw.start = false;
        }
    }
}

static void pasteSelection(Sprite* sprite)
{
    s32 l = getIndexPosX(sprite);
    s32 t = getIndexPosY(sprite);
    s32 r = l + sprite->size;
    s32 b = t + sprite->size;

    for(s32 sy = t, i = 0; sy < b; sy++)
        for(s32 sx = l; sx < r; sx++)
            uli_tilesheet_setpix(&sprite->sheet, sx, sy, sprite->select.back[i++]);

    uli_rect* rect = &sprite->select.rect;

    l += rect->x;
    t += rect->y;
    r = l + rect->w;
    b = t + rect->h;

    for(s32 sy = t, i = 0; sy < b; sy++)
        for(s32 sx = l; sx < r; sx++)
            uli_tilesheet_setpix(&sprite->sheet, sx, sy, sprite->select.front[i++]);

    history_add(sprite->history);
}

static void copySelection(Sprite* sprite)
{
    uli_rect rect = getSpriteRect(sprite);
    s32 r = rect.x + rect.w;
    s32 b = rect.y + rect.h;

    for(s32 sy = rect.y, i = 0; sy < b; sy++)
        for(s32 sx = rect.x; sx < r; sx++)
            sprite->select.back[i++] = uli_tilesheet_getpix(&sprite->sheet, sx, sy);

    {
        uli_rect* rect = &sprite->select.rect;
        memset(sprite->select.front, 0, CANVAS_SIZE * CANVAS_SIZE);

        for(s32 j = rect->y, index = 0; j < (rect->y + rect->h); j++)
            for(s32 i = rect->x; i < (rect->x + rect->w); i++)
            {
                u8* color = &sprite->select.back[i+j*sprite->size];
                sprite->select.front[index++] = *color;
                *color = sprite->color2;
            }
    }
}

static void processSelectCanvasMouse(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    uli_rect rect = {x, y, CANVAS_SIZE, CANVAS_SIZE};
    const s32 Size = CANVAS_SIZE / sprite->size;

    bool endDrag = false;

    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);

        s32 mx = uli_api_mouse(uli).x - x;
        s32 my = uli_api_mouse(uli).y - y;

        mx -= mx % Size;
        my -= my % Size;

        drawCursorBorder(sprite, x + mx, y + my, Size, Size);

        if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
        {
            if(sprite->select.drag)
            {
                s32 x = mx / Size;
                s32 y = my / Size;

                s32 rl = MIN(x, sprite->select.start.x);
                s32 rt = MIN(y, sprite->select.start.y);
                s32 rr = MAX(x, sprite->select.start.x);
                s32 rb = MAX(y, sprite->select.start.y);

                sprite->select.rect = (uli_rect){rl, rt, rr - rl + 1, rb - rt + 1};
            }
            else
            {
                sprite->select.drag = true;
                sprite->select.start = (uli_point){mx / Size, my / Size};
                sprite->select.rect = (uli_rect){sprite->select.start.x, sprite->select.start.y, 1, 1};
            }
        }
        else endDrag = sprite->select.drag;
    }
    else endDrag = !uli->ram->input.mouse.left && sprite->select.drag;

    if(endDrag)
    {
        copySelection(sprite);
        sprite->select.drag = false;
    }
}

static void floodFill(Sprite* sprite, s32 l, s32 t, s32 r, s32 b, s32 x, s32 y, u8 color, u8 fill)
{
    if(uli_tilesheet_getpix(&sprite->sheet, x, y) == color)
    {
        uli_tilesheet_setpix(&sprite->sheet, x, y, fill);

        if(x > l) floodFill(sprite, l, t, r, b, x-1, y, color, fill);
        if(x < r) floodFill(sprite, l, t, r, b, x+1, y, color, fill);
        if(y > t) floodFill(sprite, l, t, r, b, x, y-1, color, fill);
        if(y < b) floodFill(sprite, l, t, r, b, x, y+1, color, fill);
    }
}

static void replaceColor(Sprite* sprite, s32 l, s32 t, s32 r, s32 b, s32 x, s32 y, u8 color, u8 fill)
{
    for(s32 sy = t; sy <= b; sy++)
        for(s32 sx = l; sx <= r; sx++)
            if(uli_tilesheet_getpix(&sprite->sheet, sx, sy) == color)
                uli_tilesheet_setpix(&sprite->sheet, sx, sy, fill);
}

static void processFillCanvasMouse(Sprite* sprite, s32 x, s32 y, s32 l, s32 t)
{
    uli_mem* uli = sprite->uli;
    uli_rect rect = {x, y, CANVAS_SIZE, CANVAS_SIZE};
    const s32 Size = CANVAS_SIZE / sprite->size;

    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);

        s32 mx = uli_api_mouse(uli).x - x;
        s32 my = uli_api_mouse(uli).y - y;

        mx -= mx % Size;
        my -= my % Size;

        drawCursorBorder(sprite, x + mx, y + my, Size, Size);

        bool left = checkMouseClick(sprite->studio, &rect, uli_mouse_left);
        bool right = checkMouseClick(sprite->studio, &rect, uli_mouse_right);

        if(left || right)
        {
            s32 sx = l + mx / Size;
            s32 sy = t + my / Size;

            u8 color = uli_tilesheet_getpix(&sprite->sheet, sx, sy);
            u8 fill = left ? sprite->color : sprite->color2;

            if(color != fill)
            {
                uli_api_key(uli, uli_key_ctrl)
                    ? replaceColor(sprite, l, t, l + sprite->size-1, t + sprite->size-1, sx, sy, color, fill)
                    : floodFill(sprite, l, t, l + sprite->size-1, t + sprite->size-1, sx, sy, color, fill);
            }

            history_add(sprite->history);
        }
    }
}

static bool hasCanvasSelection(Sprite* sprite)
{
    return sprite->mode == SPRITE_SELECT_MODE && sprite->select.rect.w && sprite->select.rect.h;
}

static void drawBrushSlider(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    enum {Count = BRUSH_SIZES, Size = 5};

    uli_rect rect = {x, y, Size, (Size+1)*Count};

    bool over = false;
    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);

        showTooltip(sprite->studio, "BRUSH SIZE");
        over = true;

        if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
        {
            s32 my = uli_api_mouse(uli).y - y;

            sprite->brushSize = Count - my / (Size+1);
        }
    }

    uli_api_rect(uli, x+1, y, Size-2, Size*Count, uli_color_black);

    for(s32 i = 0; i < Count; i++)
    {
        s32 offset = y + i*(Size+1);

        uli_api_rect(uli, x, offset, Size, Size, uli_color_black);
        uli_api_rect(uli, x + 6, offset + 2, Count - i, 1, uli_color_black);
    }

    uli_api_rect(uli, x+2, y+1, 1, Size*Count+1, (over ? uli_color_white : uli_color_grey));

    s32 offset = y + (Count - sprite->brushSize)*(Size+1);
    uli_api_rect(uli, x, offset, Size, Size, uli_color_black);
    uli_api_rect(uli, x+1, offset+1, Size-2, Size-2, (over ? uli_color_white : uli_color_grey));
}

static void drawCanvasVBank1(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    const s32 Size = CANVAS_SIZE / sprite->size;
    const uli_rect rect = getSpriteRect(sprite);

    const uli_rect canvasRect = {x, y, CANVAS_SIZE, CANVAS_SIZE};
    if(checkMouseDown(sprite->studio, &canvasRect, uli_mouse_middle))
    {
        s32 mx = uli_api_mouse(uli).x - x;
        s32 my = uli_api_mouse(uli).y - y;
        sprite->color = uli_tilesheet_getpix(&sprite->sheet, rect.x + mx / Size, rect.y + my / Size);
    }

    drawPanelBorder(uli, canvasRect.x - 1, canvasRect.y - 1, canvasRect.w + 2, canvasRect.h + 2);
    uli_api_rectb(uli, canvasRect.x - 1, canvasRect.y - 1, canvasRect.w + 2, canvasRect.h + 2, uli_color_black);

    if(!sprite->palette.edit)
    {
        switch(sprite->mode)
        {
        case SPRITE_DRAW_MODE:
            processDrawCanvasMouse(sprite, x, y, rect.x, rect.y);
            drawBrushSlider(sprite, x - 15, y + 20);
            break;
        case SPRITE_PICK_MODE: processPickerCanvasMouse(sprite, x, y, rect.x, rect.y); break;
        case SPRITE_SELECT_MODE: processSelectCanvasMouse(sprite, x, y); break;
        case SPRITE_FILL_MODE: processFillCanvasMouse(sprite, x, y, rect.x, rect.y); break;
        }
    }

    if(hasCanvasSelection(sprite))
        drawSelection(sprite, x + sprite->select.rect.x * Size - 1, y + sprite->select.rect.y * Size - 1,
            sprite->select.rect.w * Size + 2, sprite->select.rect.h * Size + 2);
    else
    {
        char buf[sizeof "#9999"];
        s32 index = sprite->index + uli_blit_calc_index(&sprite->blit);
        sprintf(buf, sprite->hexindex ? "0x%02X" : "#%i", index);

        s32 w = CANVAS_SIZE - strlen(buf) * ULI_FONT_WIDTH;
        const uli_rect rect = {x + w / 2, ULI_SPRITESIZE + 2, w, ULI_FONT_HEIGHT};

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                sprite->hexindex = !sprite->hexindex;
            }
        }

        uli_api_print(uli, buf, rect.x, rect.y + 1, uli_color_black, true, 1, false);
        uli_api_print(uli, buf, rect.x, rect.y, uli_color_white, true, 1, false);
    }
}

static void drawCanvas(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    uli_rect rect = getSpriteRect(sprite);
    s32 r = rect.x + rect.w;
    s32 b = rect.y + rect.h;

    const s32 Size = CANVAS_SIZE / sprite->size;

    for(s32 sy = rect.y, j = y; sy < b; sy++, j += Size)
        for(s32 sx = rect.x, i = x; sx < r; sx++, i += Size)
            uli_api_rect(uli, i, j, Size, Size, uli_tilesheet_getpix(&sprite->sheet, sx, sy));
}

static void upCanvas(Sprite* sprite)
{
    uli_rect* rect = &sprite->select.rect;
    if(rect->y > 0) rect->y--;
    pasteSelection(sprite);
}

static void downCanvas(Sprite* sprite)
{
    uli_rect* rect = &sprite->select.rect;
    if(rect->y + rect->h < sprite->size) rect->y++;
    pasteSelection(sprite);
}

static void leftCanvas(Sprite* sprite)
{
    uli_rect* rect = &sprite->select.rect;
    if(rect->x > 0) rect->x--;
    pasteSelection(sprite);
}

static void rightCanvas(Sprite* sprite)
{
    uli_rect* rect = &sprite->select.rect;
    if(rect->x + rect->w < sprite->size) rect->x++;
    pasteSelection(sprite);
}

static void rotateSelectRect(Sprite* sprite)
{
    uli_rect rect = sprite->select.rect;

    s32 selection_center_x = rect.x + rect.w/2;
    s32 selection_center_y = rect.y + rect.h/2;

    // Rotate
    sprite->select.rect.w = rect.h;
    sprite->select.rect.h = rect.w;

    // Make the new center be at the position of the previous center
    sprite->select.rect.x -= (sprite->select.rect.x + sprite->select.rect.w/2) - selection_center_x;
    sprite->select.rect.y -= (sprite->select.rect.y + sprite->select.rect.h/2) - selection_center_y;

    // Check if we are not out of boundaries
    if (sprite->select.rect.x < 0) sprite->select.rect.x = 0;
    if (sprite->select.rect.y < 0) sprite->select.rect.y = 0;

    if (sprite->select.rect.x + sprite->select.rect.w >= sprite->size)
    {
        sprite->select.rect.x -= sprite->select.rect.x + sprite->select.rect.w - sprite->size;
    }

    if (sprite->select.rect.y + sprite->select.rect.h >= sprite->size)
    {
        sprite->select.rect.y -= sprite->select.rect.y + sprite->select.rect.h - sprite->size;
    }
}

static void rotateCanvas(Sprite* sprite)
{
    u8* buffer = (u8*)malloc(CANVAS_SIZE*CANVAS_SIZE);

    if(buffer)
    {
        {
            uli_rect rect = sprite->select.rect;
            const s32 Size = rect.h * rect.w;
            s32 diff = 0;

            for(s32 y = 0, i = 0; y < rect.w; y++)
                for(s32 x = 0; x < rect.h; x++)
                {
                    diff = rect.w * (x + 1) -y;
                    buffer[i++] = sprite->select.front[Size - diff];
                }

            for (s32 i = 0; i<Size; i++)
                sprite->select.front[i] = buffer[i];

            rotateSelectRect(sprite);
            pasteSelection(sprite);
            history_add(sprite->history);
        }

        free(buffer);
    }
}

static void deleteCanvas(Sprite* sprite)
{
    uli_rect* rect = &sprite->select.rect;

    s32 left = getIndexPosX(sprite) + rect->x;
    s32 top = getIndexPosY(sprite) + rect->y;
    s32 right = left + rect->w;
    s32 bottom = top + rect->h;

    for(s32 pixel_y = top; pixel_y < bottom; pixel_y++)
        for(s32 pixel_x = left; pixel_x < right; pixel_x++)
            uli_tilesheet_setpix(&sprite->sheet, pixel_x, pixel_y, sprite->color2);

    clearCanvasSelection(sprite);

    history_add(sprite->history);
}

static void flipCanvasHorz(Sprite* sprite)
{
    uli_rect* rect = &sprite->select.rect;

    s32 sprite_x = getIndexPosX(sprite);
    s32 sprite_y = getIndexPosY(sprite);

    s32 right = sprite_x + rect->x + rect->w/2;
    s32 bottom = sprite_y + rect->y + rect->h;

    for(s32 y = sprite_y + rect->y; y < bottom; y++)
        for(s32 x = sprite_x + rect->x, i = sprite_x + rect->x + rect->w - 1; x < right; x++, i--)
        {
            u8 color = uli_tilesheet_getpix(&sprite->sheet, x, y);
            uli_tilesheet_setpix(&sprite->sheet, x, y, uli_tilesheet_getpix(&sprite->sheet, i, y));
            uli_tilesheet_setpix(&sprite->sheet, i, y, color);
        }

    history_add(sprite->history);
    copySelection(sprite);
}

static void flipCanvasVert(Sprite* sprite)
{
    uli_rect* rect = &sprite->select.rect;

    s32 sprite_x = getIndexPosX(sprite);
    s32 sprite_y = getIndexPosY(sprite);

    s32 right = sprite_x + rect->x + rect->w;
    s32 bottom = sprite_y + rect->y + rect->h/2;

    for(s32 y = sprite_y + rect->y, i = sprite_y + rect->y + rect->h - 1; y < bottom; y++, i--)
        for(s32 x = sprite_x + rect->x; x < right; x++)
        {
            u8 color = uli_tilesheet_getpix(&sprite->sheet, x, y);
            uli_tilesheet_setpix(&sprite->sheet, x, y, uli_tilesheet_getpix(&sprite->sheet, x, i));
            uli_tilesheet_setpix(&sprite->sheet, x, i, color);
        }

    history_add(sprite->history);
    copySelection(sprite);
}

static s32* getSpriteIndexes(Sprite* sprite)
{
    static s32 indexes[ULI_SPRITESIZE*ULI_SPRITESIZE+1];
    memset(indexes, -1, sizeof indexes);

    u16 sheet_cols = ULI_SPRITESHEET_COLS * sprite->blit.pages;

    {
        uli_rect r = {sprite->index % sheet_cols, sprite->index / sheet_cols
            , sprite->size / ULI_SPRITESIZE, sprite->size / ULI_SPRITESIZE};

        s32 c = 0;
        for(s32 j = r.y; j < r.h + r.y; j++)
            for(s32 i = r.x; i < r.w + r.x; i++)
                indexes[c++] = (i + j * sheet_cols) + sprite->blit.bank * ULI_BANK_SPRITES;
    }

    return indexes;
}

static void drawFlags(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    if(hasCanvasSelection(sprite)) return;

    enum {Size = 5};

    u8* flags = getBankFlags(sprite->studio)->data;
    u8 or = 0;
    u8 and = 0xff;

    const s32* indexes = getSpriteIndexes(sprite);

    {
        const s32* i = indexes;
        while(*i >= 0)
        {
            u8 mask = flags[*i++];
            or |= mask;
            and &= mask;
        }
    }

    for(s32 i = 0; i < BITS_IN_BYTE; i++)
    {
        const u8 mask = 1 << i;
        uli_rect rect = {x, y + (Size+1)*i, Size, Size};

        bool over = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);
            over = true;

            SHOW_TOOLTIP(sprite->studio, "set flag [%i]", i);

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                const s32* i = indexes;

                if(or & mask)
                    while(*i >= 0)
                        flags[*i++] &= ~mask;
                else
                    while(*i >= 0)
                        flags[*i++] |= mask;
            }
        }

        uli_api_rect(uli, rect.x, rect.y, Size, Size, uli_color_black);

        u8 flagColor = i + 2;

        if(or & mask)
            uli_api_pix(uli, rect.x + 2, rect.y + 2, flagColor, false);
        else if(over)
            uli_api_rect(uli, rect.x + 1, rect.y + 1, Size - 2, Size - 2, flagColor);

        if(and & mask)
        {
            uli_api_rect(uli, rect.x + 1, rect.y + 1, Size - 2, Size - 2, flagColor);
            uli_api_pix(uli, rect.x + 3, rect.y + 1, uli_color_white, false);
        }

        uli_api_print(uli, (char[]){'0' + i, '\0'}, rect.x + (Size+2), rect.y, uli_color_light_grey, false, 1, true);
    }

    // draw flags editbox
    {
        uli_rect rect = {x, y + (Size+1)*BITS_IN_BYTE + 2, ULI_ALTFONT_WIDTH * 2 + 1, ULI_FONT_HEIGHT + 1};

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            showTooltip(sprite->studio, "flags hex value");

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                sprite->flags.edit = true;
                sprite->flags.pos = (uli_api_mouse(uli).x - rect.x) / (ULI_ALTFONT_WIDTH + 1);
            }
        }
        else if(checkMouseDown(sprite->studio, &(uli_rect){0, 0, ULI78_WIDTH, ULI78_HEIGHT}, uli_mouse_left))
        {
            sprite->flags.edit = false;
        }

        if(sprite->flags.edit)
        {
            drawPanelBorder(uli, rect.x, rect.y, rect.w, rect.h);
            uli_api_rect(uli, rect.x, rect.y, rect.w, rect.h, uli_color_black);

            uli_api_rect(uli, rect.x + sprite->flags.pos * ULI_ALTFONT_WIDTH, rect.y, ULI_ALTFONT_WIDTH + 1, rect.h, uli_color_red);
        }

        char buf[sizeof "FF"];
        sprintf(buf, "%02X", and);

        if(!sprite->flags.edit)
            uli_api_print(uli, buf, rect.x + 1, rect.y + 2, uli_color_black, false, 1, true);

        uli_api_print(uli, buf, rect.x + 1, rect.y + 1, uli_color_white, false, 1, true);
    }
}

static void switchBitMode(Sprite* sprite, uli_bpp bpp)
{
    uli_blit_update_bpp(&sprite->blit, bpp);
    updateIndex(sprite);
    initTileSheet(sprite);

    sprite->color  %= 1 << sprite->blit.mode;
    sprite->color2 %= 1 << sprite->blit.mode;
}

static void drawBitMode(Sprite* sprite, s32 x, s32 y, s32 w, s32 h)
{
    uli_mem* uli = sprite->uli;
    s32 label_w = uli_api_print(uli, "BPP :", x+2, y, uli_color_dark_grey, false, 1, true);
    x += label_w+4;
    w -= label_w+4;

    enum {Modes = 3, SizeY = 5, SizeX = 5, OffsetX = 15};

    s32 centerX = x + w / 2;

    for(s32 i = 0; i < Modes; i++)
    {
        uli_bpp mode = 1 << (2-i);
        bool current = mode == sprite->blit.mode;

        uli_rect rect = {centerX - SizeX / 2 + (i-1) * OffsetX, y, SizeX, SizeY};

        bool over = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);
            over = true;

            if(mode > 1)
                SHOW_TOOLTIP(sprite->studio, "%iBITS PER PIXEL", mode);
            else
                SHOW_TOOLTIP(sprite->studio, "%iBIT PER PIXEL", mode);

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                switchBitMode(sprite, mode);
            }
        }

        u8 label_color = current ? uli_color_white : uli_color_dark_grey;

        uli_api_rect(uli, rect.x, rect.y, SizeX, SizeY, uli_color_dark_grey);
        if (current) {
            uli_api_rect(uli, rect.x+1, rect.y+1, SizeX-2, SizeY-2, uli_color_yellow-i);
            uli_api_pix(uli, rect.x+3, rect.y+1,uli_color_white, false);
        }
        else if (over)
            uli_api_rect(uli, rect.x+1, rect.y+1, SizeX-2, SizeY-2, uli_color_light_grey);

        uli_api_print(uli, (char[]){'0' + mode, '\0'}, rect.x - 4, rect.y, label_color, false, 1, true);
    }
}

static void drawMoveButtons(Sprite* sprite)
{
    if(hasCanvasSelection(sprite))
    {
        enum { x = 24 };
        enum { y = 20 };

        static const u8 Icons[] = {uli_icon_bigup, uli_icon_bigdown, uli_icon_bigleft, uli_icon_bigright};

        static const uli_rect Rects[] =
        {
            {x + (CANVAS_SIZE - ULI_SPRITESIZE)/2, y - ULI_SPRITESIZE, ULI_SPRITESIZE, ULI_SPRITESIZE/2},
            {x + (CANVAS_SIZE - ULI_SPRITESIZE)/2, y + CANVAS_SIZE + ULI_SPRITESIZE/2, ULI_SPRITESIZE, ULI_SPRITESIZE/2},
            {x - ULI_SPRITESIZE, y + (CANVAS_SIZE - ULI_SPRITESIZE)/2, ULI_SPRITESIZE/2, ULI_SPRITESIZE},
            {x + CANVAS_SIZE + ULI_SPRITESIZE/2, y + (CANVAS_SIZE - ULI_SPRITESIZE)/2, ULI_SPRITESIZE/2, ULI_SPRITESIZE},
        };

        static void(* const Func[])(Sprite*) = {upCanvas, downCanvas, leftCanvas, rightCanvas};

        bool down = false;
        for(s32 i = 0; i < COUNT_OF(Icons); i++)
        {
            down = false;

            if(checkMousePos(sprite->studio, &Rects[i]))
            {
                setCursor(sprite->studio, uli_cursor_hand);

                if(checkMouseDown(sprite->studio, &Rects[i], uli_mouse_left)) down = true;

                if(checkMouseClick(sprite->studio, &Rects[i], uli_mouse_left))
                    Func[i](sprite);
            }

            drawBitIcon(sprite->studio, Icons[i], Rects[i].x, Rects[i].y+1, down ? uli_color_white : uli_color_black);

            if(!down) drawBitIcon(sprite->studio, Icons[i], Rects[i].x, Rects[i].y, uli_color_white);
        }
    }
}

static void drawRGBSlider(Sprite* sprite, s32 x, s32 y, u8* value)
{
    uli_mem* uli = sprite->uli;

    enum {Size = CANVAS_SIZE, Max = 255};

    {
        uli_rect rect = {x, y-2, Size, 5};

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
            {
                s32 mx = uli_api_mouse(uli).x - x;
                *value = mx * Max / (Size-1);
            }
        }

        uli_api_rect(uli, x, y+1, Size, 1, uli_color_black);
        uli_api_rect(uli, x, y, Size, 1, uli_color_white);

        {
            s32 offset = x + *value * (Size-1) / Max - 2;
            drawBitIcon(sprite->studio, uli_icon_pos, offset, y-1, uli_color_black);
            drawBitIcon(sprite->studio, uli_icon_pos, offset, y-2, uli_color_white);
        }
    }

    {
        uli_rect rect = {x - 4, y - 1, 2, 3};

        bool down = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
                (*value)--;
        }

        if(down)
        {
            drawBitIcon(sprite->studio, uli_icon_tinyleft, rect.x-1, rect.y, uli_color_white);
        }
        else
        {
            drawBitIcon(sprite->studio, uli_icon_tinyleft, rect.x-1, rect.y, uli_color_black);
            drawBitIcon(sprite->studio, uli_icon_tinyleft, rect.x-1, rect.y-1, uli_color_white);
        }
    }

    {
        uli_rect rect = {x + Size + 2, y - 1, 2, 3};

        bool down = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
                (*value)++;
        }

        if(down)
        {
            drawBitIcon(sprite->studio, uli_icon_tinyright, rect.x-1, rect.y, uli_color_white);
        }
        else
        {
            drawBitIcon(sprite->studio, uli_icon_tinyright, rect.x-1, rect.y, uli_color_black);
            drawBitIcon(sprite->studio, uli_icon_tinyright, rect.x-1, rect.y-1, uli_color_white);
        }
    }
}

static void pasteColor(Sprite* sprite)
{
    bool ovr = sprite->palette.vbank1;
    if(!fromClipboard(&getBankPalette(sprite->studio, ovr)->colors[sprite->color], sizeof(uli_rgb), false, true, false))
        fromClipboard(getBankPalette(sprite->studio, ovr)->data, sizeof(uli_palette), false, true, false);
}

static void drawRGBTools(Sprite* sprite, s32 x, s32 y)
{
    {
        enum{Size = 5};

        uli_rect rect = {x, y, Size, Size};

        bool over = false;
        bool down = false;

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            showTooltip(sprite->studio, "COPY PALETTE");
            over = true;

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
                toClipboard(getBankPalette(sprite->studio, sprite->palette.vbank1)->data, sizeof(uli_palette), false);
        }

        if(down)
        {
            drawBitIcon(sprite->studio, uli_icon_copy, rect.x-1, rect.y, uli_color_light_grey);
        }
        else
        {
            drawBitIcon(sprite->studio, uli_icon_copy, rect.x-1, rect.y, uli_color_black);
            drawBitIcon(sprite->studio, uli_icon_copy, rect.x-1, rect.y-1, (over ? uli_color_light_grey : uli_color_white));
        }
    }

    {
        enum{Size = 5};

        uli_rect rect = {x, y + 8, Size, Size};
        bool over = false;
        bool down = false;

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            showTooltip(sprite->studio, "PASTE PALETTE");
            over = true;

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                pasteColor(sprite);
            }
        }

        if(down)
        {
            drawBitIcon(sprite->studio, uli_icon_paste, rect.x-1, rect.y, uli_color_light_grey);
        }
        else
        {
            drawBitIcon(sprite->studio, uli_icon_paste, rect.x-1, rect.y, uli_color_black);
            drawBitIcon(sprite->studio, uli_icon_paste, rect.x-1, rect.y-1, (over ? uli_color_light_grey : uli_color_white));
        }
    }
}

static void drawRGBSliders(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    enum
    {
        Cols = BITS_IN_BYTE / ULI_PALETTE_BPP,
        Rows = sizeof(uli_rgb),
        Width = ULI_FONT_WIDTH + 1,
        Height = ULI_FONT_HEIGHT + 1
    };

    u8* data = &getBankPalette(sprite->studio, sprite->palette.vbank1)->data[sprite->color * Rows];

    {
        uli_rect rect = {x - 20, y - 3, ULI_FONT_WIDTH * Cols + 1, ULI_FONT_HEIGHT * Rows + 1};

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
            {
                s32 mx = uli_api_mouse(uli).x - rect.x;
                s32 my = uli_api_mouse(uli).y - rect.y;

                sprite->palette.focus = mx / Width + my / Height * Cols;
            }
        }
        else if(checkMouseDown(sprite->studio, &(uli_rect){0, 0, ULI78_WIDTH, ULI78_HEIGHT}, uli_mouse_left))
        {
            sprite->palette.focus = -1;
        }

        bool hasFocus = sprite->palette.focus >= 0;
        if(hasFocus)
        {
            drawPanelBorder(sprite->uli, rect.x, rect.y, rect.w, rect.h);
            uli_api_rect(sprite->uli, rect.x, rect.y, rect.w, rect.h, uli_color_black);
        }

        for(s32 i = 0; i < Rows; i++)
        {
            char buf[sizeof "FF"];
            sprintf(buf, "%02X", data[i]);
            uli_api_print(uli, buf, rect.x + 1, rect.y + i * ULI_FONT_HEIGHT + 1,
                hasFocus ? uli_color_grey : uli_color_light_grey, true, 1, false);
        }

        if(hasFocus)
        {
            s32 col = sprite->palette.focus % Cols;
            s32 row = sprite->palette.focus / Cols;
            s32 x = rect.x + col * ULI_FONT_WIDTH;
            s32 y = rect.y + row * ULI_FONT_HEIGHT;
            uli_api_rect(sprite->uli, x, y, Width, Height, uli_color_red);

            {
                char buf[sizeof "FF"];
                sprintf(buf, "%02X", data[row]);
                uli_api_print(uli, (char[]){buf[col], '\0'}, x + 1, y + 1, uli_color_black, true, 1, false);
            }
        }
    }

    for(s32 i = 0; i < Rows; i++)
        drawRGBSlider(sprite, x, y + ULI_FONT_HEIGHT * i, &data[i]);

    drawRGBTools(sprite, x + 74, y);
}

static uli_palette_dimensions getPaletteDimensions(Sprite* sprite)
{

    uli_bpp bpp = sprite->blit.mode;

    s32 cols = bpp == uli_bpp_4 ? PALETTE_COLS : bpp == uli_bpp_2 ? 4 : 2;
    s32 rows = bpp == uli_bpp_4 ? PALETTE_ROWS : 1;

    s32 cell_w = (PALETTE_COLS / cols) * PALETTE_CELL_SIZE;
    s32 cell_h = (PALETTE_ROWS / rows) * PALETTE_CELL_SIZE;

    return (uli_palette_dimensions){cell_w, cell_h, cols, rows, cols*rows};
}

static void drawPaletteVBank1(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;
    uli_rect rect = {x, y, PALETTE_WIDTH-1, PALETTE_HEIGHT-1};
    uli_palette_dimensions palette = getPaletteDimensions(sprite);

    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);

        s32 mx = uli_api_mouse(uli).x - x;
        s32 my = uli_api_mouse(uli).y - y;

        mx /= palette.cell_w;
        my /= palette.cell_h;

        s32 index = mx + my * palette.cols;

        SHOW_TOOLTIP(sprite->studio, "color [%02i]", index);

        bool left = checkMouseDown(sprite->studio, &rect, uli_mouse_left);
        bool right = checkMouseDown(sprite->studio, &rect, uli_mouse_right);

        if(left || right)
        {
            if(left) sprite->color = index;
            if(right) sprite->color2 = index;
        }
    }

    enum {Gap = 1};

    drawPanelBorder(uli, x - Gap, y - Gap, PaletteW + Gap, PaletteH + Gap);

    for(s32 row = 0, i = 0; row < palette.rows; row++)
        for(s32 col = 0; col < palette.cols; col++)
        {
            uli_api_rectb(uli, x + col * palette.cell_w - Gap, y + row * palette.cell_h - Gap,
                palette.cell_w + Gap, palette.cell_h + Gap, uli_color_black);
        }

    {
        s32 offsetX = x + (sprite->color % PALETTE_COLS) * palette.cell_w;
        s32 offsetY = y + (sprite->color / PALETTE_COLS) * palette.cell_h;
        uli_api_rectb(uli, offsetX - 1, offsetY - 1, palette.cell_w + 1, palette.cell_h + 1, uli_color_white);
    }

    {
        s32 offsetX = x + (sprite->color2 % PALETTE_COLS) * palette.cell_w;
        s32 offsetY = y + (sprite->color2 / PALETTE_COLS) * palette.cell_h;

        for(u8 i=0; i<palette.cell_w+1;i+=2) {
            uli_api_pix(uli, offsetX+i-1, offsetY-1, uli_color_white, false);
            uli_api_pix(uli, offsetX+i-1, offsetY + palette.cell_h-1, uli_color_white, false);
        }

        for(u8 i=0; i<palette.cell_h+1;i+=2) {
            uli_api_pix(uli, offsetX-1, offsetY+i-1, uli_color_white, false);
            uli_api_pix(uli, offsetX+palette.cell_w-1, offsetY + i-1, uli_color_white, false);
        }
    }

    if(sprite->advanced)
    {
        uli_rect rect = {x - 22, y + 1, 19, 5};

        bool down = false;
        bool over = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);
            over = true;

            showTooltip(sprite->studio, "VBANK0 PALETTE");

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
                sprite->palette.vbank1 = false;
        }

        {
            static const char* Label = "bank0";
            if(!sprite->palette.vbank1)
                uli_api_print(uli, Label, rect.x, rect.y + 1, uli_color_black, false, 1, true);

            uli_api_print(uli, Label, rect.x, rect.y, sprite->palette.vbank1 ? uli_color_dark_grey : uli_color_white, false, 1, true);
        }
    }

    if(sprite->advanced)
    {
        uli_rect rect = {x - 22, y + 9, 19, 5};

        bool down = false;
        bool over = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);
            over = true;

            showTooltip(sprite->studio, "VBANK1 PALETTE");

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
                sprite->palette.vbank1 = true;
        }

        {
            static const char* Label = "bank1";
            if(sprite->palette.vbank1)
                uli_api_print(uli, Label, rect.x, rect.y + 1, uli_color_black, false, 1, true);

            uli_api_print(uli, Label, rect.x, rect.y, sprite->palette.vbank1 ? uli_color_white : uli_color_dark_grey, false, 1, true);
        }
    }

    if(sprite->advanced)
    {
        uli_rect rect = {x + PALETTE_WIDTH + 3, y + (PALETTE_HEIGHT-8)/2-1, 8, 8};

        bool down = false;
        bool over = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);
            over = true;

            showTooltip(sprite->studio, "EDIT PALETTE");

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
                down = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                sprite->palette.edit = !sprite->palette.edit;

                if(!sprite->palette.edit)
                    sprite->palette.focus = -1;
            }
        }

        if(sprite->palette.edit || down)
        {
            drawBitIcon(sprite->studio, uli_icon_rgb, rect.x, rect.y+1, (over ? uli_color_light_grey : uli_color_white));
        }
        else
        {
            drawBitIcon(sprite->studio, uli_icon_rgb, rect.x, rect.y+1, uli_color_black);
            drawBitIcon(sprite->studio, uli_icon_rgb, rect.x, rect.y, (over ? uli_color_light_grey : uli_color_white));
        }
    }
}

static void drawPalette(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;
    uli_palette_dimensions palette = getPaletteDimensions(sprite);

    for(s32 row = 0, i = 0; row < palette.rows; row++)
        for(s32 col = 0; col < palette.cols; col++)
            uli_api_rect(uli, x + col * palette.cell_w, y + row * palette.cell_h, palette.cell_w-1, palette.cell_h-1, i++);
}

static void selectSprite(Sprite* sprite, s32 x, s32 y)
{
    {
        s32 size = ULI_SPRITESHEET_SIZE - sprite->size;
        if(x < 0) x = 0;
        if(y < 0) y = 0;
        if(x > size) x = size;
        if(y > size) y = size;
    }
    sprite->x = x / ULI_SPRITESIZE;
    sprite->y = y / ULI_SPRITESIZE;
    updateIndex(sprite);
}

static void updateSpriteSize(Sprite* sprite, s32 size)
{
    if(size != sprite->size)
    {
        sprite->size = size;
        selectSprite(sprite, sprite->x*ULI_SPRITESIZE, sprite->y*ULI_SPRITESIZE);
    }
}

static void drawSheetVBank1(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    uli_rect rect = {x, y, ULI_SPRITESHEET_SIZE, ULI_SPRITESHEET_SIZE};

    uli_api_rectb(uli, rect.x - 1, rect.y - 1, rect.w + 2, rect.h + 2, uli_color_white);

    for(s32 i = 1; i < rect.h; i += 4)
    {
        if (sprite->blit.page > 0)
        {
            uli_api_pix(uli, rect.x-1, rect.y + i, uli_color_black, false);
            uli_api_pix(uli, rect.x-1, rect.y + i + 1, uli_color_black, false);
        }

        if (sprite->blit.page < sprite->blit.pages - 1)
        {
            uli_api_pix(uli, rect.x+rect.w, rect.y + i, uli_color_black, false);
            uli_api_pix(uli, rect.x+rect.w, rect.y + i + 1, uli_color_black, false);
        }
    }

    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);

        if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
        {
            s32 offset = (sprite->size - ULI_SPRITESIZE) / 2;
            selectSprite(sprite, uli_api_mouse(uli).x - x - offset, uli_api_mouse(uli).y - y - offset);
        }
    }

    s32 bx = sprite->x*ULI_SPRITESIZE + x - 1;
    s32 by = sprite->y*ULI_SPRITESIZE + y - 1;

    uli_api_rectb(uli, bx, by, sprite->size + 2, sprite->size + 2, uli_color_white);
}

static void drawSheet(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;
    tiles2ram(uli->ram, sprite->src);

    uli_blit blit = sprite->blit;
    SCOPE(uli->ram->vram.blit.segment = ULI_DEFAULT_BLIT_MODE)
    {
        uli_point start =
        {
            x - blit.page * ULI_SPRITESHEET_SIZE + sprite->anim.pos.page,
            y - blit.bank * ULI_SPRITESHEET_SIZE + sprite->anim.pos.bank
        }, pos = start;

        for(blit.bank = 0; blit.bank < ULI_SPRITE_BANKS; ++blit.bank, pos.y += ULI_SPRITESHEET_SIZE, pos.x = start.x)
        {
            for(blit.page = 0; blit.page < blit.pages; ++blit.page, pos.x += ULI_SPRITESHEET_SIZE)
            {
                uli->ram->vram.blit.segment = uli_blit_calc_segment(&blit);
                uli_api_spr(uli, 0, pos.x, pos.y, ULI_SPRITESHEET_COLS, ULI_SPRITESHEET_COLS, NULL, 0, 1, uli_no_flip, uli_no_rotate);
            }
        }
    }
}

static void flipSpriteHorz(Sprite* sprite)
{
    uli_rect rect = getSpriteRect(sprite);
    s32 r = rect.x + rect.w/2;
    s32 b = rect.y + rect.h;

    for(s32 y = rect.y; y < b; y++)
        for(s32 x = rect.x, i = rect.x + rect.w - 1; x < r; x++, i--)
        {
            u8 color = uli_tilesheet_getpix(&sprite->sheet, x, y);
            uli_tilesheet_setpix(&sprite->sheet, x, y, uli_tilesheet_getpix(&sprite->sheet, i, y));
            uli_tilesheet_setpix(&sprite->sheet, i, y, color);
        }

    history_add(sprite->history);
}

static void flipSpriteVert(Sprite* sprite)
{
    uli_rect rect = getSpriteRect(sprite);
    s32 r = rect.x + rect.w;
    s32 b = rect.y + rect.h/2;

    for(s32 y = rect.y, i = rect.y + rect.h - 1; y < b; y++, i--)
        for(s32 x = rect.x; x < r; x++)
        {
            u8 color = uli_tilesheet_getpix(&sprite->sheet, x, y);
            uli_tilesheet_setpix(&sprite->sheet, x, y, uli_tilesheet_getpix(&sprite->sheet, x, i));
            uli_tilesheet_setpix(&sprite->sheet, x, i, color);
        }

    history_add(sprite->history);
}

static void rotateSprite(Sprite* sprite)
{
    const s32 Size = sprite->size;
    u8* buffer = (u8*)malloc(Size * Size);

    if(buffer)
    {
        {
            uli_rect rect = getSpriteRect(sprite);
            s32 r = rect.x + rect.w;
            s32 b = rect.y + rect.h;

            for(s32 y = rect.y, i = 0; y < b; y++)
                for(s32 x = rect.x; x < r; x++)
                    buffer[i++] = uli_tilesheet_getpix(&sprite->sheet, x, y);

            for(s32 y = rect.y, j = 0; y < b; y++, j++)
                for(s32 x = rect.x, i = 0; x < r; x++, i++)
                    uli_tilesheet_setpix(&sprite->sheet, x, y, buffer[j + (Size-i-1)*Size]);

            history_add(sprite->history);
        }

        free(buffer);
    }
}

static inline bool is4bpp(Sprite* sprite)
{
    return sprite->blit.mode == 4;
}

static void deleteSprite(Sprite* sprite)
{
    uli_rect rect = getSpriteRect(sprite);
    s32 r = rect.x + rect.w;
    s32 b = rect.y + rect.h;

    for(s32 y = rect.y; y < b; y++)
        for(s32 x = rect.x; x < r; x++)
            uli_tilesheet_setpix(&sprite->sheet, x, y, sprite->color2);

    if(is4bpp(sprite))
    {
        u8* flags = getBankFlags(sprite->studio)->data;
        for(const s32* it = getSpriteIndexes(sprite); *it >= 0; ++it)
            flags[*it] = 0;
    }

    clearCanvasSelection(sprite);

    history_add(sprite->history);
}

static void(* const SpriteToolsFunc[])(Sprite*) = {flipSpriteHorz, flipSpriteVert, rotateSprite, deleteSprite};
static void(* const CanvasToolsFunc[])(Sprite*) = {flipCanvasHorz, flipCanvasVert, rotateCanvas, deleteCanvas};

static void drawSpriteTools(Sprite* sprite, s32 x, s32 y)
{
    static const u8 Icons[] = {uli_icon_fliphorz, uli_icon_flipvert, uli_icon_rotate, uli_icon_erase};
    static const char* Tooltips[] = {"FLIP HORZ [5]", "FLIP VERT [6]", "ROTATE [7]", "ERASE [8]"};

    enum{Gap = ULI_SPRITESIZE + 3};

    for(s32 i = 0; i < COUNT_OF(Icons); i++)
    {
        bool pushed = false;
        bool over = false;

        uli_rect rect = {x + i * Gap, y, ULI_SPRITESIZE, ULI_SPRITESIZE};

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            over = true;

            showTooltip(sprite->studio, Tooltips[i]);

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left)) pushed = true;

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                if(hasCanvasSelection(sprite))
                {
                    CanvasToolsFunc[i](sprite);
                }
                else
                {
                    SpriteToolsFunc[i](sprite);
                    clearCanvasSelection(sprite);
                }
            }
        }

        if(pushed)
        {
            drawBitIcon(sprite->studio, Icons[i], rect.x, y + 1, (over ? uli_color_light_grey : uli_color_white));
        }
        else
        {
            drawBitIcon(sprite->studio, Icons[i], rect.x, y+1, uli_color_black);
            drawBitIcon(sprite->studio, Icons[i], rect.x, y, (over ? uli_color_light_grey : uli_color_white));
        }
    }
}

static void drawTools(Sprite* sprite, s32 x, s32 y)
{
    enum{Gap = ULI_SPRITESIZE + 3};

    static const u8 Icons[] = {uli_icon_bigpen, uli_icon_bigpicker, uli_icon_bigselect, uli_icon_bigfill};

    for(s32 i = 0; i < COUNT_OF(Icons); i++)
    {
        uli_rect rect = {x + i * Gap, y, ULI_SPRITESIZE, ULI_SPRITESIZE};

        bool over = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);
            over = true;

            static const char* Tooltips[] = {"BRUSH [1]", "COLOR PICKER [2]", "SELECT [3]", "FILL [4]"};

            showTooltip(sprite->studio, Tooltips[i]);

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                sprite->mode = i;

                clearCanvasSelection(sprite);
            }
        }

        bool pushed = i == sprite->mode;

        if(pushed)
        {
            drawBitIcon(sprite->studio, uli_icon_down, rect.x, y - 5, uli_color_black);
            drawBitIcon(sprite->studio, uli_icon_down, rect.x, y - 6, uli_color_white);

            drawBitIcon(sprite->studio, Icons[i], rect.x, y + 1, (over ? uli_color_light_grey : uli_color_white));
        }
        else
        {
            drawBitIcon(sprite->studio, Icons[i], rect.x, y+1, uli_color_black);
            drawBitIcon(sprite->studio, Icons[i], rect.x, y, (over ? uli_color_light_grey : uli_color_white));
        }
    }

    drawSpriteTools(sprite, x + COUNT_OF(Icons) * Gap + 1, y);
}

static inline s32 getClipboardSpritesSize(Sprite* sprite)
{
    return sprite->size * sprite->size * ULI_PALETTE_BPP / BITS_IN_BYTE;
}

static inline s32 getClipboardFlagsSize(Sprite* sprite)
{
    return is4bpp(sprite) ? sprite->size * sprite->size / (ULI_SPRITESIZE * ULI_SPRITESIZE) : 0;
}

static inline s32 getClipboardSize(Sprite* sprite)
{
    return getClipboardSpritesSize(sprite) + getClipboardFlagsSize(sprite);
}

static void copyToClipboard(Sprite* sprite)
{
    s32 size = getClipboardSize(sprite);

    u8* buffer = malloc(size);
    SCOPE(free(buffer))
    {
        uli_rect rect = getSpriteRect(sprite);
        s32 r = rect.x + rect.w;
        s32 b = rect.y + rect.h;

        for(s32 y = rect.y, i = 0; y < b; y++)
            for(s32 x = rect.x; x < r; x++)
                uli_tool_poke4(buffer, i++, uli_tilesheet_getpix(&sprite->sheet, x, y) & 0xf);

        if(is4bpp(sprite))
        {
            u8* ptr = buffer + getClipboardSpritesSize(sprite);

            const u8* flags = getBankFlags(sprite->studio)->data;
            for(const s32* it = getSpriteIndexes(sprite); *it >= 0; ++it)
                *ptr++ = flags[*it];
        }

        toClipboard(buffer, size, true);
    }
}

static void cutToClipboard(Sprite* sprite)
{
    copyToClipboard(sprite);
    deleteSprite(sprite);
}

static void copyFromClipboard(Sprite* sprite)
{
    s32 size = getClipboardSize(sprite);

    u8* buffer = malloc(size);
    SCOPE(free(buffer))
    {
        if(fromClipboard(buffer, size, true, false, true))
        {
            uli_rect rect = getSpriteRect(sprite);
            s32 r = rect.x + rect.w;
            s32 b = rect.y + rect.h;

            for(s32 y = rect.y, i = 0; y < b; y++)
                for(s32 x = rect.x; x < r; x++)
                    uli_tilesheet_setpix(&sprite->sheet, x, y, uli_tool_peek4(buffer, i++));

            if(is4bpp(sprite))
            {
                const u8* ptr = buffer + getClipboardSpritesSize(sprite);
                u8* flags = getBankFlags(sprite->studio)->data;

                for(const s32* it = getSpriteIndexes(sprite); *it >= 0; ++it)
                    flags[*it] = *ptr++;
            }

            history_add(sprite->history);
        }
    }
}

static void upSprite(Sprite* sprite)
{
    if (sprite->y > 0) sprite->y--;
    updateIndex(sprite);
}

static void downSprite(Sprite* sprite)
{
    if ((sprite->y + sprite->size/ULI_SPRITESIZE) < ULI_SPRITESHEET_COLS) sprite->y++;
    updateIndex(sprite);
}

static void leftSprite(Sprite* sprite)
{
    if (sprite->x > 0) sprite->x--;
    updateIndex(sprite);
}

static void rightSprite(Sprite* sprite)
{
    if ((sprite->x + sprite->size/ULI_SPRITESIZE) < ULI_SPRITESHEET_COLS) sprite->x++;
    updateIndex(sprite);
}

static void undo(Sprite* sprite)
{
    history_undo(sprite->history);
}

static void redo(Sprite* sprite)
{
    history_redo(sprite->history);
}

static void switchBanks(Sprite* sprite)
{
    if(isIdle(sprite))
    {
        s32 bank = (sprite->blit.bank + 1) % ULI_SPRITE_BANKS;
        Anim* anim = sprite->anim.bank.items;
        anim->start = (bank - sprite->blit.bank) * ULI_SPRITESHEET_SIZE;
        sprite->anim.movie = resetMovie(&sprite->anim.bank);

        sprite->blit.bank = bank;

        updateIndex(sprite);
        initTileSheet(sprite);
    }
}

static void drawTab(Sprite* sprite, s32 x, s32 y, s32 w, s32 h, u8 icon, bool active, bool over)
{
    uli_color tab_color = active ? uli_color_white : over ? uli_color_light_grey : uli_color_dark_grey;
    uli_color label_color = active ? uli_color_dark_grey : uli_color_grey;

    uli_api_rect(sprite->uli, x+1, y, w-1, h, tab_color);
    uli_api_line(sprite->uli, x, y+1, x, y+h-2, tab_color);

    if (active)
    {
        uli_api_line(sprite->uli, x+1, y + h, x + w-1, y + h, uli_color_black);
        uli_api_pix(sprite->uli, x, y-1 + h, label_color, false);
    }

    drawBitIcon(sprite->studio, icon, x + 1, y, label_color);
}

static void drawBankTabs(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    if(hasCanvasSelection(sprite)) return;

    enum {Banks = 2, SizeY = 7, SizeX = 9};

    static const u8 Icons[] = {uli_icon_tiles, uli_icon_sprites};
    static const char* tooltips[] = {"TILES [tab]", "SPRITES [tab]"};

    for(s32 i = 0; i < Banks; i++)
    {
        bool current = i == sprite->blit.bank;

        uli_rect rect = {x - SizeX, y + (SizeY + 1) * i, SizeX, SizeY};

        bool over = false;
        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);
            over = true;

            showTooltip(sprite->studio, tooltips[i]);

            if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            {
                if (!current)
                {
                    switchBanks(sprite);
                }
            }
        }

        drawTab(sprite, rect.x, rect.y, SizeX, SizeY, Icons[i], current, over);
    }
}

static void updateBrushSize(Sprite* sprite, s32 val)
{
    sprite->brushSize = (sprite->brushSize + val + (BRUSH_SIZES - 1)) % BRUSH_SIZES + 1;
}

static void updateColorIndex(Sprite* sprite, s32 val)
{
    s32 colors = 1 << sprite->blit.mode;
    sprite->color = (sprite->color + val + colors) % colors;
}

static void processKeyboard(Sprite* sprite)
{
    uli_mem* uli = sprite->uli;

    switch(getClipboardEvent(sprite->studio))
    {
    case ULI_CLIPBOARD_CUT: cutToClipboard(sprite); break;
    case ULI_CLIPBOARD_COPY: copyToClipboard(sprite); break;
    case ULI_CLIPBOARD_PASTE: copyFromClipboard(sprite); break;
    default: break;
    }

    if(uli_api_key(uli, uli_key_alt))
        return;

    if(sprite->palette.edit && sprite->palette.focus >= 0)
    {
        enum{Cols = BITS_IN_BYTE / ULI_PALETTE_BPP, Rows = sizeof(uli_rgb)};
        s32 col = sprite->palette.focus % Cols;
        s32 row = sprite->palette.focus / Cols;

        if(keyWasPressed(sprite->studio, uli_key_up))           --row;
        else if(keyWasPressed(sprite->studio, uli_key_down))    ++row;
        else if(keyWasPressed(sprite->studio, uli_key_left))    --col;
        else if(keyWasPressed(sprite->studio, uli_key_right))   ++col;
        else
        {
            char sym = getKeyboardText(sprite->studio);

            if(isxdigit(sym))
            {
                u8* data = &getBankPalette(sprite->studio, sprite->palette.vbank1)->data[sprite->color * Rows + row];
                char buf[sizeof "FF"];
                sprintf(buf, "%02X", *data);
                buf[col] = toupper(sym);
                *data = (u8)strtol(buf, NULL, 16);
                ++col;
            }
        }

        sprite->palette.focus = (col + row * Cols + Cols * Rows) % (Cols * Rows);
    }
    else if(sprite->flags.edit)
    {
        if(keyWasPressed(sprite->studio, uli_key_left))
        {
            if(sprite->flags.pos)
                sprite->flags.pos--;
        }
        else if(keyWasPressed(sprite->studio, uli_key_right))
        {
            if(!sprite->flags.pos)
                sprite->flags.pos++;
        }
        else
        {
            char sym = getKeyboardText(sprite->studio);

            if(isxdigit(sym))
            {
                u8 mask = 0xff;

                u8* flags = getBankFlags(sprite->studio)->data;
                const s32* indexes = getSpriteIndexes(sprite);

                for(const s32* i = indexes; *i; i++)
                    mask &= flags[*i];

                char buf[sizeof "FF"];
                sprintf(buf, "%02X", mask);
                buf[sprite->flags.pos] = toupper(sym);
                u8 value = strtol(buf, NULL, 16);

                for(const s32* i = indexes; *i; i++)
                    flags[*i] = value;

                if(!sprite->flags.pos)
                    sprite->flags.pos++;
            }
        }
    }
    else
    {
        bool ctrl = uli_api_key(uli, uli_key_ctrl);

        if(ctrl)
        {
            if(keyWasPressed(sprite->studio, uli_key_z))        undo(sprite);
            else if(keyWasPressed(sprite->studio, uli_key_y))   redo(sprite);

            else if(keyWasPressed(sprite->studio, uli_key_left))    leftViewport(sprite);
            else if(keyWasPressed(sprite->studio, uli_key_right))   rightViewport(sprite);

            else if(keyWasPressed(sprite->studio, uli_key_tab))
                switchBitMode(sprite, sprite->blit.mode == uli_bpp_4
                    ? uli_bpp_2
                    : sprite->blit.mode == uli_bpp_2
                        ? uli_bpp_1
                        : uli_bpp_4);
        }
        else
        {
            if(hasCanvasSelection(sprite))
            {
                if(!sprite->select.drag)
                {
                    if(keyWasPressed(sprite->studio, uli_key_up))           upCanvas(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_down))    downCanvas(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_left))    leftCanvas(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_right))   rightCanvas(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_delete))  deleteCanvas(sprite);
                    // To be able to change tool after selection
                    else if(keyWasPressed(sprite->studio, uli_key_1))       sprite->mode = SPRITE_DRAW_MODE;
                    else if(keyWasPressed(sprite->studio, uli_key_2))       sprite->mode = SPRITE_PICK_MODE;
                    else if(keyWasPressed(sprite->studio, uli_key_3))       sprite->mode = SPRITE_SELECT_MODE;
                    else if(keyWasPressed(sprite->studio, uli_key_4))       sprite->mode = SPRITE_FILL_MODE;
                    else if(keyWasPressed(sprite->studio, uli_key_5))       flipCanvasHorz(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_6))       flipCanvasVert(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_7))       rotateCanvas(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_8))       deleteCanvas(sprite);
                }
            }
            else
            {
                if(keyWasPressed(sprite->studio, uli_key_up))           upSprite(sprite);
                else if(keyWasPressed(sprite->studio, uli_key_down))    downSprite(sprite);
                else if(keyWasPressed(sprite->studio, uli_key_left))    leftSprite(sprite);
                else if(keyWasPressed(sprite->studio, uli_key_right))   rightSprite(sprite);
                else if(keyWasPressed(sprite->studio, uli_key_delete))  deleteSprite(sprite);
                else if(keyWasPressed(sprite->studio, uli_key_tab))     switchBanks(sprite);

                if(!sprite->palette.edit)
                {

                    if(keyWasPressed(sprite->studio, uli_key_1))        sprite->mode = SPRITE_DRAW_MODE;
                    else if(keyWasPressed(sprite->studio, uli_key_2))   sprite->mode = SPRITE_PICK_MODE;
                    else if(keyWasPressed(sprite->studio, uli_key_3))   sprite->mode = SPRITE_SELECT_MODE;
                    else if(keyWasPressed(sprite->studio, uli_key_4))   sprite->mode = SPRITE_FILL_MODE;

                    else if(keyWasPressed(sprite->studio, uli_key_5))   flipSpriteHorz(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_6))   flipSpriteVert(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_7))   rotateSprite(sprite);
                    else if(keyWasPressed(sprite->studio, uli_key_8))   deleteSprite(sprite);

                    if(sprite->mode == SPRITE_DRAW_MODE)
                    {
                        if(keyWasPressed(sprite->studio, uli_key_minus))                updateBrushSize(sprite, -1);
                        else if(keyWasPressed(sprite->studio, uli_key_equals))          updateBrushSize(sprite, +1);
                        else if(keyWasPressed(sprite->studio, uli_key_leftbracket))     updateColorIndex(sprite, -1);
                        else if(keyWasPressed(sprite->studio, uli_key_rightbracket))    updateColorIndex(sprite, +1);
                    }
                }
            }
        }
    }
}


static void drawSpriteToolbar(Sprite* sprite)
{
    uli_mem* uli = sprite->uli;

    uli_api_rect(uli, 0, 0, ULI78_WIDTH, TOOLBAR_SIZE, uli_color_white);

    // draw sprite size control
    {
        uli_rect rect = {ULI78_WIDTH - 58, 1, 23, 5};

        if(checkMousePos(sprite->studio, &rect))
        {
            setCursor(sprite->studio, uli_cursor_hand);

            showTooltip(sprite->studio, "CANVAS ZOOM");

            if(checkMouseDown(sprite->studio, &rect, uli_mouse_left))
            {
                s32 mx = uli_api_mouse(uli).x - rect.x;
                mx /= 6;

                s32 size = 1;
                while(mx--) size <<= 1;

                updateSpriteSize(sprite, size * ULI_SPRITESIZE);
            }
        }

        for(s32 i = 0; i < 4; i++)
            uli_api_rect(uli, rect.x + i*6, 1, 5, 5, uli_color_black);

        uli_api_rect(uli, rect.x, 2, 23, 3, uli_color_black);
        uli_api_rect(uli, rect.x+1, 3, 21, 1, uli_color_white);

        s32 size = sprite->size / ULI_SPRITESIZE, val = 0;
        while(size >>= 1) val++;

        uli_api_rect(uli, rect.x + val*6, 1, 5, 5, uli_color_black);
        uli_api_rect(uli, rect.x+1 + val*6, 2, 3, 3, uli_color_white);
    }

    {
        u8 nbPages = sprite->blit.pages;

        if (nbPages > 1) {
            enum {SizeX = 7, SizeY = TOOLBAR_SIZE};

            for(s32 page = 0; page < nbPages; page++)
            {
                bool active = page == sprite->blit.page;

                uli_rect rect = {ULI78_WIDTH - 1 - 7*(nbPages-page), 0, 7, TOOLBAR_SIZE};

                bool over = false;
                if(checkMousePos(sprite->studio, &rect))
                {
                    setCursor(sprite->studio, uli_cursor_hand);
                    over = true;

                    SHOW_TOOLTIP(sprite->studio, "PAGE %i", page + 1);

                    if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
                    {
                        selectViewportPage(sprite, page);
                    }
                }

                if (active) uli_api_rect(uli, rect.x, rect.y, rect.w, rect.h, uli_color_black);
                uli_api_print(uli, (char[]){'1' + page, '\0'}, rect.x + 2, rect.y + 1, active ? uli_color_white : uli_color_grey, false, 1, true);
            }
        }
    }
}

static void scanline(uli_mem* uli, s32 row, void* data)
{
    Sprite* sprite = (Sprite*)data;

    if(row == 0)
        memcpy(&uli->ram->vram.palette, getBankPalette(sprite->studio, sprite->palette.vbank1), sizeof(uli_palette));
}

static void drawAdvancedButton(Sprite* sprite, s32 x, s32 y)
{
    uli_mem* uli = sprite->uli;

    uli_rect rect = {x, y, 8, 5};

    bool over = false;
    if(checkMousePos(sprite->studio, &rect))
    {
        setCursor(sprite->studio, uli_cursor_hand);
        over = true;
        showTooltip(sprite->studio, "ADVANCED MODE");

        if(checkMouseClick(sprite->studio, &rect, uli_mouse_left))
            sprite->advanced = !sprite->advanced;

        if(!sprite->advanced)
        {
            sprite->palette.edit = false;
            sprite->palette.focus = -1;
        }
    }

    enum {Size = 3, Gap = 1};

    uli_api_rect(uli, rect.x, rect.y, rect.w, rect.h, uli_color_black);
    uli_api_rect(uli, rect.x + Gap + (sprite->advanced ? Size : 0), rect.y + Gap, Size, Size, over ? uli_color_light_grey : uli_color_grey);
}

static void tick(Sprite* sprite)
{
    uli_mem* uli = sprite->uli;

    processAnim(sprite->anim.movie, sprite);

    // process scroll
    {
        uli78_input* input = &uli->ram->input;

        if(input->mouse.scrolly)
        {
            s32 size = sprite->size;
            s32 delta = input->mouse.scrolly;

            if(delta > 0)
            {
                if(size < (ULI_SPRITESIZE * ULI_SPRITESIZE)) size <<= 1;
            }
            else if(size > ULI_SPRITESIZE) size >>= 1;

            updateSpriteSize(sprite, size);
        }
    }

    processKeyboard(sprite);

    drawSheet(sprite, SheetX, SheetY);
    drawCanvas(sprite, CanvasX, CanvasY);
    drawPalette(sprite, PaletteX, PaletteY);

    VBANK(uli, 1)
    {
        uli_api_cls(uli, uli->ram->vram.vars.clear = uli_color_cyan);

        static const uli_rect bg[] =
        {
            {0, ToolbarH, SheetX, CanvasY-ToolbarH},
            {0, CanvasY, CanvasX, CanvasH},
            {CanvasX + CanvasW, CanvasY, SheetX - (CanvasX + CanvasW), CanvasH},

            {0, CanvasY + CanvasH, SheetX, PaletteY - CanvasY - CanvasH},

            {0, PaletteY, PaletteX, PaletteH},
            {PaletteX + PaletteW, PaletteY, SheetX - PaletteX - PaletteW, PaletteH},

            {0, PaletteY + PaletteH, SheetX, ULI78_HEIGHT - PaletteY - PaletteH},
        };

        memcpy(uli->ram->vram.palette.data, getConfig(sprite->studio)->cart->bank0.palette.vbank0.data, sizeof(uli_palette));

        for(const uli_rect* r = bg; r < bg + COUNT_OF(bg); r++)
            uli_api_rect(uli, r->x, r->y, r->w, r->h, uli_color_grey);

        drawCanvasVBank1(sprite, 24, 20);
        drawMoveButtons(sprite);

        if(sprite->advanced)
        {
            if(is4bpp(sprite))
                drawFlags(sprite, 24+64+7, 20+8);

            drawBitMode(sprite, PaletteX, PaletteY + PaletteH + 2, PaletteW, 8);
        }

        drawBankTabs(sprite, SheetX, 8);

        sprite->palette.edit
            ? drawRGBSliders(sprite, 24, 91)
            : drawTools(sprite, 12, 96);

        drawPaletteVBank1(sprite, 24, 112);
        drawSheetVBank1(sprite, ULI78_WIDTH - ULI_SPRITESHEET_SIZE - 1, 7);
        drawAdvancedButton(sprite, 4, 11);

        drawSpriteToolbar(sprite);
        drawToolbar(sprite->studio, uli, false);
    }

    sprite->tickCounter++;
}

static void onStudioEvent(Sprite* sprite, StudioEvent event)
{
    switch(event)
    {
    case ULI_TOOLBAR_CUT: cutToClipboard(sprite); break;
    case ULI_TOOLBAR_COPY: copyToClipboard(sprite); break;
    case ULI_TOOLBAR_PASTE: copyFromClipboard(sprite); break;
    case ULI_TOOLBAR_UNDO: undo(sprite); break;
    case ULI_TOOLBAR_REDO: redo(sprite); break;
    }
}

static void emptyDone(void* data) {}

static void setIdle(void* data)
{
    Sprite* sprite = data;
    sprite->anim.movie = resetMovie(&sprite->anim.idle);
}

static void freeAnim(Sprite* sprite)
{
    FREE(sprite->anim.bank.items);
    FREE(sprite->anim.page.items);
}

void initSprite(Sprite* sprite, Studio* studio, uli_tiles* src)
{
    if(sprite->select.back == NULL) sprite->select.back = (u8*)malloc(CANVAS_SIZE*CANVAS_SIZE);
    if(sprite->select.front == NULL) sprite->select.front = (u8*)malloc(CANVAS_SIZE*CANVAS_SIZE);
    if(sprite->history) history_delete(sprite->history);
    freeAnim(sprite);

    *sprite = (Sprite)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .tick = tick,
        .tickCounter = 0,
        .src = src,
        .x = 1,
        .y = 0,
        .advanced = false,
        .blit = {0},
        .color = 2,
        .color2 = 0,
        .size = ULI_SPRITESIZE,
        .palette =
        {
            .edit = false,
            .focus = -1,
        },
        .brushSize = 1,
        .select =
        {
            .rect = {0,0,0,0},
            .start = {0,0},
            .drag = false,
            .back = sprite->select.back,
            .front = sprite->select.front,
        },
        .mode = SPRITE_DRAW_MODE,
        .history = history_create(src, ULI_SPRITES * sizeof(uli_tile)),
        .anim =
        {
            .idle = {.done = emptyDone,},

            .bank = MOVIE_DEF(STUDIO_ANIM_TIME, setIdle,
            {
                {0, 0, STUDIO_ANIM_TIME, &sprite->anim.pos.bank, AnimEaseIn},
            }),

            .page = MOVIE_DEF(STUDIO_ANIM_TIME, setIdle,
            {
                {0, 0, STUDIO_ANIM_TIME, &sprite->anim.pos.page, AnimEaseIn},
            }),
        },
        .event = onStudioEvent,
        .scanline = scanline,
    };

    sprite->anim.movie = resetMovie(&sprite->anim.idle);

    switchBitMode(sprite, ULI_DEFAULT_BIT_DEPTH);
}

void freeSprite(Sprite* sprite)
{
    freeAnim(sprite);
    free(sprite->select.back);
    free(sprite->select.front);
    history_delete(sprite->history);
    free(sprite);
}
