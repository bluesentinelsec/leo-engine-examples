#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_OBSTACLES 20
#define MAX_COLLECTIBLES 15
#define PLAYER_SPEED 3
#define WORLD_WIDTH 2000
#define WORLD_HEIGHT 1500

typedef struct {
    leo_Rectangle rect;
} Obstacle;

typedef struct {
    leo_Vector2 pos;
    float radius;
    bool collected;
} Collectible;

typedef struct Camera2DDemoState {
    bool fullscreen;
    bool one_frame;
    
    // Player
    leo_Rectangle player;
    
    // World objects
    Obstacle obstacles[MAX_OBSTACLES];
    Collectible collectibles[MAX_COLLECTIBLES];
    
    // Camera
    leo_Camera2D camera;
    
    // Game state
    int score;
} Camera2DDemoState;

static bool demo_setup(leo_GameContext *ctx) {
    Camera2DDemoState *state = (Camera2DDemoState *)ctx->user_data;
    
    state->fullscreen = true;
    leo_SetFullscreen(state->fullscreen);
    
    // Initialize player at world center
    state->player = (leo_Rectangle){WORLD_WIDTH/2 - 8, WORLD_HEIGHT/2 - 8, 16, 16};
    
    // Initialize camera
    state->camera.target = (leo_Vector2){state->player.x + 8, state->player.y + 8};
    state->camera.offset = (leo_Vector2){640, 360}; // Screen center
    state->camera.rotation = 0.0f;
    state->camera.zoom = 1.0f;
    
    // Create obstacles (rocks/walls)
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        state->obstacles[i].rect = (leo_Rectangle){
            (float)(rand() % (WORLD_WIDTH - 100) + 50),
            (float)(rand() % (WORLD_HEIGHT - 100) + 50),
            (float)(40 + rand() % 60),
            (float)(40 + rand() % 60)
        };
    }
    
    // Create collectibles (coins)
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        state->collectibles[i] = (Collectible){
            {(float)(rand() % (WORLD_WIDTH - 100) + 50), (float)(rand() % (WORLD_HEIGHT - 100) + 50)},
            8.0f,
            false
        };
    }
    
    state->score = 0;
    return true;
}

static bool check_collision(leo_Rectangle a, leo_Rectangle b) {
    return (a.x < b.x + b.width && a.x + a.width > b.x &&
            a.y < b.y + b.height && a.y + a.height > b.y);
}

static bool check_circle_rect_collision(leo_Vector2 center, float radius, leo_Rectangle rect) {
    float dx = center.x - (rect.x + rect.width/2);
    float dy = center.y - (rect.y + rect.height/2);
    float distance = dx*dx + dy*dy;
    return distance < (radius + rect.width/2) * (radius + rect.width/2);
}

static void demo_update(leo_GameContext *ctx) {
    Camera2DDemoState *state = (Camera2DDemoState *)ctx->user_data;
    
    // Store old position for collision checking
    leo_Rectangle old_player = state->player;
    
    // Player movement
    if (leo_IsKeyDown(KEY_RIGHT) || leo_IsKeyDown(KEY_D)) {
        state->player.x += PLAYER_SPEED;
    }
    if (leo_IsKeyDown(KEY_LEFT) || leo_IsKeyDown(KEY_A)) {
        state->player.x -= PLAYER_SPEED;
    }
    if (leo_IsKeyDown(KEY_DOWN) || leo_IsKeyDown(KEY_S)) {
        state->player.y += PLAYER_SPEED;
    }
    if (leo_IsKeyDown(KEY_UP) || leo_IsKeyDown(KEY_W)) {
        state->player.y -= PLAYER_SPEED;
    }
    
    // World boundaries
    if (state->player.x < 0) state->player.x = 0;
    if (state->player.y < 0) state->player.y = 0;
    if (state->player.x + state->player.width > WORLD_WIDTH) 
        state->player.x = WORLD_WIDTH - state->player.width;
    if (state->player.y + state->player.height > WORLD_HEIGHT) 
        state->player.y = WORLD_HEIGHT - state->player.height;
    
    // Collision with obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (check_collision(state->player, state->obstacles[i].rect)) {
            state->player = old_player; // Revert movement
            break;
        }
    }
    
    // Collect items
    leo_Vector2 player_center = {state->player.x + 8, state->player.y + 8};
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        if (!state->collectibles[i].collected && 
            check_circle_rect_collision(state->collectibles[i].pos, state->collectibles[i].radius, state->player)) {
            state->collectibles[i].collected = true;
            state->score++;
        }
    }
    
    // Update camera to follow player
    state->camera.target = (leo_Vector2){state->player.x + 8, state->player.y + 8};
    
    // Toggle fullscreen
    if (leo_IsKeyReleased(KEY_TAB)) {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
    }
    
    if (state->one_frame && ctx->frame >= 1) {
        leo_GameQuit(ctx);
    }
}

static void demo_render_ui(leo_GameContext *ctx) {
    Camera2DDemoState *state = (Camera2DDemoState *)ctx->user_data;
    
    leo_BeginMode2D(state->camera);
    
    // Draw ground (large green rectangle)
    leo_DrawRectangle(0, 0, WORLD_WIDTH, WORLD_HEIGHT, LEO_GREEN);
    
    // Draw world grid for reference
    for (int x = 0; x < WORLD_WIDTH; x += 100) {
        leo_DrawLine(x, 0, x, WORLD_HEIGHT, (leo_Color){0, 150, 0, 100});
    }
    for (int y = 0; y < WORLD_HEIGHT; y += 100) {
        leo_DrawLine(0, y, WORLD_WIDTH, y, (leo_Color){0, 150, 0, 100});
    }
    
    // Draw obstacles (brown rectangles)
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        leo_Rectangle r = state->obstacles[i].rect;
        leo_DrawRectangle((int)r.x, (int)r.y, (int)r.width, (int)r.height, 
                         (leo_Color){101, 67, 33, 255}); // Brown
    }
    
    // Draw collectibles (yellow circles)
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        if (!state->collectibles[i].collected) {
            leo_DrawCircle((int)state->collectibles[i].pos.x, (int)state->collectibles[i].pos.y, 
                          state->collectibles[i].radius, LEO_YELLOW);
        }
    }
    
    // Draw player (red rectangle)
    leo_DrawRectangle((int)state->player.x, (int)state->player.y, 
                     (int)state->player.width, (int)state->player.height, LEO_RED);
    
    leo_EndMode2D();
    
    // UI overlay (not affected by camera)
    char score_text[64];
    SDL_snprintf(score_text, sizeof(score_text), "Score: %d/%d", state->score, MAX_COLLECTIBLES);
    leo_DrawText(score_text, 10, 10, 24, LEO_WHITE);
    
    leo_DrawText("WASD/Arrows: Move", 10, 40, 16, LEO_WHITE);
    leo_DrawText("Tab: Toggle Fullscreen", 10, 60, 16, LEO_WHITE);
    leo_DrawText("Collect yellow coins!", 10, 80, 16, LEO_WHITE);
}

static void demo_shutdown(leo_GameContext *ctx) {
    // No cleanup needed for this demo
}

bool Camera2DDemo(bool oneFrame) {
    Camera2DDemoState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
        .score = 0,
    };
    
    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Camera 2D Demo - Top-Down Explorer",
        .target_fps = 60,
        .logical_width = 1280,
        .logical_height = 720,
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
    
    return (leo_GameRun(&cfg, &cb) == 0);
}
