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

#include <uli78_types.h>
#include <string.h>

typedef bool(*fs_list_callback)(const char* name, const char* title, const char* hash, s32 id, void* data, bool dir);
typedef void(*fs_done_callback)(void* data);
typedef void(*fs_isdir_callback)(bool dir, void* data);
typedef void(*fs_load_callback)(const u8* buffer, s32 size, void* data);

typedef struct uli_fs uli_fs;
struct uli_net;

uli_fs*     uli_fs_create   (const char* path, struct uli_net* net);
const char* uli_fs_path     (uli_fs* fs, const char* name);
const char* uli_fs_pathroot (uli_fs* fs, const char* name);

void    uli_fs_enum         (uli_fs* fs, fs_list_callback onItem, fs_done_callback onDone, void* data);
void    uli_fs_isdir_async  (uli_fs* fs, const char* name, fs_isdir_callback callback, void* data);
void    uli_fs_hashload     (uli_fs* fs, const char* name, const char* hash, fs_load_callback callback, void* data);
bool    uli_fs_delfile      (uli_fs* fs, const char* name);
bool    uli_fs_deldir       (uli_fs* fs, const char* name);
bool    uli_fs_save         (uli_fs* fs, const char* name, const void* data, s32 size, bool overwrite);
bool    uli_fs_saveroot     (uli_fs* fs, const char* name, const void* data, s32 size, bool overwrite);
void*   uli_fs_load         (uli_fs* fs, const char* name, s32* size);
void*   uli_fs_loadroot     (uli_fs* fs, const char* name, s32* size);
bool    uli_fs_makedir      (uli_fs* fs, const char* name);
bool    uli_fs_exists       (uli_fs* fs, const char* name);
void    uli_fs_openfolder   (uli_fs* fs);
bool    uli_fs_isdir        (uli_fs* fs, const char* dir);
bool    uli_fs_isroot       (uli_fs* fs);
bool    uli_fs_ispubdir     (uli_fs* fs);
void    uli_fs_changedir    (uli_fs* fs, const char* dir);
void    uli_fs_dir          (uli_fs* fs, char* out);
void    uli_fs_dirback      (uli_fs* fs);
void    uli_fs_homedir      (uli_fs* fs);

u64     fs_date     (const char* name);
bool    fs_exists   (const char* name);
bool    fs_isdir    (const char* path);
void*   fs_read     (const char* path, s32* size);
bool    fs_write    (const char* path, const void* data, s32 size);
void    fs_enum     (const char* path, fs_list_callback callback, void* data);

const char* fs_apppath();
const char* fs_appfolder();
