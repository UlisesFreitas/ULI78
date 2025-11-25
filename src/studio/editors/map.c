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

#include "map.h"
#include "ext/history.h"

#define MAP_WIDTH (ULI78_WIDTH)
#define MAP_HEIGHT (ULI78_HEIGHT - TOOLBAR_SIZE)
#define MAP_X (0)
#define MAP_Y (TOOLBAR_SIZE)

#define MAX_SCROLL_X (ULI_MAP_WIDTH * ULI_SPRITESIZE)
#define MAX_SCROLL_Y (ULI_MAP_HEIGHT * ULI_SPRITESIZE)

#define ICON_SIZE 7

#define MIN_SCALE 1
#define MAX_SCALE 4
#define FILL_STACK_SIZE (ULI_MAP_WIDTH*ULI_MAP_HEIGHT)

static void normalizeMap(s32* x, s32* y)
{
    while(*x < 0) *x += MAX_SCROLL_X;
    while(*y < 0) *y += MAX_SCROLL_Y;
    while(*x >= MAX_SCROLL_X) *x -= MAX_SCROLL_X;
    while(*y >= MAX_SCROLL_Y) *y -= MAX_SCROLL_Y;
}

static uli_point getTileOffset(Map* map)
{
    return (uli_point){(map->sheet.rect.w - 1)*ULI_SPRITESIZE / 2, (map->sheet.rect.h - 1)*ULI_SPRITESIZE / 2};
}

static void getMouseMap(Map* map, s32* x, s32* y)
{
    uli_mem* uli = map->uli;
    uli_point offset = getTileOffset(map);

    s32 mx = uli_api_mouse(uli).x + map->scroll.x - offset.x;
    s32 my = uli_api_mouse(uli).y + map->scroll.y - offset.y;

    normalizeMap(&mx, &my);

    *x = mx / ULI_SPRITESIZE;
    *y = my / ULI_SPRITESIZE;
}

static s32 drawWorldButton(Map* map, s32 x, s32 y)
{
    enum{Size = 8};

    x -= Size;

    uli_rect rect = {x, y, Size, ICON_SIZE};

    bool over = false;

    if(checkMousePos(map->studio, &rect))
    {
        setCursor(map->studio, uli_cursor_hand);

        over = true;

        showTooltip(map->studio, "WORLD MAP [tab]");

        if(checkMouseClick(map->studio, &rect, uli_mouse_left))
            setStudioMode(map->studio, ULI_WORLD_MODE);
    }

    drawBitIcon(map->studio, uli_icon_world, x, y, over ? uli_color_grey : uli_color_light_grey);

    return x;

}

static s32 drawGridButton(Map* map, s32 x, s32 y)
{
    x -= ICON_SIZE;

    uli_rect rect = {x, y, ICON_SIZE, ICON_SIZE};

    bool over = false;

    if(checkMousePos(map->studio, &rect))
    {
        setCursor(map->studio, uli_cursor_hand);

        over = true;

        showTooltip(map->studio, "SHOW/HIDE GRID [`]");

        if(checkMouseClick(map->studio, &rect, uli_mouse_left))
            map->canvas.grid = !map->canvas.grid;
    }

    drawBitIcon(map->studio, uli_icon_grid, x, y, map->canvas.grid ? uli_color_black : over ? uli_color_grey : uli_color_light_grey);

    return x;
}

static inline bool isIdle(Map* map)
{
    return map->anim.movie == &map->anim.idle;
}

static inline bool sheetVisible(Map* map)
{
    return map->anim.pos.sheet >= 0;
}

static s32 drawSheetButton(Map* map, s32 x, s32 y)
{
    x -= ICON_SIZE;

    uli_rect rect = {x, y, ICON_SIZE, ICON_SIZE};

    bool over = false;
    if(checkMousePos(map->studio, &rect))
    {
        setCursor(map->studio, uli_cursor_hand);

        over = true;
        showTooltip(map->studio, "SHOW TILES [shift]");

        if(isIdle(map) && checkMouseClick(map->studio, &rect, uli_mouse_left))
        {
            map->anim.movie = resetMovie(sheetVisible(map) ? &map->anim.hide : &map->anim.show);
            map->sheet.keep = true;
        }
    }

    drawBitIcon(map->studio, sheetVisible(map) ? uli_icon_up : uli_icon_down, rect.x, rect.y,
        over ? uli_color_grey : uli_color_light_grey);

    return x;
}

static s32 drawToolButton(Map* map, s32 x, s32 y, u8 icon, s32 width, const char* tip, s32 mode)
{
    x -= width;

    uli_rect rect = {x, y, width, ICON_SIZE};

    bool over = false;
    if(checkMousePos(map->studio, &rect))
    {
        setCursor(map->studio, uli_cursor_hand);

        over = true;

        showTooltip(map->studio, tip);

        if(checkMouseClick(map->studio, &rect, uli_mouse_left))
        {
            map->mode = mode;
        }
    }

    drawBitIcon(map->studio, icon, rect.x, rect.y, map->mode == mode ? uli_color_black : over ? uli_color_grey : uli_color_light_grey);

    return x;
}

static s32 drawFillButton(Map* map, s32 x, s32 y)
{
    enum{Size = 8};

    return drawToolButton(map, x, y, uli_icon_fill, Size, "FILL [4]", MAP_FILL_MODE);
}

static s32 drawSelectButton(Map* map, s32 x, s32 y)
{
    return drawToolButton(map, x, y, uli_icon_select, ICON_SIZE, "SELECT [3]", MAP_SELECT_MODE);
}

static s32 drawHandButton(Map* map, s32 x, s32 y)
{
    return drawToolButton(map, x, y, uli_icon_hand, ICON_SIZE, "DRAG MAP [2]", MAP_DRAG_MODE);
}

static s32 drawPenButton(Map* map, s32 x, s32 y)
{
    return drawToolButton(map, x, y, uli_icon_pen, ICON_SIZE, "DRAW [1]", MAP_DRAW_MODE);
}

