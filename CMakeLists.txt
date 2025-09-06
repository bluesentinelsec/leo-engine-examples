cmake_minimum_required(VERSION 3.18.4)

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
# Runtime search paths (skip for Emscripten)
# =========================
if (NOT EMSCRIPTEN)
    if (APPLE)
        set(_RPATH_LIST "@loader_path/../Frameworks")
    elseif (UNIX)
        set(_RPATH_LIST "\$ORIGIN" "\$ORIGIN/lib")
    endif ()

    if (UNIX AND NOT APPLE)
        set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    endif ()

    set(CMAKE_BUILD_RPATH "${_RPATH_LIST}")
    set(CMAKE_INSTALL_RPATH "${_RPATH_LIST}")
endif ()

# =========================
# Options
# =========================
set(LEO_RESOURCES_DIR "resources" CACHE STRING "Fallback resources dir")
set(LEO_RESOURCES_PACK "resources.leopack" CACHE STRING "Optional .leopack file")

# =========================
# Engine (FetchContent)
# =========================
include(FetchContent)
set(LEO_BUILD_SHARED ON CACHE BOOL "Build Leo as shared" FORCE)
set(LEO_BUILD_TESTS OFF CACHE BOOL "Skip tests" FORCE)
set(LEO_VENDOR_SDL ON CACHE BOOL "Use vendored SDL3" FORCE)

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
        src/getopt.c
        src/getopt.h
)

