//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//
// xplat ChakraRtInterface - calls JSRT APIs directly (statically linked)
// On non-Windows platforms, ChakraCore is linked statically so we don't need
// dynamic DLL loading. This class provides the same interface as the Windows
// version but forwards directly to the JSRT C API.
//

//
// Forward declarations for JSRT APIs that are implemented in Jsrt.cpp but
// only declared in ChakraCommonWindows.h (which is not included on xplat).
// These functions ARE compiled into the static library; they just lack
// public header declarations on non-Windows platforms.
//
#ifndef _WIN32
extern "C" {
    JsErrorCode JsPointerToString(
        const WCHAR *stringValue,
        size_t stringLength,
        JsValueRef *value);

    JsErrorCode JsStringToPointer(
        JsValueRef value,
        const WCHAR **stringValue,
        size_t *stringLength);

    JsErrorCode JsGetPropertyIdFromName(
        const WCHAR *name,
        JsPropertyIdRef *propertyId);

    JsErrorCode JsRunScript(
        const WCHAR *script,
        JsSourceContext sourceContext,
        const WCHAR *sourceUrl,
        JsValueRef *result);

    JsErrorCode JsSerializeScript(
        const WCHAR *script,
        unsigned char *buffer,
        unsigned int *bufferSize);

    JsErrorCode JsRunSerializedScript(
        const WCHAR *script,
        unsigned char *buffer,
        JsSourceContext sourceContext,
        const WCHAR *sourceUrl,
        JsValueRef *result);
}
#endif // !_WIN32

struct JsAPIHooks
{
    // Not used on xplat - kept for interface compatibility
};

class ChakraRTInterface
{
public:
    typedef void(CHAKRA_CALLBACK * HostPrintUsageFuncPtr)();

    struct ArgInfo
    {
        int argc;
        LPWSTR* argv;
        HostPrintUsageFuncPtr hostPrintUsage;
        BSTR* filename;
    };

private:
    static bool m_usageStringPrinted;
    static ArgInfo m_argInfo;

#ifdef ENABLE_TEST_HOOKS
    static bool m_testHooksSetup;
    static bool m_testHooksInitialized;
    static TestHooks m_testHooks;

    static HRESULT ParseConfigFlags();
#endif

public:
#ifdef ENABLE_TEST_HOOKS
    static HRESULT OnChakraCoreLoaded(TestHooks& testHooks);
#endif

    // On xplat, we don't load a DLL - just initialize directly
    static HINSTANCE LoadChakraDll(ArgInfo& argInfo);
    static void UnloadChakraDll(HINSTANCE library);

#ifdef ENABLE_TEST_HOOKS
    static HRESULT SetAssertToConsoleFlag(bool flag)
    {
        if (m_testHooksSetup && m_testHooks.pfSetAssertToConsoleFlag)
            return m_testHooks.pfSetAssertToConsoleFlag(flag);
        return E_NOTIMPL;
    }

    static HRESULT SetConfigFlags(int argc, LPWSTR argv[], ICustomConfigFlags* customConfigFlags)
    {
        if (m_testHooksSetup && m_testHooks.pfSetConfigFlags)
            return m_testHooks.pfSetConfigFlags(argc, argv, customConfigFlags);
        return E_NOTIMPL;
    }

    static HRESULT GetFileNameFlag(BSTR * filename)
    {
        if (m_testHooksSetup && m_testHooks.pfGetFilenameFlag)
            return m_testHooks.pfGetFilenameFlag(filename);
        return E_NOTIMPL;
    }

    static HRESULT PrintConfigFlagsUsageString()
    {
        m_usageStringPrinted = true;
        if (m_testHooksSetup && m_testHooks.pfPrintConfigFlagsUsageString)
            return m_testHooks.pfPrintConfigFlagsUsageString();
        return E_NOTIMPL;
    }

#ifdef CHECK_MEMORY_LEAK
    static bool IsEnabledCheckMemoryFlag()
    {
        if (m_testHooksSetup && m_testHooks.pfIsEnabledCheckMemoryLeakFlag)
            return m_testHooks.pfIsEnabledCheckMemoryLeakFlag();
        return false;
    }

    static HRESULT SetCheckMemoryLeakFlag(bool flag)
    {
        if (m_testHooksSetup && m_testHooks.pfSetCheckMemoryLeakFlag)
            return m_testHooks.pfSetCheckMemoryLeakFlag(flag);
        return E_NOTIMPL;
    }