static void drawTileIndex(Map* map, s32 x, s32 y)
{
    uli_mem* uli = map->uli;
    s32 index = -1;

    if(sheetVisible(map))
    {
        uli_rect rect = {ULI78_WIDTH - ULI_SPRITESHEET_SIZE - 1, TOOLBAR_SIZE, ULI_SPRITESHEET_SIZE, ULI_SPRITESHEET_SIZE};

        if(checkMousePos(map->studio, &rect))
        {
            s32 mx = uli_api_mouse(uli).x - rect.x;
            s32 my = uli_api_mouse(uli).y - rect.y;

            mx /= ULI_SPRITESIZE;
            my /= ULI_SPRITESIZE;

            index = my * map->sheet.blit.pages * ULI_SPRITESHEET_COLS + mx + uli_blit_calc_index(&map->sheet.blit);
        }
    }
    else
    {
        uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

        if(checkMousePos(map->studio, &rect))
        {
            s32 tx = 0, ty = 0;
            getMouseMap(map, &tx, &ty);
            map2ram(uli->ram, map->src);
            index = uli_api_mget(map->uli, tx, ty);
        }
    }

    if(index >= 0)
    {
        char buf[sizeof "#9999"];
        sprintf(buf, "#%03i", index);
        uli_api_print(map->uli, buf, x, y, uli_color_light_grey, true, 1, false);
    }
}

static void drawBppButtons(Map* map, s32 x, s32 y)
{
    uli_mem* uli = map->uli;

    static const char Labels[] = "421";

    for(s32 i = 0; i < sizeof Labels - 1; i++)
    {
        uli_rect rect = {x + i * ULI_ALTFONT_WIDTH, y, ULI_ALTFONT_WIDTH, ULI_FONT_HEIGHT};
        uli_bpp mode = 1 << (2 - i);

        bool hover = false;
        if(checkMousePos(map->studio, &rect))
        {
            setCursor(map->studio, uli_cursor_hand);
            hover = true;

            if(mode > 1)
                SHOW_TOOLTIP(map->studio, "%iBITS PER PIXEL", mode);
            else
                SHOW_TOOLTIP(map->studio, "%iBIT PER PIXEL", mode);

            if(checkMouseClick(map->studio, &rect, uli_mouse_left))
            {
                uli_blit_update_bpp(&map->sheet.blit, mode);
            }
        }

        const char* label = (char[]){Labels[i], '\0'};
        uli_api_print(uli, label, rect.x, rect.y,
            mode == map->sheet.blit.mode
                ? uli_color_dark_grey
                : hover
                    ? uli_color_grey
                    : uli_color_light_grey,
            true, 1, true);
    }
}

static void drawBankButtons(Map* map, s32 x, s32 y)
{
    uli_mem* uli = map->uli;

    enum{Size = 6};

    static const u8 Icons[] = {uli_icon_tiles, uli_icon_sprites};

    for(s32 i = 0; i < COUNT_OF(Icons); i++)
    {
        uli_rect rect = {x + i * Size, y, Size, Size};

        bool hover = false;
        if(checkMousePos(map->studio, &rect))
        {
            setCursor(map->studio, uli_cursor_hand);
            hover = true;

            showTooltip(map->studio, i ? "SPRITES" : "TILES");

            if(isIdle(map) && checkMouseClick(map->studio, &rect, uli_mouse_left))
            {
                Anim* anim = map->anim.bank.items;
                anim->start = (i - map->sheet.blit.bank) * ULI_SPRITESHEET_SIZE;
                map->anim.movie = resetMovie(&map->anim.bank);

                map->sheet.blit.bank = i;
            }
        }

        drawBitIcon(map->studio, Icons[i], rect.x, rect.y,
            i == map->sheet.blit.bank
                ? uli_color_dark_grey
                : hover
                    ? uli_color_grey
                    : uli_color_light_grey);
    }
}

static void drawPagesButtons(Map* map, s32 x, s32 y)
{
    uli_mem* uli = map->uli;

    enum{Width = ULI_ALTFONT_WIDTH + 1, Height = TOOLBAR_SIZE};

    for(s32 i = 0; i < map->sheet.blit.pages; i++)
    {
        uli_rect rect = {x + i * Width - 1, y, Width, Height};

        bool hover = false;
        if(checkMousePos(map->studio, &rect))
        {
            setCursor(map->studio, uli_cursor_hand);
            hover = true;

            SHOW_TOOLTIP(map->studio, "PAGE %i", i);

            if(isIdle(map) && checkMouseClick(map->studio, &rect, uli_mouse_left))
            {
                Anim* anim = map->anim.page.items;
                anim->start = (i - map->sheet.blit.page) * ULI_SPRITESHEET_SIZE;
                map->anim.movie = resetMovie(&map->anim.page);

                map->sheet.blit.page = i;
            }
        }

        bool active = i == map->sheet.blit.page;
        if(active)
        {
            uli_api_rect(uli, rect.x, rect.y, Width, Height, uli_color_black);
        }

        const char* label = (char[]){i + '1', '\0'};
        uli_api_print(uli, label, rect.x + 1, rect.y + 1,
            active
                ? uli_color_white
                : hover
                    ? uli_color_grey
                    : uli_color_light_grey,
            true, 1, true);
    }
}

static void drawMapToolbar(Map* map, s32 x, s32 y)
{
    uli_api_rect(map->uli, 0, 0, ULI78_WIDTH, TOOLBAR_SIZE, uli_color_white);

    drawTileIndex(map, ULI78_WIDTH/2 - ULI_FONT_WIDTH, y);

    x = drawSheetButton(map, x, 0);

    if(sheetVisible(map))
    {
        drawBankButtons(map, 183, 0);
        drawBppButtons(map, 199, 1);

        if(map->sheet.blit.pages > 1)
            drawPagesButtons(map, map->sheet.blit.pages == 4 ? 213 : 222, 0);
    }
    else
    {
        x = drawFillButton(map, x, 0);
        x = drawSelectButton(map, x, 0);
        x = drawHandButton(map, x, 0);
        x = drawPenButton(map, x, 0);

        x = drawGridButton(map, x - 5, 0);
        drawWorldButton(map, x, 0);
    }
}

