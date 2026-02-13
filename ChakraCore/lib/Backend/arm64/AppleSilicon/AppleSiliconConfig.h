//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------
// Apple Silicon JIT Configuration Header
//
// This header defines conditional compilation macros and configuration flags
// for Apple Silicon (macOS ARM64) JIT support in ChakraCore.
//
// Key constraints on Apple Silicon:
// 1. STP/LDP (Store/Load Pair) instructions are prohibited in JIT code
// 2. JIT memory must use MAP_JIT flag and pthread_jit_write_protect_np()
// 3. Code signing and entitlements required for JIT execution
// 4. Apple-specific ABI and calling conventions
//-------------------------------------------------------------------------------------------------------

// Detect Apple Silicon platform
#if defined(__APPLE__) && defined(__arm64__) && defined(APPLE_SILICON_JIT)
    #define IS_APPLE_SILICON 1
#else
    #define IS_APPLE_SILICON 0
#endif

//-------------------------------------------------------------------------------------------------------
// Stack Operation Configuration
//-------------------------------------------------------------------------------------------------------

#ifdef PROHIBIT_STP_LDP
    // Note: PROHIBIT_STP_LDP is still defined for build compatibility, but
    // the prolog/epilog now uses STP/LDP paired instructions for correctness.
    #define USE_INDIVIDUAL_STACK_OPS 0
    #define APPLE_SILICON_STACK_OPERATIONS 0
#else
    // Other platforms can use efficient STP/LDP instructions
    #define USE_INDIVIDUAL_STACK_OPS 0
    #define APPLE_SILICON_STACK_OPERATIONS 0
#endif

//-------------------------------------------------------------------------------------------------------
// Memory Management Configuration
//-------------------------------------------------------------------------------------------------------

#ifdef APPLE_JIT_MEMORY_MANAGEMENT
    // Apple Silicon requires special JIT memory allocation
    #define USE_APPLE_JIT_ALLOCATOR 1
    #define REQUIRES_JIT_WRITE_PROTECTION 1
#else
    #define USE_APPLE_JIT_ALLOCATOR 0
    #define REQUIRES_JIT_WRITE_PROTECTION 0
#endif

//-------------------------------------------------------------------------------------------------------
// Instruction Validation Configuration
//-------------------------------------------------------------------------------------------------------

#if IS_APPLE_SILICON
    // Enable instruction validation on Apple Silicon to catch prohibited ops
    #define VALIDATE_APPLE_SILICON_INSTRUCTIONS 1
    #define ENABLE_JIT_INSTRUCTION_CHECKS 1
#else
    #define VALIDATE_APPLE_SILICON_INSTRUCTIONS 0
    #define ENABLE_JIT_INSTRUCTION_CHECKS 0
#endif

//-------------------------------------------------------------------------------------------------------
// Platform-Specific Feature Flags
//-------------------------------------------------------------------------------------------------------

#if IS_APPLE_SILICON

    // Stack management features
    #define APPLE_SILICON_PROLOG_EPILOG 1
    #define APPLE_SILICON_REGISTER_SAVES 1
    #define APPLE_SILICON_FRAME_POINTER 1

    // Memory management features  
    #define APPLE_SILICON_JIT_PAGES 1
    #define APPLE_SILICON_CODE_SIGNING 1

    // Performance features
    #define APPLE_SILICON_OPTIMIZATIONS 1
    #define APPLE_SILICON_CACHE_FRIENDLY 1

    // Debug features
    #define APPLE_SILICON_DEBUG_INFO 1
    #define APPLE_SILICON_CRASH_REPORTING 1

#else

    // Disable Apple Silicon specific features on other platforms
    #define APPLE_SILICON_PROLOG_EPILOG 0
    #define APPLE_SILICON_REGISTER_SAVES 0
    #define APPLE_SILICON_FRAME_POINTER 0
    #define APPLE_SILICON_JIT_PAGES 0
    #define APPLE_SILICON_CODE_SIGNING 0
    #define APPLE_SILICON_OPTIMIZATIONS 0
    #define APPLE_SILICON_CACHE_FRIENDLY 0
    #define APPLE_SILICON_DEBUG_INFO 0
    #define APPLE_SILICON_CRASH_REPORTING 0

#endif

//-------------------------------------------------------------------------------------------------------
// Compatibility Macros
//-------------------------------------------------------------------------------------------------------

// Macro to conditionally compile Apple Silicon vs standard ARM64 code
#define IF_APPLE_SILICON(apple_code, standard_code) \
    do { \
        if constexpr (IS_APPLE_SILICON) { \
            apple_code; \
        } else { \
            standard_code; \
        } \
    } while (0)

// Macro for conditional definitions
#if IS_APPLE_SILICON
    #define APPLE_SILICON_ONLY(code) code
    #define STANDARD_ARM64_ONLY(code)
#else
    #define APPLE_SILICON_ONLY(code)
    #define STANDARD_ARM64_ONLY(code) code
#endif

//-------------------------------------------------------------------------------------------------------
// Assert and Debug Macros
//-------------------------------------------------------------------------------------------------------

#if IS_APPLE_SILICON && defined(_DEBUG)
    #define APPLE_SILICON_ASSERT(condition, message) \
        AssertMsg(condition, "Apple Silicon JIT: " message)
    
    #define APPLE_SILICON_DEBUG_LOG(message) \
        DebugPrint("Apple Silicon JIT: " message "\n")
#else
    #define APPLE_SILICON_ASSERT(condition, message)
    #define APPLE_SILICON_DEBUG_LOG(message)
#endif

//-------------------------------------------------------------------------------------------------------
// Version and Build Information
//-------------------------------------------------------------------------------------------------------

#define APPLE_SILICON_JIT_VERSION_MAJOR 1
#define APPLE_SILICON_JIT_VERSION_MINOR 0
#define APPLE_SILICON_JIT_VERSION_PATCH 0

#define APPLE_SILICON_JIT_VERSION_STRING "1.0.0"
#define APPLE_SILICON_JIT_BUILD_DATE __DATE__
#define APPLE_SILICON_JIT_BUILD_TIME __TIME__

//-------------------------------------------------------------------------------------------------------
// Configuration Validation
//-------------------------------------------------------------------------------------------------------

#if IS_APPLE_SILICON
    // Validate that required macros are defined
    #ifndef APPLE_SILICON_JIT
        #error "APPLE_SILICON_JIT must be defined when building for Apple Silicon"
    #endif
    
    // STP/LDP are now used in prolog/epilog - no restriction needed
    // #ifndef PROHIBIT_STP_LDP
    //     #error "PROHIBIT_STP_LDP must be defined when building for Apple Silicon"
    // #endif
    
    // #if !USE_INDIVIDUAL_STACK_OPS
    //     #error "Individual stack operations required for Apple Silicon"
    // #endif
    
    // Warn about potential performance implications
    #pragma message("Building with Apple Silicon JIT support - individual stack operations enabled")
#endif

//-------------------------------------------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------------------------------------------

namespace Js
{
    // Forward declarations for Apple Silicon specific classes
    #if IS_APPLE_SILICON
        class AppleSiliconStackManager;
        class AppleSiliconJITAllocator;  
        class AppleSiliconInstructionValidator;
    #endif
}

#endif // APPLE_SILICON_CONFIG_H