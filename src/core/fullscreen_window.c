#include <leo/leo.h>

#include <stdbool.h>
#include <stdio.h>

/* ----------------------------------------------------------
   Per-demo state (carried in ctx->user_data)
   ---------------------------------------------------------- */
typedef struct FullscreenState
{
    bool fullscreen;
    bool one_frame;
} FullscreenState;

/* ----------------------------------------------------------
   Callbacks
   ---------------------------------------------------------- */
static bool demo_setup(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;
    state->fullscreen = true;
    leo_SetFullscreen(state->fullscreen);
    printf("Fullscreen enabled (startup)\n");
    return true; // success
}

static void demo_update(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;

    // Toggle fullscreen on Tab key release
    if (leo_IsKeyReleased(KEY_TAB))
    {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
        printf("Fullscreen %s\n", state->fullscreen ? "enabled" : "disabled");
    }

    // Escape hatch for CI/CD: quit after one frame
    if (state->one_frame && ctx->frame >= 1)
    {
        leo_GameQuit(ctx);
    }
}

static void demo_render_ui(leo_GameContext *ctx)
{
    leo_DrawFPS(10, 10);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    printf("Shutting down Fullscreen Demo.\n");
}

/* ----------------------------------------------------------
   Entrypoint for demo registry
   ---------------------------------------------------------- */
bool FullscreenDemo(bool oneFrame)
{
    FullscreenState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
    };

    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Fullscreen Demo",
        .target_fps = 60,
        .logical_width = 0,
        .logical_height = 0,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = LEO_ORANGE,
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
