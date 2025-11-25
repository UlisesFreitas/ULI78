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

#include "surf.h"
#include "studio/fs.h"
#include "studio/net.h"
#include "studio/config.h"
#include "console.h"
#include "menu.h"
#include "ext/gif.h"
#include "ext/png.h"

#if defined(ULI78_PRO)
#include "studio/project.h"
#else
#include "cart.h"
#endif

#include <string.h>

#define MAIN_OFFSET 4
#define MENU_HEIGHT 10
#define ANIM 10
#define PAGE 5
#define COVER_WIDTH 140
#define COVER_HEIGHT 116
#define COVER_Y 5
#define COVER_X (ULI78_WIDTH - COVER_WIDTH - COVER_Y)
#define COVER_FADEIN 96
#define COVER_FADEOUT 256
#define CAN_OPEN_URL (__ULI_WINDOWS__ || __ULI_LINUX__ || __ULI_MACOSX__ || __ULI_ANDROID__)

static const char* PngExt = PNG_EXT;

typedef struct SurfItem SurfItem;

struct SurfItem
{
    char* label;
    char* name;
    char* hash;
    s32 id;
    uli_screen* cover;

    uli_palette* palette;

    bool coverLoading;
    bool dir;
    bool project;
};

typedef struct
{
    SurfItem* items;
    s32 count;
    Surf* surf;
    fs_done_callback done;
    void* data;
} AddMenuItemData;

static void drawTopToolbar(Surf* surf, s32 x, s32 y)
{
    uli_mem* uli = surf->uli;

    enum{Height = MENU_HEIGHT};

    uli_api_rect(uli, x, y, ULI78_WIDTH, Height, uli_color_grey);
    uli_api_rect(uli, x, y + Height, ULI78_WIDTH, 1, uli_color_black);

    {
        static const char Label[] = "ULI-78 SURF";
        s32 xl = x + MAIN_OFFSET;
        s32 yl = y + (Height - ULI_FONT_HEIGHT)/2;
        uli_api_print(uli, Label, xl, yl+1, uli_color_black, true, 1, false);
        uli_api_print(uli, Label, xl, yl, uli_color_white, true, 1, false);
    }

    enum{Gap = 10, TipX = 150, SelectWidth = 54};

    u8 colorkey = 0;
    tiles2ram(uli->ram, &getConfig(surf->studio)->cart->bank0.tiles);
    uli_api_spr(uli, 12, TipX, y+1, 1, 1, &colorkey, 1, 1, uli_no_flip, uli_no_rotate);
    {
        static const char Label[] = "SELECT";
        uli_api_print(uli, Label, TipX + Gap, y+3, uli_color_black, true, 1, false);
        uli_api_print(uli, Label, TipX + Gap, y+2, uli_color_white, true, 1, false);
    }

    uli_api_spr(uli, 13, TipX + SelectWidth, y + 1, 1, 1, &colorkey, 1, 1, uli_no_flip, uli_no_rotate);
    {
        static const char Label[] = "BACK";
        uli_api_print(uli, Label, TipX + Gap + SelectWidth, y +3, uli_color_black, true, 1, false);
        uli_api_print(uli, Label, TipX + Gap + SelectWidth, y +2, uli_color_white, true, 1, false);
    }
}

static SurfItem* getMenuItem(Surf* surf)
{
    return &surf->menu.items[surf->menu.pos];
}

