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

    if (!leo_MountResourcePack("resources.leopack", "password", 1))
    {
        printf("❌ Unable to mount resources.leopack\n");
        return false;
    }
    else
    {
        printf("✅ Successfully mounted resources.leopack\n");
    }

    // Load an image that we know exists in the Python-created pack
    state->image = leo_LoadTexture("images/background_320x200.png");
    
    if (state->image._handle == NULL) {
        printf("❌ Failed to load image from VFS\n");
        return false;
    }

    printf("✅ Loaded image from VFS: %dx%d\n", state->image.width, state->image.height);
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

    // Position to draw at center of screen
    float x = (1536 - state->image.width) / 2.0f;
    float y = (1024 - state->image.height) / 2.0f;
    leo_Vector2 pos = {x, y};

    // Draw with white tint (no color change)
    leo_DrawTextureRec(state->image, src, pos, LEO_WHITE);
}

static void demo_render_ui(leo_GameContext *ctx)
{
    leo_DrawFPS(10, 10);
    
    // Show that we're loading from VFS
    leo_DrawText("Image loaded from Python-created resources.leopack", 10, 40, 20, LEO_GREEN);
    leo_DrawText("VFS Path: images/background_320x200.png", 10, 70, 16, LEO_WHITE);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    ImageDemoState *state = (ImageDemoState *)ctx->user_data;
    leo_UnloadTexture(&state->image);

    printf("✅ Image Demo shutdown complete\n");
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
        .window_title = "Image Demo - VFS Loading",
        .target_fps = 60,
        .logical_width = 1536,
        .logical_height = 1024,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = LEO_GRAY,
        .start_paused = false,
        .user_data = &state,
    };

    leo_GameCallbacks cb = {
        .on_setup = demo_setup,
        .on_update = demo_update,
        .on_render_ui = demo_render,  // Use render_ui callback for rendering
        .on_shutdown = demo_shutdown,
    };

    return (leo_GameRun(&cfg, &cb) == 0);
}
