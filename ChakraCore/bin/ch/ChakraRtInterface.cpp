//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Core/AtomLockGuids.h"

#ifdef ENABLE_TEST_HOOKS
// Forward-declare the entry point defined in ch.cpp so we can pass it
// directly to OnChakraCoreLoaded, avoiding the PAL GetProcAddress lookup
// which deadlocks on xplat static builds.
extern "C" HRESULT CHAKRA_CALLBACK OnChakraCoreLoadedEntry(TestHooks& testHooks);

bool ChakraRTInterface::m_testHooksSetup = false;
bool ChakraRTInterface::m_testHooksInitialized = false;
TestHooks ChakraRTInterface::m_testHooks = { 0 };
#endif

bool ChakraRTInterface::m_usageStringPrinted = false;
ChakraRTInterface::ArgInfo ChakraRTInterface::m_argInfo = { 0 };

/*static*/
HINSTANCE ChakraRTInterface::LoadChakraDll(ArgInfo& argInfo)
{
    m_argInfo = argInfo;

#ifdef ENABLE_TEST_HOOKS
    // On xplat, ChakraCore is statically linked. We call OnChakraCoreLoaded
    // with the OnChakraCoreLoadedEntry function pointer directly, bypassing
    // the GetProcAddress lookup which deadlocks on xplat (the PAL module
    // list lock is not safe to use this early in static builds).
    ::OnChakraCoreLoaded(OnChakraCoreLoadedEntry);
#endif

    if (m_usageStringPrinted)
    {
        return nullptr;
    }

    // Return a non-null sentinel value to indicate success.
    // On xplat we don't actually load a DLL.
    return reinterpret_cast<HINSTANCE>(1);
}

/*static*/
void ChakraRTInterface::UnloadChakraDll(HINSTANCE library)
{
    // Nothing to do on xplat - ChakraCore is statically linked
}

#ifdef ENABLE_TEST_HOOKS
/*static*/
HRESULT ChakraRTInterface::ParseConfigFlags()
{
    HRESULT hr = S_OK;

    if (m_testHooks.pfSetAssertToConsoleFlag)
    {
        SetAssertToConsoleFlag(true);
    }

    if (m_testHooks.pfSetConfigFlags)
    {
        hr = SetConfigFlags(m_argInfo.argc, m_argInfo.argv, &HostConfigFlags::flags);
        if (hr != S_OK && !m_usageStringPrinted)
        {
            if (m_argInfo.hostPrintUsage)
            {
                m_argInfo.hostPrintUsage();
            }
            m_usageStringPrinted = true;
        }
    }

    if (hr == S_OK)
    {
        if (m_argInfo.filename != nullptr)
        {
            *(m_argInfo.filename) = nullptr;
            if (m_testHooks.pfGetFilenameFlag != nullptr)
            {
                hr = GetFileNameFlag(m_argInfo.filename);
                if (hr != S_OK)
                {
                    // Don't print error here - the caller will handle
                    // providing a filename from argv if needed
                    hr = S_OK;
                }
            }
        }
    }

    return S_OK;
}

/*static*/
HRESULT ChakraRTInterface::OnChakraCoreLoaded(TestHooks& testHooks)
{
    if (!m_testHooksInitialized)
    {
        m_testHooks = testHooks;
        m_testHooksSetup = true;
        m_testHooksInitialized = true;
        return ParseConfigFlags();
    }

    return S_OK;
}
#endif // ENABLE_TEST_HOOKS