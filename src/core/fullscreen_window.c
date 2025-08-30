/**
 * @file fullscreen_window.c
 * @brief Demonstrates Leo Engine’s fullscreen toggle behavior.
 *
 * ## Overview
 *
 * This demo shows how to enter fullscreen mode in Leo Engine and how users can
 * toggle between fullscreen and windowed modes at runtime.
 * - The window is created with leo_InitWindow() at 1280×720.
 * - Fullscreen mode is immediately enabled with leo_SetFullscreen(true).
 * - Pressing the **Tab** key toggles fullscreen on or off.
 *
 * ## Under the Hood
 *
 * - leo_InitWindow() creates a resizable SDL3 window by default.
 * - leo_SetFullscreen() wraps SDL_SetWindowFullscreen(), promoting the window
 *   to fullscreen or restoring it back to windowed mode.
 * - The input system (leo_IsKeyReleased(KEY_TAB)) detects when the Tab key is
 *   released. On that frame, the demo flips an internal @c fullscreen flag and
 *   applies the new state with leo_SetFullscreen().
 *
 * ## Summary
 *
 * - Starts in fullscreen at 1280×720.
 * - Press Tab to toggle between fullscreen and windowed.
 * - Uses Leo Engine’s keyboard system and window API together.
 * - Demonstrates how to handle runtime display mode changes safely across
 *   platforms.
 */

#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

bool FullscreenDemo(bool oneFrame)
{
    if (!leo_InitWindow(1280, 720, "Fullscreen Demo"))
    {
        return false;
    }

    bool fullscreen = true;
    leo_SetFullscreen(fullscreen);

    leo_SetTargetFPS(60);
    while (!leo_WindowShouldClose())
    {
        float deltaTime = leo_GetFrameTime();

        // --- toggle fullscreen on Tab key press ---
        if (leo_IsKeyReleased(KEY_TAB))
        {
            fullscreen = !fullscreen;
            leo_SetFullscreen(fullscreen);
            printf("Fullscreen %s\n", fullscreen ? "enabled" : "disabled");
        }

        // update game logic

        leo_ClearBackground(0, 0, 128, 255);
        leo_BeginDrawing();
        // draw your objects
        leo_EndDrawing();

        if (oneFrame)
            break; // escape hatch for CICD testing
    }
    leo_CloseWindow();
    return true;
}
