//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Core/AtomLockGuids.h"

unsigned int MessageBase::s_messageCount = 0;

LPCWSTR hostName = _u("ch");

#ifdef ENABLE_TEST_HOOKS
extern "C"
HRESULT CHAKRA_CALLBACK OnChakraCoreLoadedEntry(TestHooks& testHooks)
{
    return ChakraRTInterface::OnChakraCoreLoaded(testHooks);
}
#endif

JsRuntimeAttributes jsrtAttributes = JsRuntimeAttributeAllowScriptInterrupt;

LPCWSTR JsErrorCodeToString(JsErrorCode jsErrorCode)
{
    switch (jsErrorCode)
    {
    case JsNoError:
        return _u("JsNoError");
    case JsErrorInvalidArgument:
        return _u("JsErrorInvalidArgument");
    case JsErrorNullArgument:
        return _u("JsErrorNullArgument");
    case JsErrorNoCurrentContext:
        return _u("JsErrorNoCurrentContext");
    case JsErrorInExceptionState:
        return _u("JsErrorInExceptionState");
    case JsErrorNotImplemented:
        return _u("JsErrorNotImplemented");
    case JsErrorWrongThread:
        return _u("JsErrorWrongThread");
    case JsErrorRuntimeInUse:
        return _u("JsErrorRuntimeInUse");
    case JsErrorBadSerializedScript:
        return _u("JsErrorBadSerializedScript");
    case JsErrorInDisabledState:
        return _u("JsErrorInDisabledState");
    case JsErrorCannotDisableExecution:
        return _u("JsErrorCannotDisableExecution");
    case JsErrorHeapEnumInProgress:
        return _u("JsErrorHeapEnumInProgress");
    case JsErrorOutOfMemory:
        return _u("JsErrorOutOfMemory");
    case JsErrorScriptException:
        return _u("JsErrorScriptException");
    case JsErrorScriptCompile:
        return _u("JsErrorScriptCompile");
    case JsErrorScriptTerminated:
        return _u("JsErrorScriptTerminated");
    case JsErrorFatal:
        return _u("JsErrorFatal");
    default:
        return _u("<unknown>");
    }
}

#define IfJsErrorFailLog(expr) \
    do { \
        JsErrorCode jsErrorCode = expr; \
        if ((jsErrorCode) != JsNoError) { \
            fwprintf(stderr, _u("ERROR: %ls failed. JsErrorCode=0x%x (%ls)\n"), \
                _u(#expr), jsErrorCode, JsErrorCodeToString(jsErrorCode)); \
            fflush(stderr); \
            goto Error; \
        } \
    } while (0)

void CHAKRA_CALLBACK PrintUsageFormat()
{
    wprintf(_u("\nUsage: ch [flaglist] filename\n"));
}

void CHAKRA_CALLBACK PrintUsage()
{
#ifndef DEBUG
    wprintf(_u("\nUsage: ch filename")
            _u("\n[flaglist] is not supported for Release mode\n"));
#else
    PrintUsageFormat();
    wprintf(_u("Try 'ch -?' for help\n"));
#endif
}

static void CHAKRA_CALLBACK PromiseContinuationCallback(JsValueRef task, void *callbackState)
{
    Assert(task != JS_INVALID_REFERENCE);
    Assert(callbackState != JS_INVALID_REFERENCE);
    MessageQueue * messageQueue = (MessageQueue *)callbackState;

    WScriptJsrt::CallbackMessage *msg = new WScriptJsrt::CallbackMessage(0, task);
    messageQueue->Push(msg);
}

HRESULT RunScript(LPCWSTR fileName, LPCWSTR fileContents, BYTE *bcBuffer, char16 *fullPath)
{
    HRESULT hr = S_OK;
    MessageQueue * messageQueue = new MessageQueue();
    WScriptJsrt::AddMessageQueue(messageQueue);

    IfJsErrorFailLog(ChakraRTInterface::JsSetPromiseContinuationCallback(PromiseContinuationCallback, (void*)messageQueue));

    Assert(fileContents != nullptr || bcBuffer != nullptr);
    JsErrorCode runScript;
    if (bcBuffer != nullptr)
    {
        runScript = ChakraRTInterface::JsRunSerializedScript(fileContents, bcBuffer, WScriptJsrt::GetNextSourceContext(), fullPath, nullptr /*result*/);
    }
    else
    {
        runScript = ChakraRTInterface::JsRunScript(fileContents, WScriptJsrt::GetNextSourceContext(), fullPath, nullptr /*result*/);
    }

    if (runScript != JsNoError)
    {
        WScriptJsrt::PrintException(fileName, runScript);
    }
    else
    {
        // Repeatedly flush the message queue until it's empty. It is necessary to loop on this
        // because setTimeout can add scripts to execute.
        do
        {
            IfFailGo(messageQueue->ProcessAll(fileName));
        } while (!messageQueue->IsEmpty());
    }
Error:
    if (messageQueue != nullptr)
    {
        delete messageQueue;
    }
    return hr;
}

HRESULT ExecuteTest(LPCWSTR fileName)
{
    HRESULT hr = S_OK;
    LPCWSTR fileContents = nullptr;
    JsRuntimeHandle runtime = JS_INVALID_RUNTIME_HANDLE;
    bool isUtf8 = false;
    LPCOLESTR contentsRaw = nullptr;
    UINT lengthBytes = 0;
    hr = Helpers::LoadScriptFromFile(fileName, fileContents, &isUtf8, &contentsRaw, &lengthBytes);
    contentsRaw; lengthBytes; // Unused for now.

    IfFailGo(hr);

    if (HostConfigFlags::flags.GenerateLibraryByteCodeHeaderIsEnabled)
    {
        jsrtAttributes = (JsRuntimeAttributes)(jsrtAttributes | JsRuntimeAttributeSerializeLibraryByteCode);
    }

    IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime));

    {
        JsContextRef context = JS_INVALID_REFERENCE;
        IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime, &context));
        IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context));
    }

    if (!WScriptJsrt::Initialize())
    {
        IfFailGo(E_FAIL);
    }

    {
        char16 fullPath[_MAX_PATH];

        if (_wfullpath(fullPath, fileName, _MAX_PATH) == nullptr)
        {
            IfFailGo(E_FAIL);
        }

        // canonicalize that path name to lower case for the profile storage
        size_t len = wcslen(fullPath);
        for (size_t i = 0; i < len; i++)
        {
            fullPath[i] = towlower(fullPath[i]);
        }

        if (HostConfigFlags::flags.SerializedIsEnabled)
        {
            if (isUtf8)
            {
                // Serialized bytecode execution - get the buffer and run
                BYTE *bcBuffer = nullptr;
                DWORD bcBufferSize = 0;
                IfJsErrorFailLog(ChakraRTInterface::JsSerializeScript(fileContents, bcBuffer, &bcBufferSize));
                if (bcBufferSize > 0)
                {
                    bcBuffer = new BYTE[bcBufferSize];
                    DWORD newBcBufferSize = bcBufferSize;
                    IfJsErrorFailLog(ChakraRTInterface::JsSerializeScript(fileContents, bcBuffer, &newBcBufferSize));

                    // Create a new runtime for the serialized script
                    JsRuntimeHandle runtime2 = JS_INVALID_RUNTIME_HANDLE;
                    JsContextRef context2 = JS_INVALID_REFERENCE;
                    JsContextRef current = JS_INVALID_REFERENCE;

                    IfJsErrorFailLog(ChakraRTInterface::JsCreateRuntime(jsrtAttributes, nullptr, &runtime2));
                    IfJsErrorFailLog(ChakraRTInterface::JsCreateContext(runtime2, &context2));
                    IfJsErrorFailLog(ChakraRTInterface::JsGetCurrentContext(&current));
                    IfJsErrorFailLog(ChakraRTInterface::JsSetCurrentContext(context2));

                    if (!WScriptJsrt::Initialize())
                    {
                        delete[] bcBuffer;
                        IfFailGo(E_FAIL);
                    }

                    IfFailGo(RunScript(fileName, fileContents, bcBuffer, fullPath));

                    ChakraRTInterface::JsSetCurrentContext(current);
                    ChakraRTInterface::JsDisposeRuntime(runtime2);
                    delete[] bcBuffer;
                }
            }
            else
            {
                fwprintf(stderr, _u("FATAL ERROR: Serialized flag can only be used on UTF8 file, exiting\n"));
                IfFailGo(E_FAIL);
            }
        }
        else
        {
            IfFailGo(RunScript(fileName, fileContents, nullptr, fullPath));
        }
    }

