cmake_minimum_required(VERSION 3.25)

# =========================
# Project
# =========================
set(PROJECT_NAME "leo-engine-showcase")

project(${PROJECT_NAME}
    VERSION 0.1.0
    DESCRIPTION "Showcase Leo Engine games and demos"
    HOMEPAGE_URL "https://github.com/bluesentinelsec/leo-engine-examples"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================
# Runtime search paths
# =========================
if(APPLE)
  set(_RPATH_LIST "@executable_path/lib")
elseif(UNIX)
  set(_RPATH_LIST "\$ORIGIN" "\$ORIGIN/lib")
endif()

if(UNIX AND NOT APPLE)
  set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
endif()

set(CMAKE_BUILD_RPATH "${_RPATH_LIST}")
set(CMAKE_INSTALL_RPATH "${_RPATH_LIST}")

# =========================
# Options
# =========================
set(LEO_RESOURCES_DIR "resources" CACHE STRING "Fallback resources dir")
set(LEO_RESOURCES_PACK "resources.leopack" CACHE STRING "Optional .leopack file")

# =========================
# Engine (FetchContent)
# =========================
include(FetchContent)
set(LEO_BUILD_SHARED ON  CACHE BOOL "Build Leo as shared" FORCE)
set(LEO_BUILD_TESTS  OFF CACHE BOOL "Skip tests"          FORCE)
set(LEO_VENDOR_SDL   ON  CACHE BOOL "Use vendored SDL3"   FORCE)

FetchContent_Declare(
    leo
    GIT_REPOSITORY https://github.com/bluesentinelsec/leo-engine
    GIT_TAG main
)
FetchContent_MakeAvailable(leo)

# =========================
# Sources
# =========================
set(SOURCE_FILES
    src/main.c
    src/win_dll_dirs.c
    src/demos.c
    src/demos.h
    src/demo_basic.c
    src/core/resizable_window.c
    src/core/fullscreen_window.c
    external/getopt/getopt.c
    external/getopt/getopt.h
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Leo::Runtime)

set(RUNTIME_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

# ---- Windows/MSVC delay-load ----
if(WIN32 AND MSVC)
  target_link_options(${PROJECT_NAME} PRIVATE
      /DELAYLOAD:SDL3.dll
      /DELAYLOAD:leo.dll)
  target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)
  target_compile_definitions(${PROJECT_NAME} PRIVATE LEO_WIN_DELAYLOAD=1)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    MACOSX_RPATH TRUE
)