static void drawBottomToolbar(Surf* surf, s32 x, s32 y)
{
    uli_mem* uli = surf->uli;

    enum{Height = MENU_HEIGHT};

    uli_api_rect(uli, x, y, ULI78_WIDTH, Height, uli_color_grey);
    uli_api_rect(uli, x, y + Height, ULI78_WIDTH, 1, uli_color_black);
    {
        char label[ULINAME_MAX + 1];
        char dir[ULINAME_MAX];
        uli_fs_dir(surf->fs, dir);

        sprintf(label, "/%s", dir);
        s32 xl = x + MAIN_OFFSET;
        s32 yl = y + (Height - ULI_FONT_HEIGHT)/2;
        uli_api_print(uli, label, xl, yl+1, uli_color_black, true, 1, false);
        uli_api_print(uli, label, xl, yl, uli_color_white, true, 1, false);
    }

#ifdef CAN_OPEN_URL

    if(surf->menu.count > 0 && getMenuItem(surf)->hash)
    {
        enum{Gap = 10, TipX = 134, SelectWidth = 54};

        u8 colorkey = 0;

        tiles2ram(uli->ram, &getConfig(surf->studio)->cart->bank0.tiles);
        uli_api_spr(uli, 15, TipX + SelectWidth, y + 1, 1, 1, &colorkey, 1, 1, uli_no_flip, uli_no_rotate);
        {
            static const char Label[] = "WEBSITE";
            uli_api_print(uli, Label, TipX + Gap + SelectWidth, y + 3, uli_color_black, true, 1, false);
            uli_api_print(uli, Label, TipX + Gap + SelectWidth, y + 2, uli_color_white, true, 1, false);
        }
    }
#endif

}

static void drawMenu(Surf* surf, s32 x, s32 y)
{
    uli_mem* uli = surf->uli;

    enum {Height = MENU_HEIGHT};

    uli_api_rect(uli, 0, y + (MENU_HEIGHT - surf->anim.val.menuHeight) / 2, ULI78_WIDTH, surf->anim.val.menuHeight, uli_color_red);

    s32 ym = y - surf->menu.pos * MENU_HEIGHT + (MENU_HEIGHT - ULI_FONT_HEIGHT) / 2 - surf->anim.val.pos;
    for(s32 i = 0; i < surf->menu.count; i++, ym += Height)
    {
        const char* name = surf->menu.items[i].label;

        if (ym > (-(ULI_FONT_HEIGHT + 1)) && ym <= ULI78_HEIGHT)
        {
            uli_api_print(uli, name, x + MAIN_OFFSET, ym + 1, uli_color_black, false, 1, false);
            uli_api_print(uli, name, x + MAIN_OFFSET, ym, uli_color_white, false, 1, false);
        }
    }
}

static inline void cutExt(char* name, const char* ext)
{
    name[strlen(name)-strlen(ext)] = '\0';
}

static bool addMenuItem(const char* name, const char* title, const char* hash, s32 id, void* ptr, bool dir)
{
    AddMenuItemData* data = (AddMenuItemData*)ptr;

    static const char CartExt[] = CART_EXT;

    if(dir
        || uli_tool_has_ext(name, CartExt)
        || uli_tool_has_ext(name, PngExt)
#if defined(ULI78_PRO)
        || project_ext(name)
#endif
        )
    {
        data->items = realloc(data->items, sizeof(SurfItem) * ++data->count);
        SurfItem* item = &data->items[data->count-1];

        *item = (SurfItem)
        {
            .name = strdup(name),
            .hash = hash ? strdup(hash) : NULL,
            .id = id,
            .dir = dir,
        };

        if(dir)
        {
            char folder[ULINAME_MAX];
            sprintf(folder, "[%s]", name);
            item->label = strdup(folder);
        }
        else
        {
            item->label = title ? strdup(title) : strdup(name);

            if(uli_tool_has_ext(name, CartExt))
                cutExt(item->label, CartExt);
            else
                item->project = true;
        }
    }

    return true;
}

static int itemcmp(const void* a, const void* b)
{
    const SurfItem* item1 = a;
    const SurfItem* item2 = b;

    if(item1->dir != item2->dir)
        return item1->dir ? -1 : 1;
    else if(item1->dir && item2->dir)
        return strcmp(item1->name, item2->name);

    return 0;
}

static void addMenuItemsDone(void* data)
{
    AddMenuItemData* addMenuItemData = data;
    Surf* surf = addMenuItemData->surf;

    surf->menu.items = addMenuItemData->items;
    surf->menu.count = addMenuItemData->count;

    if(!uli_fs_ispubdir(surf->fs))
        qsort(surf->menu.items, surf->menu.count, sizeof *surf->menu.items, itemcmp);

    if (addMenuItemData->done)
        addMenuItemData->done(addMenuItemData->data);

    free(addMenuItemData);

    surf->loading = false;
}

