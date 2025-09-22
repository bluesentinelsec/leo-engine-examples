#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    // Player
    float player_x, player_y;
    float player_speed;
    float spawn_x, spawn_y;  // Respawn position
    bool player_alive;

    // Camera
    leo_Camera2D camera;

    // Textures
    leo_Texture2D dirt_texture;
    leo_Texture2D tree_texture;
    leo_Texture2D hero_texture;
    leo_Texture2D enemy_texture;

    // Tiled map
    leo_TiledMap *map;

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

    // Load Tiled map
    state->map = leo_tiled_load("maps/demo.json", NULL);
    if (!state->map) {
        printf("❌ Failed to load Tiled map\n");
        return false;
    }
    printf("✅ Loaded Tiled map: %dx%d tiles\n", state->map->width, state->map->height);

    // Find player spawn from map
    const leo_TiledObjectLayer *playerLayer = leo_tiled_find_object_layer(state->map, "player");
    if (playerLayer && playerLayer->object_count > 0) {
        state->player_x = playerLayer->objects[0].x;
        state->player_y = playerLayer->objects[0].y;
        printf("✅ Player spawn: (%.0f, %.0f)\n", state->player_x, state->player_y);
    } else {
        // Fallback spawn
        state->player_x = 100.0f;
        state->player_y = 100.0f;
        printf("⚠️ Using fallback player spawn: (%.0f, %.0f)\n", state->player_x, state->player_y);
    }
    
    // Store spawn position for respawning
    state->spawn_x = state->player_x;
    state->spawn_y = state->player_y;
    state->player_alive = true;
    
    state->player_speed = 150.0f;

    // Initialize camera
    int w = leo_GetScreenWidth();
    int h = leo_GetScreenHeight();
    state->camera.target = (leo_Vector2){state->player_x, state->player_y};
    state->camera.offset = (leo_Vector2){w / 2.0f, h / 2.0f}; // True screen center
    state->camera.rotation = 0.0f;
    state->camera.zoom = 1.0f;

    return true;
}

/* ----------------------------------------------------------
   Update
   ---------------------------------------------------------- */
