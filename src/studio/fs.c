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

#include "studio.h"
#include "fs.h"
#include "net.h"
#include "ext/json.h"

#if defined(BAREMETALPI) || defined(_3DS)
  #ifdef EN_DEBUG
    #define dbg(...) printf(__VA_ARGS__)
  #else
    #define dbg(...)
  #endif
#endif

#if defined(BAREMETALPI)
#include "../../circle-stdlib/libs/circle/addon/fatfs/ff.h"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if defined(__ULI_WINDOWS__)
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#if defined(__ULI_WINDOWS__)
#define SLASH_SYMBOL ('\\')
#else
#define SLASH_SYMBOL ('/')
#endif

#if defined (__ULI_MACOSX__)
#include <mach-o/dyld.h>
#endif

static const char* PublicDir = ULI_HOST;

struct uli_fs
{
    char dir[ULINAME_MAX];
    char work[ULINAME_MAX];
    uli_net* net;
};

#if defined(__EMSCRIPTEN__)
void syncfs()
{
    EM_ASM({Module.syncFSRequests++;});
}
#endif

const char* uli_fs_pathroot(uli_fs* fs, const char* name)
{
    static char path[ULINAME_MAX];

    snprintf(path, sizeof path, "%s%s", fs->dir, name);

#if defined(__ULI_WINDOWS__)
    char* ptr = path;
    while (*ptr)
    {
        if (*ptr == '/') *ptr = SLASH_SYMBOL;
        ptr++;
    }
#endif

    return path;
}

const char* uli_fs_path(uli_fs* fs, const char* name)
{
    static char path[ULINAME_MAX+1];

    if(*name == '/')
        strncpy(path, name + 1, sizeof path);
    else if(strlen(fs->work))
        snprintf(path, sizeof path, "%s/%s", fs->work, name);
    else
        strncpy(path, name, sizeof path);

    return uli_fs_pathroot(fs, path);
}

static bool isRoot(uli_fs* fs)
{
    return fs->work[0] == '\0';
}

bool uli_fs_isroot(uli_fs* fs)
{
    return isRoot(fs);
}

static bool isPublicRoot(uli_fs* fs)
{
    return strcmp(fs->work, PublicDir) == 0;
}


static bool isPublic(uli_fs* fs)
{
    return memcmp(fs->work, PublicDir, STRLEN(PublicDir)) == 0;
}

bool uli_fs_ispubdir(uli_fs* fs)
{
    return isPublic(fs);
}

#if defined(__ULI_WINDOWS__)

typedef wchar_t FsString;

#define __S(x) L ## x
#define _S(x) __S(x)

static const FsString* utf8ToString(const char* str)
{
    FsString* wstr = malloc(ULINAME_MAX * sizeof(FsString));

    MultiByteToWideChar(CP_UTF8, 0, str, ULINAME_MAX, wstr, ULINAME_MAX);

    return wstr;
}

static const char* stringToUtf8(const FsString* wstr)
{
    char* str = malloc(ULINAME_MAX * sizeof(char));

    WideCharToMultiByte(CP_UTF8, 0, wstr, ULINAME_MAX, str, ULINAME_MAX, 0, 0);

    return str;
}

#if defined(__ULI_WIN7__)

time_t FileTimeToTimeT(FILETIME* ft) {
    ULARGE_INTEGER ull;
    ull.LowPart = ft->dwLowDateTime;
    ull.HighPart = ft->dwHighDateTime;
    return (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);
}

// There is a bug in the toolchain when compiling with v141_xp
// This shim is a workaround for that, using GetFileAttributesEx
// see https://stackoverflow.com/questions/32452777/visual-c-2015-express-stat-not-working-on-windows-xp
static int _wstat_win32_shim(const wchar_t* path, struct _stat* buffer)
{
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &fileInfo))
    {
        return -1;
    }

    memset(buffer, 0, sizeof(struct _stat));

    buffer->st_mode = _S_IREAD;
    if (!(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
    {
        buffer->st_mode |= _S_IWRITE;
    }
    if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        buffer->st_mode |= _S_IFDIR;
    }
    else
    {
        buffer->st_mode |= _S_IFREG;
    }

    // Set the file size
    buffer->st_size = ((_off_t)fileInfo.nFileSizeHigh << 32) | fileInfo.nFileSizeLow;

    // Set the file times
    buffer->st_mtime = FileTimeToTimeT(&fileInfo.ftLastWriteTime);
    buffer->st_atime = FileTimeToTimeT(&fileInfo.ftLastAccessTime);
    buffer->st_ctime = FileTimeToTimeT(&fileInfo.ftCreationTime);
    return 0;
}

