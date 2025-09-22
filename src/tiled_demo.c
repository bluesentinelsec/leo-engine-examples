#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    float x, y;
    float spawn_x, spawn_y;
    float speed;
    bool alive;
} PlayerData;

typedef struct {
    float x, y;
    float origin_x, origin_y;
    float speed;
    float direction_x, direction_y;
    float change_timer;
} EnemyData;

typedef struct {
    // Camera
    leo_Camera2D camera;

    // Textures
    leo_Texture2D dirt_texture;
    leo_Texture2D tree_texture;
    leo_Texture2D hero_texture;
    leo_Texture2D enemy_texture;

    // Tiled map
    leo_TiledMap *map;

    // Actor system
    leo_ActorSystem *actor_system;
    leo_Actor *player_actor;

    bool one_frame;
} ZeldaDemoState;

/* ----------------------------------------------------------
   Collision helpers
   ---------------------------------------------------------- */
static void check_single_enemy_collision(leo_Actor *actor, void *user_data);

static bool check_tree_collision(ZeldaDemoState *state, leo_Rectangle entity_rect) {
    if (!state->map) return false;
    
    const leo_TiledTileLayer *treeLayer = leo_tiled_find_tile_layer(state->map, "tree-layer");
    if (!treeLayer) return false;
    
    for (int y = 0; y < treeLayer->height; y++) {
        for (int x = 0; x < treeLayer->width; x++) {
            uint32_t gid = leo_tiled_get_gid(treeLayer, x, y);
            if (gid == 2) { // Tree tile
                leo_Rectangle tree_rect = {x * 32.0f, y * 32.0f, 32, 32};
                if (leo_CheckCollisionRecs(entity_rect, tree_rect)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void check_enemy_collisions(leo_Actor *self) {
    ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(self));
    PlayerData *player_data = (PlayerData *)leo_actor_userdata(self);
    
    if (!player_data->alive) return;
    
    leo_Rectangle player_rect = {player_data->x, player_data->y, 32, 32};
    
    // Check collision with all enemies
    leo_Actor *root = leo_actor_system_root(state->actor_system);
    leo_actor_for_each_child(root, check_single_enemy_collision, &player_rect);
}

static void check_single_enemy_collision(leo_Actor *actor, void *user_data) {
    leo_Rectangle *player_rect = (leo_Rectangle *)user_data;
    
    if (strcmp(leo_actor_name(actor), "enemy") != 0) return;
    
    EnemyData *enemy_data = (EnemyData *)leo_actor_userdata(actor);
    leo_Rectangle enemy_rect = {enemy_data->x, enemy_data->y, 32, 32};
    
    if (leo_CheckCollisionRecs(*player_rect, enemy_rect)) {
        // Get player from parent and kill them
        ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(actor));
        if (state->player_actor) {
            PlayerData *player_data = (PlayerData *)leo_actor_userdata(state->player_actor);
            player_data->alive = false;
        }
    }
}

/* ----------------------------------------------------------
   Player Actor
   ---------------------------------------------------------- */
static void player_update(leo_Actor *self, float dt) {
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(self));
    
    if (!data->alive) return;

    float dx = 0, dy = 0;
    if (leo_IsKeyDown(KEY_A) || leo_IsKeyDown(KEY_LEFT)) dx = -1;
    if (leo_IsKeyDown(KEY_D) || leo_IsKeyDown(KEY_RIGHT)) dx = 1;
    if (leo_IsKeyDown(KEY_W) || leo_IsKeyDown(KEY_UP)) dy = -1;
    if (leo_IsKeyDown(KEY_S) || leo_IsKeyDown(KEY_DOWN)) dy = 1;

    float new_x = data->x + dx * data->speed * dt;
    float new_y = data->y + dy * data->speed * dt;

    // Check tree collisions with full rectangle
    leo_Rectangle new_rect = {new_x, new_y, 32, 32};
    if (!check_tree_collision(state, new_rect)) {
        data->x = new_x;
        data->y = new_y;
    }
    
    // Check enemy collisions
    check_enemy_collisions(self);
}

static void player_render(leo_Actor *self) {
    ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(self));
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    
    if (data->alive) {
        leo_DrawTextureRec(state->hero_texture, (leo_Rectangle){0, 0, 32, 32}, 
                         (leo_Vector2){data->x, data->y}, LEO_WHITE);
    }
}

static leo_ActorVTable player_vtable = {
    .on_update = player_update,
    .on_render = player_render,
};

/* ----------------------------------------------------------
   Enemy Actor
   ---------------------------------------------------------- */
static void enemy_update(leo_Actor *self, float dt) {
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(self));
    
    // Change direction randomly every 1-3 seconds
    data->change_timer -= dt;
    if (data->change_timer <= 0) {
        data->direction_x = (float)(rand() % 3 - 1); // -1, 0, or 1
        data->direction_y = (float)(rand() % 3 - 1);
        data->change_timer = 1.0f + (float)(rand() % 200) / 100.0f; // 1-3 seconds
    }
    
    float new_x = data->x + data->direction_x * data->speed * dt;
    float new_y = data->y + data->direction_y * data->speed * dt;
    
    // Keep within 100 pixels of origin
    float max_distance = 100.0f;
    if (new_x < data->origin_x - max_distance) new_x = data->origin_x - max_distance;
    if (new_x > data->origin_x + max_distance) new_x = data->origin_x + max_distance;
    if (new_y < data->origin_y - max_distance) new_y = data->origin_y + max_distance;
    if (new_y > data->origin_y + max_distance) new_y = data->origin_y + max_distance;
    
    // World bounds (assuming 32x32 tiles and reasonable map size)
    if (new_x < 0) new_x = 0;
    if (new_y < 0) new_y = 0;
    if (state->map) {
        float world_width = state->map->width * 32.0f - 32;
        float world_height = state->map->height * 32.0f - 32;
        if (new_x > world_width) new_x = world_width;
        if (new_y > world_height) new_y = world_height;
    }
    
    // Check tree collision
    leo_Rectangle new_rect = {new_x, new_y, 32, 32};
    if (!check_tree_collision(state, new_rect)) {
        data->x = new_x;
        data->y = new_y;
    } else {
        // Change direction on collision
        data->direction_x = -data->direction_x;
        data->direction_y = -data->direction_y;
        data->change_timer = 0.5f; // Change direction soon
    }
}