static void resetMenu(Surf* surf)
{
    if(surf->menu.items)
    {
        for(s32 i = 0; i < surf->menu.count; i++)
        {
            SurfItem* item = &surf->menu.items[i];

            free(item->name);

            FREE(item->hash);
            FREE(item->cover);
            FREE(item->label);
            FREE(item->palette);
        }

        free(surf->menu.items);

        surf->menu.items = NULL;
        surf->menu.count = 0;
    }

    surf->menu.pos = 0;
}

static void updateMenuItemCover(Surf* surf, s32 pos, const u8* cover, s32 size)
{
    SurfItem* item = &surf->menu.items[pos];

    gif_image* image = gif_read_data(cover, size);

    if(image)
    {
        item->cover = malloc(sizeof(uli_screen));
        item->palette = malloc(sizeof(uli_palette));

        if (image->width == ULI78_WIDTH
            && image->height == ULI78_HEIGHT
            && image->colors <= ULI_PALETTE_SIZE)
        {
            memcpy(item->palette, image->palette, image->colors * sizeof(uli_rgb));

            for(s32 i = 0; i < ULI78_WIDTH * ULI78_HEIGHT; i++)
                uli_tool_poke4(item->cover->data, i, image->buffer[i]);
        }
        else
        {
            memset(item->cover, 0, sizeof(uli_screen));
            memset(item->palette, 0, sizeof(uli_palette));
        }

        gif_close(image);
    }
}

typedef struct
{
    Surf* surf;
    s32 pos;
    char cachePath[ULINAME_MAX];
    char dir[ULINAME_MAX];
} CoverLoadingData;

static void coverLoaded(const net_get_data* netData)
{
    CoverLoadingData* coverLoadingData = netData->calldata;
    Surf* surf = coverLoadingData->surf;

    if (netData->type == net_get_done)
    {
        uli_fs_saveroot(surf->fs, coverLoadingData->cachePath, netData->done.data, netData->done.size, false);

        char dir[ULINAME_MAX];
        uli_fs_dir(surf->fs, dir);

        if(strcmp(dir, coverLoadingData->dir) == 0)
            updateMenuItemCover(surf, coverLoadingData->pos, netData->done.data, netData->done.size);
    }

    switch (netData->type)
    {
    case net_get_done:
    case net_get_error:
        free(coverLoadingData);
        break;
    default: break;
    }
}

static void requestCover(Surf* surf, SurfItem* item)
{
    CoverLoadingData coverLoadingData = {surf, surf->menu.pos};
    uli_fs_dir(surf->fs, coverLoadingData.dir);

    const char* hash = item->hash;
    sprintf(coverLoadingData.cachePath, ULI_CACHE "%s.gif", hash);

    {
        s32 size = 0;
        void* data = uli_fs_loadroot(surf->fs, coverLoadingData.cachePath, &size);

        if (data)
        {
            updateMenuItemCover(surf, surf->menu.pos, data, size);
            free(data);
        }
    }

    char path[ULINAME_MAX];
    sprintf(path, "/cart/%s/cover.gif", hash);

    uli_net_get(surf->net, path, coverLoaded, MOVE(coverLoadingData));
}

