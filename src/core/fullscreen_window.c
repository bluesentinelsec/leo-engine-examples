#include "leo/keys.h"
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
        if (leo_IsKeyPressed(KEY_TAB))
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
