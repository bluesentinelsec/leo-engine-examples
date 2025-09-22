#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool one_frame;
    
    // Player data (no actor system)
    float player_x, player_y;
    float player_speed;
    
    // Textures
    leo_Texture2D dirt_texture;
    leo_Texture2D tree_texture;
    leo_Texture2D hero_texture;
    leo_Texture2D enemy_texture;
    
    leo_Camera2D camera;
} CameraFollowTestState;

// Demo Implementation
static bool demo_setup(leo_GameContext *ctx) {
    CameraFollowTestState *state = (CameraFollowTestState *)ctx->user_data;
    
    // Mount VFS
    if (!leo_MountResourcePack("resources.leopack", "password", 1)) {
        printf("❌ Unable to mount resources.leopack\n");
        return false;
    }
    printf("✅ Successfully mounted resources.leopack\n");
    
    // Load textures
    state->dirt_texture = leo_LoadTexture("images/dirt_32x32.png");
    state->tree_texture = leo_LoadTexture("images/tree_32x32.png");
    state->hero_texture = leo_LoadTexture("images/hero_32x32.png");
    state->enemy_texture = leo_LoadTexture("images/enemy_32x32.png");
    
    printf("Texture loading results:\n");
    printf("  Dirt: %s\n", state->dirt_texture._handle ? "OK" : "FAILED");
    printf("  Tree: %s\n", state->tree_texture._handle ? "OK" : "FAILED");
    printf("  Hero: %s\n", state->hero_texture._handle ? "OK" : "FAILED");
    printf("  Enemy: %s\n", state->enemy_texture._handle ? "OK" : "FAILED");
    
    // Initialize player
    state->player_x = 400;
    state->player_y = 300;
    state->player_speed = 200.0f;
    
    // Initialize camera
    state->camera.target = (leo_Vector2){400, 300};
    state->camera.offset = (leo_Vector2){400, 300}; // Screen center
    state->camera.rotation = 0.0f;
    state->camera.zoom = 1.0f;
    
    printf("✅ Camera Follow Test initialized\n");
    return true;
}

static void demo_update(leo_GameContext *ctx) {
    CameraFollowTestState *state = (CameraFollowTestState *)ctx->user_data;
    
    if (state->one_frame) {
        leo_GameQuit(ctx);
        return;
    }
    
    float old_x = state->player_x;
    float old_y = state->player_y;
    
    // Update player position
    if (leo_IsKeyDown(KEY_W)) state->player_y -= state->player_speed * ctx->dt;
    if (leo_IsKeyDown(KEY_S)) state->player_y += state->player_speed * ctx->dt;
    if (leo_IsKeyDown(KEY_A)) state->player_x -= state->player_speed * ctx->dt;
    if (leo_IsKeyDown(KEY_D)) state->player_x += state->player_speed * ctx->dt;
    
    // Clamp player to world bounds (matching the grid size)
    float world_min_x = -1000;
    float world_min_y = -1000;
    float world_max_x = 1000;
    float world_max_y = 1000;
    
    if (state->player_x < world_min_x) state->player_x = world_min_x;
    if (state->player_y < world_min_y) state->player_y = world_min_y;
    if (state->player_x > world_max_x) state->player_x = world_max_x;
    if (state->player_y > world_max_y) state->player_y = world_max_y;
    
    // Log player movement
    if (old_x != state->player_x || old_y != state->player_y) {
        printf("Player moved: (%.1f,%.1f) -> (%.1f,%.1f)\n", old_x, old_y, state->player_x, state->player_y);
    }
    
    float old_cam_x = state->camera.target.x;
    float old_cam_y = state->camera.target.y;
    
    // Update camera to follow player automatically
    state->camera.target = (leo_Vector2){state->player_x, state->player_y};
    if (old_cam_x != state->camera.target.x || old_cam_y != state->camera.target.y) {
        printf("Camera following player: (%.1f,%.1f) -> (%.1f,%.1f)\n", 
               old_cam_x, old_cam_y, state->camera.target.x, state->camera.target.y);
    }
}