#endif

#define freeString(S) free((void*)S)


#define ULI_DIR _WDIR
#define uli_dirent _wdirent
#define uli_stat_struct _stat

#define uli_opendir _wopendir
#define uli_readdir _wreaddir
#define uli_closedir _wclosedir
#define uli_rmdir _wrmdir

// use the shim (see above) if we're targeting Windows XP
#if defined(__ULI_WIN7__)
    #define uli_stat _wstat_win32_shim
#else
    #define uli_stat _wstat
#endif
#define uli_remove _wremove
#define uli_fopen _wfopen
#define uli_mkdir(name) _wmkdir(name)
#define uli_strncpy wcsncpy
#define uli_strncat wcsncat
#define uli_strlen wcslen

#else

typedef char FsString;

#define _S(x) (x)

#define utf8ToString(S) (S)
#define stringToUtf8(S) (S)

#define freeString(S)

#define ULI_DIR DIR
#define uli_dirent dirent
#define uli_stat_struct stat

#define uli_opendir opendir
#define uli_readdir readdir
#define uli_closedir closedir
#define uli_rmdir rmdir
#define uli_stat stat
#define uli_remove remove
#define uli_fopen fopen
#define uli_mkdir(name) mkdir(name, 0777)
#define uli_strncpy strncpy
#define uli_strncat strncat
#define uli_strlen strlen

#endif

typedef struct
{
    fs_list_callback item;
    fs_done_callback done;
    void* data;
} NetDirData;

static void onDirResponse(const net_get_data* netData)
{
    NetDirData* netDirData = (NetDirData*)netData->calldata;

    if(netData->type == net_get_done)
    {
        if(json_parse((char*)netData->done.data, netData->done.size))
        {
            typedef char string[ULINAME_MAX];

            string name, hash, filename;

            s32 folders = json_array("folders", 0);
            for(s32 i = 0, size = json_array_size(folders); i < size; i++)
            {
                if(json_string("name", json_array_item(folders, i), name, sizeof name))
                    netDirData->item(name, NULL, NULL, 0, netDirData->data, true);
            }

            s32 files = json_array("files", 0);
            for(s32 i = 0, size = json_array_size(files); i < size; i++)
            {
                s32 item = json_array_item(files, i);

                if(json_string("name", item, name, sizeof name)
                    && json_string("hash", item, hash, sizeof hash)
                    && json_string("filename", item, filename, sizeof filename))
                    netDirData->item(filename, name, hash, json_int("id", item), netDirData->data, false);
            }
        }
    }

    switch (netData->type)
    {
    case net_get_done:
    case net_get_error:
        netDirData->done(netDirData->data);
        free(netDirData);
        break;
    default: break;
    }
}

