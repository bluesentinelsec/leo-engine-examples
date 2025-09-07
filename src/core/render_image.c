#include "leo/engine.h"
#include <leo/leo.h>

#include <stdbool.h>
#include <stdio.h>

typedef struct ImageDemoState
{
    leo_Texture2D image; /* GPU texture */
    bool one_frame;      /* quit after one frame? */
} ImageDemoState;

static bool demo_setup(leo_GameContext *ctx)
{
    ImageDemoState *state = (ImageDemoState *)ctx->user_data;

    // Load the image from resources
    state->image = leo_LoadTexture("resources/images/ai_vista_1536x1024.png");

    printf("Loaded image %dx%d\n", state->image.width, state->image.height);
    return true; // success
}

static void demo_update(leo_GameContext *ctx)
{
    ImageDemoState *state = (ImageDemoState *)ctx->user_data;

    // Escape hatch for CI/CD: quit after one frame
    if (state->one_frame && ctx->frame >= 1)
    {
        leo_GameQuit(ctx);
    }
}

static void demo_render(leo_GameContext *ctx)
{
    ImageDemoState *state = (ImageDemoState *)ctx->user_data;

    // Full source rectangle = entire texture
    leo_Rectangle src = {
        .x = 0,
        .y = 0,
        .width = (float)state->image.width,
        .height = (float)state->image.height,
    };

    // Position to draw at top-left corner of screen
    leo_Vector2 pos = {0.0f, 0.0f};

    // Draw with white tint (no color change)
    leo_DrawTextureRec(state->image, src, pos, LEO_WHITE);
}

static void demo_render_ui(leo_GameContext *ctx)
{
    leo_DrawFPS(10, 10);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    ImageDemoState *state = (ImageDemoState *)ctx->user_data;
    leo_UnloadTexture(&state->image);

    printf("Shutting down Image Demo.\n");
}

bool ImageDemo(bool oneFrame)
{
    ImageDemoState state = {
        .image = {0},
        .one_frame = oneFrame,
    };

    leo_GameConfig cfg = {
        .window_width = 1536,
        .window_height = 1024,
        .window_title = "Image Demo",
        .target_fps = 60,
        .logical_width = 1536,
        .logical_height = 1024,
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

    cb.on_update = demo_update;
    cb.on_render_ui = demo_render;
    return (leo_GameRun(&cfg, &cb) == 0);
}