    static HRESULT SetEnableCheckMemoryLeakOutput(bool flag)
    {
        if (m_testHooksSetup && m_testHooks.pfSetEnableCheckMemoryLeakOutput)
            return m_testHooks.pfSetEnableCheckMemoryLeakOutput(flag);
        return E_NOTIMPL;
    }
#endif // CHECK_MEMORY_LEAK

    static HRESULT GetCrashOnExceptionFlag(bool * flag)
    {
#ifdef SECURITY_TESTING
        if (m_testHooksSetup && m_testHooks.pfGetCrashOnExceptionFlag)
            return m_testHooks.pfGetCrashOnExceptionFlag(flag);
#endif
        return E_UNEXPECTED;
    }

    static void NotifyUnhandledException(PEXCEPTION_POINTERS exceptionInfo)
    {
        if (m_testHooksSetup && m_testHooks.pfnNotifyUnhandledException != nullptr)
        {
            m_testHooks.pfnNotifyUnhandledException(exceptionInfo);
        }
    }
#endif // ENABLE_TEST_HOOKS

    // -------------------------------------------------------------------------
    // Direct JSRT API wrappers
    // Call the C API directly since we're statically linked.
    // On xplat the "legacy" wide-char APIs are forward-declared above.
    // -------------------------------------------------------------------------