static void loadCover(Surf* surf)
{
    uli_mem* uli = surf->uli;

    SurfItem* item = getMenuItem(surf);

    if(item->coverLoading)
        return;

    item->coverLoading = true;

    if(!uli_fs_ispubdir(surf->fs))
    {

        s32 size = 0;
        void* data = uli_fs_load(surf->fs, item->name, &size);

        if(data)
        {
            uli_cartridge* cart = (uli_cartridge*)malloc(sizeof(uli_cartridge));

            if(cart)
            {

                if(uli_tool_has_ext(item->name, PngExt))
                {
                    uli_cartridge* pngcart = loadPngCart((png_buffer){data, size});

                    if(pngcart)
                    {
                        memcpy(cart, pngcart, sizeof(uli_cartridge));
                        free(pngcart);
                    }
                    else memset(cart, 0, sizeof(uli_cartridge));
                }
#if defined(ULI78_PRO)
                else if(project_ext(item->name))
                    uli_project_load(item->name, data, size, cart);
#endif
                else
                    uli_cart_load(cart, data, size);

                if(!EMPTY(cart->bank0.screen.data) && !EMPTY(cart->bank0.palette.vbank0.data))
                {
                    memcpy((item->palette = malloc(sizeof(uli_palette))), &cart->bank0.palette.vbank0, sizeof(uli_palette));
                    memcpy((item->cover = malloc(sizeof(uli_screen))), &cart->bank0.screen, sizeof(uli_screen));
                }

                free(cart);
            }

            free(data);
        }
    }
    else if(item->hash && !item->cover)
    {
        requestCover(surf, item);
    }
}

static void initItemsAsync(Surf* surf, fs_done_callback callback, void* calldata)
{
    resetMenu(surf);

    surf->loading = true;

    char dir[ULINAME_MAX];
    uli_fs_dir(surf->fs, dir);

    AddMenuItemData data = { NULL, 0, surf, callback, calldata};

    if(strcmp(dir, "") != 0)
        addMenuItem("..", NULL, NULL, 0, &data, true);

    uli_fs_enum(surf->fs, addMenuItem, addMenuItemsDone, MOVE(data));
}

typedef struct
{
    Surf* surf;
    char* last;
} GoBackDirDoneData;

static void onGoBackDirDone(void* data)
{
    GoBackDirDoneData* goBackDirDoneData = data;
    Surf* surf = goBackDirDoneData->surf;

    char current[ULINAME_MAX];
    uli_fs_dir(surf->fs, current);

    for(s32 i = 0; i < surf->menu.count; i++)
    {
        const SurfItem* item = &surf->menu.items[i];

        if(item->dir)
        {
            char path[ULINAME_MAX];

            if(strlen(current))
                sprintf(path, "%s/%s", current, item->name);
            else strcpy(path, item->name);

            if(strcmp(path, goBackDirDoneData->last) == 0)
            {
                surf->menu.pos = i;
                break;
            }
        }
    }

    free(goBackDirDoneData->last);
    free(goBackDirDoneData);

    surf->anim.movie = resetMovie(&surf->anim.goback.show);
}

static void onGoBackDir(void* data)
{
    Surf* surf = data;
    char last[ULINAME_MAX];
    uli_fs_dir(surf->fs, last);

    uli_fs_dirback(surf->fs);

    GoBackDirDoneData goBackDirDoneData = {surf, strdup(last)};
    initItemsAsync(surf, onGoBackDirDone, MOVE(goBackDirDoneData));
}

static void onGoToDirDone(void* data)
{
    Surf* surf = data;
    surf->anim.movie = resetMovie(&surf->anim.gotodir.show);
}

static void onGoToDir(void* data)
{
    Surf* surf = data;
    SurfItem* item = getMenuItem(surf);

    uli_fs_changedir(surf->fs, item->name);
    initItemsAsync(surf, onGoToDirDone, surf);
}

static void goBackDir(Surf* surf)
{
    char dir[ULINAME_MAX];
    uli_fs_dir(surf->fs, dir);

    if(strcmp(dir, "") != 0)
    {
        playSystemSfx(surf->studio, 2);

        surf->anim.movie = resetMovie(&surf->anim.goback.hide);
    }
}

static void changeDirectory(Surf* surf, const char* name)
{
    if (strcmp(name, "..") == 0)
    {
        goBackDir(surf);
    }
    else
    {
        playSystemSfx(surf->studio, 2);
        surf->anim.movie = resetMovie(&surf->anim.gotodir.hide);
    }
}

static void autoSave(Surf* surf)
{
    const char* save_directory = "/downloads";
    const char* cart_name = surf->console->rom.name;

    if(!uli_fs_isdir(surf->console->fs, save_directory))
    {
        uli_fs_makedir(surf->console->fs, save_directory);
    }

    forceAutoSave(surf->console, cart_name);
}

