//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"
#include "JitAsmTrace.h"

//-------------------------------------------------------------------------------------------------------
// JIT Assembly Tracing Integration Bridge
//
// This file provides simplified JIT tracing integration without external dependencies.
// For now, tracing is controlled by build-time configuration and runtime environment.
//-------------------------------------------------------------------------------------------------------

// Global flag for JIT tracing - can be set via environment variable or debug builds
static bool g_jitTracingEnabled = false;

extern "C" bool IsTraceJitAsmEnabled()
{
    // Check environment variable on first call
    static bool initialized = false;
    if (!initialized)
    {
        const char* envVar = getenv("CHAKRA_TRACE_JIT_ASM");
        if (envVar && (strcmp(envVar, "1") == 0 || strcmp(envVar, "true") == 0))
        {
            g_jitTracingEnabled = true;
        }
        
#ifdef _DEBUG
        // Enable by default in debug builds if environment variable is not explicitly set to "0"
        if (!envVar || strcmp(envVar, "0") != 0)
        {
            g_jitTracingEnabled = true;
        }
#endif
        
        initialized = true;
    }
    
    return g_jitTracingEnabled;
}

// Allow runtime control of tracing
extern "C" void SetTraceJitAsmEnabled(bool enabled)
{
    g_jitTracingEnabled = enabled;
}

// Static initialization of tracing system
class JitAsmTraceInitializer
{
public:
    JitAsmTraceInitializer()
    {
        // Initialize tracing system when the module loads
        InitializeJitAsmTracing();
    }
    
    ~JitAsmTraceInitializer()
    {
        // Cleanup when module unloads
        ShutdownJitAsmTracing();
    }
};

// Global initializer instance - ensures tracing system is ready
static JitAsmTraceInitializer g_traceInitializer;

//-------------------------------------------------------------------------------------------------------
// Enhanced tracing functions with proper integration
//-------------------------------------------------------------------------------------------------------

bool IsJitAsmTraceRequested()
{
    return IsTraceJitAsmEnabled();
}

void InitializeJitAsmTracing()
{
    // Enable tracing if command line flag is set
    bool enabled = IsTraceJitAsmEnabled();
    JitAsmTracer::SetEnabled(enabled);
    
    if (enabled)
    {
        // Set up default output to stderr for immediate visibility
        JitAsmTracer::SetVerbosity(2); // Full analysis
        
        // Print initialization message
        fprintf(stderr, "\n=== ChakraCore JIT Assembly Tracing Enabled ===\n");
        fprintf(stderr, "Environment: Set CHAKRA_TRACE_JIT_ASM=1 to enable\n");
        fprintf(stderr, "Functions will be traced with disassembly and analysis.\n");
        fprintf(stderr, "================================================\n\n");
    }
}

void ShutdownJitAsmTracing()
{
    if (JitAsmTracer::IsEnabled())
    {
        fprintf(stderr, "\n=== JIT Assembly Tracing Session Complete ===\n");
        JitAsmTracer::SetEnabled(false);
    }
}

//-------------------------------------------------------------------------------------------------------
// Additional utility functions for enhanced tracing
//-------------------------------------------------------------------------------------------------------

namespace JitTraceUtils
{
    // Helper to determine if a function is worth tracing based on various criteria
    bool ShouldTraceFunction(Func* func)
    {
        if (!func || !IsTraceJitAsmEnabled())
        {
            return false;
        }
        
        // Always trace user functions, but we might want to filter system functions
        // For now, trace everything when enabled
        return true;
    }
    
    // Enhanced tracing with additional context
    void TraceJitFunctionWithContext(Func* func, void* codeAddress, size_t codeSize, const char* phase)
    {
        if (!ShouldTraceFunction(func))
        {
            return;
        }
        
        // Print phase information
        if (phase && *phase)
        {
            fprintf(stderr, "\n>>> JIT Phase: %s <<<\n", phase);
        }
        
        // Use the main tracing functionality
        TRACE_JIT_FUNCTION(func, codeAddress, codeSize);
    }
}

//-------------------------------------------------------------------------------------------------------
// Enhanced macro definitions for different tracing contexts
//-------------------------------------------------------------------------------------------------------

#define TRACE_JIT_FUNCTION_WITH_PHASE(func, codeAddr, codeSize, phase) \
    do { \
        if (JitAsmTracer::IsEnabled()) { \
            JitTraceUtils::TraceJitFunctionWithContext(func, codeAddr, codeSize, phase); \
        } \
    } while (0)

// Export the enhanced macros for use in other files
#ifndef JITASMTRACE_INTEGRATION_H
#define JITASMTRACE_INTEGRATION_H

// Re-export the utility functions
namespace JitTrace = JitTraceUtils;

#endif // JITASMTRACE_INTEGRATION_H