Error:
    ChakraRTInterface::JsSetCurrentContext(nullptr);

    if (runtime != JS_INVALID_RUNTIME_HANDLE)
    {
        ChakraRTInterface::JsDisposeRuntime(runtime);
    }

    _flushall();

    return hr;
}

#ifdef _WIN32
int _cdecl wmain(int argc, __in_ecount(argc) LPWSTR argv[])
#else
int main(int argc, char** argv)
#endif
{
#ifndef _WIN32
    // Convert narrow argv to wide argv for PAL compatibility
    PAL_InitializeChakraCore();

    LPWSTR* wargv = new LPWSTR[argc];
    for (int i = 0; i < argc; i++)
    {
        int len = strlen(argv[i]) + 1;
        wargv[i] = new WCHAR[len];
        for (int j = 0; j < len; j++)
        {
            wargv[i][j] = (WCHAR)argv[i][j];
        }
    }
#define argv wargv
#endif

    if (argc < 2)
    {
        PrintUsage();
#ifndef _WIN32
        for (int i = 0; i < argc; i++) delete[] wargv[i];
        delete[] wargv;
#endif
        return EXIT_FAILURE;
    }

    HostConfigFlags::pfnPrintUsage = PrintUsageFormat;

#ifdef _WIN32
    ATOM lock = ::AddAtom(szChakraCoreLock);
    AssertMsg(lock, "failed to lock chakracore.dll");
#endif

    HostConfigFlags::HandleArgsFlag(argc, argv);

    BSTR fileName = nullptr;

    ChakraRTInterface::ArgInfo argInfo = { argc, argv, PrintUsage, &fileName };
    HINSTANCE chakraLibrary = ChakraRTInterface::LoadChakraDll(argInfo);

    if (fileName == nullptr)
    {
        fileName = SysAllocString(argv[1]);
    }

    HRESULT hr = E_FAIL;

    if (chakraLibrary != nullptr)
    {
        hr = ExecuteTest(fileName);

        ChakraRTInterface::UnloadChakraDll(chakraLibrary);
    }

    if (fileName != nullptr)
    {
        SysFreeString(fileName);
    }

#ifndef _WIN32
#undef argv
    for (int i = 0; i < argc; i++) delete[] wargv[i];
    delete[] wargv;
    PAL_Terminate();
#endif

    return FAILED(hr) ? EXIT_FAILURE : EXIT_SUCCESS;
}