static void drawSheetVBank1(Map* map, s32 x, s32 y)
{
    uli_mem* uli = map->uli;
    const uli_blit* blit = &map->sheet.blit;

    uli_rect rect = {x, y, ULI_SPRITESHEET_SIZE, ULI_SPRITESHEET_SIZE};

    uli_api_rectb(map->uli, rect.x - 1, rect.y - 1 + map->anim.pos.sheet, rect.w + 2, rect.h + 2, uli_color_white);

    for(s32 i = 1; i < rect.h; i += 4)
    {
        if (blit->page > 0)
        {
            uli_api_pix(uli, rect.x-1, rect.y + i, uli_color_black, false);
            uli_api_pix(uli, rect.x-1, rect.y + i + 1, uli_color_black, false);
        }

        if (blit->page < blit->pages - 1)
        {
            uli_api_pix(uli, rect.x+rect.w, rect.y + i, uli_color_black, false);
            uli_api_pix(uli, rect.x+rect.w, rect.y + i + 1, uli_color_black, false);
        }
    }

    {
        s32 bx = map->sheet.rect.x * ULI_SPRITESIZE - 1 + x;
        s32 by = map->sheet.rect.y * ULI_SPRITESIZE - 1 + y;
        s32 bw = map->sheet.rect.w * ULI_SPRITESIZE + 2;
        s32 bh = map->sheet.rect.h * ULI_SPRITESIZE + 2;

        uli_api_rectb(map->uli, bx, by + map->anim.pos.sheet, bw, bh, uli_color_white);
    }
}

static void initBlitMode(Map* map)
{
    uli_mem* uli = map->uli;
    tiles2ram(uli->ram, getBankTiles(map->studio));
    uli->ram->vram.blit.segment = uli_blit_calc_segment(&map->sheet.blit);
}

static void resetBlitMode(uli_mem* uli)
{
    uli->ram->vram.blit.segment = ULI_DEFAULT_BLIT_MODE;
}

static void drawSheetReg(Map* map, s32 x, s32 y)
{
    uli_mem* uli = map->uli;

    uli_rect rect = {x, y, ULI_SPRITESHEET_SIZE, ULI_SPRITESHEET_SIZE};

    if(isIdle(map) && sheetVisible(map) && checkMousePos(map->studio, &rect))
    {
        setCursor(map->studio, uli_cursor_hand);

        if(checkMouseDown(map->studio, &rect, uli_mouse_left))
        {
            s32 mx = uli_api_mouse(uli).x - rect.x;
            s32 my = uli_api_mouse(uli).y - rect.y;

            mx /= ULI_SPRITESIZE;
            my /= ULI_SPRITESIZE;

            if(map->sheet.drag)
            {
                s32 rl = MIN(mx, map->sheet.start.x);
                s32 rt = MIN(my, map->sheet.start.y);
                s32 rr = MAX(mx, map->sheet.start.x);
                s32 rb = MAX(my, map->sheet.start.y);

                map->sheet.rect = (uli_rect){rl, rt, rr-rl+1, rb-rt+1};
            }
            else
            {
                map->sheet.drag = true;
                map->sheet.start = (uli_point){mx, my};
            }
        }
        else
        {
            if(map->sheet.keep && map->sheet.drag)
                map->anim.movie = resetMovie(&map->anim.hide);

            map->sheet.drag = false;
        }
    }

    uli_api_clip(uli, x, y + map->anim.pos.sheet, ULI_SPRITESHEET_SIZE, ULI_SPRITESHEET_SIZE);

    tiles2ram(uli->ram, getBankTiles(map->studio));

    uli_blit blit = map->sheet.blit;
    SCOPE(resetBlitMode(map->uli), uli_api_clip(uli, 0, 0, ULI78_WIDTH, ULI78_HEIGHT))
    {
        uli_point start =
        {
            x - blit.page * ULI_SPRITESHEET_SIZE + map->anim.pos.page,
            y - blit.bank * ULI_SPRITESHEET_SIZE + map->anim.pos.bank
        }, pos = start;

        for(blit.bank = 0; blit.bank < ULI_SPRITE_BANKS; ++blit.bank, pos.y += ULI_SPRITESHEET_SIZE, pos.x = start.x)
        {
            for(blit.page = 0; blit.page < blit.pages; ++blit.page, pos.x += ULI_SPRITESHEET_SIZE)
            {
                uli->ram->vram.blit.segment = uli_blit_calc_segment(&blit);
                uli_api_spr(uli, 0, pos.x, pos.y + map->anim.pos.sheet, ULI_SPRITESHEET_COLS, ULI_SPRITESHEET_COLS, NULL, 0, 1, uli_no_flip, uli_no_rotate);
            }
        }
    }
}

static void drawCursorPos(Map* map, s32 x, s32 y)
{
    char pos[sizeof "999:999"];

    s32 tx = 0, ty = 0;
    getMouseMap(map, &tx, &ty);

    sprintf(pos, "%03i:%03i", tx, ty);

    s32 width = uli_api_print(map->uli, pos, ULI78_WIDTH, 0, uli_color_dark_green, true, 1, false);

    s32 px = x + (ULI_SPRITESIZE + 3);
    if(px + width >= ULI78_WIDTH) px = x - (width + 2);

    s32 py = y - (ULI_FONT_HEIGHT + 2);
    if(py <= TOOLBAR_SIZE) py = y + (ULI_SPRITESIZE + 3);

    uli_api_rect(map->uli, px - 1, py - 1, width + 1, ULI_FONT_HEIGHT + 1, uli_color_white);
    uli_api_print(map->uli, pos, px, py, uli_color_light_grey, true, 1, false);

    if(map->mode == MAP_FILL_MODE && uli_api_key(map->uli, uli_key_ctrl))
    {
        uli_api_rect(map->uli, px - 1, py - 1 + ULI_FONT_HEIGHT, width + 1, ULI_FONT_HEIGHT + 1, uli_color_white);
        uli_api_print(map->uli, "replace", px, py + ULI_FONT_HEIGHT, uli_color_dark_blue, true, 1, false);
    }
}

