#include <leo/leo.h>

#include <stdbool.h>
#include <stdio.h>
#include <math.h>

/* ----------------------------------------------------------
   Virtual Resolution Constants
   ---------------------------------------------------------- */
#define VIRTUAL_WIDTH 1280
#define VIRTUAL_HEIGHT 720
#define VIRTUAL_CENTER_X (VIRTUAL_WIDTH / 2)
#define VIRTUAL_CENTER_Y (VIRTUAL_HEIGHT / 2)

/* ----------------------------------------------------------
   Per-demo state (carried in ctx->user_data)
   ---------------------------------------------------------- */
typedef struct ShapesState
{
    bool fullscreen;
    bool one_frame;
    float time;
    int shape_mode;
    leo_Camera2D camera;
    float camera_speed;
} ShapesState;

/* ----------------------------------------------------------
   Callbacks
   ---------------------------------------------------------- */
static bool demo_setup(leo_GameContext *ctx)
{
    ShapesState *state = (ShapesState *)ctx->user_data;
    state->fullscreen = false;
    state->time = 0.0f;
    state->shape_mode = 0;
    state->camera_speed = 200.0f;
    
    // Initialize camera
    int w = leo_GetScreenWidth();
    int h = leo_GetScreenHeight();
    state->camera.target = (leo_Vector2){VIRTUAL_CENTER_X, VIRTUAL_CENTER_Y};
    state->camera.offset = (leo_Vector2){w / 2.0f, h / 2.0f};
    state->camera.rotation = 0.0f;
    state->camera.zoom = 1.0f;
    
    return true;
}

