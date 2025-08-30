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

include(FetchContent)
set(LEO_BUILD_SHARED ON CACHE BOOL "Build Leo engine as a shared library" FORCE)
set(LEO_BUILD_TESTS OFF CACHE BOOL "Do not build Leo engine unit tests" FORCE)
set(LEO_VENDOR_SDL ON CACHE BOOL "Use Leo Engine's SDL3, as opposed to the systems" FORCE)

FetchContent_Declare(
    leo
    GIT_REPOSITORY https://github.com/bluesentinelsec/leo-engine
    GIT_TAG main
)
FetchContent_MakeAvailable(leo)

set(SOURCE_FILES 
    src/main.c
    external/getopt/getopt.c
    external/getopt/getopt.h
)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} PRIVATE Leo::Runtime)

if(LEO_BUILD_SHARED)
  if(WIN32)
    # Copy all dependent DLLs (Leo, SDL3, and transitive dependencies) next to the executable
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
              $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMAND_EXPAND_LISTS
      COMMENT "Copying runtime DLLs next to ${PROJECT_NAME}")
  elseif(APPLE)
    # Prefer the executable directory at runtime; copy dylibs
    set_property(TARGET ${PROJECT_NAME} PROPERTY BUILD_RPATH "@loader_path")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:Leo::Runtime>
              $<TARGET_FILE:SDL3::SDL3>
              $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMENT "Copying dylibs (Leo, SDL3) next to ${PROJECT_NAME}")
  elseif(UNIX AND NOT APPLE) # Linux/ELF
    # Prefer the executable directory at runtime; copy shared objects
    set_property(TARGET ${PROJECT_NAME} PROPERTY BUILD_RPATH "$ORIGIN")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:Leo::Runtime>
              $<TARGET_FILE:SDL3::SDL3>
              $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMENT "Copying shared objects (Leo, SDL3) next to ${PROJECT_NAME}")
  endif()
endif()