static void demo_update(leo_GameContext *ctx) {
    ZeldaDemoState *state = (ZeldaDemoState *)ctx->user_data;
    float dt = ctx->dt;

    // Handle respawn
    if (!state->player_alive && leo_IsKeyReleased(KEY_R)) {
        state->player_x = state->spawn_x;
        state->player_y = state->spawn_y;
        state->player_alive = true;
    }

    // Only move if alive
    if (state->player_alive) {
        // Store old position for collision rollback
        float old_x = state->player_x;
        float old_y = state->player_y;

        // WASD movement
        if (leo_IsKeyDown(KEY_W)) state->player_y -= state->player_speed * dt;
        if (leo_IsKeyDown(KEY_S)) state->player_y += state->player_speed * dt;
        if (leo_IsKeyDown(KEY_A)) state->player_x -= state->player_speed * dt;
        if (leo_IsKeyDown(KEY_D)) state->player_x += state->player_speed * dt;

        // Clamp player to world bounds (map size: 130x80 tiles = 4160x2560 pixels)
        float world_min_x = 0;
        float world_min_y = 0;
        float world_max_x = 4160;
        float world_max_y = 2560;

        if (state->player_x < world_min_x) state->player_x = world_min_x;
        if (state->player_y < world_min_y) state->player_y = world_min_y;
        if (state->player_x > world_max_x) state->player_x = world_max_x;
        if (state->player_y > world_max_y) state->player_y = world_max_y;

        // Check tree collisions
        const leo_TiledTileLayer *treeLayer = leo_tiled_find_tile_layer(state->map, "tree-layer");
        if (treeLayer) {
            leo_Rectangle playerRect = {state->player_x, state->player_y, 32, 32};
            
            // Check tiles around player position
            int tile_x = (int)(state->player_x / 32);
            int tile_y = (int)(state->player_y / 32);
            
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int check_x = tile_x + dx;
                    int check_y = tile_y + dy;
                    
                    if (check_x >= 0 && check_x < treeLayer->width && 
                        check_y >= 0 && check_y < treeLayer->height) {
                        
                        uint32_t gid = leo_tiled_get_gid(treeLayer, check_x, check_y);
                        if (gid == 2) { // Tree tile
                            leo_Rectangle treeRect = {check_x * 32.0f, check_y * 32.0f, 32, 32};
                            if (leo_CheckCollisionRecs(playerRect, treeRect)) {
                                // Rollback movement
                                state->player_x = old_x;
                                state->player_y = old_y;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Check enemy collisions
        const leo_TiledObjectLayer *enemyLayer = leo_tiled_find_object_layer(state->map, "enemies");
        if (enemyLayer) {
            leo_Rectangle playerRect = {state->player_x, state->player_y, 32, 32};
            
            for (int i = 0; i < enemyLayer->object_count; i++) {
                const leo_TiledObject *enemy = &enemyLayer->objects[i];
                leo_Rectangle enemyRect = {enemy->x, enemy->y, 32, 32};
                
                if (leo_CheckCollisionRecs(playerRect, enemyRect)) {
                    state->player_alive = false;
                    break;
                }
            }
        }
    }

    // Camera follows player
    state->camera.target.x = state->player_x;
    state->camera.target.y = state->player_y;

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

    // Render Tiled map layers
    if (state->map) {
        // Render dirt layer (GID 1)
        const leo_TiledTileLayer *dirtLayer = leo_tiled_find_tile_layer(state->map, "dirt-layer");
        if (dirtLayer) {
            for (int y = 0; y < dirtLayer->height; y++) {
                for (int x = 0; x < dirtLayer->width; x++) {
                    uint32_t gid = leo_tiled_get_gid(dirtLayer, x, y);
                    if (gid == 1) { // Dirt tile
                        leo_DrawTextureRec(state->dirt_texture, (leo_Rectangle){0, 0, 32, 32}, 
                                         (leo_Vector2){x * 32.0f, y * 32.0f}, LEO_WHITE);
                    }
                }
            }
        }

        // Render tree layer (GID 2)
        const leo_TiledTileLayer *treeLayer = leo_tiled_find_tile_layer(state->map, "tree-layer");
        if (treeLayer) {
            for (int y = 0; y < treeLayer->height; y++) {
                for (int x = 0; x < treeLayer->width; x++) {
                    uint32_t gid = leo_tiled_get_gid(treeLayer, x, y);
                    if (gid == 2) { // Tree tile
                        leo_DrawTextureRec(state->tree_texture, (leo_Rectangle){0, 0, 32, 32}, 
                                         (leo_Vector2){x * 32.0f, y * 32.0f}, LEO_WHITE);
                    }
                }
            }
        }

        // Render enemies from object layer
        const leo_TiledObjectLayer *enemyLayer = leo_tiled_find_object_layer(state->map, "enemies");
        if (enemyLayer) {
            for (int i = 0; i < enemyLayer->object_count; i++) {
                const leo_TiledObject *enemy = &enemyLayer->objects[i];
                leo_DrawTextureRec(state->enemy_texture, (leo_Rectangle){0, 0, 32, 32}, 
                                 (leo_Vector2){enemy->x, enemy->y}, LEO_WHITE);
            }
        }
    }

    // Draw hero (player) - only if alive
    if (state->player_alive) {
        leo_DrawTextureRec(state->hero_texture, (leo_Rectangle){0, 0, 32, 32}, (leo_Vector2){state->player_x, state->player_y}, LEO_WHITE);
    }

    leo_EndMode2D();

    // UI overlay
    leo_DrawFPS(20, 32);
    
    // Death message
    if (!state->player_alive) {
        leo_DrawText("YOU DIED! Press R to respawn", 20, 60, 20, LEO_RED);
    }
}

static void demo_shutdown(leo_GameContext *ctx) {
    ZeldaDemoState *state = (ZeldaDemoState *)ctx->user_data;
    
    // Cleanup Tiled map
    if (state->map) {
        leo_tiled_free(state->map);
        state->map = NULL;
    }
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