void fs_enum(const char* path, fs_list_callback callback, void* data)
{
#if defined(BAREMETALPI)
    dbg("fs_enum %s", path);

        if (path && *path) {
        // ok
    }
    else
    {
        return;
    }

    static char path2[ULINAME_MAX];
    strncpy(path2, path, sizeof path2);

    if (path2[strlen(path2) - 1] == '/')    // one character
                path2[strlen(path2) - 1] = 0;

    dbg("fs_enum Real %s", path2);


    DIR Directory;
    FILINFO FileInfo;
    FRESULT Result = f_findfirst (&Directory, &FileInfo, path2, "*");
    dbg("enumFilesRes %d", Result);

    for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
    {
        if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
        {
            bool result = callback(FileInfo.fname, NULL, NULL, 0, data, FileInfo.fattrib & AM_DIR);

            if (!result)
            {
                break;
            }
        }

        Result = f_findnext (&Directory, &FileInfo);
    }

#else
    ULI_DIR *dir = NULL;
    struct uli_dirent* ent = NULL;

    const FsString* pathString = utf8ToString(path);

    if ((dir = uli_opendir(pathString)) != NULL)
    {
        FsString fullPath[ULINAME_MAX] = {0};
        struct uli_stat_struct s;

        while ((ent = uli_readdir(dir)) != NULL)
        {
            if(*ent->d_name != _S('.'))
            {
				size_t pathLen = uli_strlen(pathString);
				size_t nameLen = uli_strlen(ent->d_name);

				if (pathLen + nameLen < COUNT_OF(fullPath)) {
					uli_strncpy(fullPath, pathString, COUNT_OF(fullPath));
					uli_strncat(fullPath, ent->d_name, COUNT_OF(fullPath) - pathLen - 1);
				}

                if(uli_stat(fullPath, &s) == 0)
                {
                    const char* name = stringToUtf8(ent->d_name);
                    bool result = callback(name, NULL, NULL, 0, data, S_ISDIR(s.st_mode));
                    freeString(name);

                    if(!result) break;
                }
            }
        }

        uli_closedir(dir);
    }

    freeString(pathString);
#endif
}

void uli_fs_enum(uli_fs* fs, fs_list_callback onItem, fs_done_callback onDone, void* data)
{
#ifndef BAREMETALPI
    if (isRoot(fs) && !onItem(PublicDir, NULL, NULL, 0, data, true))
    {
        onDone(data);
        return;
    }

#if defined(BUILD_EDITORS)
    if(isPublic(fs))
    {
        char request[ULINAME_MAX];
        snprintf(request, sizeof request, "/json?fn=dir&path=%s", fs->work + sizeof(ULI_HOST));

        NetDirData netDirData = { onItem, onDone, data };
        uli_net_get(fs->net, request, onDirResponse, MOVE(netDirData));

        return;
    }
#endif

#endif

    const char* path = uli_fs_path(fs, "");

    fs_enum(path, onItem, data);

    onDone(data);
}

bool uli_fs_deldir(uli_fs* fs, const char* name)
{
#if defined(BAREMETALPI)
    const char* path = uli_fs_path(fs, name);
    const FsString* pathString = utf8ToString(path);
    bool result = (f_unlink(pathString) != FR_OK);
    freeString(pathString);

    return result;
#else
#if defined(__ULI_WINDOWS__)
    const char* path = uli_fs_path(fs, name);

    const FsString* pathString = utf8ToString(path);
    bool result = uli_rmdir(pathString);
    freeString(pathString);

#else
    bool result = rmdir(uli_fs_path(fs, name));
#endif

#if defined(__EMSCRIPTEN__)
    syncfs();
#endif

    return result;
#endif
}

bool uli_fs_delfile(uli_fs* fs, const char* name)
{
    const char* path = uli_fs_path(fs, name);

    const FsString* pathString = utf8ToString(path);
#if defined(BAREMETALPI)
    bool result = (f_unlink(pathString) != FR_OK);
#else
    bool result = uli_remove(pathString);
#endif
    freeString(pathString);

#if defined(__EMSCRIPTEN__)
    syncfs();
#endif

    return result;
}

void uli_fs_homedir(uli_fs* fs)
{
    memset(fs->work, 0, sizeof fs->work);
}

void uli_fs_dirback(uli_fs* fs)
{
    if(isPublicRoot(fs))
    {
        uli_fs_homedir(fs);
        return;
    }

    char* start = fs->work;
    char* ptr = start + strlen(fs->work);

    while (ptr > start && *ptr != '/') ptr--;
    while (*ptr) *ptr++ = '\0';
}

void uli_fs_dir(uli_fs* fs, char* dir)
{
	snprintf(dir, ULINAME_MAX, "%s", fs->work);
}

void uli_fs_changedir(uli_fs* fs, const char* dir)
{
    char temp[ULINAME_MAX+1];

    if (strlen(fs->work) > 0) {
        snprintf(temp, ULINAME_MAX+1, "%s/%s", fs->work, dir);
    } else {
        snprintf(temp, ULINAME_MAX, "%s", dir);
    }

    strncpy(fs->work, temp, ULINAME_MAX - 1);

#if defined(__ULI_WINDOWS__)
    for(char *ptr = fs->work, *end = ptr + strlen(ptr); ptr < end; ptr++)
        if(*ptr == SLASH_SYMBOL)
            *ptr = '/';
#endif
}