static inline void ram2map(const uli_ram* ram, uli_map* src)
{
    memcpy(src, ram->map.data, sizeof ram->map);
}

static void setMapSprite(Map* map, s32 x, s32 y)
{
    s32 mx = map->sheet.rect.x;
    s32 my = map->sheet.rect.y;


    for(s32 j = 0; j < map->sheet.rect.h; j++)
        for(s32 i = 0; i < map->sheet.rect.w; i++)
            uli_api_mset(map->uli, (x+i)%ULI_MAP_WIDTH, (y+j)%ULI_MAP_HEIGHT, (mx+i) + (my+j) * ULI_SPRITESHEET_COLS);

    ram2map(map->uli->ram, map->src);

    history_add(map->history);
}

static uli_point getCursorPos(Map* map)
{
    uli_mem* uli = map->uli;
    uli_point offset = getTileOffset(map);

    s32 mx = uli_api_mouse(uli).x + map->scroll.x - offset.x;
    s32 my = uli_api_mouse(uli).y + map->scroll.y - offset.y;

    mx -= mx % ULI_SPRITESIZE;
    my -= my % ULI_SPRITESIZE;

    mx += -map->scroll.x;
    my += -map->scroll.y;

    return (uli_point){mx, my};
}

static void drawTileCursor(Map* map)
{
    uli_mem* uli = map->uli;

    if(map->scroll.active)
        return;

    uli_point pos = getCursorPos(map);

    {
        s32 sx = map->sheet.rect.x;
        s32 sy = map->sheet.rect.y;

        initBlitMode(map);
        uli_api_spr(uli, sx + map->sheet.blit.pages * sy * ULI_SPRITESHEET_COLS, pos.x, pos.y, map->sheet.rect.w, map->sheet.rect.h, NULL, 0, 1, uli_no_flip, uli_no_rotate);
        resetBlitMode(map->uli);
    }
}

static void drawTileCursorVBank1(Map* map)
{
    if(map->scroll.active)
        return;

    uli_point pos = getCursorPos(map);

    {
        s32 width = map->sheet.rect.w * ULI_SPRITESIZE + 2;
        s32 height = map->sheet.rect.h * ULI_SPRITESIZE + 2;
        uli_api_rectb(map->uli, pos.x - 1, pos.y - 1, width, height, uli_color_white);
    }

    drawCursorPos(map, pos.x, pos.y);
}

static void processMouseDrawMode(Map* map)
{
    uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

    setCursor(map->studio, uli_cursor_hand);

    drawTileCursor(map);

    if(checkMouseDown(map->studio, &rect, uli_mouse_left))
    {
        s32 tx = 0, ty = 0;
        getMouseMap(map, &tx, &ty);

        if(map->canvas.draw)
        {
            s32 w = tx - map->canvas.start.x;
            s32 h = ty - map->canvas.start.y;

            if(w % map->sheet.rect.w == 0 && h % map->sheet.rect.h == 0)
                setMapSprite(map, tx, ty);
        }
        else
        {
            map->canvas.draw    = true;
            map->canvas.start = (uli_point){tx, ty};
        }
    }
    else
    {
        map->canvas.draw    = false;
    }

    if(checkMouseDown(map->studio, &rect, uli_mouse_middle))
    {
        s32 tx = 0, ty = 0;
        getMouseMap(map, &tx, &ty);

        uli_mem* uli = map->uli;
        map2ram(uli->ram, map->src);
        s32 index = uli_api_mget(map->uli, tx, ty);

        map->sheet.rect = (uli_rect){index % ULI_SPRITESHEET_COLS, index / ULI_SPRITESHEET_COLS, 1, 1};
    }
}

static void processScrolling(Map* map, bool pressed)
{
    uli_mem* uli = map->uli;
    uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

    if(map->scroll.active)
    {
        if(pressed)
        {
            map->scroll.x = map->scroll.start.x - uli_api_mouse(uli).x;
            map->scroll.y = map->scroll.start.y - uli_api_mouse(uli).y;

            normalizeMap(&map->scroll.x, &map->scroll.y);

            setCursor(map->studio, uli_cursor_hand);
        }
        else map->scroll.active = false;
    }
    else if(checkMousePos(map->studio, &rect))
    {
        if(pressed)
        {
            map->scroll.active = true;

            map->scroll.start.x = uli_api_mouse(uli).x + map->scroll.x;
            map->scroll.start.y = uli_api_mouse(uli).y + map->scroll.y;
        }
    }
}

static void processMouseDragMode(Map* map)
{
    uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

    processScrolling(map, checkMouseDown(map->studio, &rect, uli_mouse_left) ||
        checkMouseDown(map->studio, &rect, uli_mouse_right));
}

static void resetSelection(Map* map)
{
    map->select.rect = (uli_rect){0,0,0,0};
}

static void drawSelectionRect(Map* map, s32 x, s32 y, s32 w, s32 h)
{
    enum{Step = 3};
    u8 color = uli_color_white;

    s32 index = map->tickCounter / 10;
    for(s32 i = x; i < (x+w); i++)      {uli_api_pix(map->uli, i, y, index++ % Step ? color : 0, false);} index++;
    for(s32 i = y; i < (y+h); i++)      {uli_api_pix(map->uli, x + w-1, i, index++ % Step ? color : 0, false);} index++;
    for(s32 i = (x+w-1); i >= x; i--)   {uli_api_pix(map->uli, i, y + h-1, index++ % Step ? color : 0, false);} index++;
    for(s32 i = (y+h-1); i >= y; i--)   {uli_api_pix(map->uli, x, i, index++ % Step ? color : 0, false);}
}