static void onCartLoaded(void* data)
{
    Surf* surf = data;

    if(surf->config->data.options.autosave)
    {
        autoSave(surf);
    }

    runGame(surf->studio);
}

static void onLoadCommandConfirmed(Studio* studio, bool yes, void* data)
{
    if(yes)
    {
        Surf* surf = data;
        SurfItem* item = getMenuItem(surf);

        if (item->hash)
        {
            surf->console->loadByHash(surf->console, item->name, item->hash, NULL, onCartLoaded, surf);
        }
        else
        {
            surf->console->load(surf->console, item->name);
            runGame(surf->studio);
        }
    }
}

static void onPlayCart(void* data)
{
    Surf* surf = data;
    SurfItem* item = getMenuItem(surf);

    studioCartChanged(surf->studio)
        ? confirmLoadCart(surf->studio, onLoadCommandConfirmed, surf)
        : onLoadCommandConfirmed(surf->studio, true, surf);
}

static void loadCart(Surf* surf)
{
    SurfItem* item = getMenuItem(surf);

    if(uli_tool_has_ext(item->name, PngExt))
    {
        s32 size = 0;
        void* data = uli_fs_load(surf->fs, item->name, &size);

        if(data)
        {
            uli_cartridge* cart = loadPngCart((png_buffer){data, size});

            if(cart)
            {
                surf->anim.movie = resetMovie(&surf->anim.play);
                free(cart);
            }
        }
    }
    else surf->anim.movie = resetMovie(&surf->anim.play);
}

static void move(Surf* surf, s32 dir)
{
    surf->menu.target = (surf->menu.pos + surf->menu.count + dir) % surf->menu.count;

    Anim* anim = surf->anim.move.items;
    anim->end = (surf->menu.target - surf->menu.pos) * MENU_HEIGHT;

    surf->anim.movie = resetMovie(&surf->anim.move);
}

static void processGamepad(Surf* surf)
{
    uli_mem* uli = surf->uli;

    enum{Frames = MENU_HEIGHT};

    {
        enum{Hold = KEYBOARD_HOLD, Period = Frames};

        enum
        {
            Up, Down, Left, Right, A, B, X, Y
        };

        if(uli_api_btnp(uli, Up, Hold, Period)
            || uli_api_keyp(uli, uli_key_up, Hold, Period))
        {
            move(surf, -1);
            playSystemSfx(surf->studio, 2);
        }
        else if(uli_api_btnp(uli, Down, Hold, Period)
            || uli_api_keyp(uli, uli_key_down, Hold, Period))
        {
            move(surf, +1);
            playSystemSfx(surf->studio, 2);
        }
        else if(uli_api_btnp(uli, Left, Hold, Period)
            || uli_api_keyp(uli, uli_key_left, Hold, Period)
            || uli_api_keyp(uli, uli_key_pageup, Hold, Period))
        {
            s32 dir = -PAGE;

            if(surf->menu.pos == 0) dir = -1;
            else if(surf->menu.pos <= PAGE) dir = -surf->menu.pos;

            move(surf, dir);
        }
        else if(uli_api_btnp(uli, Right, Hold, Period)
            || uli_api_keyp(uli, uli_key_right, Hold, Period)
            || uli_api_keyp(uli, uli_key_pagedown, Hold, Period))
        {
            s32 dir = +PAGE, last = surf->menu.count - 1;

            if(surf->menu.pos == last) dir = +1;
            else if(surf->menu.pos + PAGE >= last) dir = last - surf->menu.pos;

            move(surf, dir);
        }

        if(uli_api_btnp(uli, A, -1, -1)
            || ticEnterWasPressed(uli, -1, -1))
        {
            SurfItem* item = getMenuItem(surf);
            item->dir
                ? changeDirectory(surf, item->name)
                : loadCart(surf);
        }

        if(uli_api_btnp(uli, B, -1, -1)
            || uli_api_keyp(uli, uli_key_backspace, -1, -1))
        {
            if(uli_fs_isroot(surf->fs)) setStudioMode(surf->studio, ULI_CONSOLE_MODE);
            else goBackDir(surf);
        }

#ifdef CAN_OPEN_URL

        if(uli_api_btnp(uli, Y, -1, -1))
        {
            SurfItem* item = getMenuItem(surf);

            if(!item->dir)
            {
                char url[ULINAME_MAX];
                sprintf(url, ULI_WEBSITE "/play?cart=%i", item->id);
                uli_sys_open_url(url);
            }
        }
#endif

    }

}