typedef struct
{
    char* name;
    bool found;
    fs_isdir_callback done;
    void* data;

} EnumPublicDirsData;

static bool onEnumPublicDirs(const char* name, const char* title, const char* hash, s32 id, void* data, bool dir)
{
    EnumPublicDirsData* enumPublicDirsData = (EnumPublicDirsData*)data;

    if(strcmp(name, enumPublicDirsData->name) == 0)
    {
        enumPublicDirsData->found = true;
        return false;
    }

    return true;
}

static void onEnumPublicDirsDone(void* data)
{
    EnumPublicDirsData* enumPublicDirsData = data;
    enumPublicDirsData->done(enumPublicDirsData->found, enumPublicDirsData->data);
    free(enumPublicDirsData->name);
    free(enumPublicDirsData);
}

bool fs_isdir(const char* path)
{
#if defined(BAREMETALPI)
    FILINFO s;
    FRESULT res = f_stat(path, &s);
    if (res != FR_OK) return false;
    return s.fattrib & AM_DIR;
#else
    struct uli_stat_struct s;
    const FsString* pathString = utf8ToString(path);
    bool isdir = uli_stat(pathString, &s) == 0 && S_ISDIR(s.st_mode);
    freeString(pathString);
    return isdir;
#endif
}

bool uli_fs_isdir(uli_fs* fs, const char* name)
{
    if (*name == '.') return false;
    if (isRoot(fs) && strcmp(name, PublicDir) == 0) return true;

#if defined(BAREMETALPI)
    dbg("fsIsDirSync %s\n", name);
    FILINFO s;
    const char* path = uli_fs_path(fs, name);
    FRESULT res = f_stat(path, &s);
    if (res != FR_OK) return false;

    return s.fattrib & AM_DIR;
#else
    return fs_isdir(uli_fs_path(fs, name));
#endif
}

void uli_fs_isdir_async(uli_fs* fs, const char* name, fs_isdir_callback callback, void* data)
{
    if(isPublic(fs))
    {
        EnumPublicDirsData enumPublicDirsData = { strdup(name), false, callback, data};
        uli_fs_enum(fs, onEnumPublicDirs, onEnumPublicDirsDone, MOVE(enumPublicDirsData));
        return;
    }

    callback(uli_fs_isdir(fs, name), data);
}

bool fs_write(const char* name, const void* buffer, s32 size)
{
#if defined(BAREMETALPI)
    dbg("fs_write %s\n", name);
    FIL File;
    FRESULT res = f_open (&File, name, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        return false;
    }

    u32 written=0;

    res = f_write(&File, buffer, size, &written);

    f_close(&File);
    if (res != FR_OK)
    {
        return false;
    }
    if(written!=size)
    {
        dbg("Write size diff %d %d", size, written);
        return false;
    }
    return true;
#else
    const FsString* pathString = utf8ToString(name);
    FILE* file = uli_fopen(pathString, _S("wb"));
    freeString(pathString);

    if(file)
    {
        fwrite(buffer, 1, size, file);
        fclose(file);

#if defined(__EMSCRIPTEN__)
        syncfs();
#endif

        return true;
    }

    return false;
#endif
}

void* fs_read(const char* path, s32* size)
{
#if defined(BAREMETALPI)
    dbg("fs_read %s\n", path);
    FILINFO fi;
    FRESULT res = f_stat(path, &fi);
    if(res!=FR_OK) return NULL;
    FIL file;
    res = f_open (&file, path, FA_READ | FA_OPEN_EXISTING);
    if(res!=FR_OK) return NULL;
    *size = fi.fsize; // size is in output!
    void* buffer = malloc(*size);
    UINT read = 0;
    res = f_read(&file, buffer, fi.fsize, &read);

    f_close(&file);
    if(read!=(*size)) return NULL;

    return buffer;

#else
    const FsString* pathString = utf8ToString(path);
    FILE* file = uli_fopen(pathString, _S("rb"));
    freeString(pathString);

    void* buffer = NULL;

    if(file)
    {

        fseek(file, 0, SEEK_END);
        *size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if((buffer = malloc(*size)) && fread(buffer, *size, 1, file)) {}

        fclose(file);
    }

    return buffer;

#endif
}

