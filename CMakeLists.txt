cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME "leo-engine-showcase")

project(${PROJECT_NAME}
    VERSION 0.1.0
    DESCRIPTION "Showcase Leo Engine games and demos"
    HOMEPAGE_URL "https://github.com/bluesentinelsec/leo-engine-examples"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefer libraries next to the executable at runtime
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# Keep $ORIGIN at build/install time (escape $ for CMake)
set(CMAKE_BUILD_RPATH "\$ORIGIN")
set(CMAKE_INSTALL_RPATH "\$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

include(FetchContent)
set(LEO_BUILD_SHARED ON CACHE BOOL "Build Leo engine as a shared library" FORCE)
set(LEO_BUILD_TESTS OFF CACHE BOOL "Do not build Leo engine unit tests" FORCE)
set(LEO_VENDOR_SDL ON CACHE BOOL "Use Leo Engine's SDL3, not the system one" FORCE)

FetchContent_Declare(
    leo
    GIT_REPOSITORY https://github.com/bluesentinelsec/leo-engine
    GIT_TAG main
)
FetchContent_MakeAvailable(leo)

set(SOURCE_FILES
    src/main.c
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

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "\$ORIGIN"
    INSTALL_RPATH "\$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)

if(LEO_BUILD_SHARED)
    if(WIN32)
        # Copy all dependent DLLs (Leo, SDL3, and transitive dependencies) next to the exe
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying runtime DLLs next to ${PROJECT_NAME}"
            COMMAND_EXPAND_LISTS
        )

    elseif(APPLE)
        # Prefer the executable directory at runtime; copy dylibs next to the app
        set_property(TARGET ${PROJECT_NAME} PROPERTY BUILD_RPATH "@loader_path")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Leo::Runtime>
                $<TARGET_FILE:SDL3::SDL3>
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying dylibs (Leo, SDL3) next to ${PROJECT_NAME}"
        )
elseif(UNIX AND NOT APPLE) # Linux/ELF
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # 1) Copy the actual shared objects (Leo + versioned SDL3, e.g., libSDL3.so.0.X.Y)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:Leo::Runtime>
      $<TARGET_FILE:SDL3::SDL3>
      $<TARGET_FILE_DIR:${PROJECT_NAME}>

    # 2) Ensure SDL3 SONAME symlink: libSDL3.so.0 -> libSDL3.so.0.X.Y
    COMMAND ${CMAKE_COMMAND} -E chdir
      $<TARGET_FILE_DIR:${PROJECT_NAME}> ${CMAKE_COMMAND} -E rm -f
        $<TARGET_SONAME_FILE_NAME:SDL3::SDL3>
    COMMAND ${CMAKE_COMMAND} -E chdir
      $<TARGET_FILE_DIR:${PROJECT_NAME}> ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:SDL3::SDL3>
        $<TARGET_SONAME_FILE_NAME:SDL3::SDL3>

    # 3) Ensure Leo SONAME symlink: libleo.so.0 -> libleo.so.0.1.0
    COMMAND ${CMAKE_COMMAND} -E chdir
      $<TARGET_FILE_DIR:${PROJECT_NAME}> ${CMAKE_COMMAND} -E rm -f
        $<TARGET_SONAME_FILE_NAME:Leo::Runtime>
    COMMAND ${CMAKE_COMMAND} -E chdir
      $<TARGET_FILE_DIR:${PROJECT_NAME}> ${CMAKE_COMMAND} -E create_symlink
        $<TARGET_FILE_NAME:Leo::Runtime>
        $<TARGET_SONAME_FILE_NAME:Leo::Runtime>

    COMMENT "Copying Leo/SDL3 and ensuring SONAME links next to ${PROJECT_NAME}"
    COMMAND_EXPAND_LISTS
  )
    endif()

endif()

