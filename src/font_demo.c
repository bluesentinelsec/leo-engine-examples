#include <leo/leo.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct FontDemoState
{
    bool fullscreen;
    bool one_frame;
    leo_Font font;
    int score;
} FontDemoState;

static bool demo_setup(leo_GameContext *ctx)
{
    FontDemoState *state = (FontDemoState *)ctx->user_data;
    state->fullscreen = true;
    leo_SetFullscreen(state->fullscreen);

    if (!leo_MountResourcePack("resources.leopack", "password", 1))
    {
        printf("❌ Unable to mount resources.leopack\n");
        return false;
    }
    else
    {
        printf("✅ Successfully mounted resources.leopack\n");
    }

    state->font = leo_LoadFont("font/font.ttf", 32);
    if (!leo_IsFontReady(state->font)) {
        printf("Failed to load font\n");
        return false;
    }

    state->score = 0;

    return true;
}

static void demo_update(leo_GameContext *ctx)
{
    FontDemoState *state = (FontDemoState *)ctx->user_data;

    // Increment score every 60 frames (1 second at 60 FPS)
    if (ctx->frame % 60 == 0 && ctx->frame > 0)
    {
        state->score++;
    }

    if (leo_IsKeyReleased(KEY_TAB))
    {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
        printf("Fullscreen %s\n", state->fullscreen ? "enabled" : "disabled");
    }

    if (state->one_frame && ctx->frame >= 1)
    {
        leo_GameQuit(ctx);
    }
}

static void demo_render_ui(leo_GameContext *ctx)
{
    FontDemoState *state = (FontDemoState *)ctx->user_data;
    char buffer[64];

    // Test with static digits first
    leo_DrawTextEx(state->font, "Score: 123", (leo_Vector2){100, 100}, 32, 1, LEO_WHITE);

    // Test dynamic string
    SDL_snprintf(buffer, sizeof(buffer), "Dynamic: %d", state->score);
    leo_DrawTextEx(state->font, buffer, (leo_Vector2){100, 150}, 32, 1, LEO_WHITE);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    FontDemoState *state = (FontDemoState *)ctx->user_data;
    leo_UnloadFont(&state->font);
}

bool FontDemo(bool oneFrame)
{
    FontDemoState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
        .font = {0},
        .score = 0,
    };

    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Font Demo",
        .target_fps = 60,
        .logical_width = 1280,
        .logical_height = 720,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = LEO_GREEN,
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