# =========================
# Executable
# =========================
if (EMSCRIPTEN)
    add_executable(${PROJECT_NAME} ${SOURCE_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    target_link_libraries(${PROJECT_NAME} PRIVATE Leo::Runtime)

    target_link_options(${PROJECT_NAME} PRIVATE
        "-sUSE_SDL=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sFORCE_FILESYSTEM"
        "-sASYNCIFY"
        "--shell-file=${CMAKE_SOURCE_DIR}/static/index.html"
    )

else()
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCE_FILES})
    target_link_libraries(${PROJECT_NAME} PRIVATE Leo::Runtime)

    set(RUNTIME_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

    # ---- Windows/MSVC delay-load ----
    if (WIN32 AND MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE
                /DELAYLOAD:SDL3.dll
                /DELAYLOAD:leo.dll)
        target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)
        target_compile_definitions(${PROJECT_NAME} PRIVATE LEO_WIN_DELAYLOAD=1)
    endif ()

    set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            MACOSX_RPATH TRUE
    )

    # =========================
    # Helper scripts
    # =========================
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

    set(_SONAME_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/_package_fix_soname_links.cmake")
    file(WRITE "${_SONAME_SCRIPT}" [=[
        cmake_minimum_required(VERSION 3.25)
        if(DEFINED LIB_ABS AND EXISTS "${LIB_ABS}" AND DEFINED DEST_DIR AND DEFINED SONAME)
            get_filename_component(_actual "${LIB_ABS}" NAME)
            if(NOT _actual STREQUAL "${SONAME}")
                execute_process(COMMAND "${CMAKE_COMMAND}" -E chdir "${DEST_DIR}"
                    "${CMAKE_COMMAND}" -E create_symlink "${_actual}" "${SONAME}")
            endif()
        endif()
    ]=])

    # =========================
    # Package directories
    # =========================
    if (UNIX AND NOT APPLE)
        set(PACKAGE_DIR "${PROJECT_NAME}.Linux.$<CONFIG>")
    elseif (APPLE)
        set(PACKAGE_DIR "${PROJECT_NAME}.app")
    else ()
        set(PACKAGE_DIR "${PROJECT_NAME}.Windows.$<CONFIG>")
    endif ()
    set(PACKAGE_ABS_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_DIR}")
    set(PACKAGE_LIB_DIR "${PACKAGE_ABS_DIR}/lib")

    # =========================
    # Runtime population
    # =========================
    if (LEO_BUILD_SHARED)
        if (WIN32)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
                    "${RUNTIME_DIR}"
                    COMMAND_EXPAND_LISTS
                    COMMENT "Runtime DLLs (Windows)"
            )
        elseif (APPLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    # Copy dylibs into Frameworks
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Leo::Runtime> $<TARGET_FILE:SDL3::SDL3>
                    "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks"

                    # Fix dylib IDs
                    COMMAND install_name_tool -id "@rpath/$<TARGET_SONAME_FILE_NAME:Leo::Runtime>"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/$<TARGET_FILE_NAME:Leo::Runtime>"
                    COMMAND install_name_tool -id "@rpath/$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks/$<TARGET_FILE_NAME:SDL3::SDL3>"

                    # Create SONAME symlinks only if needed
                    COMMAND ${CMAKE_COMMAND}
                    -D LIB_ABS=$<TARGET_FILE:Leo::Runtime>
                    -D DEST_DIR=$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks
                    -D SONAME=$<TARGET_SONAME_FILE_NAME:Leo::Runtime>
                    -P ${_SONAME_SCRIPT}
                    COMMAND ${CMAKE_COMMAND}
                    -D LIB_ABS=$<TARGET_FILE:SDL3::SDL3>
                    -D DEST_DIR=$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks
                    -D SONAME=$<TARGET_SONAME_FILE_NAME:SDL3::SDL3>
                    -P ${_SONAME_SCRIPT}

                    COMMENT "Runtime dylibs (macOS bundle: copy + fix IDs + symlinks)"
            )
        elseif (UNIX)
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
        endif ()
    endif ()

    # =========================
    # Package assembly
    # =========================
    if (LEO_BUILD_SHARED)
        if (APPLE)
            set_target_properties(${PROJECT_NAME} PROPERTIES
                    MACOSX_BUNDLE TRUE
                    MACOSX_RPATH TRUE
                    MACOSX_BUNDLE_GUI_IDENTIFIER "com.bluesentinelsec.leoengine.showcase"
                    MACOSX_BUNDLE_BUNDLE_NAME "Leo Engine Showcase"
                    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
                    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
                    MACOSX_BUNDLE_COPYRIGHT "© 2025 Blue Sentinel Security"
                    INSTALL_RPATH "@loader_path/../Frameworks"
                    BUILD_RPATH "@loader_path/../Frameworks"
            )

            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/LICENSE"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/LICENSE.txt"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/README.md"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/README.txt"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/third_party_licenses"
                    "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/third_party_licenses"
                    COMMAND ${CMAKE_COMMAND}
                    -D INPUT_PACK=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}
                    -D INPUT_DIR=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}
                    -D DEST_DIR=$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources
                    -P ${_PACK_SCRIPT}
                    COMMENT "Assemble macOS app bundle"
            )
        elseif (WIN32)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PACKAGE_ABS_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> "${PACKAGE_LIB_DIR}"
                    COMMAND_EXPAND_LISTS
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_ABS_DIR}/LICENSE.txt"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/README.md" "${PACKAGE_ABS_DIR}/README.txt"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/third_party_licenses" "${PACKAGE_ABS_DIR}/third_party_licenses"
                    COMMAND ${CMAKE_COMMAND}
                    -D INPUT_PACK=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}
                    -D INPUT_DIR=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}
                    -D DEST_DIR=${PACKAGE_ABS_DIR}
                    -P ${_PACK_SCRIPT}
                    COMMENT "Assemble ${PACKAGE_DIR} (Windows)"
            )
        elseif (UNIX) # Linux
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_LIB_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PACKAGE_ABS_DIR}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Leo::Runtime> $<TARGET_FILE:SDL3::SDL3> "${PACKAGE_LIB_DIR}"
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
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_ABS_DIR}/LICENSE.txt"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/README.md" "${PACKAGE_ABS_DIR}/README.txt"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/third_party_licenses" "${PACKAGE_ABS_DIR}/third_party_licenses"
                    COMMAND ${CMAKE_COMMAND}
                    -D INPUT_PACK=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}
                    -D INPUT_DIR=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}
                    -D DEST_DIR=${PACKAGE_ABS_DIR}
                    -P ${_PACK_SCRIPT}
                    COMMENT "Assemble ${PACKAGE_DIR} (Linux)"
            )
        endif ()
    endif ()

    # =========================
    # Copy resources to build folder too
    # =========================
    if (NOT APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                -D INPUT_PACK=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_PACK}
                -D INPUT_DIR=${CMAKE_SOURCE_DIR}/${LEO_RESOURCES_DIR}
                -D DEST_DIR=${RUNTIME_DIR}
                -P ${_PACK_SCRIPT}
                COMMENT "Copy resources.leopack or resources/ to build dir"
        )
    endif ()

    # =========================
    # Convenience: run from build tree
    # =========================
    add_custom_target(run
            COMMAND "$<TARGET_FILE:${PROJECT_NAME}>"
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            DEPENDS ${PROJECT_NAME}
            COMMENT "Run from build tree"
    )
endif ()