static void drawPasteData(Map* map)
{
    uli_mem* uli = map->uli;

    s32 w = map->paste[0];
    s32 h = map->paste[1];

    u8* data = map->paste + 2;

    s32 mx = uli_api_mouse(uli).x + map->scroll.x - (w - 1)*ULI_SPRITESIZE / 2;
    s32 my = uli_api_mouse(uli).y + map->scroll.y - (h - 1)*ULI_SPRITESIZE / 2;

    uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

    if(checkMouseClick(map->studio, &rect, uli_mouse_left))
    {
        normalizeMap(&mx, &my);

        mx /= ULI_SPRITESIZE;
        my /= ULI_SPRITESIZE;

        for(s32 j = 0; j < h; j++)
            for(s32 i = 0; i < w; i++)
                uli_api_mset(uli, (mx+i)%ULI_MAP_WIDTH, (my+j)%ULI_MAP_HEIGHT, data[i + j * w]);

        ram2map(uli->ram, map->src);

        history_add(map->history);

        free(map->paste);
        map->paste = NULL;
    }
    else
    {
        mx -= mx % ULI_SPRITESIZE;
        my -= my % ULI_SPRITESIZE;

        mx += -map->scroll.x;
        my += -map->scroll.y;

        initBlitMode(map);

        for(s32 j = 0; j < h; j++)
            for(s32 i = 0; i < w; i++)
            {
                s32 index = data[i + j * w];
                s32 sx = index % ULI_SPRITESHEET_COLS;
                s32 sy = index / ULI_SPRITESHEET_COLS;
                uli_api_spr(uli, sx + map->sheet.blit.pages * sy * ULI_SPRITESHEET_COLS,
                    mx + i * ULI_SPRITESIZE, my + j * ULI_SPRITESIZE, 1, 1, NULL, 0, 1, uli_no_flip, uli_no_rotate);
            }

        resetBlitMode(map->uli);
    }
}

static void drawPasteDataVBank1(Map* map)
{
    uli_mem* uli = map->uli;
    s32 w = map->paste[0];
    s32 h = map->paste[1];

    s32 mx = uli_api_mouse(uli).x + map->scroll.x - (w - 1) * ULI_SPRITESIZE / 2;
    s32 my = uli_api_mouse(uli).y + map->scroll.y - (h - 1) * ULI_SPRITESIZE / 2;

    mx -= mx % ULI_SPRITESIZE;
    my -= my % ULI_SPRITESIZE;

    mx += -map->scroll.x;
    my += -map->scroll.y;

    drawSelectionRect(map, mx - 1, my - 1, w * ULI_SPRITESIZE + 2, h * ULI_SPRITESIZE + 2);
}

static void normalizeMapRect(s32* x, s32* y)
{
    while(*x < 0) *x += ULI_MAP_WIDTH;
    while(*y < 0) *y += ULI_MAP_HEIGHT;
    while(*x >= ULI_MAP_WIDTH) *x -= ULI_MAP_WIDTH;
    while(*y >= ULI_MAP_HEIGHT) *y -= ULI_MAP_HEIGHT;
}

static void processMouseSelectMode(Map* map)
{
    uli_mem* uli = map->uli;
    uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

    if(checkMousePos(map->studio, &rect))
    {
        if(map->paste)
            drawPasteData(map);
        else
        {
            if(checkMouseDown(map->studio, &rect, uli_mouse_left))
            {
                s32 mx = uli_api_mouse(uli).x + map->scroll.x;
                s32 my = uli_api_mouse(uli).y + map->scroll.y;

                mx /= ULI_SPRITESIZE;
                my /= ULI_SPRITESIZE;

                if(map->select.drag)
                {
                    s32 rl = MIN(mx, map->select.start.x);
                    s32 rt = MIN(my, map->select.start.y);
                    s32 rr = MAX(mx, map->select.start.x);
                    s32 rb = MAX(my, map->select.start.y);

                    map->select.rect = (uli_rect){rl, rt, rr - rl + 1, rb - rt + 1};
                }
                else
                {
                    map->select.drag = true;
                    map->select.start = (uli_point){mx, my};
                    map->select.rect = (uli_rect){map->select.start.x, map->select.start.y, 1, 1};
                }
            }
            else if(map->select.drag)
            {
                map->select.drag = false;

                if(map->select.rect.w <= 1 && map->select.rect.h <= 1)
                    resetSelection(map);
            }
        }
    }
}

typedef struct
{
    uli_point* data;
    uli_point* head;
} FillStack;

static bool push(FillStack* stack, s32 x, s32 y)
{
    if(stack->head == NULL)
    {
        stack->head = stack->data;
        stack->head->x = x;
        stack->head->y = y;

        return true;
    }

    if(stack->head < (stack->data + FILL_STACK_SIZE-1))
    {
        stack->head++;
        stack->head->x = x;
        stack->head->y = y;

        return true;
    }

    return false;
}

static bool pop(FillStack* stack, s32* x, s32* y)
{
    if(stack->head > stack->data)
    {
        *x = stack->head->x;
        *y = stack->head->y;

        stack->head--;

        return true;
    }

    if(stack->head == stack->data)
    {
        *x = stack->head->x;
        *y = stack->head->y;

        stack->head = NULL;

        return true;
    }

    return false;
}

static void fillMap(Map* map, s32 x, s32 y, u8 tile)
{
    if(tile == (map->sheet.rect.x + map->sheet.rect.y * ULI_SPRITESHEET_COLS)) return;

    static FillStack stack = {NULL, NULL};

    if(!stack.data)
        stack.data = (uli_point*)malloc(FILL_STACK_SIZE * sizeof(uli_point));

    stack.head = NULL;

    static const s32 dx[4] = {0, 1, 0, -1};
    static const s32 dy[4] = {-1, 0, 1, 0};

    if(!push(&stack, x, y)) return;

    s32 mx = map->sheet.rect.x;
    s32 my = map->sheet.rect.y;

    struct
    {
        s32 l;
        s32 t;
        s32 r;
        s32 b;
    }clip = { 0, 0, ULI_MAP_WIDTH, ULI_MAP_HEIGHT };

    if (map->select.rect.w > 0 && map->select.rect.h > 0)
    {
        clip.l = map->select.rect.x;
        clip.t = map->select.rect.y;
        clip.r = map->select.rect.x + map->select.rect.w;
        clip.b = map->select.rect.y + map->select.rect.h;
    }


    while(pop(&stack, &x, &y))
    {
        for(s32 j = 0; j < map->sheet.rect.h; j++)
            for(s32 i = 0; i < map->sheet.rect.w; i++)
                uli_api_mset(map->uli, x+i, y+j, (mx+i) + (my+j) * ULI_SPRITESHEET_COLS);

        for(s32 i = 0; i < COUNT_OF(dx); i++)
        {
            s32 nx = x + dx[i]*map->sheet.rect.w;
            s32 ny = y + dy[i]*map->sheet.rect.h;

            if(nx >= clip.l && nx < clip.r && ny >= clip.t && ny < clip.b)
            {
                bool match = true;
                for(s32 j = 0; j < map->sheet.rect.h; j++)
                    for(s32 i = 0; i < map->sheet.rect.w; i++)
                        if(uli_api_mget(map->uli, nx+i, ny+j) != tile)
                            match = false;

                if(match)
                {
                    if(!push(&stack, nx, ny)) return;
                }
            }
        }
    }
}

