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

#include "start.h"
#include "studio/fs.h"
#include "cart.h"

#if defined(__ULI_WINDOWS__)
#include <windows.h>
#else
#include <unistd.h>
#endif

static void reset(Start* start)
{
    // La animación original de parpadeo ha sido reemplazada.
    // Ahora hacemos una animación de cortinilla.
    s32 val = start->ticks * 2;
    s32 width = val * 2;

    // Dibuja dos rectángulos que se cierran hacia el centro.
    uli_api_rect(start->uli, 0, 0, width, ULI78_HEIGHT, 10);
    uli_api_rect(start->uli, ULI78_WIDTH - width, 0, width, ULI78_HEIGHT, 10);
}

static void drawHeader(Start* start)
{
    for(s32 i = 0; i < STUDIO_TEXT_BUFFER_SIZE; i++)
        uli_api_print(start->uli, (char[]){start->text[i], '\0'},
            (i % STUDIO_TEXT_BUFFER_WIDTH) * STUDIO_TEXT_WIDTH,
            (i / STUDIO_TEXT_BUFFER_WIDTH) * STUDIO_TEXT_HEIGHT,
            start->color[i], true, 1, false);
}

static void chime(Start* start)
{
    playSystemSfx(start->studio, 1);
}

static void stop_chime(Start* start)
{
    sfx_stop(start->uli, 0);
}

static void header(Start* start)
{
    drawHeader(start);
}

static void start_console(Start* start)
{
    drawHeader(start);
    setStudioMode(start->studio, ULI_CONSOLE_MODE);
}

static void tick(Start* start)
{
    // stages that have a tick count of 0 run in zero time
    // (typically this is only used to start/stop audio)
    while (start->stages[start->stage].ticks == 0) {
        start->stages[start->stage].fn(start);
        start->stage++;
    }

    uli_api_cls(start->uli, ULI_COLOR_BG);

    Stage *stage = &start->stages[start->stage];
    stage->fn(start);
    if (stage->ticks > 0) stage->ticks--;
    if (stage->ticks == 0) start->stage++;

    start->ticks++;
}

static void* _memmem(const void* haystack, size_t hlen, const void* needle, size_t nlen)
{
    const u8* p = haystack;
    size_t plen = hlen;

    if (!nlen) return NULL;

    s32 needle_first = *(u8*)needle;

    while (plen >= nlen && (p = memchr(p, needle_first, plen - nlen + 1)))
    {
        if (!memcmp(p, needle, nlen))
            return (void*)p;

        p++;
        plen = hlen - (p - (const u8*)haystack);
    }

    return NULL;
}

void initStart(Start* start, Studio* studio, const char* cart)
{
    enum duration {
        immediate = 0,
        one_second = ULI78_FRAMERATE,
        forever = -1
    };

    *start = (Start)
    {
        .studio = studio,
        .uli = getMemory(studio),
        .initialized = true,
        .tick = tick,
        .embed = false,
        .ticks = 0,
        .stage = 0,
        .stages =
        {
            { reset, .ticks = one_second },
            { chime, .ticks = immediate },
            { header, .ticks = one_second },
            { stop_chime, .ticks = immediate },
            { start_console, .ticks = forever },
        }
    };

    static const char* Header[] =
    {
        "",
        " " ULI_NAME_FULL,
        " version " ULI_VERSION,
        " " ULI_COPYRIGHT,
    };

    for(s32 i = 0; i < COUNT_OF(Header); i++)
        strcpy(&start->text[i * STUDIO_TEXT_BUFFER_WIDTH], Header[i]);

    for(s32 i = 0; i < STUDIO_TEXT_BUFFER_SIZE; i++)
        start->color[i] = uli_color_white;

#if defined(__EMSCRIPTEN__)

    if (cart)
    {
        s32 size = 0;
        void* data = fs_read(cart, &size);

        if(data) SCOPE(free(data))
        {
            uli_cart_load(&start->uli->cart, data, size);
            uli_api_reset(start->uli);
            start->embed = true;
        }
    }

#else

    {
        const char* appPath = fs_apppath();

        s32 appSize = 0;
        u8* app = fs_read(appPath, &appSize);

        if(app) SCOPE(free(app))
        {
            s32 size = appSize;
            const u8* ptr = app;

            while(true)
            {
                const EmbedHeader* header = (const EmbedHeader*)_memmem(ptr, size, CART_SIG, STRLEN(CART_SIG));

                if(header)
                {
                    if(appSize == header->appSize + sizeof(EmbedHeader) + header->cartSize)
                    {
                        u8* data = calloc(1, sizeof(uli_cartridge));

                        if(data)
                        {
                            s32 dataSize = uli_tool_unzip(data, sizeof(uli_cartridge), app + header->appSize + sizeof(EmbedHeader), header->cartSize);

                            if(dataSize)
                            {
                                uli_cart_load(&start->uli->cart, data, dataSize);
                                uli_api_reset(start->uli);
                                start->embed = true;
                            }

                            free(data);
                        }

                        break;
                    }
                    else
                    {
                        ptr = (const u8*)header + STRLEN(CART_SIG);
                        size = appSize - (s32)(ptr - app);
                    }
                }
                else break;
            }
        }
    }

#endif
}

void freeStart(Start* start)
{
    free(start);
}
