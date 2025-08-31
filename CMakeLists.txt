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
else()
  set(_RPATH_LIST "\$ORIGIN/lib")
endif()

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
if (WIN32 AND MSVC)
  target_link_options(${PROJECT_NAME} PRIVATE
      /DELAYLOAD:SDL3.dll
      /DELAYLOAD:leo.dll)
  target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)
  target_compile_definitions(${PROJECT_NAME} PRIVATE LEO_WIN_DELAYLOAD=1)
endif()

# ---- macOS/Linux RPATH ----
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "${_RPATH_LIST}"
    BUILD_RPATH "${_RPATH_LIST}"
    BUILD_WITH_INSTALL_RPATH TRUE
    MACOSX_RPATH TRUE
)

# =========================
# Helper script: copy resources
# =========================
set(_PACK_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/_package_copy_resources.cmake")
file(WRITE "${_PACK_SCRIPT}" [=[
    cmake_minimum_required(VERSION 3.25)
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
# Runtime population (with macOS symlinks)
# =========================
if(LEO_BUILD_SHARED)
    if(APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # ensure lib/ exists
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}/lib"

            # copy dylibs into lib/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Leo::Runtime> "${RUNTIME_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> "${RUNTIME_DIR}/lib"

            # add compatibility symlinks (like Linux soname)
            COMMAND ${CMAKE_COMMAND} -E chdir "${RUNTIME_DIR}/lib"
                ${CMAKE_COMMAND} -E create_symlink "libleo.0.1.0.dylib" "libleo.0.dylib"

            # resources + docs
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${RUNTIME_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE" "${RUNTIME_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/README.md" "${RUNTIME_DIR}/README.md"
            COMMENT "Populate runtime dir with dylibs + symlinks"
        )
    elseif(UNIX)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Leo::Runtime> "${RUNTIME_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> "${RUNTIME_DIR}/lib"
            COMMENT "Populate runtime dir (Linux)"
        )
    endif()
endif()

# =========================
# Package assembly (with macOS symlinks)
# =========================
if(LEO_BUILD_SHARED)
    if(APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"

            # exe in root
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"

            # dylibs in lib/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Leo::Runtime> "${PACKAGE_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> "${PACKAGE_LIB_DIR}"

            # add compatibility symlinks
            COMMAND ${CMAKE_COMMAND} -E chdir "${PACKAGE_LIB_DIR}"
                ${CMAKE_COMMAND} -E create_symlink "libleo.0.1.0.dylib" "libleo.0.dylib"

            # resources + docs
            COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}"
                -D INPUT_DIR="${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}"
                -D DEST_DIR="${PACKAGE_ABS_DIR}"
                -D RES_DIR_NAME="${LEO_RESOURCES_DIR}"
                -P "${_PACK_SCRIPT}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_ABS_DIR}/LICENSE"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/README.md" "${PACKAGE_ABS_DIR}/README.md"

            COMMENT "Assembling ${PACKAGE_DIR} with dylibs + symlinks"
        )
    elseif(UNIX)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PACKAGE_ABS_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Leo::Runtime> "${PACKAGE_LIB_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> "${PACKAGE_LIB_DIR}"
            COMMENT "Assembling ${PACKAGE_DIR} (Linux)"
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

