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
# Search current dir first, then lib/
if(APPLE)
  set(_RPATH_LIST "@loader_path" "@loader_path/lib")
else()
  set(_RPATH_LIST "\$ORIGIN" "\$ORIGIN/lib")
endif()

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_BUILD_RPATH "${_RPATH_LIST}")
set(CMAKE_INSTALL_RPATH "${_RPATH_LIST}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# =========================
# Options (tweakable)
# =========================
set(LEO_RESOURCES_DIR "resources" CACHE STRING "Path to resources directory (fallback if no .leopack)")
set(LEO_RESOURCES_PACK "resources.leopack" CACHE STRING "Path to .leopack file if available")

# =========================
# Engine (FetchContent)
# =========================
include(FetchContent)
set(LEO_BUILD_SHARED ON  CACHE BOOL "Build Leo engine as a shared library" FORCE)
set(LEO_BUILD_TESTS  OFF CACHE BOOL "Do not build Leo engine unit tests"   FORCE)
set(LEO_VENDOR_SDL   ON  CACHE BOOL "Use Leo Engine's SDL3, not system"    FORCE)

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

# ---- Windows/MSVC: delay-load SDL3.dll and leo.dll ----
if (WIN32 AND MSVC)
  # Delay-load the two DLLs (names must match the runtime file names)
  target_link_options(${PROJECT_NAME} PRIVATE
      /DELAYLOAD:SDL3.dll
      /DELAYLOAD:leo.dll)
  target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)
  target_compile_definitions(${PROJECT_NAME} PRIVATE LEO_WIN_DELAYLOAD=1)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "${_RPATH_LIST}"
    INSTALL_RPATH "${_RPATH_LIST}"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# =========================
