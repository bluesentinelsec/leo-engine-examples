#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool one_frame;
    
    // Player data (no actor system)
    float player_x, player_y;
    float player_speed;
    
    leo_Camera2D camera;
} CameraFollowTestState;

// Demo Implementation
static bool demo_setup(leo_GameContext *ctx) {
    CameraFollowTestState *state = (CameraFollowTestState *)ctx->user_data;
    
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
    
    // Draw world grid
    for (int x = -1000; x <= 1000; x += 100) {
        leo_DrawLine(x, -1000, x, 1000, LEO_GRAY);
    }
    for (int y = -1000; y <= 1000; y += 100) {
        leo_DrawLine(-1000, y, 1000, y, LEO_GRAY);
    }
    
    // Draw player
    leo_DrawRectangle((int)state->player_x - 16, (int)state->player_y - 16, 32, 32, LEO_RED);
    
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
