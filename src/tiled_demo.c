#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool one_frame;
    
    // Player data (no actor system)
    float player_x, player_y;
    float player_speed;
 
    // Camera
    leo_Camera2D camera;

    // Textures
    leo_Texture2D dirt_texture;
    leo_Texture2D tree_texture;
    leo_Texture2D hero_texture;
    leo_Texture2D enemy_texture;
} TiledDemoState;

// Demo Implementation
static bool demo_setup(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    // Mount VFS
    if (!leo_MountResourcePack("resources.leopack", "password", 1)) {
        printf("❌ Unable to mount resources.leopack\n");
        return false;
    }
    printf("✅ Successfully mounted resources.leopack\n");
    
    // Load textures
    printf("Attempting to load textures...\n");
    
    state->dirt_texture = leo_LoadTexture("images/dirt_32x32.png");
    printf("  Dirt texture: %s (handle=%p)\n", 
           state->dirt_texture._handle ? "OK" : "FAILED", 
           state->dirt_texture._handle);
    
    state->tree_texture = leo_LoadTexture("images/tree_32x32.png");
    printf("  Tree texture: %s (handle=%p)\n", 
           state->tree_texture._handle ? "OK" : "FAILED", 
           state->tree_texture._handle);
    
    state->hero_texture = leo_LoadTexture("images/hero_32x32.png");
    printf("  Hero texture: %s (handle=%p)\n", 
           state->hero_texture._handle ? "OK" : "FAILED", 
           state->hero_texture._handle);
    
    state->enemy_texture = leo_LoadTexture("images/enemy_32x32.png");
    printf("  Enemy texture: %s (handle=%p)\n", 
           state->enemy_texture._handle ? "OK" : "FAILED", 
           state->enemy_texture._handle);
    
    // Check for any leo errors
    const char *error = leo_GetError();
    if (error && strlen(error) > 0) {
        printf("Leo error: %s\n", error);
    }
    
    // Initialize player at center of screen
    state->player_x = 400;
    state->player_y = 300;
    state->player_speed = 200.0f;

    // Initialize camera
    state->camera.target = (leo_Vector2){state->player_x, state->player_y};
    state->camera.offset = (leo_Vector2){400, 300}; // Center of 800x600
    state->camera.rotation = 0.0f;
    state->camera.zoom = 1.0f;
    
    printf("✅ Tiled Demo initialized\n");
    return true;
}

static void demo_update(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    if (state->one_frame) {
        leo_GameQuit(ctx);
        return;
    }
    
    // Update player position
    if (leo_IsKeyDown(KEY_W)) state->player_y -= state->player_speed * ctx->dt;
    if (leo_IsKeyDown(KEY_S)) state->player_y += state->player_speed * ctx->dt;
    if (leo_IsKeyDown(KEY_A)) state->player_x -= state->player_speed * ctx->dt;
    if (leo_IsKeyDown(KEY_D)) state->player_x += state->player_speed * ctx->dt;

    // Clamp player to world bounds
    float world_min_x = 0;
    float world_min_y = 0;
    float world_max_x = 2000;
    float world_max_y = 2000;

    float old_x = state->player_x;
    float old_y = state->player_y;

    if (state->player_x < world_min_x) state->player_x = world_min_x;
    if (state->player_y < world_min_y) state->player_y = world_min_y;
    if (state->player_x > world_max_x) state->player_x = world_max_x;
    if (state->player_y > world_max_y) state->player_y = world_max_y;

    // Debug: Log when clamping occurs
    if (old_x != state->player_x || old_y != state->player_y) {
        printf("Player clamped: (%.1f,%.1f) -> (%.1f,%.1f)\n", old_x, old_y, state->player_x, state->player_y);
    }

    // Update camera to follow clamped player
    state->camera.target = (leo_Vector2){state->player_x, state->player_y};
}

static void demo_render_ui(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;

    // Apply persistent camera to world rendering
    leo_BeginMode2D(state->camera);
    
    // Draw world grid
    for (int x = -1000; x <= 1000; x += 100) {
        leo_DrawLine(x, -1000, x, 1000, LEO_GRAY);
    }
    for (int y = -1000; y <= 1000; y += 100) {
        leo_DrawLine(-1000, y, 1000, y, LEO_GRAY);
    }
    
    // Draw test textures at FIXED world positions near player start (400, 300)
    leo_Rectangle src = {0, 0, 32, 32};
    
    // Dirt texture at fixed world position (336, 300) - left of player start
    if (state->dirt_texture._handle) {
        leo_DrawTextureRec(state->dirt_texture, src, (leo_Vector2){336, 300}, LEO_WHITE);
    }
    
    // Tree texture at fixed world position (464, 300) - right of player start  
    if (state->tree_texture._handle) {
        leo_DrawTextureRec(state->tree_texture, src, (leo_Vector2){464, 300}, LEO_WHITE);
    }
    
    // Enemy texture at fixed world position (400, 236) - above player start
    if (state->enemy_texture._handle) {
        leo_DrawTextureRec(state->enemy_texture, src, (leo_Vector2){400, 236}, LEO_WHITE);
    }
    
    // Hero texture follows player
    if (state->hero_texture._handle) {
        leo_DrawTextureRec(state->hero_texture, src, (leo_Vector2){state->player_x, state->player_y}, LEO_WHITE);
    }
    
    leo_EndMode2D();
    
    // UI overlay
    leo_DrawText("Tiled Map Demo", 10, 10, 16, LEO_WHITE);
    leo_DrawText("WASD: Move Player", 10, 30, 12, LEO_YELLOW);
    leo_DrawText("Camera follows automatically", 10, 50, 12, LEO_YELLOW);
    
    // Debug info
    char info[128];
    snprintf(info, sizeof(info), "Player: (%.0f, %.0f)", state->player_x, state->player_y);
    leo_DrawText(info, 10, 70, 10, LEO_GREEN);
}

static void demo_shutdown(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    // Cleanup textures
    if (state->dirt_texture._handle) leo_UnloadTexture(&state->dirt_texture);
    if (state->tree_texture._handle) leo_UnloadTexture(&state->tree_texture);
    if (state->hero_texture._handle) leo_UnloadTexture(&state->hero_texture);
    if (state->enemy_texture._handle) leo_UnloadTexture(&state->enemy_texture);
    
    printf("✅ Tiled Demo shutdown\n");
}

bool TiledDemo(bool oneFrame) {
    TiledDemoState state = {0};
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