static inline bool isIdle(Surf* surf)
{
    return surf->anim.movie == &surf->anim.idle;
}

static void tick(Surf* surf)
{
    processAnim(surf->anim.movie, surf);

    if(!surf->init)
    {
        initItemsAsync(surf, NULL, NULL);
        surf->anim.movie = resetMovie(&surf->anim.show);
        surf->init = true;
    }

    uli_mem* uli = surf->uli;
    uli_api_cls(uli, ULI_COLOR_BG);

    studio_menu_anim(surf->uli, surf->ticks++);

    if (isIdle(surf) && surf->menu.count > 0)
    {
        processGamepad(surf);
        if(uli_api_keyp(uli, uli_key_escape, -1, -1))
            setStudioMode(surf->studio, ULI_CONSOLE_MODE);
    }

    if (getStudioMode(surf->studio) != ULI_SURF_MODE) return;

    if (surf->menu.count > 0)
    {
        loadCover(surf);

        uli_screen* cover = getMenuItem(surf)->cover;

        if(cover)
            memcpy(uli->ram->vram.screen.data, cover->data, sizeof(uli_screen));
    }

    VBANK(uli, 1)
    {
        uli_api_cls(uli, uli->ram->vram.vars.clear = uli_color_yellow);
        memcpy(uli->ram->vram.palette.data, getConfig(surf->studio)->cart->bank0.palette.vbank0.data, sizeof(uli_palette));

        if(surf->menu.count > 0)
        {
            drawMenu(surf, surf->anim.val.menuX, (ULI78_HEIGHT - MENU_HEIGHT)/2);
        }
        else if(!surf->loading)
        {
            static const char Label[] = "You don't have any files...";
            s32 size = uli_api_print(uli, Label, 0, -ULI_FONT_HEIGHT, uli_color_white, true, 1, false);
            uli_api_print(uli, Label, (ULI78_WIDTH - size) / 2, (ULI78_HEIGHT - ULI_FONT_HEIGHT)/2, uli_color_white, true, 1, false);
        }

        drawTopToolbar(surf, 0, surf->anim.val.topBarY - MENU_HEIGHT);
        drawBottomToolbar(surf, 0, ULI78_HEIGHT - surf->anim.val.bottomBarY);
    }
}

static void resume(Surf* surf)
{
    surf->anim.movie = resetMovie(&surf->anim.show);
}

static void scanline(uli_mem* uli, s32 row, void* data)
{
    Surf* surf = (Surf*)data;

    if(surf->menu.count > 0)
    {
        const SurfItem* item = getMenuItem(surf);

        if(item->palette)
        {
            if(row == 0)
            {
                memcpy(&uli->ram->vram.palette, item->palette, sizeof(uli_palette));
                fadePalette(&uli->ram->vram.palette, surf->anim.val.coverFade);
            }

            return;
        }
    }

    studio_menu_anim_scanline(uli, row, NULL);
}

static void emptyDone(void* data) {}

static void setIdle(void* data)
{
    Surf* surf = data;
    surf->anim.movie = resetMovie(&surf->anim.idle);
}

static void setLeftShow(void* data)
{
    Surf* surf = data;
    surf->anim.movie = resetMovie(&surf->anim.gotodir.show);
}

