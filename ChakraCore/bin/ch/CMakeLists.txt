add_executable (ch
  ch.cpp
  Helpers.cpp
  WScriptJsrt.cpp
  HostConfigFlags.cpp
  ChakraRtInterface.cpp
  ConfigParserExternals.cpp
  TestHooks.cpp
  )

include_directories(..)

# Enable PAL stdlib redirections so that wprintf, fwprintf, wcslen, wcscmp, etc.
# are remapped to PAL_wprintf, PAL_fwprintf, PAL_wcslen, PAL_wcscmp, etc.
# which accept char16_t* (WCHAR*) instead of wchar_t*.
# Without this, PAL_STDCPP_COMPAT (set globally) disables these redirections.
if(NOT WIN32)
  target_compile_definitions(ch PRIVATE USING_PAL_STDLIB=1)
endif()

target_include_directories (ch
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/bin/ChakraCore>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/Common>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/Common/Memory>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/Jsrt>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/Runtime>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/Runtime/ByteCode>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/Parser>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/Backend>
  $<BUILD_INTERFACE:${CHAKRACORE_SOURCE_DIR}/lib/SCACore>
  )

if(CC_TARGET_OS_ANDROID OR CC_TARGET_OS_LINUX)
  set(LINKER_START_GROUP -Wl,--start-group)
  set(LINKER_END_GROUP -Wl,--end-group)
elseif(CC_TARGET_OS_OSX)
  if(CC_TARGETS_X86)
    set(lib_target "${lib_target} -arch i386")
  elseif(CC_TARGETS_ARM)
    set(lib_target "${lib_target} -arch arm")
  endif()
endif()

# common link deps
set(lib_target "${lib_target}"
  -Wl,-undefined,error
  ${LINKER_START_GROUP}
  ChakraCoreStatic
  ${LINKER_END_GROUP}
  ${ICU_LIBRARIES}
  ${CC_LTO_ENABLED}
  dl
  )

if(CC_TARGET_OS_OSX)
  set(lib_target "${lib_target}"
    "-framework CoreFoundation"
    "-framework Security"
    )
elseif(NOT CC_TARGET_OS_ANDROID)
  set(lib_target "${lib_target}"
    "pthread"
    )
endif()

# Link with Capstone if available for JIT tracing
if(CAPSTONE_AVAILABLE)
  set(lib_target "${lib_target}" capstone)
endif()

target_link_libraries (ch ${lib_target})

if(NOT CC_XCODE_PROJECT)
  # Post build step to copy the ch binary to the build output
  add_custom_command(TARGET ch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/ch"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "$<TARGET_FILE:ch>"
      "${CMAKE_BINARY_DIR}/bin/ch/"
    )
endif()
