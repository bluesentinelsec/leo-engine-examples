#include <leo/leo.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

/* ----------------------------------------------------------
   Per-demo state (carried in ctx->user_data)
   ---------------------------------------------------------- */
typedef struct FullscreenState
{
    bool fullscreen;
    bool one_frame;
    float time;             // Track elapsed time for animations
    bool show_pixels;       // Toggle pixel starfield
    bool show_circles;      // Toggle pulsating circles
    bool show_lines;        // Toggle wireframe grid
    bool show_rectangles;   // Toggle bouncing rectangles
    float rect_x, rect_y;   // Rectangle position
    float rect_vx, rect_vy; // Rectangle velocity
} FullscreenState;

/* ----------------------------------------------------------
   Callbacks
   ---------------------------------------------------------- */
static bool demo_setup(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;
    state->fullscreen = true;
    state->show_pixels = true;
    state->show_circles = true;
    state->show_lines = true;
    state->show_rectangles = true;
    state->time = 0.0f;
    state->rect_x = 640.0f; // Center of 1280x720
    state->rect_y = 360.0f;
    state->rect_vx = 200.0f; // Pixels per second
    state->rect_vy = 150.0f;
    leo_SetFullscreen(state->fullscreen);
    printf("Fullscreen enabled (startup)\n");
    return true; // success
}

static void demo_update(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;
    float delta_time = leo_GetFrameTime();

    // Update time for animations
    state->time += delta_time;

    // Toggle fullscreen on Tab key release
    if (leo_IsKeyReleased(KEY_TAB))
    {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
        printf("Fullscreen %s\n", state->fullscreen ? "enabled" : "disabled");
    }

    // Toggle effects on key presses
    if (leo_IsKeyReleased(KEY_SPACE))
        state->show_pixels = !state->show_pixels;
    if (leo_IsKeyReleased(KEY_1))
        state->show_circles = !state->show_circles;
    if (leo_IsKeyReleased(KEY_2))
        state->show_lines = !state->show_lines;
    if (leo_IsKeyReleased(KEY_3))
        state->show_rectangles = !state->show_rectangles;

    // Update bouncing rectangle
    state->rect_x += state->rect_vx * delta_time;
    state->rect_y += state->rect_vy * delta_time;

    // Bounce off screen edges (1280x720)
    if (state->rect_x < 50 || state->rect_x > 1230)
        state->rect_vx = -state->rect_vx;
    if (state->rect_y < 50 || state->rect_y > 670)
        state->rect_vy = -state->rect_vy;

    // Escape hatch for CI/CD: quit after one frame
    if (state->one_frame && ctx->frame >= 1)
    {
        leo_GameQuit(ctx);
    }
}

static void demo_render_ui(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;

    // Draw FPS counter
    leo_DrawFPS(20, 32);

    // Color cycling based on time
    float t = state->time;
    leo_Color color1 = {(uint8_t)(sin(t) * 127 + 128), 0, (uint8_t)(cos(t) * 127 + 128), 255};
    leo_Color color2 = {0, (uint8_t)(sin(t + 1.0f) * 127 + 128), (uint8_t)(cos(t + 1.0f) * 127 + 128), 255};
    leo_Color color3 = {(uint8_t)(cos(t + 2.0f) * 127 + 128), (uint8_t)(sin(t + 2.0f) * 127 + 128), 0, 255};

    // Draw starfield (pixels)
    if (state->show_pixels)
    {
        for (int i = 0; i < 100; i++)
        {
            int x = (int)(sin(t + i * 0.1f) * 600 + 640); // Center at 640
            int y = (int)(cos(t + i * 0.2f) * 300 + 360); // Center at 360
            leo_DrawPixel(x, y, color1);
        }
    }

    // Draw pulsating circles
    if (state->show_circles)
    {
        for (int i = 0; i < 3; i++)
        {
            float radius = 50.0f + sin(t + i * 2.0f) * 20.0f;
            int center_x = 640 + i * 200 - 200; // Spread across screen
            int center_y = 360;
            leo_DrawCircle(center_x, center_y, radius, color2);
        }
    }

    // Draw rotating wireframe grid (lines)
    if (state->show_lines)
    {
        for (int i = -3; i <= 3; i++)
        {
            int offset = i * 100;
            float rot = sin(t * 0.5f) * 0.2f; // Small rotation
            int x1 = (int)(640 + offset * cos(rot) - 300 * sin(rot));
            int y1 = (int)(360 + offset * sin(rot) + 300 * cos(rot));
            int x2 = (int)(640 + offset * cos(rot) + 300 * sin(rot));
            int y2 = (int)(360 + offset * sin(rot) - 300 * cos(rot));
            leo_DrawLine(x1, y1, x2, y2, color3);
            // Perpendicular lines
            x1 = (int)(640 - 300 * cos(rot) + offset * sin(rot));
            y1 = (int)(360 - 300 * sin(rot) + offset * cos(rot));
            x2 = (int)(640 + 300 * cos(rot) + offset * sin(rot));
            y2 = (int)(360 + 300 * sin(rot) + offset * cos(rot));
            leo_DrawLine(x1, y1, x2, y2, color3);
        }
    }

    // Draw bouncing rectangles
    if (state->show_rectangles)
    {
        leo_DrawRectangle((int)state->rect_x - 50, (int)state->rect_y - 50, 100, 100, color1);
        leo_DrawRectangle((int)state->rect_x - 25, (int)state->rect_y - 25, 50, 50, color2);
    }
}

static void demo_shutdown(leo_GameContext *ctx)
{
    // No cleanup needed
}

/* ----------------------------------------------------------
   Entrypoint for demo registry
   ---------------------------------------------------------- */
bool GraphicsDemo(bool oneFrame)
{
    FullscreenState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
        .show_pixels = true,
        .show_circles = true,
        .show_lines = true,
        .show_rectangles = true,
        .time = 0.0f,
        .rect_x = 640.0f,
        .rect_y = 360.0f,
        .rect_vx = 200.0f,
        .rect_vy = 150.0f,
    };

    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Graphics Demo",
        .target_fps = 60,
        .logical_width = 0,
        .logical_height = 0,
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