static s32 moduloWrap(s32 x, s32 m)
{
   s32 y = x % m;
   return (y < 0) ? (y + m) : y; // always between 0 and m-1 inclusive
}

// replace tile with another tile or pattern
static void replaceTile(Map* map, s32 x, s32 y, u8 tile)
{
    if(tile == (map->sheet.rect.x + map->sheet.rect.y * ULI_SPRITESHEET_COLS)) return;

    s32 mx = map->sheet.rect.x;
    s32 my = map->sheet.rect.y;

    struct
    {
        s32 l;
        s32 t;
        s32 r;
        s32 b;
    } clip = { 0, 0, ULI_MAP_WIDTH, ULI_MAP_HEIGHT };

    if (map->select.rect.w > 0 && map->select.rect.h > 0)
    {
        clip.l = map->select.rect.x;
        clip.t = map->select.rect.y;
        clip.r = map->select.rect.x + map->select.rect.w;
        clip.b = map->select.rect.y + map->select.rect.h;
    }

    // for each tile in selection/full map
    for(s32 j = clip.t; j < clip.b; j++)
        for(s32 i = clip.l; i < clip.r; i++)
            if(uli_api_mget(map->uli, i, j) == tile)
            {
                // offset pattern based on click position
                s32 oy = moduloWrap(j - y, map->sheet.rect.h);
                s32 ox = moduloWrap(i - x, map->sheet.rect.w);

                u8 newtile = (mx + ox) + (my + oy) * ULI_SPRITESHEET_COLS;
                uli_api_mset(map->uli, i, j, newtile);
            }
}

static void processMouseFillMode(Map* map)
{
    uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

    setCursor(map->studio, uli_cursor_hand);

    drawTileCursor(map);

    if(checkMouseClick(map->studio, &rect, uli_mouse_left))
    {
        s32 tx = 0, ty = 0;
        getMouseMap(map, &tx, &ty);

        {
            uli_mem* uli = map->uli;
            map2ram(uli->ram, map->src);
            if(uli_api_key(uli, uli_key_ctrl))
                replaceTile(map, tx, ty, uli_api_mget(map->uli, tx, ty));
            else
                fillMap(map, tx, ty, uli_api_mget(map->uli, tx, ty));
            ram2map(uli->ram, map->src);
        }

        history_add(map->history);
    }
}

static void drawSelectionVBank1(Map* map)
{
    uli_rect* sel = &map->select.rect;

    if(sel->w > 0 && sel->h > 0)
    {
        s32 x = sel->x * ULI_SPRITESIZE - map->scroll.x;
        s32 y = sel->y * ULI_SPRITESIZE - map->scroll.y;
        s32 w = sel->w * ULI_SPRITESIZE;
        s32 h = sel->h * ULI_SPRITESIZE;

        while(x+w<0)x+=MAX_SCROLL_X;
        while(y+h<0)y+=MAX_SCROLL_Y;
        while(x+w>=MAX_SCROLL_X)x-=MAX_SCROLL_X;
        while(y+h>=MAX_SCROLL_Y)y-=MAX_SCROLL_Y;

        drawSelectionRect(map, x-1, y-1, w+2, h+2);
    }
}

static void drawGrid(Map* map)
{
    uli_mem* uli = map->uli;

    s32 scrollX = map->scroll.x % ULI_SPRITESIZE;
    s32 scrollY = map->scroll.y % ULI_SPRITESIZE;

    for(s32 j = -scrollY; j <= ULI78_HEIGHT-scrollY; j += ULI_SPRITESIZE)
    {
        if(j >= 0 && j < ULI78_HEIGHT)
            for(s32 i = 0; i < ULI78_WIDTH; i++)
            {
                u8 color = uli_api_pix(uli, i, j, 0, true);
                uli_api_pix(uli, i, j, (color+1)%ULI_PALETTE_SIZE, false);
            }
    }

    for(s32 j = -scrollX; j <= ULI78_WIDTH-scrollX; j += ULI_SPRITESIZE)
    {
        if(j >= 0 && j < ULI78_WIDTH)
            for(s32 i = 0; i < ULI78_HEIGHT; i++)
            {
                if((i+scrollY) % ULI_SPRITESIZE)
                {
                    u8 color = uli_api_pix(uli, j, i, 0, true);
                    uli_api_pix(uli, j, i, (color+1)%ULI_PALETTE_SIZE, false);
                }
            }
    }
}

