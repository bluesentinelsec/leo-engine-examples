#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct MouseTestState {
    bool fullscreen;
    bool one_frame;
    
    // Test targets
    leo_Rectangle targets[4];
    leo_Vector2 mouseHistory[60]; // 1 second at 60fps
    int historyIndex;
} MouseTestState;

static bool demo_setup(leo_GameContext *ctx) {
    MouseTestState *state = (MouseTestState *)ctx->user_data;
    
    state->fullscreen = false;
    state->historyIndex = 0;
    
    // Hide native OS cursor to avoid dual cursor confusion
    SDL_HideCursor();
    
    // Create test targets in each corner and center
    state->targets[0] = (leo_Rectangle){50, 50, 100, 100};      // top-left
    state->targets[1] = (leo_Rectangle){650, 50, 100, 100};     // top-right  
    state->targets[2] = (leo_Rectangle){50, 450, 100, 100};     // bottom-left
    state->targets[3] = (leo_Rectangle){650, 450, 100, 100};    // bottom-right
    
    // Clear mouse history
    for (int i = 0; i < 60; i++) {
        state->mouseHistory[i] = (leo_Vector2){-1, -1};
    }
    
    return true;
}

static void demo_update(leo_GameContext *ctx) {
    MouseTestState *state = (MouseTestState *)ctx->user_data;
    leo_Vector2 mousePos = leo_GetMousePosition();
    
    // Toggle fullscreen
    if (leo_IsKeyReleased(KEY_TAB)) {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
        
        // Move software cursor to center screen
        leo_SetMousePosition(400, 300);
        mousePos = (leo_Vector2){400, 300}; // Update our local copy
    }
    
    // In fullscreen mode, continuously clamp mouse to logical bounds
    if (state->fullscreen) {
        if (mousePos.x < 0 || mousePos.x >= 800 || mousePos.y < 0 || mousePos.y >= 600) {
            float clampedX = mousePos.x < 0 ? 0 : (mousePos.x >= 800 ? 799 : mousePos.x);
            float clampedY = mousePos.y < 0 ? 0 : (mousePos.y >= 600 ? 599 : mousePos.y);
            
            leo_SetMousePosition((int)clampedX, (int)clampedY);
            mousePos = (leo_Vector2){clampedX, clampedY};
        }
    }
    
    // Record mouse history
    state->mouseHistory[state->historyIndex] = mousePos;
    state->historyIndex = (state->historyIndex + 1) % 60;
    
    if (state->one_frame) {
        ctx->request_quit = true;
    }
}

static void demo_render_ui(leo_GameContext *ctx) {
    MouseTestState *state = (MouseTestState *)ctx->user_data;
    leo_Vector2 mousePos = leo_GetMousePosition();
    
    // Draw coordinate grid
    for (int x = 0; x < 800; x += 100) {
        leo_DrawLine(x, 0, x, 600, (leo_Color){50, 50, 50, 255});
    }
    for (int y = 0; y < 600; y += 100) {
        leo_DrawLine(0, y, 800, y, (leo_Color){50, 50, 50, 255});
    }
    
    // Draw test targets
    for (int i = 0; i < 4; i++) {
        leo_Color color = LEO_BLUE;
        if (leo_CheckCollisionPointRec(mousePos, state->targets[i])) {
            color = LEO_RED; // Highlight when mouse is over
        }
        leo_DrawRectangle((int)state->targets[i].x, (int)state->targets[i].y,
                         (int)state->targets[i].width, (int)state->targets[i].height, color);
    }
    
    // Draw mouse trail
    for (int i = 0; i < 59; i++) {
        int idx1 = (state->historyIndex + i) % 60;
        int idx2 = (state->historyIndex + i + 1) % 60;
        
        if (state->mouseHistory[idx1].x >= 0 && state->mouseHistory[idx2].x >= 0) {
            leo_Color trailColor = {255, 255, 0, (Uint8)(50 + i * 3)};
            leo_DrawLine((int)state->mouseHistory[idx1].x, (int)state->mouseHistory[idx1].y,
                        (int)state->mouseHistory[idx2].x, (int)state->mouseHistory[idx2].y, trailColor);
        }
    }
    
    // Draw current mouse position
    leo_DrawCircle((int)mousePos.x, (int)mousePos.y, 5, LEO_WHITE);
    
    // Draw coordinate info
    char info[256];
    snprintf(info, sizeof(info), "Mouse: %.1f, %.1f", mousePos.x, mousePos.y);
    leo_DrawText(info, 10, 10, 16, LEO_WHITE);
    
    snprintf(info, sizeof(info), "Screen: %dx%d", leo_GetScreenWidth(), leo_GetScreenHeight());
    leo_DrawText(info, 10, 30, 16, LEO_WHITE);
    
    snprintf(info, sizeof(info), "Mode: %s", state->fullscreen ? "FULLSCREEN" : "WINDOWED");
    leo_DrawText(info, 10, 50, 16, LEO_WHITE);
    
    leo_DrawText("Tab: Toggle Fullscreen", 10, 80, 14, LEO_GRAY);
    leo_DrawText("Move mouse over blue squares - they should turn red", 10, 100, 14, LEO_GRAY);
    leo_DrawText("Yellow trail shows mouse movement", 10, 120, 14, LEO_GRAY);
    
    // Draw corner coordinates for reference
    leo_DrawText("(0,0)", 5, 580, 12, LEO_GRAY);
    leo_DrawText("(800,0)", 760, 580, 12, LEO_GRAY);
    leo_DrawText("(0,600)", 5, 5, 12, LEO_GRAY);
    leo_DrawText("(800,600)", 750, 5, 12, LEO_GRAY);
}

static void demo_shutdown(leo_GameContext *ctx) {
    // Restore native cursor
    SDL_ShowCursor();
    (void)ctx;
}

bool MouseTestDemo(bool oneFrame) {
    MouseTestState state = {
        .one_frame = oneFrame,
    };
    
    leo_GameConfig cfg = {
        .window_width = 800,
        .window_height = 600,
        .window_title = "Leo Engine - Mouse Coordinate Test",
        .target_fps = 60,
        .logical_width = 800,
        .logical_height = 600,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = (leo_Color){20, 20, 20, 255},
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
