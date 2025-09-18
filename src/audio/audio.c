#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct AudioDemoState
{
    bool fullscreen;
    bool one_frame;
    leo_Sound sfx;     // Sound effect (coin.wav)
    leo_Sound music;   // Background music (music.wav)
    bool music_paused; // Track music pause state
} AudioDemoState;

static bool demo_setup(leo_GameContext *ctx)
{
    AudioDemoState *state = (AudioDemoState *)ctx->user_data;
    state->fullscreen = true;
    state->music_paused = false;
    leo_SetFullscreen(state->fullscreen);

    // Mount Python-created resources.leopack for VFS
    if (!leo_MountResourcePack("resources.leopack", "password", 100))
    {
        printf("❌ Failed to mount resources.leopack\n");
        return false;
    }
    else
    {
        printf("✅ Successfully mounted resources.leopack\n");
    }

    // Load sound effect from VFS
    state->sfx = leo_LoadSound("sound/coin.wav");
    if (!leo_IsSoundReady(state->sfx))
    {
        printf("❌ Failed to load sound/coin.wav from VFS\n");
        return false;
    }
    else
    {
        printf("✅ Loaded sound/coin.wav from VFS\n");
    }

    // Load music from VFS
    state->music = leo_LoadSound("music/music.wav");
    if (!leo_IsSoundReady(state->music))
    {
        printf("❌ Failed to load music/music.wav from VFS\n");
        leo_UnloadSound(&state->sfx);
        return false;
    }
    else
    {
        printf("✅ Loaded music/music.wav from VFS\n");
    }

    // Start music looping at moderate volume
    leo_SetSoundVolume(&state->music, 0.3f);
    leo_PlaySound(&state->music, 0.3f, true);
    printf("✅ Started background music\n");

    return true; // success
}

static void demo_update(leo_GameContext *ctx)
{
    AudioDemoState *state = (AudioDemoState *)ctx->user_data;

    // Toggle fullscreen on Tab key release
    if (leo_IsKeyReleased(KEY_TAB))
    {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
        printf("Fullscreen %s\n", state->fullscreen ? "enabled" : "disabled");
    }

    // Sound demo controls
    // Play sound effect on Space key press
    if (leo_IsKeyPressed(KEY_SPACE))
    {
        leo_PlaySound(&state->sfx, 0.7f, false);
        printf("Playing coin sound effect from VFS\n");
    }

    // Toggle music pause/resume on P key release
    if (leo_IsKeyReleased(KEY_P))
    {
        if (state->music_paused)
        {
            leo_ResumeSound(&state->music);
            state->music_paused = false;
            printf("Music resumed\n");
        }
        else
        {
            leo_PauseSound(&state->music);
            state->music_paused = true;
            printf("Music paused\n");
        }
    }

    static float volume = 0.3f;
    // Adjust music volume with Up/Down arrows
    if (leo_IsKeyDown(KEY_UP))
    {
        volume = volume + 0.1f * ctx->dt; // Smooth increase
        if (volume > 1.0f)
            volume = 1.0f;
        leo_SetSoundVolume(&state->music, volume);
    }
    if (leo_IsKeyDown(KEY_DOWN))
    {
        volume = volume - 0.1f * ctx->dt; // Smooth decrease
        if (volume < 0.0f)
            volume = 0.0f;
        leo_SetSoundVolume(&state->music, volume);
    }

    static float pitch = 1.0f;
    // Adjust sound effect pitch with Left/Right arrows
    if (leo_IsKeyDown(KEY_LEFT))
    {
        pitch = pitch - 0.5f * ctx->dt; // Smooth decrease
        if (pitch < 0.01f)
            pitch = 0.01f;
        leo_SetSoundPitch(&state->sfx, pitch);
    }
    if (leo_IsKeyDown(KEY_RIGHT))
    {
        pitch = pitch + 0.5f * ctx->dt; // Smooth increase
        if (pitch > 2.0f)
            pitch = 2.0f;
        leo_SetSoundPitch(&state->sfx, pitch);
    }

    static float pan = 0.0f;
    // Adjust sound effect pan with Q/E keys
    if (leo_IsKeyDown(KEY_Q))
    {
        pan = pan - 0.5f * ctx->dt; // Pan left
        if (pan < -1.0f)
            pan = -1.0f;
        leo_SetSoundPan(&state->sfx, pan);
    }
    if (leo_IsKeyDown(KEY_E))
    {
        pan = pan + 0.5f * ctx->dt; // Pan right
        if (pan > 1.0f)
            pan = 1.0f;
        leo_SetSoundPan(&state->sfx, pan);
    }

    // Stop all sounds on S key release
    if (leo_IsKeyReleased(KEY_S))
    {
        leo_StopSound(&state->sfx);
        leo_StopSound(&state->music);
        state->music_paused = false;
        printf("All sounds stopped\n");
    }

    // Escape hatch for CI/CD: quit after one frame
    if (state->one_frame && ctx->frame >= 1)
    {
        leo_GameQuit(ctx);
    }
}

static void demo_render_ui(leo_GameContext *ctx)
{
    // Draw title and VFS info
    leo_DrawText("Audio Demo - VFS Loading", 20, 20, 24, LEO_WHITE);
    leo_DrawText("Audio loaded from Python-created resources.leopack", 20, 50, 16, LEO_GREEN);

    // Draw controls
    leo_DrawText("Controls:", 20, 100, 18, LEO_YELLOW);
    leo_DrawText("SPACE - Play coin sound effect", 20, 130, 14, LEO_WHITE);
    leo_DrawText("P - Pause/Resume music", 20, 150, 14, LEO_WHITE);
    leo_DrawText("UP/DOWN - Music volume", 20, 170, 14, LEO_WHITE);
    leo_DrawText("LEFT/RIGHT - Sound pitch", 20, 190, 14, LEO_WHITE);
    leo_DrawText("Q/E - Sound pan (left/right)", 20, 210, 14, LEO_WHITE);
    leo_DrawText("S - Stop all sounds", 20, 230, 14, LEO_WHITE);
    leo_DrawText("TAB - Toggle fullscreen", 20, 250, 14, LEO_WHITE);

    // Draw FPS
    leo_DrawFPS(20, 300);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    AudioDemoState *state = (AudioDemoState *)ctx->user_data;
    // Clean up audio resources
    leo_StopSound(&state->sfx);
    leo_StopSound(&state->music);
    leo_UnloadSound(&state->sfx);
    leo_UnloadSound(&state->music);
    printf("✅ Audio Demo shutdown complete\n");
}

bool AudioDemo(bool oneFrame)
{
    AudioDemoState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
        .sfx = {0},
        .music = {0},
        .music_paused = false,
    };

    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Audio Demo - VFS Loading",
        .target_fps = 60,
        .logical_width = 0,
        .logical_height = 0,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = LEO_BLUE,
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