static void demo_render_ui(leo_GameContext *ctx) {
    CameraFollowTestState *state = (CameraFollowTestState *)ctx->user_data;
    
    printf("Render: Player(%.1f,%.1f) Camera target(%.1f,%.1f) offset(%.1f,%.1f)\n",
           state->player_x, state->player_y,
           state->camera.target.x, state->camera.target.y,
           state->camera.offset.x, state->camera.offset.y);
    
    // Apply camera transform to world rendering
    leo_BeginMode2D(state->camera);
    
    // Draw world grid (extended to cover full world bounds)
    for (int x = -1100; x <= 1100; x += 100) {
        leo_DrawLine(x, -1100, x, 1100, LEO_GRAY);
    }
    for (int y = -1100; y <= 1100; y += 100) {
        leo_DrawLine(-1100, y, 1100, y, LEO_GRAY);
    }
    
    // Draw test textures at FIXED world positions near camera spawn (400, 300)
    leo_Rectangle src = {0, 0, 32, 32};
    
    // Dirt texture at fixed world position (336, 300) - left of spawn
    if (state->dirt_texture._handle) {
        leo_DrawTextureRec(state->dirt_texture, src, (leo_Vector2){336, 300}, LEO_WHITE);
    }
    
    // Tree texture at fixed world position (464, 300) - right of spawn
    if (state->tree_texture._handle) {
        leo_DrawTextureRec(state->tree_texture, src, (leo_Vector2){464, 300}, LEO_WHITE);
    }
    
    // Enemy texture at fixed world position (400, 236) - above spawn
    if (state->enemy_texture._handle) {
        leo_DrawTextureRec(state->enemy_texture, src, (leo_Vector2){400, 236}, LEO_WHITE);
    }
    
    // Player sprite (hero texture)
    if (state->hero_texture._handle) {
        leo_DrawTextureRec(state->hero_texture, src, (leo_Vector2){state->player_x, state->player_y}, LEO_WHITE);
    } else {
        // Fallback to red rectangle if texture fails
        leo_DrawRectangle((int)state->player_x - 16, (int)state->player_y - 16, 32, 32, LEO_RED);
    }
    
    leo_EndMode2D();
    
    // UI overlay
    leo_DrawText("Camera Follow Test", 10, 10, 16, LEO_WHITE);
    leo_DrawText("WASD: Move Player", 10, 30, 12, LEO_YELLOW);
    leo_DrawText("Camera follows automatically", 10, 50, 12, LEO_YELLOW);
    
    // Debug info
    char info[128];
    snprintf(info, sizeof(info), "Player: (%.0f, %.0f)", state->player_x, state->player_y);
    leo_DrawText(info, 10, 70, 10, LEO_GREEN);
    snprintf(info, sizeof(info), "Camera: (%.0f, %.0f)", state->camera.target.x, state->camera.target.y);
    leo_DrawText(info, 10, 90, 10, LEO_BLUE);
}

static void demo_shutdown(leo_GameContext *ctx) {
    CameraFollowTestState *state = (CameraFollowTestState *)ctx->user_data;
    
    // Cleanup textures
    if (state->dirt_texture._handle) leo_UnloadTexture(&state->dirt_texture);
    if (state->tree_texture._handle) leo_UnloadTexture(&state->tree_texture);
    if (state->hero_texture._handle) leo_UnloadTexture(&state->hero_texture);
    if (state->enemy_texture._handle) leo_UnloadTexture(&state->enemy_texture);
    
    printf("✅ Camera Follow Test shutdown\n");
}

bool CameraFollowTest(bool oneFrame) {
    CameraFollowTestState state = {0};
    state.one_frame = oneFrame;
    
    leo_GameConfig config = {
        .window_width = 800,
        .window_height = 600,
        .window_title = "Leo Engine - Camera Follow Test",
        .target_fps = 60,
        .logical_width = 800,
        .logical_height = 600,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_NEAREST,
        .clear_color = LEO_BLACK,
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
