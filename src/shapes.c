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
}

static void demo_render_ui(leo_GameContext *ctx)
{
    ShapesState *state = (ShapesState *)ctx->user_data;
    
    // Begin camera mode for world rendering
    leo_BeginMode2D(state->camera);

    const int centerX = VIRTUAL_CENTER_X;
    const int centerY = VIRTUAL_CENTER_Y;
    const float t = state->time;

    switch (state->shape_mode)
    {
        case 0: // Filled shapes showcase - SPECTACULAR!
        {
            // Multiple pulsing circles with trails
            for (int i = 0; i < 12; i++)
            {
                float angle = (float)i * 0.524f + t * 0.8f; // 30 degrees apart
                float radius = 60 + 30 * sinf(t * 2.0f + (float)i * 0.5f);
                int x = centerX - 300 + (int)(120 * cosf(angle));
                int y = centerY - 100 + (int)(80 * sinf(angle));
                
                leo_Color color = {
                    (int)(255 * (0.5f + 0.5f * sinf(t + (float)i))),
                    (int)(255 * (0.5f + 0.5f * cosf(t * 1.3f + (float)i))),
                    255,
                    180
                };
                leo_DrawCircleFilled(x, y, radius, color);
            }

            // Rotating triangle constellation
            for (int layer = 0; layer < 3; layer++)
            {
                float layerAngle = t * (1.0f + layer * 0.3f);
                int layerSize = 40 + layer * 20;
                int layerRadius = 80 + layer * 40;
                
                for (int i = 0; i < 6; i++)
                {
                    float angle = (float)i * 1.047f + layerAngle; // 60 degrees
                    int cx = centerX + (int)(layerRadius * cosf(angle));
                    int cy = centerY - 100 + (int)(layerRadius * sinf(angle));
                    
                    int x1 = cx + (int)(layerSize * cosf(angle));
                    int y1 = cy + (int)(layerSize * sinf(angle));
                    int x2 = cx + (int)(layerSize * cosf(angle + 2.094f));
                    int y2 = cy + (int)(layerSize * sinf(angle + 2.094f));
                    int x3 = cx + (int)(layerSize * cosf(angle + 4.188f));
                    int y3 = cy + (int)(layerSize * sinf(angle + 4.188f));
                    
                    leo_Color color = {
                        100 + layer * 50,
                        255 - layer * 60,
                        150 + layer * 30,
                        200 - layer * 40
                    };
                    leo_DrawTriangleFilled(x1, y1, x2, y2, x3, y3, color);
                }
            }

            // Morphing polygon garden
            for (int i = 0; i < 8; i++)
            {
                float baseAngle = (float)i * 0.785f; // 45 degrees
                int baseX = centerX + 200 + (int)(150 * cosf(baseAngle));
                int baseY = centerY - 100 + (int)(100 * sinf(baseAngle));
                
                int sides = 3 + (int)(3 * (0.5f + 0.5f * sinf(t * 2.0f + (float)i)));
                int polyPoints[16]; // Max 8 sides = 16 coordinates
                
                int radius = 30 + (int)(20 * sinf(t * 3.0f + (float)i));
                for (int j = 0; j < sides; j++)
                {
                    float polyAngle = (float)j * (6.283f / (float)sides) + t * 0.5f;
                    polyPoints[j * 2] = baseX + (int)(radius * cosf(polyAngle));
                    polyPoints[j * 2 + 1] = baseY + (int)(radius * sinf(polyAngle));
                }
                
                leo_Color color = {
                    (int)(255 * (0.3f + 0.7f * sinf(t + (float)i * 0.7f))),
                    (int)(255 * (0.3f + 0.7f * cosf(t * 1.1f + (float)i * 0.7f))),
                    (int)(255 * (0.3f + 0.7f * sinf(t * 0.8f + (float)i * 0.7f))),
                    160
                };
                leo_DrawPolyFilled(polyPoints, sides, color);
            }

            // Cascading rectangles
            for (int i = 0; i < 15; i++)
            {
                float cascade = t * 2.0f + (float)i * 0.3f;
                int x = centerX - 60 + (int)(i * 8 * sinf(cascade));
                int y = centerY + 50 + i * 6;
                int w = 120 - i * 4;
                int h = 80 - i * 3;
                
                leo_Color fillColor = {
                    50 + i * 10,
                    150 + (int)(50 * sinf(cascade)),
                    255 - i * 8,
                    255 - i * 15
                };
                leo_Color lineColor = {255, 255, 255, 100 + i * 8};
                
                leo_DrawRectangle(x, y, w, h, fillColor);
                leo_DrawRectangleLines(x, y, w, h, lineColor);
            }
            break;
        }

        case 1: // Outline shapes showcase - GEOMETRIC MADNESS!
        {
            // Concentric circle waves
            for (int wave = 0; wave < 3; wave++)
            {
                int waveX = centerX - 400 + wave * 200;
                for (int i = 1; i <= 12; i++)
                {
                    float radius = i * 15.0f + 20 * sinf(t * 2.0f + (float)i * 0.3f + (float)wave);
                    leo_Color color = {
                        (int)(255 * (0.4f + 0.6f * sinf(t + (float)i * 0.2f + (float)wave))),
                        100 + i * 12 + wave * 30,
                        255 - i * 15,
                        200
                    };
                    leo_DrawCircle(waveX, centerY, radius, color);
                }
            }

            // Dancing triangle swarm
            for (int i = 0; i < 20; i++)
            {
                float swarmAngle = (float)i * 0.314f + t * 1.5f;
                float distance = 100 + 50 * sinf(t * 2.0f + (float)i * 0.4f);
                int swarmX = centerX + (int)(distance * cosf(swarmAngle));
                int swarmY = centerY + (int)(distance * sinf(swarmAngle));
                
                float triAngle = t * 3.0f + (float)i;
                int size = 20 + (int)(15 * sinf(t * 4.0f + (float)i));
                
                int x1 = swarmX + (int)(size * cosf(triAngle));
                int y1 = swarmY + (int)(size * sinf(triAngle));
                int x2 = swarmX + (int)(size * cosf(triAngle + 2.094f));
                int y2 = swarmY + (int)(size * sinf(triAngle + 2.094f));
                int x3 = swarmX + (int)(size * cosf(triAngle + 4.188f));
                int y3 = swarmY + (int)(size * sinf(triAngle + 4.188f));
                
                leo_Color color = {
                    255,
                    (int)(255 * (0.3f + 0.7f * sinf(t + (float)i * 0.5f))),
                    100 + (int)(100 * cosf(t * 1.2f + (float)i * 0.5f)),
                    180
                };
                leo_DrawTriangle(x1, y1, x2, y2, x3, y3, color);
            }

            // Polygon kaleidoscope
            for (int layer = 0; layer < 4; layer++)
            {
                int sides = 5 + layer;
                float layerAngle = t * (0.5f + layer * 0.2f);
                int layerRadius = 80 + layer * 30;
                
                for (int copy = 0; copy < 6; copy++)
                {
                    float copyAngle = (float)copy * 1.047f; // 60 degrees
                    int copyX = centerX + 300 + (int)(100 * cosf(copyAngle));
                    int copyY = centerY + (int)(100 * sinf(copyAngle));
                    
                    int polyPoints[16];
                    for (int i = 0; i < sides; i++)
                    {
                        float angle = (float)i * (6.283f / (float)sides) + layerAngle;
                        polyPoints[i * 2] = copyX + (int)(layerRadius * cosf(angle));
                        polyPoints[i * 2 + 1] = copyY + (int)(layerRadius * sinf(angle));
                    }
                    
                    leo_Color color = {
                        255 - layer * 40 - copy * 20,
                        150 + layer * 20,
                        255 - copy * 30,
                        150 - layer * 20
                    };
                    leo_DrawPoly(polyPoints, sides, color);
                }
            }

            // Rectangle spiral
            for (int i = 0; i < 25; i++)
            {
                float spiralAngle = (float)i * 0.5f + t;
                int spiralRadius = i * 8;
                int spiralX = centerX + (int)(spiralRadius * cosf(spiralAngle));
                int spiralY = centerY + 200 + (int)(spiralRadius * sinf(spiralAngle));
                
                int size = 30 - i;
                if (size > 5)
                {
                    leo_Color color = {
                        100 + i * 6,
                        255 - i * 8,
                        100 + (int)(100 * sinf(t + (float)i * 0.3f)),
                        255 - i * 8
                    };
                    leo_DrawRectangleLines(spiralX - size/2, spiralY - size/2, size, size, color);
                }
            }
            break;
        }

        case 2: // Mixed shapes animation - PARTICLE EXPLOSION!
        {
            // Multi-layer orbital system
            for (int layer = 0; layer < 4; layer++)
            {
                int numOrbiters = 6 + layer * 2;
                float layerSpeed = 0.8f + layer * 0.3f;
                int layerRadius = 100 + layer * 50;
                
                for (int i = 0; i < numOrbiters; i++)
                {
                    float angle = (float)i * (6.283f / (float)numOrbiters) + t * layerSpeed;
                    int x = centerX + (int)(layerRadius * cosf(angle));
                    int y = centerY + (int)(layerRadius * sinf(angle));
                    
                    // Orbiting circles with trails
                    int size = 8 + layer * 4 + (int)(6 * sinf(t * 3.0f + angle));
                    leo_Color color = {
                        (int)(255 * (0.4f + 0.6f * sinf(angle + t))),
                        (int)(255 * (0.4f + 0.6f * cosf(angle * 1.3f + t))),
                        255 - layer * 40,
                        200 - layer * 30
                    };
                    leo_DrawCircleFilled(x, y, size, color);
                    
                    // Add trailing triangles
                    if (layer % 2 == 0)
                    {
                        float trailAngle = angle + 3.14159f; // Opposite direction
                        int tx = x + (int)(20 * cosf(trailAngle));
                        int ty = y + (int)(20 * sinf(trailAngle));
                        int tsize = size / 2;
                        
                        int tx1 = tx + (int)(tsize * cosf(trailAngle));
                        int ty1 = ty + (int)(tsize * sinf(trailAngle));
                        int tx2 = tx + (int)(tsize * cosf(trailAngle + 2.094f));
                        int ty2 = ty + (int)(tsize * sinf(trailAngle + 2.094f));
                        int tx3 = tx + (int)(tsize * cosf(trailAngle + 4.188f));
                        int ty3 = ty + (int)(tsize * sinf(trailAngle + 4.188f));
                        
                        leo_Color trailColor = color;
                        trailColor.a = 100;
                        leo_DrawTriangleFilled(tx1, ty1, tx2, ty2, tx3, ty3, trailColor);
                    }
                }
            }

            // Central pulsing polygon morphing
            int sides = 3 + (int)(5 * (0.5f + 0.5f * sinf(t * 1.5f)));
            float pulseSize = 60 + 40 * sinf(t * 2.5f);
            int morphPoints[16];
            
            for (int i = 0; i < sides; i++)
            {
                float angle = (float)i * (6.283f / (float)sides) + t;
                float radius = pulseSize + 20 * sinf(t * 4.0f + (float)i);
                morphPoints[i * 2] = centerX + (int)(radius * cosf(angle));
                morphPoints[i * 2 + 1] = centerY + (int)(radius * sinf(angle));
            }
            
            leo_Color morphColor = {
                (int)(255 * (0.6f + 0.4f * sinf(t * 2.0f))),
                (int)(255 * (0.6f + 0.4f * cosf(t * 1.7f))),
                (int)(255 * (0.6f + 0.4f * sinf(t * 2.3f))),
                180
            };
            leo_DrawPolyFilled(morphPoints, sides, morphColor);
            
            // Outline version for extra effect
            morphColor.a = 255;
            leo_DrawPoly(morphPoints, sides, morphColor);

            // Particle burst system
            for (int burst = 0; burst < 3; burst++)
            {
                float burstTime = fmodf(t + (float)burst * 2.0f, 6.0f);
                if (burstTime < 3.0f)
                {
                    int burstX = centerX + (burst - 1) * 200;
                    int burstY = centerY - 150;
                    
                    for (int p = 0; p < 30; p++)
                    {
                        float particleAngle = (float)p * 0.209f; // ~12 degrees
                        float particleSpeed = 50 + (float)p * 3;
                        float particleDistance = particleSpeed * burstTime;
                        
                        int px = burstX + (int)(particleDistance * cosf(particleAngle));
                        int py = burstY + (int)(particleDistance * sinf(particleAngle));
                        
                        float life = 1.0f - (burstTime / 3.0f);
                        int size = (int)(8 * life);
                        
                        if (size > 1)
                        {
                            leo_Color pColor = {
                                (int)(255 * life),
                                (int)(255 * life * (0.5f + 0.5f * sinf((float)p))),
                                (int)(255 * life * (0.5f + 0.5f * cosf((float)p))),
                                (int)(255 * life)
                            };
                            
                            if (p % 3 == 0)
                                leo_DrawCircleFilled(px, py, size, pColor);
                            else if (p % 3 == 1)
                                leo_DrawRectangle(px - size, py - size, size * 2, size * 2, pColor);
                            else
                            {
                                int tx1 = px, ty1 = py - size;
                                int tx2 = px - size, ty2 = py + size;
                                int tx3 = px + size, ty3 = py + size;
                                leo_DrawTriangleFilled(tx1, ty1, tx2, ty2, tx3, ty3, pColor);
                            }
                        }
                    }
                }
            }
            break;
        }

        case 3: // Stress test - ABSOLUTE CHAOS!
        {
            // Massive grid of animated shapes
            for (int x = 0; x < 30; x++)
            {
                for (int y = 0; y < 20; y++)
                {
                    int posX = centerX - 900 + x * 60;
                    int posY = centerY - 600 + y * 60;
                    
                    // Wave-based animation
                    float wave = sinf(t * 2.0f + (float)x * 0.2f + (float)y * 0.15f);
                    float phase = (float)(x + y) * 0.1f + t * 0.5f;
                    
                    // Dynamic colors
                    leo_Color color = {
                        (int)(128 + 127 * sinf(phase)),
                        (int)(128 + 127 * cosf(phase * 1.3f)),
                        (int)(128 + 127 * sinf(phase * 0.7f + wave)),
                        200 + (int)(55 * wave)
                    };

                    // Size variation
                    int baseSize = 8 + (int)(6 * wave);
                    
                    int shapeType = (x * 7 + y * 11) % 6; // More variety
                    switch (shapeType)
                    {
                        case 0:
                            leo_DrawCircleFilled(posX, posY, baseSize, color);
                            break;
                        case 1:
                            leo_DrawRectangle(posX - baseSize, posY - baseSize, baseSize * 2, baseSize * 2, color);
                            break;
                        case 2:
                            leo_DrawTriangleFilled(posX, posY - baseSize, posX - baseSize, posY + baseSize, posX + baseSize, posY + baseSize, color);
                            break;
                        case 3:
                            leo_DrawRectangleLines(posX - baseSize, posY - baseSize, baseSize * 2, baseSize * 2, color);
                            break;
                        case 4:
                            leo_DrawCircle(posX, posY, baseSize, color);
                            break;
                        case 5:
                            leo_DrawTriangle(posX, posY - baseSize, posX - baseSize, posY + baseSize, posX + baseSize, posY + baseSize, color);
                            break;
                    }
                }
            }

            // Overlay: Massive rotating polygons
            for (int layer = 0; layer < 5; layer++)
            {
                int sides = 6 + layer;
                float layerAngle = t * (0.3f + layer * 0.1f);
                int layerRadius = 200 + layer * 100;
                
                for (int copy = 0; copy < 3; copy++)
                {
                    float copyAngle = (float)copy * 2.094f; // 120 degrees
                    int copyX = centerX + (int)(300 * cosf(copyAngle));
                    int copyY = centerY + (int)(200 * sinf(copyAngle));
                    
                    int polyPoints[16];
                    for (int i = 0; i < sides; i++)
                    {
                        float angle = (float)i * (6.283f / (float)sides) + layerAngle;
                        polyPoints[i * 2] = copyX + (int)(layerRadius * cosf(angle));
                        polyPoints[i * 2 + 1] = copyY + (int)(layerRadius * sinf(angle));
                    }
                    
                    leo_Color color = {
                        255 - layer * 30,
                        100 + layer * 30,
                        255 - copy * 60,
                        80 - layer * 10
                    };
                    leo_DrawPoly(polyPoints, sides, color);
                }
            }

            // Chaos mode: Random shapes everywhere
            for (int chaos = 0; chaos < 100; chaos++)
            {
                // Pseudo-random based on time and index
                float seed = t * 0.1f + (float)chaos * 0.37f;
                float randX = sinf(seed * 7.13f) * 800;
                float randY = cosf(seed * 5.79f) * 400;
                float randSize = 5 + 15 * (0.5f + 0.5f * sinf(seed * 3.21f));
                
                int x = centerX + (int)randX;
                int y = centerY + (int)randY;
                int size = (int)randSize;
                
                leo_Color chaosColor = {
                    (int)(255 * (0.5f + 0.5f * sinf(seed * 2.1f))),
                    (int)(255 * (0.5f + 0.5f * cosf(seed * 1.7f))),
                    (int)(255 * (0.5f + 0.5f * sinf(seed * 2.9f))),
                    100 + (int)(100 * (0.5f + 0.5f * sinf(seed)))
                };
                
                int chaosType = (int)(sinf(seed * 4.33f) * 3 + 3) % 4;
                switch (chaosType)
                {
                    case 0:
                        leo_DrawCircleFilled(x, y, size, chaosColor);
                        break;
                    case 1:
                        leo_DrawRectangle(x - size, y - size, size * 2, size * 2, chaosColor);
                        break;
                    case 2:
                        leo_DrawTriangleFilled(x, y - size, x - size, y + size, x + size, y + size, chaosColor);
                        break;
                    case 3:
                        leo_DrawRectangleLines(x - size, y - size, size * 2, size * 2, chaosColor);
                        break;
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
    
    // UI overlay (rendered in screen space)
    leo_DrawFPS(20, 32);
    
    // Instructions
    leo_DrawText("SPACE: Change mode", 20, 60, 20, LEO_WHITE);
    leo_DrawText("TAB: Toggle fullscreen", 20, 85, 20, LEO_WHITE);
    leo_DrawText("WASD/Arrows: Move camera", 20, 110, 20, LEO_WHITE);
    leo_DrawText("Q/E: Zoom in/out", 20, 135, 20, LEO_WHITE);
    leo_DrawText("R: Reset camera", 20, 160, 20, LEO_WHITE);
    
    // Current mode
    const char* modeNames[] = {"Spectacular Fills", "Geometric Madness", "Particle Explosion", "Absolute Chaos"};
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