static void freeAnim(Surf* surf)
{
    FREE(surf->anim.show.items);
    FREE(surf->anim.play.items);
    FREE(surf->anim.move.items);
    FREE(surf->anim.gotodir.show.items);
    FREE(surf->anim.gotodir.hide.items);
    FREE(surf->anim.goback.show.items);
    FREE(surf->anim.goback.hide.items);
}

static void moveDone(void* data)
{
    Surf* surf = data;
    surf->menu.pos = surf->menu.target;
    surf->anim.val.pos = 0;
    surf->anim.movie = resetMovie(&surf->anim.idle);
}

void initSurf(Surf* surf, Studio* studio, struct Console* console)
{
    freeAnim(surf);

    *surf = (Surf)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .console = console,
        .config = console->config,
        .fs = console->fs,
        .net = console->net,
        .tick = tick,
        .ticks = 0,
        .init = false,
        .loading = true,
        .resume = resume,
        .menu =
        {
            .pos = 0,
            .items = NULL,
            .count = 0,
        },
        .anim =
        {
            .idle = {.done = emptyDone,},

            .show = MOVIE_DEF(ANIM, setIdle,
            {
                {0, MENU_HEIGHT, ANIM, &surf->anim.val.topBarY, AnimEaseIn},
                {0, MENU_HEIGHT, ANIM, &surf->anim.val.bottomBarY, AnimEaseIn},
                {-ULI78_WIDTH, 0, ANIM, &surf->anim.val.menuX, AnimEaseIn},
                {0, MENU_HEIGHT, ANIM, &surf->anim.val.menuHeight, AnimEaseIn},
                {COVER_FADEOUT, COVER_FADEIN, ANIM, &surf->anim.val.coverFade, AnimEaseIn},
            }),

            .play = MOVIE_DEF(ANIM, onPlayCart,
            {
                {MENU_HEIGHT, 0, ANIM, &surf->anim.val.topBarY, AnimEaseIn},
                {MENU_HEIGHT, 0, ANIM, &surf->anim.val.bottomBarY, AnimEaseIn},
                {0, -ULI78_WIDTH, ANIM, &surf->anim.val.menuX, AnimEaseIn},
                {MENU_HEIGHT, 0, ANIM, &surf->anim.val.menuHeight, AnimEaseIn},
                {COVER_FADEIN, COVER_FADEOUT, ANIM, &surf->anim.val.coverFade, AnimEaseIn},
            }),

            .move = MOVIE_DEF(9, moveDone, {{0, 0, 9, &surf->anim.val.pos, AnimLinear}}),

            .gotodir =
            {
                .show = MOVIE_DEF(ANIM, setIdle,
                {
                    {ULI78_WIDTH, 0, ANIM, &surf->anim.val.menuX, AnimEaseIn},
                    {0, MENU_HEIGHT, ANIM, &surf->anim.val.menuHeight, AnimEaseIn},
                }),

                .hide = MOVIE_DEF(ANIM, onGoToDir,
                {
                    {0, -ULI78_WIDTH, ANIM, &surf->anim.val.menuX, AnimEaseIn},
                    {MENU_HEIGHT, 0, ANIM, &surf->anim.val.menuHeight, AnimEaseIn},
                }),
            },

            .goback =
            {
                .show = MOVIE_DEF(ANIM, setIdle,
                {
                    {-ULI78_WIDTH, 0, ANIM, &surf->anim.val.menuX, AnimEaseIn},
                    {0, MENU_HEIGHT, ANIM, &surf->anim.val.menuHeight, AnimEaseIn},
                }),

                .hide = MOVIE_DEF(ANIM, onGoBackDir,
                {
                    {0, ULI78_WIDTH, ANIM, &surf->anim.val.menuX, AnimEaseIn},
                    {MENU_HEIGHT, 0, ANIM, &surf->anim.val.menuHeight, AnimEaseIn},
                }),
            },
        },
        .scanline = scanline,
    };

    surf->anim.movie = resetMovie(&surf->anim.idle);

    uli_fs_makedir(surf->fs, ULI_CACHE);
}

void freeSurf(Surf* surf)
{
    freeAnim(surf);
    resetMenu(surf);
    free(surf);
}
