#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------------------
# Apple Silicon JIT Build Configuration
#
# This CMake file handles the integration of Apple Silicon-specific JIT files
# into the ChakraCore build system. It conditionally includes Apple Silicon
# files when building for macOS ARM64.
#-------------------------------------------------------------------------------------------------------

# Only include Apple Silicon files when building for Apple Silicon
if(CC_APPLE_SILICON AND CC_TARGETS_ARM64 AND CC_TARGET_OS_OSX)

    message(STATUS "Including Apple Silicon JIT files in build")

    # Apple Silicon specific source files
    set(APPLE_SILICON_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/AppleSiliconStackMD.cpp
    )

    # Apple Silicon specific header files
    set(APPLE_SILICON_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/AppleSiliconConfig.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/AppleSiliconStackMD.h
    )

    # Add Apple Silicon files to the backend source list
    list(APPEND ARM64_BACKEND_SOURCES ${APPLE_SILICON_SOURCES})
    list(APPEND ARM64_BACKEND_HEADERS ${APPLE_SILICON_HEADERS})

    # Set Apple Silicon specific include directories
    set(APPLE_SILICON_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon
    )

    # Add include directories for Apple Silicon headers
    include_directories(${APPLE_SILICON_INCLUDE_DIRS})

    # Apple Silicon specific compiler definitions
    add_definitions(-DAPPLE_SILICON_JIT_ENABLED=1)
    add_definitions(-DUSE_APPLE_SILICON_STACK_MANAGER=1)

    # Apple Silicon specific compile flags
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DAPPLE_SILICON_DEBUG_ENABLED=1)
        add_definitions(-DAPPLE_SILICON_VALIDATE_INSTRUCTIONS=1)
    endif()

    # Apple Silicon memory management requirements
    if(APPLE_JIT_MEMORY_MANAGEMENT)
        add_definitions(-DAPPLE_SILICON_MEMORY_MANAGEMENT=1)

        # Link against required Apple frameworks for JIT memory management
        find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
        find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

        list(APPEND APPLE_SILICON_LINK_LIBRARIES
            ${FOUNDATION_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
            pthread
        )
    endif()

    # Apple Silicon JIT performance optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_definitions(-DAPPLE_SILICON_OPTIMIZED=1)

        # Enable Apple Silicon specific optimizations
        set(APPLE_SILICON_COMPILE_FLAGS
            -ffast-math
            -fno-strict-aliasing
            -funroll-loops
        )

        # Apply optimization flags to Apple Silicon sources
        set_source_files_properties(${APPLE_SILICON_SOURCES}
            PROPERTIES COMPILE_FLAGS "${APPLE_SILICON_COMPILE_FLAGS}")
    endif()

    # Apple Silicon code signing and entitlements
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Note: For production builds, proper code signing may be required
        message(STATUS "Apple Silicon Release build - consider code signing for JIT")
        add_definitions(-DAPPLE_SILICON_CODE_SIGNING_REQUIRED=1)
    endif()

    # Create Apple Silicon library target (optional)
    if(BUILD_APPLE_SILICON_LIBRARY)
        add_library(AppleSiliconJIT STATIC ${APPLE_SILICON_SOURCES})

        target_include_directories(AppleSiliconJIT PUBLIC
            ${APPLE_SILICON_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/..
        )

        target_compile_definitions(AppleSiliconJIT PUBLIC
            APPLE_SILICON_JIT=1
            PROHIBIT_STP_LDP=1
        )

        if(APPLE_SILICON_LINK_LIBRARIES)
            target_link_libraries(AppleSiliconJIT ${APPLE_SILICON_LINK_LIBRARIES})
        endif()

        # Set target properties for Apple Silicon library
        set_target_properties(AppleSiliconJIT PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON
            POSITION_INDEPENDENT_CODE ON
        )

        # Install Apple Silicon library if requested
        if(INSTALL_APPLE_SILICON_LIBRARY)
            install(TARGETS AppleSiliconJIT
                ARCHIVE DESTINATION lib
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin
            )

            install(FILES ${APPLE_SILICON_HEADERS}
                DESTINATION include/ChakraCore/AppleSilicon
            )
        endif()
    endif()

    # Apple Silicon specific tests
    if(BUILD_TESTS AND APPLE_SILICON_BUILD_TESTS)
        set(APPLE_SILICON_TEST_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/Tests/AppleSiliconStackTests.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/Tests/AppleSiliconValidationTests.cpp
        )

        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/Tests)
            add_executable(AppleSiliconJITTests ${APPLE_SILICON_TEST_SOURCES})

            target_include_directories(AppleSiliconJITTests PRIVATE
                ${APPLE_SILICON_INCLUDE_DIRS}
                ${CMAKE_CURRENT_SOURCE_DIR}/..
            )

            target_link_libraries(AppleSiliconJITTests
                ChakraCore
                ${APPLE_SILICON_LINK_LIBRARIES}
            )

            # Add to CTest if available
            if(CMAKE_TESTING_ENABLED)
                add_test(NAME AppleSiliconJITTests
                    COMMAND AppleSiliconJITTests)
            endif()
        endif()
    endif()

    # Apple Silicon documentation and examples
    if(BUILD_DOCUMENTATION)
        set(APPLE_SILICON_DOCS
            ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/README.md
            ${CMAKE_CURRENT_SOURCE_DIR}/AppleSilicon/IMPLEMENTATION_NOTES.md
        )

        if(INSTALL_DOCUMENTATION)
            install(FILES ${APPLE_SILICON_DOCS}
                DESTINATION share/doc/ChakraCore/AppleSilicon
            )
        endif()
    endif()

    # Print Apple Silicon build configuration summary
    message(STATUS "Apple Silicon JIT Configuration Summary:")
    message(STATUS "  Source files: ${APPLE_SILICON_SOURCES}")
    message(STATUS "  Header files: ${APPLE_SILICON_HEADERS}")
    message(STATUS "  Include dirs: ${APPLE_SILICON_INCLUDE_DIRS}")
    if(APPLE_SILICON_LINK_LIBRARIES)
        message(STATUS "  Link libraries: ${APPLE_SILICON_LINK_LIBRARIES}")
    endif()
    message(STATUS "  Debug enabled: ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Instruction validation: ${APPLE_SILICON_VALIDATE_INSTRUCTIONS}")

else()
    # Not building for Apple Silicon
    message(STATUS "Apple Silicon JIT files excluded from build (not Apple Silicon target)")

    # Define empty macros for non-Apple Silicon builds
    add_definitions(-DAPPLE_SILICON_JIT_ENABLED=0)
    add_definitions(-DUSE_APPLE_SILICON_STACK_MANAGER=0)

endif()

#-------------------------------------------------------------------------------------------------------
# Apple Silicon Build Options
#-------------------------------------------------------------------------------------------------------

# Option to build Apple Silicon library separately
option(BUILD_APPLE_SILICON_LIBRARY
    "Build Apple Silicon JIT as separate library" OFF)

# Option to install Apple Silicon library
option(INSTALL_APPLE_SILICON_LIBRARY
    "Install Apple Silicon JIT library" OFF)

# Option to build Apple Silicon tests
option(APPLE_SILICON_BUILD_TESTS
    "Build Apple Silicon specific tests" OFF)

# Option to enable Apple Silicon performance profiling
option(APPLE_SILICON_ENABLE_PROFILING
    "Enable Apple Silicon JIT performance profiling" OFF)

if(APPLE_SILICON_ENABLE_PROFILING AND CC_APPLE_SILICON)
    add_definitions(-DAPPLE_SILICON_PROFILING_ENABLED=1)
    message(STATUS "Apple Silicon JIT profiling enabled")
endif()

#-------------------------------------------------------------------------------------------------------
# Apple Silicon Validation and Debugging
#-------------------------------------------------------------------------------------------------------

if(CC_APPLE_SILICON)
    # Validate that required CMake variables are set
    # Note: APPLE_SILICON_JIT and PROHIBIT_STP_LDP are C preprocessor defines
    # added via add_definitions() in the top-level CMakeLists.txt, not CMake variables.
    # We validate using the CMake-level variable CC_APPLE_SILICON instead.
    if(NOT CC_APPLE_SILICON)
        message(FATAL_ERROR "CC_APPLE_SILICON must be set for Apple Silicon builds")
    endif()

    # Check for required Apple frameworks
    if(APPLE_JIT_MEMORY_MANAGEMENT)
        if(NOT FOUNDATION_FRAMEWORK)
            message(WARNING "Foundation framework not found - Apple Silicon JIT memory management may not work")
        endif()
    endif()

    # Validate compiler support
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(STATUS "Apple Silicon JIT: Using compatible Clang compiler")
    else()
        message(WARNING "Apple Silicon JIT: Untested compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()

    # Check macOS version compatibility
    if(CMAKE_SYSTEM_VERSION VERSION_LESS "11.0")
        message(WARNING "Apple Silicon JIT requires macOS 11.0 or later for optimal compatibility")
    endif()
endif()

#-------------------------------------------------------------------------------------------------------
# Apple Silicon Feature Configuration
#-------------------------------------------------------------------------------------------------------

if(CC_APPLE_SILICON)
    # Configure features based on build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(APPLE_SILICON_DEBUG_FEATURES ON)
        set(APPLE_SILICON_VALIDATE_INSTRUCTIONS ON)
        set(APPLE_SILICON_ENABLE_LOGGING ON)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(APPLE_SILICON_DEBUG_FEATURES OFF)
        set(APPLE_SILICON_VALIDATE_INSTRUCTIONS ON)
        set(APPLE_SILICON_ENABLE_LOGGING OFF)
    else()
        set(APPLE_SILICON_DEBUG_FEATURES OFF)
        set(APPLE_SILICON_VALIDATE_INSTRUCTIONS OFF)
        set(APPLE_SILICON_ENABLE_LOGGING OFF)
    endif()

    # Apply feature configurations
    if(APPLE_SILICON_DEBUG_FEATURES)
        add_definitions(-DAPPLE_SILICON_DEBUG_FEATURES=1)
    endif()

    if(APPLE_SILICON_VALIDATE_INSTRUCTIONS)
        add_definitions(-DAPPLE_SILICON_VALIDATE_INSTRUCTIONS=1)
    endif()

    if(APPLE_SILICON_ENABLE_LOGGING)
        add_definitions(-DAPPLE_SILICON_ENABLE_LOGGING=1)
    endif()
endif()
