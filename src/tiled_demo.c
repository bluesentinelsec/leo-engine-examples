#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool one_frame;
} TiledDemoState;

static bool demo_setup(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    printf("✅ Tiled Demo initialized\n");
    return true;
}

static void demo_update(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    if (state->one_frame) {
        leo_GameQuit(ctx);
        return;
    }
}

static void demo_render_ui(leo_GameContext *ctx) {
    // Title
    leo_DrawText("Tiled Map Demo", 10, 10, 16, LEO_WHITE);
    
    // FPS counter
    leo_DrawFPS(10, 30);
    
    // Retro resolution info
    leo_DrawText("Resolution: 320x200 (Retro)", 10, 50, 12, LEO_GRAY);
}

static void demo_shutdown(leo_GameContext *ctx) {
    printf("✅ Tiled Demo shutdown complete\n");
}

bool TiledDemo(bool oneFrame) {
    TiledDemoState state = {0};
    state.one_frame = oneFrame;
    
    leo_GameConfig config = {
        .window_width = 960,  // 320 * 3 for nice scaling
        .window_height = 600, // 200 * 3 for nice scaling
        .window_title = "Leo Engine - Tiled Map Demo",
        .target_fps = 60,
        .logical_width = 320,
        .logical_height = 200,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_NEAREST, // Pixel-perfect for retro look
        .clear_color = {32, 32, 64, 255}, // Dark blue retro background
        .user_data = &state,
    };
    
    leo_GameCallbacks callbacks = {
        .on_setup = demo_setup,
        .on_update = demo_update,
        .on_render_ui = demo_render_ui,
        .on_shutdown = demo_shutdown,
    };
    
    return leo_GameRun(&config, &callbacks) == 0;
}