static void demo_update(leo_GameContext *ctx)
{
    ShapesState *state = (ShapesState *)ctx->user_data;
    state->time += leo_GetFrameTime();
    float dt = leo_GetFrameTime();

    // Toggle fullscreen on Tab key release
    if (leo_IsKeyReleased(KEY_TAB))
    {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
        printf("Fullscreen %s\n", state->fullscreen ? "enabled" : "disabled");
    }

    // Cycle through shape modes with SPACE
    if (leo_IsKeyReleased(KEY_SPACE))
    {
        state->shape_mode = (state->shape_mode + 1) % 4;
        printf("Shape mode: %d\n", state->shape_mode);
    }

    // Camera controls
    if (leo_IsKeyDown(KEY_W) || leo_IsKeyDown(KEY_UP))
        state->camera.target.y -= state->camera_speed * dt;
    if (leo_IsKeyDown(KEY_S) || leo_IsKeyDown(KEY_DOWN))
        state->camera.target.y += state->camera_speed * dt;
    if (leo_IsKeyDown(KEY_A) || leo_IsKeyDown(KEY_LEFT))
        state->camera.target.x -= state->camera_speed * dt;
    if (leo_IsKeyDown(KEY_D) || leo_IsKeyDown(KEY_RIGHT))
        state->camera.target.x += state->camera_speed * dt;

    // Camera zoom
    if (leo_IsKeyDown(KEY_Q))
        state->camera.zoom *= 1.0f + dt;
    if (leo_IsKeyDown(KEY_E))
        state->camera.zoom *= 1.0f - dt;
    
    // Clamp zoom
    if (state->camera.zoom < 0.1f) state->camera.zoom = 0.1f;
    if (state->camera.zoom > 5.0f) state->camera.zoom = 5.0f;

    // Reset camera
    if (leo_IsKeyReleased(KEY_R))
    {
        state->camera.target = (leo_Vector2){VIRTUAL_CENTER_X, VIRTUAL_CENTER_Y};
        state->camera.zoom = 1.0f;
        state->camera.rotation = 0.0f;
    }

    // Escape hatch for CI/CD: quit after one frame
    if (state->one_frame && ctx->frame >= 1)
    {
        leo_GameQuit(ctx);
    }

    // RENDERING HAPPENS HERE
    leo_BeginDrawing();
    leo_ClearBackground(20, 20, 30, 255);

    // Begin camera mode for world rendering
    leo_BeginMode2D(state->camera);

    const int centerX = VIRTUAL_CENTER_X;
    const int centerY = VIRTUAL_CENTER_Y;
    const float t = state->time;

    switch (state->shape_mode)
    {
        case 0: // Filled shapes showcase
        {
            // Animated filled circle - smoother pulsing
            float radius = 80 + 15 * sinf(t * 1.5f);
            leo_Color circleColor = {255, 120 + (int)(30 * sinf(t * 0.8f)), 100, 255};
            leo_DrawCircleFilled(centerX - 200, centerY - 100, radius, circleColor);

            // Rotating filled triangle - slower rotation
            float angle = t * 1.0f;
            int size = 60;
            int x1 = centerX + (int)(size * cosf(angle));
            int y1 = centerY - 100 + (int)(size * sinf(angle));
            int x2 = centerX + (int)(size * cosf(angle + 2.094f)); // 120 degrees
            int y2 = centerY - 100 + (int)(size * sinf(angle + 2.094f));
            int x3 = centerX + (int)(size * cosf(angle + 4.188f)); // 240 degrees
            int y3 = centerY - 100 + (int)(size * sinf(angle + 4.188f));
            
            leo_Color triangleColor = {100, 255, 150, 255};
            leo_DrawTriangleFilled(x1, y1, x2, y2, x3, y3, triangleColor);

            // Animated filled polygon (hexagon) - gentler pulsing
            int hexPoints[12];
            int hexRadius = 50 + (int)(10 * sinf(t * 2.0f));
            for (int i = 0; i < 6; i++)
            {
                float hexAngle = (float)i * 1.047f + t * 0.5f; // 60 degrees per side, slower rotation
                hexPoints[i * 2] = centerX + 200 + (int)(hexRadius * cosf(hexAngle));
                hexPoints[i * 2 + 1] = centerY - 100 + (int)(hexRadius * sinf(hexAngle));
            }
            leo_Color hexColor = {255, 200, 100, 200};
            leo_DrawPolyFilled(hexPoints, 6, hexColor);

            // Static filled rectangle with outline
            leo_DrawRectangle(centerX - 60, centerY + 50, 120, 80, (leo_Color){50, 150, 255, 255});
            leo_DrawRectangleLines(centerX - 60, centerY + 50, 120, 80, (leo_Color){255, 255, 255, 255});
            break;
        }

        case 1: // Outline shapes showcase
        {
            // Multiple concentric circles
            for (int i = 1; i <= 5; i++)
            {
                float radius = i * 25.0f;
                leo_Color color = {255 - i * 40, 100 + i * 30, 255, 255};
                leo_DrawCircle(centerX - 200, centerY, radius, color);
            }

            // Triangle outline with animated vertices
            int tri1X = centerX + (int)(80 * cosf(t));
            int tri1Y = centerY - 50 + (int)(30 * sinf(t * 2.0f));
            int tri2X = centerX + 80;
            int tri2Y = centerY + 50;
            int tri3X = centerX - 80;
            int tri3Y = centerY + 50;
            leo_DrawTriangle(tri1X, tri1Y, tri2X, tri2Y, tri3X, tri3Y, (leo_Color){255, 255, 100, 255});

            // Polygon outline (star shape)
            int starPoints[10];
            for (int i = 0; i < 5; i++)
            {
                float outerAngle = (float)i * 1.256f + t * 0.5f; // 72 degrees
                float innerAngle = outerAngle + 0.628f; // 36 degrees offset
                
                starPoints[i * 4] = centerX + 200 + (int)(60 * cosf(outerAngle));
                starPoints[i * 4 + 1] = centerY + (int)(60 * sinf(outerAngle));
                starPoints[i * 4 + 2] = centerX + 200 + (int)(25 * cosf(innerAngle));
                starPoints[i * 4 + 3] = centerY + (int)(25 * sinf(innerAngle));
            }
            // Rearrange for proper star drawing
            int orderedStar[10];
            for (int i = 0; i < 5; i++)
            {
                orderedStar[i * 2] = starPoints[i * 4];
                orderedStar[i * 2 + 1] = starPoints[i * 4 + 1];
            }
            leo_DrawPoly(orderedStar, 5, (leo_Color){255, 150, 150, 255});

            // Rectangle outlines of different sizes
            for (int i = 0; i < 3; i++)
            {
                int size = 40 + i * 20;
                leo_Color rectColor = {100 + i * 50, 255 - i * 50, 100, 255};
                leo_DrawRectangleLines(centerX - size/2, centerY + 100 + i * 10, size, size/2, rectColor);
            }
            break;
        }

        case 2: // Mixed shapes animation
        {
            // Orbiting filled circles - slower, smoother movement
            for (int i = 0; i < 8; i++)
            {
                float angle = (float)i * 0.785f + t * 0.8f; // 45 degrees apart, slower orbit
                int orbitRadius = 150;
                int x = centerX + (int)(orbitRadius * cosf(angle));
                int y = centerY + (int)(orbitRadius * sinf(angle));
                
                // Gentler color transitions
                leo_Color color = {
                    (int)(128 + 80 * sinf(angle * 0.5f)),
                    (int)(128 + 80 * cosf(angle * 0.5f)),
                    255,
                    200
                };
                leo_DrawCircleFilled(x, y, 15, color);
            }

            // Central pulsing triangle - gentler pulsing
            float pulseSize = 40 + 15 * sinf(t * 2.5f);
            leo_DrawTriangleFilled(
                centerX, centerY - (int)pulseSize,
                centerX - (int)pulseSize, centerY + (int)pulseSize,
                centerX + (int)pulseSize, centerY + (int)pulseSize,
                (leo_Color){255, 255, 255, 180}
            );
            break;
        }

        case 3: // Stress test - many shapes
        {
            // Grid of small shapes with smoother animations
            for (int x = 0; x < 20; x++)
            {
                for (int y = 0; y < 12; y++)
                {
                    int posX = centerX - 600 + x * 60;
                    int posY = centerY - 330 + y * 55;
                    
                    // Slower, smoother color transitions
                    float phase = (float)(x + y) * 0.2f + t * 0.3f;
                    leo_Color color = {
                        (int)(128 + 64 * sinf(phase)),
                        (int)(128 + 64 * cosf(phase * 0.7f)),
                        (int)(128 + 64 * sinf(phase * 0.5f)),
                        255
                    };

                    int shapeType = (x + y) % 4;
                    switch (shapeType)
                    {
                        case 0:
                            leo_DrawCircleFilled(posX, posY, 12, color);
                            break;
                        case 1:
                            leo_DrawRectangle(posX - 8, posY - 8, 16, 16, color);
                            break;
                        case 2:
                            leo_DrawTriangleFilled(posX, posY - 10, posX - 8, posY + 6, posX + 8, posY + 6, color);
                            break;
                        case 3:
                            leo_DrawRectangleLines(posX - 10, posY - 10, 20, 20, color);
                            break;
                    }
                }
            }
            break;
        }
    }

    // Draw world grid for reference
    leo_Color gridColor = {40, 40, 50, 255};
    for (int x = -2000; x <= 2000; x += 100)
    {
        leo_DrawLine(x, -1500, x, 1500, gridColor);
    }
    for (int y = -1500; y <= 1500; y += 100)
    {
        leo_DrawLine(-2000, y, 2000, y, gridColor);
    }

    // End camera mode
    leo_EndMode2D();

    leo_EndDrawing();
}