bool fs_exists(const char* name)
{
#if defined(BAREMETALPI)
    dbg("fs_exists %s\n", name);
    FILINFO s;

    FRESULT res = f_stat(name, &s);
    return res == FR_OK;
#else
    struct uli_stat_struct s;

    const FsString* pathString = utf8ToString(name);
    bool ret = uli_stat(pathString, &s) == 0;
    freeString(pathString);

    return ret;
#endif
}

bool uli_fs_exists(uli_fs* fs, const char* name)
{
    return fs_exists(uli_fs_path(fs, name));
}

u64 fs_date(const char* path)
{
#if defined(BAREMETALPI)
    dbg("fs_date %s\n", path);
    // TODO BAREMETALPI
    return 0;
#else
    struct uli_stat_struct s;

    const FsString* pathString = utf8ToString(path);
    s32 ret = uli_stat(pathString, &s);
    freeString(pathString);

    if(ret == 0 && S_ISREG(s.st_mode))
    {
        return s.st_mtime;
    }

    return 0;
#endif
}

bool uli_fs_save(uli_fs* fs, const char* name, const void* data, s32 size, bool overwrite)
{
    if(!overwrite)
    {
        if(uli_fs_exists(fs, name))
            return false;
    }

    return fs_write(uli_fs_path(fs, name), data, size);
}

bool uli_fs_saveroot(uli_fs* fs, const char* name, const void* data, s32 size, bool overwrite)
{
    const char* path = uli_fs_pathroot(fs, name);

    if(!overwrite)
    {
        if(fs_exists(path))
            return false;
    }

    return fs_write(path, data, size);
}

typedef struct
{
    uli_fs* fs;
    fs_load_callback done;
    void* data;
    char* cachePath;
} LoadFileByHashData;

static void fileByHashLoaded(const net_get_data* netData)
{
    LoadFileByHashData* loadFileByHashData = netData->calldata;

    if (netData->type == net_get_done)
    {
        uli_fs_saveroot(loadFileByHashData->fs, loadFileByHashData->cachePath, netData->done.data, netData->done.size, false);
        loadFileByHashData->done(netData->done.data, netData->done.size, loadFileByHashData->data);
    }

    switch (netData->type)
    {
    case net_get_done:
    case net_get_error:

        free(loadFileByHashData->cachePath);
        free(loadFileByHashData);
        break;
    default: break;
    }
}

void uli_fs_hashload(uli_fs* fs, const char* name, const char* hash, fs_load_callback callback, void* data)
{
#if defined(BAREMETALPI)
    // TODO BAREMETALPI
    return;
#else

    char cachePath[ULINAME_MAX];
    snprintf(cachePath, sizeof cachePath, ULI_CACHE "%s.uli", hash);

    {
        s32 size = 0;
        void* buffer = uli_fs_loadroot(fs, cachePath, &size);
        if (buffer)
        {
            callback(buffer, size, data);
            free(buffer);
            return;
        }
    }

#if defined(BUILD_EDITORS)
    char path[ULINAME_MAX];
    snprintf(path, sizeof path, "/cart/%s/%s", hash, name);

    LoadFileByHashData loadFileByHashData = { fs, callback, data, strdup(cachePath) };
    uli_net_get(fs->net, path, fileByHashLoaded, MOVE(loadFileByHashData));
#endif

#endif
}