# Helper script files (generated)
# =========================
# Decide whether to copy .leopack or a resources folder
set(_PACK_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/_package_copy_resources.cmake")
file(WRITE "${_PACK_SCRIPT}" [=[
    cmake_minimum_required(VERSION 3.25)
    # Args: -DINPUT_PACK=... -DINPUT_DIR=... -DDEST_DIR=... -DRES_DIR_NAME=...
    if(DEFINED INPUT_PACK AND EXISTS "${INPUT_PACK}")
        get_filename_component(_pack_name "${INPUT_PACK}" NAME)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${INPUT_PACK}" "${DEST_DIR}/${_pack_name}")
        message(STATUS "Copied pack: ${_pack_name}")
    elseif(DEFINED INPUT_DIR AND IS_DIRECTORY "${INPUT_DIR}")
        if(NOT DEFINED RES_DIR_NAME OR RES_DIR_NAME STREQUAL "")
            set(RES_DIR_NAME "resources")
        endif()
        execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${INPUT_DIR}" "${DEST_DIR}/${RES_DIR_NAME}")
        message(STATUS "Copied folder: ${RES_DIR_NAME}/")
    else()
        message(STATUS "No resources found (.leopack nor folder). Skipping.")
    endif()
]=])

# Create SONAME symlink inside a destination dir (Linux)
set(_SONAME_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/_package_fix_soname_links.cmake")
file(WRITE "${_SONAME_SCRIPT}" [=[
    cmake_minimum_required(VERSION 3.25)
    # -DLIB_ABS=... -DDEST_DIR=... -DSONAME=...
    if(NOT DEFINED LIB_ABS OR NOT EXISTS "${LIB_ABS}")
        message(FATAL_ERROR "LIB_ABS not provided or doesn't exist")
    endif()
    if(NOT DEFINED DEST_DIR OR DEST_DIR STREQUAL "")
        message(FATAL_ERROR "DEST_DIR not provided")
    endif()
    get_filename_component(_actual "${LIB_ABS}" NAME)
    if(DEFINED SONAME AND NOT SONAME STREQUAL "")
        execute_process(COMMAND "${CMAKE_COMMAND}" -E chdir "${DEST_DIR}" "${CMAKE_COMMAND}" -E rm -f "${SONAME}")
        execute_process(COMMAND "${CMAKE_COMMAND}" -E chdir "${DEST_DIR}" "${CMAKE_COMMAND}" -E create_symlink "${_actual}" "${SONAME}")
        message(STATUS "Created SONAME link: ${SONAME} -> ${_actual} in ${DEST_DIR}")
    else()
        message(STATUS "SONAME not provided; skipping link for ${_actual}")
    endif()
]=])

# =========================
# Destinations
# =========================
# 1) Runtime dir (where the exe is)
set(RUNTIME_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

# 2) Package directory (always produced)
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
# Always-on: populate the RUNTIME (build) directory
#  - Copy libs to the BUILD ROOT (next to exe) + SONAME links there
# =========================
if(LEO_BUILD_SHARED)
    if(WIN32)
        # NOTE: Windows won't search lib/ automatically; this keeps DLLs next to the exe.
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
                "${RUNTIME_DIR}"
            COMMAND_EXPAND_LISTS
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${RUNTIME_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/LICENSE" "${RUNTIME_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/README.md" "${RUNTIME_DIR}/README.md"
            COMMENT "Populate runtime dir (Windows)"
        )
    elseif(APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}"
            # dylibs next to the exe
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime>
                $<TARGET_FILE:SDL3::SDL3>
                "${RUNTIME_DIR}"
            # resources + docs to root
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${RUNTIME_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/LICENSE" "${RUNTIME_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/README.md" "${RUNTIME_DIR}/README.md"
            COMMENT "Populate runtime dir (macOS)"
        )
    elseif(UNIX AND NOT APPLE) # Linux
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}"
            # .so files next to the exe
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime>
                $<TARGET_FILE:SDL3::SDL3>
                "${RUNTIME_DIR}"
            # SONAME symlinks in the build root
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS="$<TARGET_FILE:Leo::Runtime>"
                -D DEST_DIR="${RUNTIME_DIR}"
                -D SONAME="$<TARGET_SONAME_FILE_NAME:Leo::Runtime>"
                -P "${_SONAME_SCRIPT}"
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS="$<TARGET_FILE:SDL3::SDL3>"
                -D DEST_DIR="${RUNTIME_DIR}"
                -D SONAME="$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>"
                -P "${_SONAME_SCRIPT}"
            # resources + docs to root
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${RUNTIME_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/LICENSE" "${RUNTIME_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/README.md" "${RUNTIME_DIR}/README.md"
            COMMENT "Populate runtime dir (Linux)"
            COMMAND_EXPAND_LISTS
        )
    endif()
else()
    # Static build: still drop resources + docs into runtime dir root
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}"
        COMMAND ${CMAKE_COMMAND}
            -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
            -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
            -D DEST_DIR="${RUNTIME_DIR}"
            -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
            -P "${_PACK_SCRIPT}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/LICENSE" "${RUNTIME_DIR}/LICENSE"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/README.md" "${RUNTIME_DIR}/README.md"
        COMMENT "Populate runtime dir (static build)"
    )
endif()

# =========================
# Always-on: assemble the PACKAGE directory (libs ONLY in lib/)
# =========================
if(LEO_BUILD_SHARED)
    if(WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_ABS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
            # exe in root
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${PROJECT_NAME}>
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            # DLLs into lib/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
                "${PACKAGE_LIB_DIR}"
            COMMAND_EXPAND_LISTS
            # proactively remove any stray DLLs in root (harmless if none)
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:Leo::Runtime>"
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:SDL3::SDL3>"
            # resources to root
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${PACKAGE_ABS_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            # docs to root
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_ABS_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/README.md" "${PACKAGE_ABS_DIR}/README.md"
            COMMENT "Assembling ${PACKAGE_DIR} (Windows) with libs only in lib/"
        )
    elseif(APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_ABS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
            # exe in root
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${PROJECT_NAME}>
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            # dylibs into lib/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime>
                $<TARGET_FILE:SDL3::SDL3>
                "${PACKAGE_LIB_DIR}"
            # proactively remove any stray dylibs in root
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:Leo::Runtime>"
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:SDL3::SDL3>"
            # resources + docs
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${PACKAGE_ABS_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_ABS_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/README.md" "${PACKAGE_ABS_DIR}/README.md"
            COMMENT "Assembling ${PACKAGE_DIR} (macOS) with libs only in lib/"
        )
    elseif(UNIX AND NOT APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_ABS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
            # exe in root
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${PROJECT_NAME}>
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            # .so files into lib/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime>
                $<TARGET_FILE:SDL3::SDL3>
                "${PACKAGE_LIB_DIR}"
            # SONAME symlinks in package lib/
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS="$<TARGET_FILE:Leo::Runtime>"
                -D DEST_DIR="${PACKAGE_LIB_DIR}"
                -D SONAME="$<TARGET_SONAME_FILE_NAME:Leo::Runtime>"
                -P "${_SONAME_SCRIPT}"
            COMMAND ${CMAKE_COMMAND}
                -D LIB_ABS="$<TARGET_FILE:SDL3::SDL3>"
                -D DEST_DIR="${PACKAGE_LIB_DIR}"
                -D SONAME="$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>"
                -P "${_SONAME_SCRIPT}"
            # proactively remove any stray libs/symlinks in root (harmless if none)
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:Leo::Runtime>"
                "${PACKAGE_ABS_DIR}/$<TARGET_SONAME_FILE_NAME:Leo::Runtime>"
                "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:SDL3::SDL3>"
                "${PACKAGE_ABS_DIR}/$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>"
            # resources + docs to root
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${PACKAGE_ABS_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_ABS_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/README.md" "${PACKAGE_ABS_DIR}/README.md"
            COMMENT "Assembling ${PACKAGE_DIR} (Linux) with libs only in lib/"
            COMMAND_EXPAND_LISTS
        )
    endif()
else()
    # Static build: no lib/ subtree needed
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_ABS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND}
            -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
            -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
            -D DEST_DIR="${PACKAGE_ABS_DIR}"
            -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
            -P "${_PACK_SCRIPT}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_ABS_DIR}/LICENSE"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/README.md" "${PACKAGE_ABS_DIR}/README.md"
        COMMENT "Assembling ${PACKAGE_DIR} (static build)"
    )
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

