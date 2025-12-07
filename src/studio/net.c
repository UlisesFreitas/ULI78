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

#include "net.h"
#include "defines.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define URL_SIZE 2048





#if defined(USE_NAETT)

#include <naett.h>

typedef struct
{
    naettReq* req;
    naettRes* res;
    char url[URL_SIZE];

    net_get_callback callback;
    void* calldata;
} HttpGet;

struct uli_net
{
    char host[URL_SIZE];

    HttpGet** requests;
    s32 count;
};

#if defined(__ANDROID__)
#include <jni.h>
JNIEnv *Android_JNI_GetEnv();
#endif

uli_net* uli_net_create(const char* host)
{
#if defined(__ANDROID__)
    JNIEnv *env = Android_JNI_GetEnv();
    JavaVM *vm = NULL;
    (*env)->GetJavaVM(env, &vm);

    naettInit(vm);
#else
    naettInit(NULL);
#endif

    uli_net* net = NEW(uli_net);
    memset(net, 0, sizeof(uli_net));

    strcpy(net->host, host);

    return net;
}

void uli_net_get(uli_net* net, const char* url, net_get_callback callback, void* calldata)
{
    HttpGet* get = NEW(HttpGet);
    memset(get, 0, sizeof *get);

    sprintf(get->url, "%s%s", net->host, url);

    get->req = naettRequest(get->url, naettMethod("GET"), naettHeader("accept", "*/*"));
    get->res = naettMake(get->req);
    get->callback = callback;
    get->calldata = calldata;

    net->requests = realloc(net->requests, sizeof *net->requests * ++net->count);
    net->requests[net->count - 1] = get;
}

void uli_net_close(uli_net* net)
{
    for(s32 i = 0; i < net->count; i++)
    {
        HttpGet *it = net->requests[i];

        if(it)
        {
            naettClose(it->res);
            naettFree(it->req);
            free(it);
        }
    }

    if(net->requests)
        free(net->requests);
}

void uli_net_start(uli_net *net) {}

void uli_net_end(uli_net *net)
{
    if(!net->requests)
        return;

    for(s32 i = 0; i < net->count; i++)
    {
        const HttpGet *it = net->requests[i];

        if(it && naettComplete(it->res))
        {
            s32 status = naettGetStatus(it->res);

            net_get_data getData =
            {
                .calldata = it->calldata,
                .url = it->url,
            };

            if(status == 200)
            {
                getData.type = net_get_done;
                getData.done.data = (u8*)naettGetBody(it->res, &getData.done.size);
            }
            else
            {
                getData.type = net_get_error;
                getData.error.code = status;
            }

            it->callback(&getData);

            naettClose(it->res);
            naettFree(it->req);

            free(net->requests[i]);
            net->requests[i] = NULL;
        }
    }
}

#else

uli_net* uli_net_create(const char* host) {return NULL;}
void uli_net_get(uli_net* net, const char* url, net_get_callback callback, void* calldata) {}
void uli_net_close(uli_net* net) {}
void uli_net_start(uli_net *net) {}
void uli_net_end(uli_net *net) {}

#endif