static void demo_render_ui(leo_GameContext *ctx)
{
    ShapesState *state = (ShapesState *)ctx->user_data;
    
    leo_DrawFPS(20, 32);
    
    // Instructions
    leo_DrawText("SPACE: Change mode", 20, 60, 20, LEO_WHITE);
    leo_DrawText("TAB: Toggle fullscreen", 20, 85, 20, LEO_WHITE);
    leo_DrawText("WASD/Arrows: Move camera", 20, 110, 20, LEO_WHITE);
    leo_DrawText("Q/E: Zoom in/out", 20, 135, 20, LEO_WHITE);
    leo_DrawText("R: Reset camera", 20, 160, 20, LEO_WHITE);
    
    // Current mode
    const char* modeNames[] = {"Filled Shapes", "Outline Shapes", "Animation", "Stress Test"};
    char modeText[64];
    snprintf(modeText, sizeof(modeText), "Mode: %s", modeNames[state->shape_mode]);
    leo_DrawText(modeText, 20, 190, 20, LEO_YELLOW);
    
    // Camera info
    char cameraText[128];
    snprintf(cameraText, sizeof(cameraText), "Camera: (%.0f, %.0f) Zoom: %.2f", 
             state->camera.target.x, state->camera.target.y, state->camera.zoom);
    leo_DrawText(cameraText, 20, 215, 20, LEO_GREEN);
}

static void demo_shutdown(leo_GameContext *ctx)
{
}

/* ----------------------------------------------------------
   Entrypoint for demo registry
   ---------------------------------------------------------- */
bool ShapesDemo(bool oneFrame)
{
    ShapesState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
        .time = 0.0f,
        .shape_mode = 0,
    };

    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Leo Engine - New Shape Functions Demo",
        .target_fps = 60,
        .logical_width = VIRTUAL_WIDTH,
        .logical_height = VIRTUAL_HEIGHT,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = LEO_BLACK,
        .start_paused = false,
        .user_data = &state,
    };

    leo_GameCallbacks cb = {
        .on_setup = demo_setup,
        .on_update = demo_update,
        .on_render_ui = demo_render_ui,
        .on_shutdown = demo_shutdown,
    };

    return (leo_GameRun(&cfg, &cb) == 0);
}