    static JsErrorCode CHAKRA_CALLBACK JsCreateRuntime(JsRuntimeAttributes attributes, JsThreadServiceCallback threadService, JsRuntimeHandle *runtime)
    {
        return ::JsCreateRuntime(attributes, threadService, runtime);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCreateContext(JsRuntimeHandle runtime, JsContextRef *newContext)
    {
        return ::JsCreateContext(runtime, newContext);
    }

    static JsErrorCode CHAKRA_CALLBACK JsSetCurrentContext(JsContextRef context)
    {
        return ::JsSetCurrentContext(context);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetCurrentContext(JsContextRef* context)
    {
        return ::JsGetCurrentContext(context);
    }

    static JsErrorCode CHAKRA_CALLBACK JsDisposeRuntime(JsRuntimeHandle runtime)
    {
        return ::JsDisposeRuntime(runtime);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCreateObject(JsValueRef *object)
    {
        return ::JsCreateObject(object);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCreateExternalObject(void *data, JsFinalizeCallback callback, JsValueRef *object)
    {
        return ::JsCreateExternalObject(data, callback, object);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCreateFunction(JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function)
    {
        return ::JsCreateFunction(nativeFunction, callbackState, function);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState, JsValueRef *function)
    {
        return ::JsCreateNamedFunction(name, nativeFunction, callbackState, function);
    }

    static JsErrorCode CHAKRA_CALLBACK JsPointerToString(const char16 *stringValue, size_t length, JsValueRef *value)
    {
        return ::JsPointerToString(stringValue, length, value);
    }

    static JsErrorCode CHAKRA_CALLBACK JsSetProperty(JsValueRef object, JsPropertyIdRef property, JsValueRef value, bool useStrictRules)
    {
        return ::JsSetProperty(object, property, value, useStrictRules);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetGlobalObject(JsValueRef *globalObject)
    {
        return ::JsGetGlobalObject(globalObject);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetUndefinedValue(JsValueRef *undefinedValue)
    {
        return ::JsGetUndefinedValue(undefinedValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetTrueValue(JsValueRef *trueValue)
    {
        return ::JsGetTrueValue(trueValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetFalseValue(JsValueRef *falseValue)
    {
        return ::JsGetFalseValue(falseValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsConvertValueToString(JsValueRef value, JsValueRef *stringValue)
    {
        return ::JsConvertValueToString(value, stringValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsConvertValueToNumber(JsValueRef value, JsValueRef *numberValue)
    {
        return ::JsConvertValueToNumber(value, numberValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsConvertValueToBoolean(JsValueRef value, JsValueRef *booleanValue)
    {
        return ::JsConvertValueToBoolean(value, booleanValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsStringToPointer(JsValueRef value, const char16 **stringValue, size_t *length)
    {
        return ::JsStringToPointer(value, stringValue, length);
    }

    static JsErrorCode CHAKRA_CALLBACK JsBooleanToBool(JsValueRef value, bool* boolValue)
    {
        return ::JsBooleanToBool(value, boolValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetPropertyIdFromName(const char16 *name, JsPropertyIdRef *propertyId)
    {
        return ::JsGetPropertyIdFromName(name, propertyId);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetProperty(JsValueRef object, JsPropertyIdRef property, JsValueRef* value)
    {
        return ::JsGetProperty(object, property, value);
    }

    static JsErrorCode CHAKRA_CALLBACK JsHasProperty(JsValueRef object, JsPropertyIdRef property, bool *hasProperty)
    {
        return ::JsHasProperty(object, property, hasProperty);
    }

    static JsErrorCode CHAKRA_CALLBACK JsRunScript(const char16 *script, JsSourceContext sourceContext, const char16 *sourceUrl, JsValueRef* result)
    {
        return ::JsRunScript(script, sourceContext, sourceUrl, result);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCallFunction(JsValueRef function, JsValueRef* arguments, unsigned short argumentCount, JsValueRef *result)
    {
        return ::JsCallFunction(function, arguments, argumentCount, result);
    }

    static JsErrorCode CHAKRA_CALLBACK JsNumberToDouble(JsValueRef value, double* doubleValue)
    {
        return ::JsNumberToDouble(value, doubleValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsNumberToInt(JsValueRef value, int* intValue)
    {
        return ::JsNumberToInt(value, intValue);
    }

    static JsErrorCode CHAKRA_CALLBACK JsDoubleToNumber(double doubleValue, JsValueRef* value)
    {
        return ::JsDoubleToNumber(doubleValue, value);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetExternalData(JsValueRef object, void **data)
    {
        return ::JsGetExternalData(object, data);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCreateArray(unsigned int length, JsValueRef *result)
    {
        return ::JsCreateArray(length, result);
    }

    static JsErrorCode CHAKRA_CALLBACK JsCreateError(JsValueRef message, JsValueRef *error)
    {
        return ::JsCreateError(message, error);
    }

    static JsErrorCode CHAKRA_CALLBACK JsSetException(JsValueRef exception)
    {
        return ::JsSetException(exception);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetAndClearException(JsValueRef *exception)
    {
        return ::JsGetAndClearException(exception);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetRuntime(JsContextRef context, JsRuntimeHandle *runtime)
    {
        return ::JsGetRuntime(context, runtime);
    }

    static JsErrorCode CHAKRA_CALLBACK JsRelease(JsRef ref, unsigned int* count)
    {
        return ::JsRelease(ref, count);
    }

    static JsErrorCode CHAKRA_CALLBACK JsAddRef(JsRef ref, unsigned int* count)
    {
        return ::JsAddRef(ref, count);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetValueType(JsValueRef value, JsValueType *type)
    {
        return ::JsGetValueType(value, type);
    }

    static JsErrorCode CHAKRA_CALLBACK JsSetIndexedProperty(JsValueRef object, JsValueRef index, JsValueRef value)
    {
        return ::JsSetIndexedProperty(object, index, value);
    }

    // Note: the actual Jsrt.cpp implementation uses unsigned int* for bufferSize,
    // and DWORD on xplat PAL is uint32_t (== unsigned int), so we match that here.
    static JsErrorCode CHAKRA_CALLBACK JsSerializeScript(const char16 *script, BYTE *buffer, DWORD *bufferSize)
    {
        return ::JsSerializeScript(script, buffer, reinterpret_cast<unsigned int *>(bufferSize));
    }

    static JsErrorCode CHAKRA_CALLBACK JsRunSerializedScript(const char16 *script, BYTE *buffer, JsSourceContext sourceContext, const char16 *sourceUrl, JsValueRef* result)
    {
        return ::JsRunSerializedScript(script, buffer, sourceContext, sourceUrl, result);
    }

    static JsErrorCode CHAKRA_CALLBACK JsSetPromiseContinuationCallback(JsPromiseContinuationCallback callback, void *callbackState)
    {
        return ::JsSetPromiseContinuationCallback(callback, callbackState);
    }

    static JsErrorCode CHAKRA_CALLBACK JsGetContextOfObject(JsValueRef object, JsContextRef* context)
    {
        return ::JsGetContextOfObject(object, context);
    }
};