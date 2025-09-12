#include <leo/leo.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ----------------------------------------------------------
   Per-demo state (carried in ctx->user_data)
   ---------------------------------------------------------- */
typedef struct VFSState
{
    bool one_frame;
    bool vfs_mounted;
    int test_step;
    char status_text[512];
} VFSState;

/* ----------------------------------------------------------
   VFS Test Functions
   ---------------------------------------------------------- */
static void test_mount_pack(VFSState *state)
{
    printf("=== Testing VFS Pack Mount ===\n");
    
    // Try to mount the Python-created resources.leopack
    bool success = leo_MountResourcePack("resources.leopack", "password", 100);
    if (success) {
        state->vfs_mounted = true;
        snprintf(state->status_text, sizeof(state->status_text), 
                "✅ Successfully mounted resources.leopack");
        printf("✅ Successfully mounted resources.leopack\n");
    } else {
        snprintf(state->status_text, sizeof(state->status_text), 
                "❌ Failed to mount resources.leopack");
        printf("❌ Failed to mount resources.leopack\n");
    }
}

static void test_asset_listing(VFSState *state)
{
    if (!state->vfs_mounted) return;
    
    printf("=== Testing Asset Access ===\n");
    
    // Test common asset paths that should exist in resources.leopack
    const char* test_assets[] = {
        "maps/map.json",
        "music/music.wav", 
        "images/background_320x200.png",
        "images/character_64x64.png",
        "font/font.ttf",
        "sound/coin.wav"
    };
    
    int found_count = 0;
    int total_count = sizeof(test_assets) / sizeof(test_assets[0]);
    
    for (int i = 0; i < total_count; i++) {
        leo_AssetInfo info;
        if (leo_StatAsset(test_assets[i], &info)) {
            printf("✅ Found: %s (%zu bytes, from_pack=%d)\n", 
                   test_assets[i], info.size, info.from_pack);
            found_count++;
        } else {
            printf("❌ Missing: %s\n", test_assets[i]);
        }
    }
    
    snprintf(state->status_text, sizeof(state->status_text), 
            "Asset Check: %d/%d found", found_count, total_count);
}

static void test_asset_loading(VFSState *state)
{
    if (!state->vfs_mounted) return;
    
    printf("=== Testing Asset Loading ===\n");
    
    // Test loading a small text file (map.json)
    size_t size;
    char* content = leo_LoadTextAsset("maps/map.json", &size);
    if (content) {
        printf("✅ Loaded maps/map.json (%zu bytes)\n", size);
        printf("First 100 chars: %.100s...\n", content);
        free(content);
        
        snprintf(state->status_text, sizeof(state->status_text), 
                "✅ Successfully loaded and read map.json (%zu bytes)", size);
    } else {
        printf("❌ Failed to load maps/map.json\n");
        snprintf(state->status_text, sizeof(state->status_text), 
                "❌ Failed to load map.json");
    }
}

static void test_streaming_api(VFSState *state)
{
    if (!state->vfs_mounted) return;
    
    printf("=== Testing Streaming API ===\n");
    
    // Test streaming a larger file
    leo_AssetInfo info;
    leo_AssetStream* stream = leo_OpenAsset("music/music.wav", &info);
    if (stream) {
        printf("✅ Opened music/music.wav for streaming\n");
        printf("   Size: %zu bytes, From pack: %s\n", 
               info.size, info.from_pack ? "yes" : "no");
        
        // Read first 44 bytes (WAV header)
        char header[44];
        size_t read = leo_AssetRead(stream, header, sizeof(header));
        if (read == 44) {
            printf("   WAV header: %.4s format, %.4s chunk\n", 
                   header, header + 8);
        }
        
        // Test seeking
        if (leo_AssetSeek(stream, 0, LEO_SEEK_END)) {
            long long end_pos = leo_AssetTell(stream);
            printf("   Seek test: file size = %lld bytes\n", end_pos);
        }
        
        leo_CloseAsset(stream);
        snprintf(state->status_text, sizeof(state->status_text), 
                "✅ Streaming API works - opened %zu byte audio file", info.size);
    } else {
        printf("❌ Failed to open music/music.wav for streaming\n");
        snprintf(state->status_text, sizeof(state->status_text), 
                "❌ Streaming API failed");
    }
}

/* ----------------------------------------------------------
   Callbacks
   ---------------------------------------------------------- */
static bool demo_setup(leo_GameContext *ctx)
{
    VFSState *state = (VFSState *)ctx->user_data;
    state->vfs_mounted = false;
    state->test_step = 0;
    strcpy(state->status_text, "Starting VFS tests...");
    
    printf("=== Leo Engine VFS Demo ===\n");
    printf("Testing Python-created resources.leopack compatibility\n\n");
    
    return true;
}

static void demo_update(leo_GameContext *ctx)
{
    VFSState *state = (VFSState *)ctx->user_data;
    
    // Run tests sequentially over multiple frames
    if (ctx->frame == 60) {  // Wait a second before starting
        test_mount_pack(state);
        state->test_step = 1;
    } else if (ctx->frame == 120 && state->test_step == 1) {
        test_asset_listing(state);
        state->test_step = 2;
    } else if (ctx->frame == 180 && state->test_step == 2) {
        test_asset_loading(state);
        state->test_step = 3;
    } else if (ctx->frame == 240 && state->test_step == 3) {
        test_streaming_api(state);
        state->test_step = 4;
    }
    
    // Quit after tests complete or on escape
    if (leo_IsKeyPressed(KEY_ESCAPE) || 
        (state->one_frame && ctx->frame >= 300) ||
        (state->test_step >= 4 && ctx->frame >= 360)) {
        leo_GameQuit(ctx);
    }
}

static void demo_render_ui(leo_GameContext *ctx)
{
    VFSState *state = (VFSState *)ctx->user_data;
    
    // Draw title
    leo_DrawText("Leo Engine VFS Demo", 20, 20, 24, LEO_WHITE);
    leo_DrawText("Testing Python-created resources.leopack", 20, 50, 16, LEO_GRAY);
    
    // Draw current status
    leo_DrawText("Status:", 20, 100, 18, LEO_YELLOW);
    leo_DrawText(state->status_text, 20, 130, 16, LEO_WHITE);
    
    // Draw instructions
    leo_DrawText("Press ESC to exit", 20, 200, 14, LEO_GRAY);
    
    // Draw frame counter and FPS
    char frame_text[64];
    snprintf(frame_text, sizeof(frame_text), "Frame: %lld", (long long)ctx->frame);
    leo_DrawText(frame_text, 20, 250, 14, LEO_GRAY);
    leo_DrawFPS(20, 270);
    
    // Show test progress
    if (state->test_step > 0) {
        char progress[64];
        snprintf(progress, sizeof(progress), "Test Progress: %d/4", state->test_step);
        leo_DrawText(progress, 20, 300, 16, LEO_GREEN);
    }
}

static void demo_shutdown(leo_GameContext *ctx)
{
    printf("\n=== VFS Demo Complete ===\n");
    leo_ClearMounts();
}

/* ----------------------------------------------------------
   Entrypoint for demo registry
   ---------------------------------------------------------- */
bool VFSDemo(bool oneFrame)
{
    VFSState state = {
        .one_frame = oneFrame,
        .vfs_mounted = false,
        .test_step = 0,
    };
    strcpy(state.status_text, "Initializing...");

    leo_GameConfig cfg = {
        .window_width = 800,
        .window_height = 600,
        .window_title = "VFS Demo - Python Pack Compatibility",
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
