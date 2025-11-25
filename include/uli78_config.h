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

#if defined(__APPLE__)
// TODO: this disables macos config
#   include "AvailabilityMacros.h"
#   include "TargetConditionals.h"
// #    ifndef TARGET_OS_IPHONE
#       undef __ULI_MACOSX__
#       define __ULI_MACOSX__ 1
#       define ULI_MODULE_EXT ".dylib"
// #    endif /* TARGET_OS_IPHONE */
#endif /* defined(__APPLE__) */

#if !defined(__LIBRETRO__)
#   if defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#       undef __ULI_WINDOWS__
#       define __ULI_WINDOWS__ 1
#       define ULI_MODULE_EXT ".dll"
#       if defined(_MSC_VER) && defined(_USING_V110_SDK71_)
#           define __ULI_WIN7__ 1
#       endif
#   endif
#   if defined(ANDROID) || defined(__ANDROID__)
#       undef __ULI_ANDROID__
#       define __ULI_ANDROID__ 1
#       define ULI_MODULE_EXT ".so"
#   elif (defined(linux) || defined(__linux) || defined(__linux__))
#       undef __ULI_LINUX__
#       define __ULI_LINUX__ 1
#       define ULI_MODULE_EXT ".so"
#   endif
#endif

#if defined(ULI_RUNTIME_STATIC)
#   define ULI_EXPORT
#else
#   if defined(__ULI_WINDOWS__)
#       define ULI_EXPORT __declspec(dllexport)
#   elif defined(__GNUC__) && __GNUC__ >= 4
#       define ULI_EXPORT __attribute__ ((visibility("default")))
#   endif
#endif

#ifndef ULI78_API
#   if defined(ULI78_SHARED)
#       if defined(__ULI_WINDOWS__)
#           define ULI78_API __declspec(dllexport)
#       elif defined(__ULI_LINUX__)
#           define ULI78_API __attribute__ ((visibility("default")))
#       endif
#   else
#       define ULI78_API
#   endif
#endif

#if defined(ANDROID) || defined(__ANDROID__) || defined(BAREMETALPI) || defined(_3DS)
#   define ULI78_FFT_UNSUPPORTED 1
#endif