static void drawMapReg(Map* map)
{
    uli_mem* uli = map->uli;
    uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};

    bool handle = !sheetVisible(map) && checkMousePos(map->studio, &rect);
    bool space = uli_api_key(uli, uli_key_space);

    if(handle)
        processScrolling(map,
            ((space || map->mode == MAP_DRAG_MODE) && checkMouseDown(map->studio, &rect, uli_mouse_left)) ||
                checkMouseDown(map->studio, &rect, uli_mouse_right));

    {
        s32 scrollX = map->scroll.x % ULI_SPRITESIZE;
        s32 scrollY = map->scroll.y % ULI_SPRITESIZE;

        map2ram(uli->ram, map->src);

        initBlitMode(map);
        uli_api_map(uli, map->scroll.x / ULI_SPRITESIZE, map->scroll.y / ULI_SPRITESIZE,
            ULI_MAP_SCREEN_WIDTH + 1, ULI_MAP_SCREEN_HEIGHT + 1, -scrollX, -scrollY, 0, 0, 1, NULL, NULL);
        resetBlitMode(map->uli);

        if (map->canvas.grid)
            drawGrid(map);
    }

    if(handle && !space)
    {
        static void(*const Handlers[])(Map*) = {processMouseDrawMode, processMouseDragMode, processMouseSelectMode, processMouseFillMode};
        Handlers[map->mode](map);
    }
}

static void undo(Map* map)
{
    history_undo(map->history);
}

static void redo(Map* map)
{
    history_redo(map->history);
}

static void copySelectionToClipboard(Map* map)
{
    uli_rect* sel = &map->select.rect;

    if(sel->w > 0 && sel->h > 0)
    {
        s32 size = sel->w * sel->h + 2;
        u8* buffer = malloc(size);

        if(buffer)
        {
            buffer[0] = sel->w;
            buffer[1] = sel->h;

            u8* ptr = buffer + 2;

            for(s32 j = sel->y; j < sel->y+sel->h; j++)
                for(s32 i = sel->x; i < sel->x+sel->w; i++)
                {
                    s32 x = i, y = j;
                    normalizeMapRect(&x, &y);

                    s32 index = x + y * ULI_MAP_WIDTH;
                    *ptr++ = map->src->data[index];
                }

            toClipboard(buffer, size, true);
            free(buffer);
        }
    }
}

static void copyToClipboard(Map* map)
{
    copySelectionToClipboard(map);
    resetSelection(map);
}

static void deleteSelection(Map* map)
{
    uli_rect* sel = &map->select.rect;

    if(sel->w > 0 && sel->h > 0)
    {
        for(s32 j = sel->y; j < sel->y+sel->h; j++)
            for(s32 i = sel->x; i < sel->x+sel->w; i++)
            {
                s32 x = i, y = j;
                normalizeMapRect(&x, &y);

                s32 index = x + y * ULI_MAP_WIDTH;
                map->src->data[index] = 0;
            }

        history_add(map->history);
    }
}

static void cutToClipboard(Map* map)
{
    copySelectionToClipboard(map);
    deleteSelection(map);
    resetSelection(map);
}

static void copyFromClipboard(Map* map)
{
    if(uli_sys_clipboard_has())
    {
        char* clipboard = uli_sys_clipboard_get();

        if(clipboard)
        {
            s32 size = (s32)strlen(clipboard)/2;

            if(size > 2)
            {
                u8* data = malloc(size);

                uli_tool_str2buf(clipboard, (s32)strlen(clipboard), data, true);

                if(data[0] * data[1] == size - 2)
                {
                    map->paste = data;
                    map->mode = MAP_SELECT_MODE;
                }
                else free(data);
            }

            uli_sys_clipboard_free(clipboard);
        }
    }
}

static inline bool keyWasPressedOnce(Map* map, s32 key)
{
    uli_mem* uli = map->uli;

    return uli_api_keyp(uli, key, -1, -1);
}

static void processKeyboard(Map* map)
{
    uli_mem* uli = map->uli;

    if(isIdle(map))
    {
        if(!sheetVisible(map) && keyWasPressedOnce(map, uli_key_shift))
        {
                map->anim.movie = resetMovie(&map->anim.show);
                map->sheet.keep = true;
        }
        else
        {
            if(map->sheet.keep && sheetVisible(map) && keyWasPressedOnce(map, uli_key_shift))
                map->anim.movie = resetMovie(&map->anim.hide);
        }
    }

    if(uli->ram->input.keyboard.data == 0) return;

    bool ctrl = uli_api_key(uli, uli_key_ctrl);

    switch(getClipboardEvent(map->studio))
    {
    case ULI_CLIPBOARD_CUT: cutToClipboard(map); break;
    case ULI_CLIPBOARD_COPY: copyToClipboard(map); break;
    case ULI_CLIPBOARD_PASTE: copyFromClipboard(map); break;
    default: break;
    }

    if(uli_api_key(uli, uli_key_alt))
        return;

    if(ctrl)
    {
        if(keyWasPressed(map->studio, uli_key_z))        undo(map);
        else if(keyWasPressed(map->studio, uli_key_y))   redo(map);
    }
    else
    {
        if(keyWasPressed(map->studio, uli_key_tab)) setStudioMode(map->studio, ULI_WORLD_MODE);
        else if(keyWasPressed(map->studio, uli_key_1)) map->mode = MAP_DRAW_MODE;
        else if(keyWasPressed(map->studio, uli_key_2)) map->mode = MAP_DRAG_MODE;
        else if(keyWasPressed(map->studio, uli_key_3)) map->mode = MAP_SELECT_MODE;
        else if(keyWasPressed(map->studio, uli_key_4)) map->mode = MAP_FILL_MODE;
        else if(keyWasPressed(map->studio, uli_key_delete)) deleteSelection(map);
        else if(keyWasPressed(map->studio, uli_key_grave)) map->canvas.grid = !map->canvas.grid;
    }

    enum{Step = 1};

    if(uli_api_key(uli, uli_key_up)) map->scroll.y -= Step;
    if(uli_api_key(uli, uli_key_down)) map->scroll.y += Step;
    if(uli_api_key(uli, uli_key_left)) map->scroll.x -= Step;
    if(uli_api_key(uli, uli_key_right)) map->scroll.x += Step;

    static const uli_key Keycodes[] = {uli_key_up, uli_key_down, uli_key_left, uli_key_right};

    for(s32 i = 0; i < COUNT_OF(Keycodes); i++)
        if(uli_api_key(uli, Keycodes[i]))
        {
            normalizeMap(&map->scroll.x, &map->scroll.y);
            break;
        }
}

