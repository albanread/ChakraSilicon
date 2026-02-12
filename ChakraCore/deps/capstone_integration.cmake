#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------------------
# Capstone Disassembly Engine Integration
#
# This file integrates the Capstone disassembly engine into ChakraCore for JIT assembly tracing.
# Capstone provides multi-platform, multi-architecture disassembly framework.
#
# Features enabled:
# - x86-64 disassembly for JIT code analysis on Intel/AMD platforms
# - ARM64 disassembly for JIT code analysis on ARM platforms (including Apple Silicon)
# - Static library integration
# - Header-only interface for ChakraCore
#-------------------------------------------------------------------------------------------------------

# Always build Capstone for JIT tracing (controlled by -TraceJitAsm flag at runtime)
message(STATUS "Capstone: Integrating disassembly engine for JIT tracing")

    # Set Capstone build options - minimal configuration
    set(CAPSTONE_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
    set(CAPSTONE_BUILD_STATIC ON CACHE BOOL "Build static library" FORCE)
    set(CAPSTONE_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    set(CAPSTONE_BUILD_CSTOOL OFF CACHE BOOL "Build cstool" FORCE)

    # Architecture support - enable x86 and ARM64 for ChakraCore
    # Disable architectures we don't need
    set(CAPSTONE_ARM_SUPPORT OFF CACHE BOOL "ARM support" FORCE)
    set(CAPSTONE_MIPS_SUPPORT OFF CACHE BOOL "MIPS support" FORCE)
    set(CAPSTONE_PPC_SUPPORT OFF CACHE BOOL "PowerPC support" FORCE)
    set(CAPSTONE_SPARC_SUPPORT OFF CACHE BOOL "Sparc support" FORCE)
    set(CAPSTONE_SYSZ_SUPPORT OFF CACHE BOOL "SystemZ support" FORCE)
    set(CAPSTONE_XCORE_SUPPORT OFF CACHE BOOL "XCore support" FORCE)
    set(CAPSTONE_M68K_SUPPORT OFF CACHE BOOL "M68K support" FORCE)
    set(CAPSTONE_TMS320C64X_SUPPORT OFF CACHE BOOL "TMS320C64x support" FORCE)
    set(CAPSTONE_M680X_SUPPORT OFF CACHE BOOL "M680x support" FORCE)
    set(CAPSTONE_EVM_SUPPORT OFF CACHE BOOL "EVM support" FORCE)
    set(CAPSTONE_RISCV_SUPPORT OFF CACHE BOOL "RISC-V support" FORCE)
    set(CAPSTONE_WASM_SUPPORT OFF CACHE BOOL "WebAssembly support" FORCE)
    set(CAPSTONE_BPF_SUPPORT OFF CACHE BOOL "BPF support" FORCE)
    set(CAPSTONE_MOS65XX_SUPPORT OFF CACHE BOOL "MOS65XX support" FORCE)
    set(CAPSTONE_SH_SUPPORT OFF CACHE BOOL "SH support" FORCE)
    set(CAPSTONE_TRICORE_SUPPORT OFF CACHE BOOL "TriCore support" FORCE)

    # Enable architectures based on target
    if(CC_TARGETS_AMD64)
        set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "x86 support" FORCE)
        set(CAPSTONE_ARM64_SUPPORT OFF CACHE BOOL "ARM64 support" FORCE)
        message(STATUS "Capstone: Enabling x86-64 disassembly support")
    elseif(CC_TARGETS_ARM64)
        set(CAPSTONE_X86_SUPPORT OFF CACHE BOOL "x86 support" FORCE)
        set(CAPSTONE_ARM64_SUPPORT ON CACHE BOOL "ARM64 support" FORCE)
        message(STATUS "Capstone: Enabling ARM64 disassembly support")
    elseif(CC_TARGETS_ARM)
        set(CAPSTONE_X86_SUPPORT OFF CACHE BOOL "x86 support" FORCE)
        set(CAPSTONE_ARM64_SUPPORT ON CACHE BOOL "ARM64 support" FORCE)
        message(STATUS "Capstone: Enabling ARM64 disassembly support (ARM32 will use ARM64 engine)")
    else()
        # Default: enable both for universal builds
        set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "x86 support" FORCE)
        set(CAPSTONE_ARM64_SUPPORT ON CACHE BOOL "ARM64 support" FORCE)
        message(STATUS "Capstone: Enabling both x86-64 and ARM64 disassembly support")
    endif()

    # Disable features we don't need
    set(CAPSTONE_DIET OFF CACHE BOOL "Diet mode" FORCE)
    set(CAPSTONE_X86_REDUCE OFF CACHE BOOL "x86 reduce mode" FORCE)
    set(CAPSTONE_X86_ATT_DISABLE OFF CACHE BOOL "Disable AT&T syntax" FORCE)

    # Build configuration
    set(CAPSTONE_USE_SYS_DYN_MEM ON CACHE BOOL "Use system dynamic memory" FORCE)
    set(CAPSTONE_BUILD_LEGACY_TESTS OFF CACHE BOOL "Build legacy tests" FORCE)

    # Compiler settings alignment with ChakraCore
    if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # Suppress warnings from Capstone that might conflict with ChakraCore build settings
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
    endif()

    # Store original warning flags and restore after Capstone
    set(CHAKRA_ORIGINAL_C_FLAGS ${CMAKE_C_FLAGS})
    set(CHAKRA_ORIGINAL_CXX_FLAGS ${CMAKE_CXX_FLAGS})

    # Add Capstone subdirectory
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/capstone/CMakeLists.txt")
        add_subdirectory(deps/capstone EXCLUDE_FROM_ALL)

        # Create alias for easier reference
        add_library(Chakra::Capstone ALIAS capstone)

        # Set properties for integration
        set_target_properties(capstone PROPERTIES
            FOLDER "Dependencies"
            POSITION_INDEPENDENT_CODE ON
        )

        # Determine which architectures were enabled
        set(ARCH_LIST "")
        if(CAPSTONE_X86_SUPPORT)
            list(APPEND ARCH_LIST "x86-64")
        endif()
        if(CAPSTONE_ARM64_SUPPORT)
            list(APPEND ARCH_LIST "ARM64")
        endif()
        string(REPLACE ";" ", " ARCH_STRING "${ARCH_LIST}")

        message(STATUS "Capstone: Successfully integrated (${ARCH_STRING} support)")
        set(CAPSTONE_FOUND TRUE)

        # Define preprocessor macro for ChakraCore
        add_definitions(-DENABLE_CAPSTONE_DISASM=1)

    else()
        message(WARNING "Capstone: Source not found at deps/capstone - JIT assembly tracing disabled")
        set(CAPSTONE_FOUND FALSE)
    endif()

    # Restore ChakraCore compiler flags
    set(CMAKE_C_FLAGS ${CHAKRA_ORIGINAL_C_FLAGS})
    set(CMAKE_CXX_FLAGS ${CHAKRA_ORIGINAL_CXX_FLAGS})



