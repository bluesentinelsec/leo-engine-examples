/**
 * @file resizable_window.c
 * @brief Demonstrates Leo Engine’s default resizable window behavior.
 *
 * ## Overview
 *
 * This demo shows how a Leo Engine resizable window behaves.
 * The window is created with the leo_InitWindow() call, which internally uses
 * SDL3’s @c SDL_WINDOW_RESIZABLE flag. This allows the user to freely resize the
 * application window at runtime.
 *
 * ## Under the Hood
 *
 * - leo_InitWindow() sets up an SDL3 window and renderer with resizable flags.
 * - The engine’s global state keeps track of the current window size. When resized,
 *   SDL3 updates this automatically, and Leo reports the new size through
 *   leo_GetScreenWidth() and leo_GetScreenHeight().
 * - Since no logical resolution is enabled in this demo, SDL3 does **not** scale or
 *   letterbox the output. Your rendering directly matches the physical window pixels.
 *
 * ## Summary
 *
 * - Resizable by default: the user can change window size freely.
 * - Coordinate system tracks the physical window size at all times.
 * - This makes it easy to experiment with resizing behavior before introducing
 *   a fixed logical resolution in other demos.
 */

#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

bool ResizableDemo(bool oneFrame)
{
    if (!leo_InitWindow(1280, 720, "Resizable Demo"))
    {
        return false;
    }

    leo_SetTargetFPS(60);
    while (!leo_WindowShouldClose())
    {
        float deltaTime = leo_GetFrameTime();

        // update game logic
        printf("Window size: %dx%d\n", leo_GetScreenWidth(), leo_GetScreenHeight());

        leo_ClearBackground(0, 128, 0, 255);
        leo_BeginDrawing();
        // draw your objects
        leo_EndDrawing();

        if (oneFrame)
            break; // escape hatch for CICD testing
    }
    leo_CloseWindow();
    return true;
}
