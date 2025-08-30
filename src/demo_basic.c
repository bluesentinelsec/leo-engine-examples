#include <leo/leo.h>

#include <stdbool.h>
#include <stdio.h>

bool RunDemo_Basic(bool one_frame)
{
    if (!leo_InitWindow(1280, 720, "Basic Demo"))
    {
        fprintf(stderr, "%s\n", leo_GetError());
        return false;
    }

    leo_SetTargetFPS(60);

    while (!leo_WindowShouldClose())
    {
        float deltaTime = leo_GetFrameTime();
        // update game logic

        leo_ClearBackground(0, 255, 255, 255);

        leo_BeginDrawing();

        // draw actors

        leo_EndDrawing();

        // an escape hatch for CICD verification
        if (one_frame)
        {
            break;
        }
    }

    leo_CloseWindow();

    return true;
}
