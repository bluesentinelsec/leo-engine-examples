#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    // Player
    float player_x, player_y;
    float player_speed;

    // Camera
    leo_Camera2D camera;

    // Textures
    leo_Texture2D dirt_texture;
    leo_Texture2D tree_texture;
    leo_Texture2D hero_texture;
    leo_Texture2D enemy_texture;

    bool one_frame;
} ZeldaDemoState;

/* ----------------------------------------------------------
   Setup
   ---------------------------------------------------------- */
static bool demo_setup(leo_GameContext *ctx) {
    ZeldaDemoState *state = (ZeldaDemoState *)ctx->user_data;

    // Mount resource pack with password + compression enabled
    if (!leo_MountResourcePack("resources.leopack", "password", 1)) {
        printf("❌ Failed to mount resources.leopack\n");
        return false;
    }
    printf("✅ Mounted resources.leopack\n");

    // Load textures from VFS
    state->dirt_texture  = leo_LoadTexture("images/dirt_32x32.png");
    state->tree_texture  = leo_LoadTexture("images/tree_32x32.png");
    state->hero_texture  = leo_LoadTexture("images/hero_32x32.png");
    state->enemy_texture = leo_LoadTexture("images/enemy_32x32.png");

    // Init player
    state->player_x = 100.0f;
    state->player_y = 100.0f;
    state->player_speed = 150.0f;

    // Initialize camera
    int w = leo_GetScreenWidth();
    int h = leo_GetScreenHeight();
    assert(w > 0 && h > 0);
    printf("Screen size: %dx%d\n", w, h);

    state->camera.target = (leo_Vector2){state->player_x, state->player_y};
    state->camera.offset = (leo_Vector2){w / 2.0f, h / 2.0f}; // True screen center
    state->camera.rotation = 0.0f;
    state->camera.zoom = 1.0f;

    printf("Camera initialized: target=(%.1f, %.1f), offset=(%.1f, %.1f)\n",
           state->camera.target.x, state->camera.target.y,
           state->camera.offset.x, state->camera.offset.y);

    return true;
}

/* ----------------------------------------------------------
   Update
   ---------------------------------------------------------- */
static void demo_update(leo_GameContext *ctx) {
    ZeldaDemoState *state = (ZeldaDemoState *)ctx->user_data;
    float dt = ctx->dt;

    // WASD movement
    if (leo_IsKeyDown(KEY_W)) state->player_y -= state->player_speed * dt;
    if (leo_IsKeyDown(KEY_S)) state->player_y += state->player_speed * dt;
    if (leo_IsKeyDown(KEY_A)) state->player_x -= state->player_speed * dt;
    if (leo_IsKeyDown(KEY_D)) state->player_x += state->player_speed * dt;

    // Camera follows player
    state->camera.target.x = state->player_x;
    state->camera.target.y = state->player_y;

    // Debug info each frame (throttled to every 60 frames)
    if (ctx->frame % 60 == 0) {
        printf("[Frame %lld] Player=(%.1f, %.1f) CameraTarget=(%.1f, %.1f) Offset=(%.1f, %.1f)\n",
               (long long)ctx->frame,
               state->player_x, state->player_y,
               state->camera.target.x, state->camera.target.y,
               state->camera.offset.x, state->camera.offset.y);
    }

    // Escape hatch (CI/CD)
    if (state->one_frame && ctx->frame >= 1) {
        leo_GameQuit(ctx);
    }
}

/* ----------------------------------------------------------
   Render world + UI
   ---------------------------------------------------------- */
static void demo_render_ui(leo_GameContext *ctx) {
    ZeldaDemoState *state = (ZeldaDemoState *)ctx->user_data;

    // Begin world render with camera
    leo_BeginMode2D(state->camera);

    // Draw background tiles (simple repeated dirt for demo)
    for (int y = -5; y < 20; y++) {
        for (int x = -10; x < 20; x++) {
            leo_DrawTextureRec(
                state->dirt_texture,
                (leo_Rectangle){0, 0, 32, 32},
                (leo_Vector2){x * 32.0f, y * 32.0f},
                LEO_WHITE
            );
        }
    }

    // Draw hero (player)
    leo_DrawTextureRec(
        state->hero_texture,
        (leo_Rectangle){0, 0, 32, 32},
        (leo_Vector2){state->player_x, state->player_y},
        LEO_WHITE
    );

    // Example enemy + tree
    leo_DrawTextureRec(
        state->tree_texture,
        (leo_Rectangle){0, 0, 32, 32},
        (leo_Vector2){200, 200},
        LEO_WHITE
    );

    leo_DrawTextureRec(
        state->enemy_texture,
        (leo_Rectangle){0, 0, 32, 32},
        (leo_Vector2){300, 150},
        LEO_WHITE
    );

    leo_EndMode2D();

    // UI overlay
    leo_DrawFPS(20, 32);
}

static void demo_shutdown(leo_GameContext *ctx) {
    (void)ctx;
}

/* ----------------------------------------------------------
   Entrypoint
   ---------------------------------------------------------- */
bool ZeldaDemo(bool oneFrame) {
    ZeldaDemoState state = {
        .player_x = 0,
        .player_y = 0,
        .player_speed = 150.0f,
        .one_frame = oneFrame,
    };

    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Zelda-like Demo",
        .target_fps = 60,
        .logical_width = 0,
        .logical_height = 0,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_NEAREST,
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

bool TiledDemo(bool oneFrame) {
    ZeldaDemoState state = {0};
    state.one_frame = oneFrame;
    
    leo_GameConfig config = {
        .window_width = 800,
        .window_height = 600,
        .window_title = "Leo Engine - Tiled Map Demo",
        .target_fps = 60,
        .logical_width = 800,
        .logical_height = 600,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_NEAREST,
        .clear_color = {32, 32, 64, 255}, // Dark blue background
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