# Export Capstone availability for ChakraCore components
set(CAPSTONE_AVAILABLE ${CAPSTONE_FOUND} CACHE BOOL "Capstone availability" FORCE)

if(CAPSTONE_FOUND)
    # Create interface library for ChakraCore components
    add_library(ChakraCapstone INTERFACE)

    target_link_libraries(ChakraCapstone INTERFACE capstone)
    target_include_directories(ChakraCapstone INTERFACE
        $<TARGET_PROPERTY:capstone,INTERFACE_INCLUDE_DIRECTORIES>
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/capstone/include
    )

    # Add compiler definitions for architecture support
    target_compile_definitions(ChakraCapstone INTERFACE
        ENABLE_CAPSTONE_DISASM=1
        CAPSTONE_X86_ATT_DISABLE_NO
    )

    if(CAPSTONE_X86_SUPPORT)
        target_compile_definitions(ChakraCapstone INTERFACE CAPSTONE_HAS_X86=1)
    endif()

    if(CAPSTONE_ARM64_SUPPORT)
        target_compile_definitions(ChakraCapstone INTERFACE CAPSTONE_HAS_ARM64=1)
    endif()

    message(STATUS "Capstone: Interface library 'ChakraCapstone' created")

    # Print configuration summary
    message(STATUS "Capstone Configuration:")
    message(STATUS "  - Static Library: ON")
    if(CAPSTONE_X86_SUPPORT)
        message(STATUS "  - x86-64 Support: ON")
    else()
        message(STATUS "  - x86-64 Support: OFF")
    endif()
    if(CAPSTONE_ARM64_SUPPORT)
        message(STATUS "  - ARM64 Support: ON")
    else()
        message(STATUS "  - ARM64 Support: OFF")
    endif()
    message(STATUS "  - AT&T Syntax: ON")
    message(STATUS "  - Intel Syntax: ON")
    message(STATUS "  - Integration: ChakraCapstone interface")
endif()
