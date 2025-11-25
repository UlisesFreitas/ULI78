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

#pragma once

#include "api.h"

struct uli_script
{
    u8 id;
    const char* name;
    const char* fileExtension;
    const char* projectComment;
    struct
    {
        bool(*init)(uli_mem* memory, const char* code);
        void(*close)(uli_mem* memory);

        uli_tick tick;
        uli_boot boot;
        uli_blit_callback callback;
    };

    const uli_outline_item* (*getOutline)(const char* code, s32* size);
    void (*eval)(uli_mem* uli, const char* code);

    const char* blockCommentStart;
    const char* blockCommentEnd;
    const char* blockCommentStart2;
    const char* blockCommentEnd2;
    const char* blockStringStart;
    const char* blockStringEnd;
    const char* stdStringStartEnd;
    const char* singleComment;
    const char* blockEnd;

    const char* const *keywords;
    s32 keywordsCount;

    uli_lang_isalnum lang_isalnum;
    bool useStructuredEdition;
    bool useBinarySection;

    s32 api_keywordsCount;
    const char** api_keywords;

    struct uli_demo
    {
        const u8* data;
        s32 size;
        const char* name;
    } demo, mark, *demos;

};

typedef struct uli_script uli_script;

const uli_script* uli_get_script(uli_mem* memory);
void uli_add_script(const uli_script* script);
const uli_script** uli_scripts();

#define FOREACH_LANG(script) \
    for(const uli_script **MACROVAR(it) = uli_scripts(), *script = *MACROVAR(it); *MACROVAR(it); script = *++MACROVAR(it))

#define SCRIPT_CONFIG ScriptConfig

#if defined(ULI_RUNTIME_STATIC)
#define EXPORT_SCRIPT(X) CONCAT(X, SCRIPT_CONFIG)
#else
#define EXPORT_SCRIPT(X) SCRIPT_CONFIG
#endif
