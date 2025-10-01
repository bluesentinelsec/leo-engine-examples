#include <leo/leo.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct FontDemoState
{
    bool fullscreen;
    bool one_frame;
    leo_Font font16;
    leo_Font font24;
    leo_Font font32;
    leo_Font font48;
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

    state->font16 = leo_LoadFont("font/font.ttf", 16);
    state->font24 = leo_LoadFont("font/font.ttf", 24);
    state->font32 = leo_LoadFont("font/font.ttf", 32);
    state->font48 = leo_LoadFont("font/font.ttf", 48);

    if (!leo_IsFontReady(state->font16) || !leo_IsFontReady(state->font24) || !leo_IsFontReady(state->font32) ||
        !leo_IsFontReady(state->font48))
    {
        printf("Failed to load fonts\n");
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

    // Title
    leo_DrawTextEx(state->font32, "Leo Font Size Demo", (leo_Vector2){50, 50}, 32, 1, LEO_WHITE);

    // Dynamic score counter
    SDL_snprintf(buffer, sizeof(buffer), "Score: %d", state->score);
    leo_DrawTextEx(state->font24, buffer, (leo_Vector2){50, 100}, 24, 1, LEO_YELLOW);

    // Font size demonstrations
    leo_DrawTextEx(state->font16, "16px: The quick brown fox jumps over the lazy dog", (leo_Vector2){50, 150}, 16, 1,
                   LEO_WHITE);
    leo_DrawTextEx(state->font24, "24px: The quick brown fox jumps over the lazy dog", (leo_Vector2){50, 180}, 24, 1,
                   LEO_GREEN);
    leo_DrawTextEx(state->font32, "32px: The quick brown fox jumps over the lazy dog", (leo_Vector2){50, 220}, 32, 1,
                   LEO_BLUE);
    leo_DrawTextEx(state->font48, "48px: The quick brown fox jumps", (leo_Vector2){50, 270}, 48, 1, LEO_RED);

    // Size comparison with same text
    leo_DrawTextEx(state->font16, "Size 16", (leo_Vector2){50, 350}, 16, 1, LEO_WHITE);
    leo_DrawTextEx(state->font24, "Size 24", (leo_Vector2){150, 350}, 24, 1, LEO_WHITE);
    leo_DrawTextEx(state->font32, "Size 32", (leo_Vector2){250, 350}, 32, 1, LEO_WHITE);
    leo_DrawTextEx(state->font48, "Size 48", (leo_Vector2){400, 350}, 48, 1, LEO_WHITE);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    FontDemoState *state = (FontDemoState *)ctx->user_data;
    leo_UnloadFont(&state->font16);
    leo_UnloadFont(&state->font24);
    leo_UnloadFont(&state->font32);
    leo_UnloadFont(&state->font48);
}

bool FontDemo(bool oneFrame)
{
    FontDemoState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
        .font16 = {0},
        .font24 = {0},
        .font32 = {0},
        .font48 = {0},
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