static void tick(Map* map)
{
    uli_mem* uli = map->uli;
    map->tickCounter++;

    processAnim(map->anim.movie, map);

    // process scroll
    if(uli->ram->input.mouse.scrolly < 0)
    {
        setStudioMode(map->studio, ULI_WORLD_MODE);
        return;
    }

    processKeyboard(map);

    drawMapReg(map);
    drawSheetReg(map, ULI78_WIDTH - ULI_SPRITESHEET_SIZE - 1, TOOLBAR_SIZE);

    VBANK(uli, 1)
    {
        uli_api_cls(uli, uli->ram->vram.vars.clear = uli_color_dark_blue);

        memcpy(uli->ram->vram.palette.data, getConfig(map->studio)->cart->bank0.palette.vbank0.data, sizeof(uli_palette));

        uli_api_clip(uli, 0, TOOLBAR_SIZE, ULI78_WIDTH - (sheetVisible(map) ? ULI_SPRITESHEET_SIZE+2 : 0), ULI78_HEIGHT - TOOLBAR_SIZE);
        {
            s32 screenScrollX = map->scroll.x % ULI78_WIDTH;
            s32 screenScrollY = map->scroll.y % ULI78_HEIGHT;

            uli_api_line(uli, 0, ULI78_HEIGHT - screenScrollY, ULI78_WIDTH, ULI78_HEIGHT - screenScrollY, uli_color_grey);
            uli_api_line(uli, ULI78_WIDTH - screenScrollX, 0, ULI78_WIDTH - screenScrollX, ULI78_HEIGHT, uli_color_grey);
        }
        uli_api_clip(uli, 0, 0, ULI78_WIDTH, ULI78_HEIGHT);

        drawSheetVBank1(map, ULI78_WIDTH - ULI_SPRITESHEET_SIZE - 1, TOOLBAR_SIZE);

        {
            uli_rect rect = {MAP_X, MAP_Y, MAP_WIDTH, MAP_HEIGHT};
            if(!sheetVisible(map) && checkMousePos(map->studio, &rect) && !uli_api_key(uli, uli_key_space))
            {
                switch(map->mode)
                {
                case MAP_DRAW_MODE:
                case MAP_FILL_MODE:
                    drawTileCursorVBank1(map);
                    break;
                case MAP_SELECT_MODE:
                    if(map->paste)
                        drawPasteDataVBank1(map);
                    break;
                default:
                    break;
                }
            }
        }

        if(!sheetVisible(map))
            drawSelectionVBank1(map);

        drawMapToolbar(map, ULI78_WIDTH, 1);
        drawToolbar(map->studio, map->uli, false);
    }
}

static void onStudioEvent(Map* map, StudioEvent event)
{
    switch(event)
    {
    case ULI_TOOLBAR_CUT:   cutToClipboard(map); break;
    case ULI_TOOLBAR_COPY:  copyToClipboard(map); break;
    case ULI_TOOLBAR_PASTE: copyFromClipboard(map); break;
    case ULI_TOOLBAR_UNDO:  undo(map); break;
    case ULI_TOOLBAR_REDO:  redo(map); break;
    default: break;
    }
}

static void scanline(uli_mem* uli, s32 row, void* data)
{
    Map* map = data;
    if(row == 0)
        memcpy(&uli->ram->vram.palette, getBankPalette(map->studio, false), sizeof(uli_palette));
}

static void emptyDone(void* data) {}

static void setIdle(void* data)
{
    Map* map = data;
    map->anim.movie = resetMovie(&map->anim.idle);
}

static void freeAnim(Map* map)
{
    FREE(map->anim.show.items);
    FREE(map->anim.hide.items);
    FREE(map->anim.bank.items);
    FREE(map->anim.page.items);
}

void initMap(Map* map, Studio* studio, uli_map* src)
{
    enum {SheetStart = -(ULI_SPRITESHEET_SIZE + TOOLBAR_SIZE)};

    if(map->history) history_delete(map->history);
    freeAnim(map);

    *map = (Map)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .tick = tick,
        .src = src,
        .mode = MAP_DRAW_MODE,
        .canvas =
        {
            .grid = true,
            .draw = false,
            .start = {0, 0},
        },
        .sheet =
        {
            .rect = {0, 0, 1, 1},
            .start = {0, 0},
            .drag = false,
            .blit = {0},
        },
        .select =
        {
            .rect = {0, 0, 0, 0},
            .start = {0, 0},
            .drag = false,
        },
        .paste = NULL,
        .tickCounter = 0,
        .scroll =
        {
            .x = 0,
            .y = 0,
            .active = false,
            .gesture = false,
            .start = {0, 0},
        },
        .history = history_create(src, sizeof(uli_map)),
        .anim =
        {
            .pos.sheet = SheetStart,

            .idle = {.done = emptyDone,},

            .show = MOVIE_DEF(STUDIO_ANIM_TIME, setIdle,
            {
                {SheetStart, 0, STUDIO_ANIM_TIME, &map->anim.pos.sheet, AnimEaseIn},
            }),

            .hide = MOVIE_DEF(STUDIO_ANIM_TIME, setIdle,
            {
                {0, SheetStart, STUDIO_ANIM_TIME, &map->anim.pos.sheet, AnimEaseIn},
            }),

            .bank = MOVIE_DEF(STUDIO_ANIM_TIME, setIdle,
            {
                {0, 0, STUDIO_ANIM_TIME, &map->anim.pos.bank, AnimEaseIn},
            }),

            .page = MOVIE_DEF(STUDIO_ANIM_TIME, setIdle,
            {
                {0, 0, STUDIO_ANIM_TIME, &map->anim.pos.page, AnimEaseIn},
            }),
        },
        .event = onStudioEvent,
        .scanline = scanline,
    };

    map->anim.movie = resetMovie(&map->anim.idle);

    normalizeMap(&map->scroll.x, &map->scroll.y);
    uli_blit_update_bpp(&map->sheet.blit, ULI_DEFAULT_BIT_DEPTH);
}

void freeMap(Map* map)
{
    freeAnim(map);
    history_delete(map->history);
    free(map);
}