static void enemy_render(leo_Actor *self) {
    ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(self));
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    
    leo_DrawTextureRec(state->enemy_texture, (leo_Rectangle){0, 0, 32, 32}, 
                     (leo_Vector2){data->x, data->y}, LEO_WHITE);
}

static leo_ActorVTable enemy_vtable = {
    .on_update = enemy_update,
    .on_render = enemy_render,
};

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

    // Create actor system
    state->actor_system = leo_actor_system_create();
    leo_Actor *root = leo_actor_system_root(state->actor_system);
    leo_actor_set_userdata(root, state);

    // Create player actor
    PlayerData *player_data = malloc(sizeof(PlayerData));
    const leo_TiledObjectLayer *playerLayer = leo_tiled_find_object_layer(state->map, "player");
    if (playerLayer && playerLayer->object_count > 0) {
        player_data->x = playerLayer->objects[0].x;
        player_data->y = playerLayer->objects[0].y;
        printf("✅ Player spawn: (%.0f, %.0f)\n", player_data->x, player_data->y);
    } else {
        player_data->x = 100.0f;
        player_data->y = 100.0f;
        printf("⚠️ Using fallback player spawn: (%.0f, %.0f)\n", player_data->x, player_data->y);
    }
    
    player_data->spawn_x = player_data->x;
    player_data->spawn_y = player_data->y;
    player_data->speed = 150.0f;
    player_data->alive = true;

    leo_ActorDesc player_desc = {
        .name = "player",
        .vtable = &player_vtable,
        .user_data = player_data,
    };
    state->player_actor = leo_actor_spawn(root, &player_desc);

    // Create enemies from map
    const leo_TiledObjectLayer *enemyLayer = leo_tiled_find_object_layer(state->map, "enemies");
    if (enemyLayer) {
        for (int i = 0; i < enemyLayer->object_count; i++) {
            EnemyData *enemy_data = malloc(sizeof(EnemyData));
            enemy_data->x = enemyLayer->objects[i].x;
            enemy_data->y = enemyLayer->objects[i].y;
            enemy_data->origin_x = enemy_data->x;
            enemy_data->origin_y = enemy_data->y;
            enemy_data->speed = 30.0f;
            enemy_data->direction_x = (float)(rand() % 3 - 1);
            enemy_data->direction_y = (float)(rand() % 3 - 1);
            enemy_data->change_timer = 1.0f;

            leo_ActorDesc enemy_desc = {
                .name = "enemy",
                .vtable = &enemy_vtable,
                .user_data = enemy_data,
            };
            leo_actor_spawn(root, &enemy_desc);
        }
    }

    // Initialize camera
    int w = leo_GetScreenWidth();
    int h = leo_GetScreenHeight();
    state->camera.target = (leo_Vector2){player_data->x, player_data->y};
    state->camera.offset = (leo_Vector2){w / 2.0f, h / 2.0f};
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
    if (state->player_actor) {
        PlayerData *player_data = (PlayerData *)leo_actor_userdata(state->player_actor);
        
        if (!player_data->alive && leo_IsKeyReleased(KEY_R)) {
            player_data->x = player_data->spawn_x;
            player_data->y = player_data->spawn_y;
            player_data->alive = true;
        }
        
        // Update camera to follow player
        state->camera.target.x = player_data->x;
        state->camera.target.y = player_data->y;
    }

    // Update all actors
    leo_actor_system_update(state->actor_system, dt);

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
    }

    // Render all actors
    leo_actor_system_render(state->actor_system);

    leo_EndMode2D();

    // UI overlay
    leo_DrawFPS(20, 32);
    
    // Death message
    if (state->player_actor) {
        PlayerData *player_data = (PlayerData *)leo_actor_userdata(state->player_actor);
        if (!player_data->alive) {
            leo_DrawText("YOU DIED! Press R to respawn", 20, 60, 20, LEO_RED);
        }
    }
}

static void demo_shutdown(leo_GameContext *ctx) {
    ZeldaDemoState *state = (ZeldaDemoState *)ctx->user_data;
    
    // Cleanup actor system (this will free all actors and their data)
    if (state->actor_system) {
        leo_actor_system_destroy(state->actor_system);
        state->actor_system = NULL;
    }
    
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
    ZeldaDemoState state = {0};
    state.one_frame = oneFrame;

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

