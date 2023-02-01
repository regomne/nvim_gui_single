cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

project(nvim_gui_single CXX)

set(TARGET_NAME nvim_gui_single)

set(SOURCE_FILES
  src/main.cpp
  src/defer.h
  src/log.h
  src/log.cpp

  src/res.rc
)

add_executable(${TARGET_NAME} WIN32
  ${SOURCE_FILES}
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    UNICODE
    _UNICODE
)

set_property(TARGET ${TARGET_NAME} PROPERTY
             MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

