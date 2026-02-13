//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "Codex/Utf8Codex.h"

HRESULT Helpers::LoadScriptFromFile(LPCWSTR filename, LPCWSTR& contents, bool* isUtf8Out, LPCWSTR* contentsRawOut, UINT* lengthBytesOut, bool printFileOpenError)
{
    HRESULT hr = S_OK;
    LPCWSTR contentsRaw = nullptr;
    UINT lengthBytes = 0;
    bool isUtf8 = false;
    contents = nullptr;
    byte * pRawBytes = nullptr;

#ifdef _WIN32
    FILE * file;
#else
    PAL_FILE * file;
#endif

    //
    // Open the file as a binary file to prevent CRT from handling encoding, line-break conversions,
    // etc.
    //
    if (_wfopen_s(&file, filename, _u("rb")) != 0)
    {
        if (printFileOpenError)
        {
            fwprintf(stderr, _u("Error in opening file '%ls' "), filename);
            fwprintf(stderr, _u("\n"));
        }
        hr = E_FAIL;
        goto Error;
    }

    if (file == nullptr)
    {
        if (printFileOpenError)
        {
            fwprintf(stderr, _u("Error in opening file '%ls': file is null\n"), filename);
        }
        hr = E_FAIL;
        goto Error;
    }

    //
    // Determine the file length, in bytes.
    //
    fseek(file, 0, SEEK_END);
    lengthBytes = ftell(file);
    fseek(file, 0, SEEK_SET);
    contentsRaw = (LPCWSTR)HeapAlloc(GetProcessHeap(), 0, lengthBytes + sizeof(WCHAR));
    if (nullptr == contentsRaw)
    {
        fwprintf(stderr, _u("out of memory"));
        hr = E_OUTOFMEMORY;
        fclose(file);
        goto Error;
    }

    //
    // Read the entire content as a binary block.
    //
    fread((void*)contentsRaw, sizeof(char), lengthBytes, file);
    fclose(file);
    *(WCHAR*)((byte*)contentsRaw + lengthBytes) = _u('\0'); // Null terminate it. Could be LPCWSTR.

    //
    // Read encoding, handling any conversion to Unicode.
    //
    // Warning: The UNICODE buffer for parsing is supposed to be provided by the host.
    // This is not a complete read of the encoding. Some encodings like UTF7, UTF1, EBCDIC, SCSU, BOCU could be
    // wrongly classified as ANSI
    //
    pRawBytes = (byte*)contentsRaw;
    if ((0xEF == *pRawBytes && 0xBB == *(pRawBytes + 1) && 0xBF == *(pRawBytes + 2)))
    {
        isUtf8 = true;
    }
    else if (0xFFFE == *contentsRaw || (0x0000 == *contentsRaw && 0xFEFF == *(contentsRaw + 1)))
    {
        // unicode unsupported
        fwprintf(stderr, _u("unsupported file encoding"));
        hr = E_UNEXPECTED;
        goto Error;
    }
    else if (0xFEFF == *contentsRaw)
    {
        // unicode LE
        contents = contentsRaw;
    }
    else
    {
        // Assume UTF8
        isUtf8 = true;
    }


    if (isUtf8)
    {
        utf8::DecodeOptions decodeOptions = utf8::doAllowInvalidWCHARs;

        LPCUTF8 pbUtf8 = (LPCUTF8)pRawBytes;
        LPCUTF8 pbEnd = pbUtf8 + lengthBytes;

        UINT cUtf16Chars = utf8::ByteIndexIntoCharacterIndex(pRawBytes, lengthBytes, decodeOptions);
        contents = (LPCWSTR)HeapAlloc(GetProcessHeap(), 0, (cUtf16Chars + 1) * sizeof(WCHAR));
        if (nullptr == contents)
        {
            fwprintf(stderr, _u("out of memory"));
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        utf8::DecodeUnitsIntoAndNullTerminate((char16*) contents, pbUtf8, pbEnd, decodeOptions);
    }

Error:
    if (SUCCEEDED(hr) && isUtf8Out)
    {
        Assert(contentsRawOut);
        Assert(lengthBytesOut);
        *isUtf8Out = isUtf8;
        *contentsRawOut = contentsRaw;
        *lengthBytesOut = lengthBytes;
    }
    else if (contentsRaw && (contentsRaw != contents)) // Otherwise contentsRaw is lost. Free it if it is different to contents.
    {
        HeapFree(GetProcessHeap(), 0, (void*)contentsRaw);
    }

    if (contents && FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, (void*)contents);
        contents = nullptr;
    }

    return hr;
}