# =========================
# Helper scripts
# =========================
# Copy resources
set(_PACK_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/_package_copy_resources.cmake")
file(WRITE "${_PACK_SCRIPT}" [=[
    cmake_minimum_required(VERSION 3.25)
    if(DEFINED INPUT_PACK AND EXISTS "${INPUT_PACK}")
        get_filename_component(_pack_name "${INPUT_PACK}" NAME)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${INPUT_PACK}" "${DEST_DIR}/${_pack_name}")
    elseif(DEFINED INPUT_DIR AND IS_DIRECTORY "${INPUT_DIR}")
        if(NOT DEFINED RES_DIR_NAME OR RES_DIR_NAME STREQUAL "")
            set(RES_DIR_NAME "resources")
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${INPUT_DIR}" "${DEST_DIR}/${RES_DIR_NAME}")
    endif()
]=])

# Linux SONAME symlinks
set(_SONAME_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/_package_fix_soname_links.cmake")
file(WRITE "${_SONAME_SCRIPT}" [=[
    cmake_minimum_required(VERSION 3.25)
    if(DEFINED LIB_ABS AND EXISTS "${LIB_ABS}" AND DEFINED DEST_DIR AND DEFINED SONAME)
        get_filename_component(_actual "${LIB_ABS}" NAME)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E chdir "${DEST_DIR}"
            "${CMAKE_COMMAND}" -E create_symlink "${_actual}" "${SONAME}")
    endif()
]=])

# =========================
# Package directories
# =========================
if(UNIX AND NOT APPLE)
    set(PACKAGE_DIR "${PROJECT_NAME}.Linux.$<CONFIG>")
elseif(APPLE)
    set(PACKAGE_DIR "${PROJECT_NAME}.macOS.$<CONFIG>")
else()
    set(PACKAGE_DIR "${PROJECT_NAME}.Windows.$<CONFIG>")
endif()
set(PACKAGE_ABS_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_DIR}")
set(PACKAGE_LIB_DIR "${PACKAGE_ABS_DIR}/lib")

# =========================
# Runtime population
# =========================
if(LEO_BUILD_SHARED)
    if(WIN32)
        # Copy DLLs next to exe
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
                "${RUNTIME_DIR}"
            COMMAND_EXPAND_LISTS
            COMMENT "Runtime DLLs (Windows)"
        )
    elseif(APPLE)
        # Copy dylibs into lib/ and add symlink for compat
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime> $<TARGET_FILE:SDL3::SDL3> "${RUNTIME_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E chdir "${RUNTIME_DIR}/lib"
                ${CMAKE_COMMAND} -E create_symlink "libleo.0.1.0.dylib" "libleo.0.dylib"
            COMMENT "Runtime dylibs (macOS)"
        )
    elseif(UNIX)
        # Copy .so into lib/ and create SONAME symlinks
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime> $<TARGET_FILE:SDL3::SDL3> "${RUNTIME_DIR}/lib"
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS=$<TARGET_FILE:Leo::Runtime>
                -D DEST_DIR=${RUNTIME_DIR}/lib
                -D SONAME=$<TARGET_SONAME_FILE_NAME:Leo::Runtime>
                -P ${_SONAME_SCRIPT}
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS=$<TARGET_FILE:SDL3::SDL3>
                -D DEST_DIR=${RUNTIME_DIR}/lib
                -D SONAME=$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>
                -P ${_SONAME_SCRIPT}
            COMMENT "Runtime .so libs (Linux)"
        )
    endif()
endif()

# =========================
# Package assembly
# =========================
if(LEO_BUILD_SHARED)
    if(APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PACKAGE_ABS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime> $<TARGET_FILE:SDL3::SDL3> "${PACKAGE_LIB_DIR}"
            # macOS symlink for compat
            COMMAND ${CMAKE_COMMAND} -E chdir "${PACKAGE_LIB_DIR}"
                ${CMAKE_COMMAND} -E create_symlink "libleo.0.1.0.dylib" "libleo.0.dylib"
            COMMENT "Assemble ${PACKAGE_DIR} (macOS)"
        )
    elseif(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PACKAGE_ABS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> "${PACKAGE_LIB_DIR}"
            COMMAND_EXPAND_LISTS
            COMMENT "Assemble ${PACKAGE_DIR} (Windows)"
        )
    elseif(UNIX) # Linux
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PACKAGE_ABS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime> $<TARGET_FILE:SDL3::SDL3> "${PACKAGE_LIB_DIR}"
            # SONAME symlinks
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS=$<TARGET_FILE:Leo::Runtime>
                -D DEST_DIR=${PACKAGE_LIB_DIR}
                -D SONAME=$<TARGET_SONAME_FILE_NAME:Leo::Runtime>
                -P ${_SONAME_SCRIPT}
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS=$<TARGET_FILE:SDL3::SDL3>
                -D DEST_DIR=${PACKAGE_LIB_DIR}
                -D SONAME=$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>
                -P ${_SONAME_SCRIPT}
            COMMENT "Assemble ${PACKAGE_DIR} (Linux)"
        )
    endif()
endif()

# =========================
# Convenience: run from build tree
# =========================
add_custom_target(run
    COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    DEPENDS ${PROJECT_NAME}
    COMMENT "Run from build tree"
)

