#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    leo_Animation animation;
    leo_AnimationPlayer player;
    bool one_frame;
    float speed_multiplier;
} AnimationDemoState;

static bool demo_setup(leo_GameContext *ctx) {
    AnimationDemoState *state = (AnimationDemoState *)ctx->user_data;
    
    // Mount the resource pack first
    if (!leo_MountResourcePack("resources.leopack", "password", 1)) {
        printf("❌ Unable to mount resources.leopack\n");
        return false;
    }
    printf("✅ Successfully mounted resources.leopack\n");
    
    // Load the 32x32 4-frame animation from VFS
    state->animation = leo_LoadAnimation(
        "images/animation_32x32_4_frames.png",  // VFS path (no "resources/" prefix)
        32,    // frame width
        32,    // frame height  
        4,     // frame count
        0.2f,  // 0.2 seconds per frame (5 FPS)
        true   // loop
    );
    
    // Check if animation loaded successfully
    if (state->animation.texture._handle == NULL) {
        printf("❌ Failed to load animation from VFS\n");
        return false;
    }
    printf("✅ Loaded animation from VFS: %dx%d, %d frames\n", 
           state->animation.frameWidth, state->animation.frameHeight, state->animation.frameCount);
    
    // Create animation player
    state->player = leo_CreateAnimationPlayer(&state->animation);
    
    // Start playing
    leo_PlayAnimation(&state->player);
    
    state->speed_multiplier = 1.0f;
    
    return true;
}

static void demo_update(leo_GameContext *ctx) {
    AnimationDemoState *state = (AnimationDemoState *)ctx->user_data;
    
    if (state->one_frame) {
        leo_GameQuit(ctx);
        return;
    }
    
    // Speed controls
    if (leo_IsKeyPressed(KEY_UP)) {
        state->speed_multiplier += 0.5f;
        if (state->speed_multiplier > 3.0f) state->speed_multiplier = 3.0f;
    }
    if (leo_IsKeyPressed(KEY_DOWN)) {
        state->speed_multiplier -= 0.5f;
        if (state->speed_multiplier < 0.1f) state->speed_multiplier = 0.1f;
    }
    
    // Play/pause controls
    if (leo_IsKeyPressed(KEY_SPACE)) {
        if (state->player.playing) {
            leo_PauseAnimation(&state->player);
        } else {
            leo_PlayAnimation(&state->player);
        }
    }
    
    // Reset animation
    if (leo_IsKeyPressed(KEY_R)) {
        leo_ResetAnimation(&state->player);
        leo_PlayAnimation(&state->player);
    }
    
    // Update animation with speed multiplier
    leo_UpdateAnimation(&state->player, ctx->dt * state->speed_multiplier);
}

static void demo_render_ui(leo_GameContext *ctx) {
    AnimationDemoState *state = (AnimationDemoState *)ctx->user_data;
    
    // Draw single animated sprite in center
    leo_DrawAnimation(&state->player, 350, 200);
    
    // Title and info
    leo_DrawText("Animation Demo", 10, 10, 24, LEO_WHITE);
    leo_DrawText("Loaded from resources.leopack", 10, 40, 16, LEO_GREEN);
    
    // Animation info
    char info[128];
    snprintf(info, sizeof(info), "Frame: %d/%d", 
             state->player.currentFrame + 1, state->animation.frameCount);
    leo_DrawText(info, 10, 70, 16, LEO_WHITE);
    
    snprintf(info, sizeof(info), "Speed: %.1fx", state->speed_multiplier);
    leo_DrawText(info, 10, 90, 16, LEO_WHITE);
    
    snprintf(info, sizeof(info), "Status: %s", 
             state->player.playing ? "Playing" : "Paused");
    leo_DrawText(info, 10, 110, 16, LEO_WHITE);
    
    // Controls
    leo_DrawText("Controls:", 10, 150, 16, LEO_YELLOW);
    leo_DrawText("SPACE - Play/Pause", 10, 170, 14, LEO_GRAY);
    leo_DrawText("UP/DOWN - Speed", 10, 190, 14, LEO_GRAY);
    leo_DrawText("R - Reset", 10, 210, 14, LEO_GRAY);
    leo_DrawText("ESC - Exit", 10, 230, 14, LEO_GRAY);
    
    // Frame timing visualization
    int bar_width = 200;
    int bar_x = 10;
    int bar_y = 270;
    
    leo_DrawText("Frame Progress:", bar_x, bar_y - 20, 14, LEO_WHITE);
    leo_DrawRectangle(bar_x, bar_y, bar_width, 10, LEO_GRAY);
    
    float progress = state->player.timer / state->animation.frameTime;
    int fill_width = (int)(progress * bar_width);
    leo_DrawRectangle(bar_x, bar_y, fill_width, 10, LEO_GREEN);
}

static void demo_shutdown(leo_GameContext *ctx) {
    AnimationDemoState *state = (AnimationDemoState *)ctx->user_data;
    leo_UnloadAnimation(&state->animation);
    printf("✅ Animation Demo shutdown complete\n");
}

bool AnimationDemo(bool oneFrame) {
    AnimationDemoState state = {0};
    state.one_frame = oneFrame;
    
    leo_GameConfig config = {
        .window_width = 800,
        .window_height = 600,
        .window_title = "Leo Engine - Animation Demo",
        .target_fps = 60,
        .clear_color = {50, 50, 80, 255},
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