void* uli_fs_load(uli_fs* fs, const char* name, s32* size)
{
#if defined(BAREMETALPI)
    dbg("uli_fs_load x %s\n", name);
    dbg("fs.dir %s\n", fs->dir);
    dbg("fs.work %s\n", fs->work);

    if(isPublic(fs))
    {
        dbg("Public ??\n");
        return NULL;
    }
    else
    {
        dbg("non public \n");
        const char* fp = uli_fs_path(fs, name);
        dbg("loading: %s\n", fp);


        FILINFO fi;
        FRESULT res = f_stat(fp, &fi);
        dbg("fstat done %d \n", res);

        if(res!=FR_OK)
        {
            dbg("NO F_STAT %d\n", res);
            return NULL;
        }
        FIL file;
        res = f_open (&file, fp, FA_READ | FA_OPEN_EXISTING);
        if(res!=FR_OK)
        {
            dbg("NO F_OPEN %d\n", res);
            return NULL;
        }
        dbg("BUFFERING %d\n", res);

        void* buffer = malloc(fi.fsize);
        dbg("BUFFERED %d\n", fi.fsize);

        UINT read = 0;
        res = f_read(&file, buffer, fi.fsize, &read);
        dbg("F_READ %d %ld\n", res, read);

        f_close(&file);
        if(read!=fi.fsize)
        {
            dbg("NO F_READ %d \n", res);
            return NULL;
        }
        dbg("RETURNING!!\n");
        *size = fi.fsize;
        return buffer;
    }
    return NULL;
#else

    const FsString* pathString = utf8ToString(uli_fs_path(fs, name));
    FILE* file = uli_fopen(pathString, _S("rb"));
    freeString(pathString);

    void* ptr = NULL;

    if(file)
    {
        fseek(file, 0, SEEK_END);
        *size = ftell(file);
        fseek(file, 0, SEEK_SET);

        u8* buffer = malloc(*size);

        if(buffer && fread(buffer, *size, 1, file)) ptr = buffer;

        fclose(file);
    }

    return ptr;

#endif
}

void* uli_fs_loadroot(uli_fs* fs, const char* name, s32* size)
{
    return fs_read(uli_fs_pathroot(fs, name), size);
}

bool uli_fs_makedir(uli_fs* fs, const char* name)
{
#if defined(BAREMETALPI)
    // TODO BAREMETALPI
    dbg("makeDir %s\n", name);

    const FsString* pathString = uli_fs_path(fs, name);
    char* path = strdup(pathString);
    if (path && *path) {                      // make sure result has at least
      if (path[strlen(path) - 1] == '/')    // one character
            path[strlen(path) - 1] = 0;
    }

    FRESULT res = f_mkdir(path);
    if(res != FR_OK)
    {
        dbg("Could not mkdir %s\n", name);
    }
    free(path);
    return (res != FR_OK);
#else

    const FsString* pathString = utf8ToString(uli_fs_path(fs, name));
    int result = uli_mkdir(pathString);
    freeString(pathString);

#if defined(__EMSCRIPTEN__)
    syncfs();
#endif
    return result;
#endif
}

void uli_fs_openfolder(uli_fs* fs)
{
    const char* path = uli_fs_path(fs, "");

    if(isPublic(fs))
        path = fs->dir;

    uli_sys_open_path(path);
}

#if defined(__ULI_WINDOWS__)
#define SEP "\\"
#else
#define SEP "/"
#endif

uli_fs* uli_fs_create(const char* path, uli_net* net)
{
    uli_fs* fs = (uli_fs*)calloc(1, sizeof(uli_fs));

    strncpy(fs->dir, path, ULINAME_MAX);

    if(path[strlen(path) - 1] != SEP[0])
        strcat(fs->dir, SEP);

    fs->net = net;

    return fs;
}

const char* fs_apppath()
{
    static char apppath[ULINAME_MAX];

#if defined(__ULI_WINDOWS__)
    {
        wchar_t wideAppPath[ULINAME_MAX];
        GetModuleFileNameW(NULL, wideAppPath, sizeof wideAppPath);
        WideCharToMultiByte(CP_UTF8, 0, wideAppPath, COUNT_OF(wideAppPath), apppath, COUNT_OF(apppath), 0, 0);
    }
#elif defined(__ULI_LINUX__)
    s32 size = readlink("/proc/self/exe", apppath, sizeof apppath);
    apppath[size] = '\0';
#elif defined(__ULI_MACOSX__)
    u32 size = sizeof apppath;
    _NSGetExecutablePath(apppath, &size);
#endif

    return apppath;
}

const char* fs_appfolder()
{
    static char appfolder[ULINAME_MAX];
    strcpy(appfolder, fs_apppath());

    char* pos = strrchr(appfolder, SLASH_SYMBOL);

    if(pos && pos[1])
        pos[1] = '\0';

    return appfolder;
}
