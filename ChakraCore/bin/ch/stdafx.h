//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if DBG
#else // DBG
#ifdef _MSC_VER
#pragma warning(disable: 4189)  // initialized but unused variable
#endif
#endif

#define Unused(var) (void)(var);

#define IfFailedReturn(EXPR) do { hr = (EXPR); if (FAILED(hr)) { return hr; }} while(FALSE)
#define IfFailedGoLabel(expr, label) do { hr = (expr); if (FAILED(hr)) { goto label; } } while (FALSE)
#define IfFailGo(expr) IfFailedGoLabel(hr = (expr), Error)

#define IfJsrtErrorFail(expr, ret) do { if ((expr) != JsNoError) return ret; } while (0)
#define IfJsrtErrorHR(expr) do { if((expr) != JsNoError) { hr = E_FAIL; goto Error; } } while(0)
#define IfJsrtError(expr) do { if((expr) != JsNoError) { goto Error; } } while(0)
#define IfJsrtErrorSetGo(expr) do { errorCode = (expr); if(errorCode != JsNoError) { hr = E_FAIL; goto Error; } } while(0)
#define IfFalseGo(expr) do { if(!(expr)) { hr = E_FAIL; goto Error; } } while(0)

// NOTE: Do NOT define ENABLE_TEST_HOOKS here for static builds.
// It forces ENABLE_DEBUG_CONFIG_OPTIONS (via CommonDefines.h), which changes
// the layout of Js::ConfigFlagsTable. If ch is compiled with it but the
// static library is not, struct layout mismatches cause ODR violations
// (hangs/crashes when accessing Js::Configuration::Global.flags).
// Test hooks are a dynamic-loading concept; in static builds ch and the
// library share one address space and must agree on all type layouts.

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include "CommonDefines.h"
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <io.h>
#else
// xplat includes - PAL headers must come first to establish platform types
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include "CommonDefines.h"
#include "pal.h"
// palrt.h provides Windows compat types (BYTE, DWORD, HRESULT, etc.)
#include "rt/palrt.h"

// The PAL defines __MSTYPES_DEFINED which causes ChakraCommon.h to skip
// its block that typedefs byte, DWORD, WCHAR, etc. However, the PAL
// defines BYTE but not lowercase 'byte'. ChakraCore headers (ChakraCore.h,
// ChakraDebug.h) use lowercase 'byte' extensively, so we must provide it
// here before any C++ standard library headers (which bring in std::byte
// in C++17 and would create ambiguity).
#ifndef _CHAKRA_BYTE_DEFINED
#define _CHAKRA_BYTE_DEFINED
typedef unsigned char byte;
#endif

// _countof macro - returns number of elements in a static array
#ifndef _countof
#define _countof(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

// _wfullpath - wide-char version of _fullpath.
// The PAL only provides narrow _fullpath; we implement a wide version
// by converting to/from narrow strings (assumes ASCII-compatible paths).
inline WCHAR* _wfullpath(WCHAR* absPath, const WCHAR* relPath, size_t maxLength)
{
    // Convert wide relPath to narrow
    size_t relLen = wcslen(relPath);
    char narrowRel[_MAX_PATH];
    if (relLen >= _MAX_PATH) return nullptr;
    for (size_t i = 0; i <= relLen; i++)
    {
        narrowRel[i] = (char)(relPath[i] & 0x7F);
    }

    char narrowAbs[_MAX_PATH];
    if (realpath(narrowRel, narrowAbs) == nullptr)
    {
        // realpath failed; try just copying the path as-is
        if (strlen(narrowRel) >= _MAX_PATH) return nullptr;
        strncpy(narrowAbs, narrowRel, _MAX_PATH);
        narrowAbs[_MAX_PATH - 1] = '\0';
    }

    // Convert narrow result back to wide
    size_t absLen = strlen(narrowAbs);
    if (absLen >= maxLength) return nullptr;
    for (size_t i = 0; i <= absLen; i++)
    {
        absPath[i] = (WCHAR)(unsigned char)narrowAbs[i];
    }
    return absPath;
}

// _wsplitpath_s is provided by pal/inc/rt/safecrt.h - no need to define here.


#endif // _WIN32

// Now safe to include C++ stdlib headers after byte typedef is established
#include <map>

#ifdef Assert
#undef Assert
#endif

#ifdef AssertMsg
#undef AssertMsg
#endif

#if defined(DBG)

#if !defined(CHAKRACORE_STRINGIZE)
#define CHAKRACORE_STRINGIZE_IMPL(x) #x
#define CHAKRACORE_STRINGIZE(x) CHAKRACORE_STRINGIZE_IMPL(x)
#endif

#define AssertMsg(exp, comment)   \
do { \
if (!(exp)) \
{ \
    fprintf(stderr, "ASSERTION (%s, line %d) %s %s\n", __FILE__, __LINE__, CHAKRACORE_STRINGIZE(exp), comment); \
    fflush(stderr); \
    DebugBreak(); \
} \
} while (0)
#else
#define AssertMsg(exp, comment) ((void)0)
#endif //defined(DBG)

#define Assert(exp)             AssertMsg(exp, #exp)
#define _JSRT_
#include "ChakraCommon.h"
#include "ChakraCore.h"
#include "Core/CommonTypedefs.h"
#include "TestHooksRt.h"

typedef void * Var;

#include "TestHooks.h"
#include "ChakraRtInterface.h"
#include "Helpers.h"
#include "HostConfigFlags.h"
#include "MessageQueue.h"
#include "WScriptJsrt.h"