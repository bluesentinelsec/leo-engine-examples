#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct FullscreenState
{
    bool fullscreen;
    bool one_frame;
    leo_Sound sfx;     // Sound effect (ogre3.wav)
    leo_Sound music;   // Background music (music.wav)
    bool music_paused; // Track music pause state
} FullscreenState;

static bool demo_setup(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;
    state->fullscreen = true;
    state->music_paused = false;
    leo_SetFullscreen(state->fullscreen);

    // Mount resources directory for VFS
    if (!leo_MountDirectory("resources", 100))
    {
        printf("Failed to mount resources directory\n");
        return false;
    }

    // Load sound effect and music using VFS logical paths
    state->sfx = leo_LoadSound("sound/ogre3.wav");
    if (!leo_IsSoundReady(state->sfx))
    {
        printf("Failed to load sound/ogre3.wav\n");
        return false;
    }

    state->music = leo_LoadSound("music/music.wav");
    if (!leo_IsSoundReady(state->music))
    {
        printf("Failed to load music/music.wav\n");
        leo_UnloadSound(&state->sfx);
        return false;
    }

    // Start music looping at moderate volume
    leo_SetSoundVolume(&state->music, 0.5f);
    leo_PlaySound(&state->music, 0.5f, true);

    return true; // success
}

static void demo_update(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;

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
        printf("Playing sound effect\n");
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

    static float volume = 0.5f;
    // Adjust music volume with Up/Down arrows
    if (leo_IsKeyDown(KEY_UP))
    {
        volume = volume + 0.1f * ctx->dt; // Smooth increase
        if (volume > 1.0f)
            volume = 1.0f;
        leo_SetSoundVolume(&state->music, volume);
        printf("Music volume: %.2f\n", volume);
    }
    if (leo_IsKeyDown(KEY_DOWN))
    {
        volume = volume - 0.1f * ctx->dt; // Smooth decrease
        if (volume < 0.0f)
            volume = 0.0f;
        leo_SetSoundVolume(&state->music, volume);
        printf("Music volume: %.2f\n", volume);
    }

    static float pitch = 1.0f;
    // Adjust sound effect pitch with Left/Right arrows
    if (leo_IsKeyDown(KEY_LEFT))
    {
        pitch = pitch - 0.5f * ctx->dt; // Smooth decrease
        if (pitch < 0.01f)
            pitch = 0.01f;
        leo_SetSoundPitch(&state->sfx, pitch);
        printf("Sound pitch: %.2f\n", pitch);
    }
    if (leo_IsKeyDown(KEY_RIGHT))
    {
        pitch = pitch + 0.5f * ctx->dt; // Smooth increase
        if (pitch > 2.0f)
            pitch = 2.0f;
        leo_SetSoundPitch(&state->sfx, pitch);
        printf("Sound pitch: %.2f\n", pitch);
    }

    static float pan = 1.0f;
    // Adjust sound effect pan with Q/E keys
    if (leo_IsKeyDown(KEY_Q))
    {
        pan = pan - 0.5f * ctx->dt; // Pan left
        if (pan < -1.0f)
            pan = -1.0f;
        leo_SetSoundPan(&state->sfx, pan);
        printf("Sound pan: %.2f\n", pan);
    }
    if (leo_IsKeyDown(KEY_E))
    {
        pan = pan + 0.5f * ctx->dt; // Pan right
        if (pan > 1.0f)
            pan = 1.0f;
        leo_SetSoundPan(&state->sfx, pan);
        printf("Sound pan: %.2f\n", pan);
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

static void demo_shutdown(leo_GameContext *ctx)
{
    FullscreenState *state = (FullscreenState *)ctx->user_data;
    // Clean up audio resources
    leo_StopSound(&state->sfx);
    leo_StopSound(&state->music);
    leo_UnloadSound(&state->sfx);
    leo_UnloadSound(&state->music);
}

bool AudioDemo(bool oneFrame)
{
    FullscreenState state = {
        .fullscreen = false,
        .one_frame = oneFrame,
        .sfx = {0},
        .music = {0},
        .music_paused = false,
    };

    leo_GameConfig cfg = {
        .window_width = 1280,
        .window_height = 720,
        .window_title = "Audio Demo",
        .target_fps = 60,
        .logical_width = 0,
        .logical_height = 0,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = LEO_BLACK,
        .start_paused = false,
        .user_data = &state,
    };

    leo_GameCallbacks cb = {
        .on_setup = demo_setup,
        .on_update = demo_update,
        .on_render_ui = NULL,
        .on_shutdown = demo_shutdown,
    };

    return (leo_GameRun(&cfg, &cb) == 0);
}
