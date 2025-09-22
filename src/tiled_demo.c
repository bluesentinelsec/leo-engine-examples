#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

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
    float x, y;
    float vel_x, vel_y;
    float life;
    float flash_timer;
    leo_Color color;
} Particle;

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

    // Particles
    Particle particles[1000];

    // Timing
    double update_time;
    double render_time;
    double tile_render_time;
    double actor_render_time;
    double frame_time;
    double last_frame_time;
    double collision_time;

    bool one_frame;
} ZeldaDemoState;

/* ----------------------------------------------------------
   Particle system
   ---------------------------------------------------------- */
static leo_Rectangle get_camera_bounds(leo_Camera2D camera);

static void init_particles(ZeldaDemoState *state) {
    for (int i = 0; i < 1000; i++) {
        state->particles[i].x = (float)(rand() % (state->map->width * 32));
        state->particles[i].y = (float)(rand() % (state->map->height * 32));
        state->particles[i].vel_x = ((float)(rand() % 20) - 10) * 0.3f; // -3 to 3
        state->particles[i].vel_y = ((float)(rand() % 30) + 20) * 0.5f; // 10 to 25 (falling down)
        state->particles[i].life = 1.0f;
        state->particles[i].flash_timer = (float)(rand() % 100) / 10.0f; // Random flash timing
        
        // Random leaf colors
        int color_choice = rand() % 3;
        if (color_choice == 0) state->particles[i].color = (leo_Color){139, 69, 19, 180}; // Brown
        else if (color_choice == 1) state->particles[i].color = (leo_Color){255, 215, 0, 180}; // Gold
        else state->particles[i].color = (leo_Color){255, 140, 0, 180}; // Orange
    }
}

static void update_particles(ZeldaDemoState *state, float dt) {
    for (int i = 0; i < 1000; i++) {
        Particle *p = &state->particles[i];
        
        // Move particle (falling motion)
        p->x += p->vel_x * dt;
        p->y += p->vel_y * dt;
        
        // Add gentle swaying
        p->vel_x += sinf(p->y * 0.005f) * 2.0f * dt;
        
        // Update flash timer
        p->flash_timer += dt;
        
        // Reset particle when it falls off screen
        if (p->y > state->map->height * 32.0f + 50) {
            p->x = (float)(rand() % (state->map->width * 32));
            p->y = -50.0f;
            p->vel_x = ((float)(rand() % 20) - 10) * 0.3f;
            p->vel_y = ((float)(rand() % 30) + 20) * 0.5f;
            p->flash_timer = (float)(rand() % 100) / 10.0f;
        }
        
        // Wrap horizontally
        if (p->x < -10) p->x = state->map->width * 32.0f + 10;
        if (p->x > state->map->width * 32.0f + 10) p->x = -10;
    }
}

static void render_particles(ZeldaDemoState *state) {
    leo_Rectangle cam_bounds = get_camera_bounds(state->camera);
    
    for (int i = 0; i < 1000; i++) {
        Particle *p = &state->particles[i];
        
        // Only render particles in camera view
        if (p->x >= cam_bounds.x - 10 && p->x <= cam_bounds.x + cam_bounds.width + 10 &&
            p->y >= cam_bounds.y - 10 && p->y <= cam_bounds.y + cam_bounds.height + 10) {
            
            // Flash effect - make particles brighter/dimmer
            float flash = 0.5f + 0.5f * sinf(p->flash_timer * 3.0f);
            leo_Color flash_color = p->color;
            flash_color.a = (unsigned char)(flash_color.a * flash);
            
            leo_DrawCircle((int)p->x, (int)p->y, 1.5f, flash_color);
        }
    }
}

/* ----------------------------------------------------------
   Timing helpers
   ---------------------------------------------------------- */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/* ----------------------------------------------------------
   Culling helpers
   ---------------------------------------------------------- */
static leo_Rectangle get_camera_bounds(leo_Camera2D camera) {
    float left = camera.target.x - camera.offset.x / camera.zoom;
    float top = camera.target.y - camera.offset.y / camera.zoom;
    float width = leo_GetScreenWidth() / camera.zoom;
    float height = leo_GetScreenHeight() / camera.zoom;
    return (leo_Rectangle){left, top, width, height};
}

static bool is_in_camera_bounds(leo_Rectangle bounds, float x, float y, float size) {
    leo_Rectangle obj_rect = {x, y, size, size};
    return leo_CheckCollisionRecs(bounds, obj_rect);
}

/* ----------------------------------------------------------
   Collision helpers
   ---------------------------------------------------------- */
static void check_single_enemy_collision(leo_Actor *actor, void *user_data);

static bool check_tree_collision(ZeldaDemoState *state, float x, float y) {
    if (!state->map) return false;
    
    const leo_TiledTileLayer *treeLayer = leo_tiled_find_tile_layer(state->map, "tree-layer");
    if (!treeLayer) return false;
    
    // Check the 4 corners of the 32x32 entity
    int corners[4][2] = {
        {(int)(x / 32), (int)(y / 32)},                    // top-left
        {(int)((x + 31) / 32), (int)(y / 32)},             // top-right
        {(int)(x / 32), (int)((y + 31) / 32)},             // bottom-left
        {(int)((x + 31) / 32), (int)((y + 31) / 32)}       // bottom-right
    };
    
    for (int i = 0; i < 4; i++) {
        int tile_x = corners[i][0];
        int tile_y = corners[i][1];
        
        if (tile_x >= 0 && tile_x < treeLayer->width && tile_y >= 0 && tile_y < treeLayer->height) {
            if (leo_tiled_get_gid(treeLayer, tile_x, tile_y) == 2) {
                return true;
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
    
    // Check collision with all enemies (with distance culling)
    leo_Actor *root = leo_actor_system_root(state->actor_system);
    leo_actor_for_each_child(root, check_single_enemy_collision, &player_rect);
}

static void check_single_enemy_collision(leo_Actor *actor, void *user_data) {
    leo_Rectangle *player_rect = (leo_Rectangle *)user_data;
    
    if (strcmp(leo_actor_name(actor), "enemy") != 0) return;
    
    EnemyData *enemy_data = (EnemyData *)leo_actor_userdata(actor);
    
    // Quick distance check first (cheaper than rectangle collision)
    float dx = enemy_data->x - player_rect->x;
    float dy = enemy_data->y - player_rect->y;
    float distance_sq = dx * dx + dy * dy;
    
    // Skip if too far away (64 pixels = 2 tile widths)
    if (distance_sq > 64 * 64) return;
    
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

    double collision_start = get_time_ms();

    float dx = 0, dy = 0;
    if (leo_IsKeyDown(KEY_A) || leo_IsKeyDown(KEY_LEFT)) dx = -1;
    if (leo_IsKeyDown(KEY_D) || leo_IsKeyDown(KEY_RIGHT)) dx = 1;
    if (leo_IsKeyDown(KEY_W) || leo_IsKeyDown(KEY_UP)) dy = -1;
    if (leo_IsKeyDown(KEY_S) || leo_IsKeyDown(KEY_DOWN)) dy = 1;

    float new_x = data->x + dx * data->speed * dt;
    float new_y = data->y + dy * data->speed * dt;

    // World bounds
    if (new_x < 0) new_x = 0;
    if (new_y < 0) new_y = 0;
    if (state->map) {
        float world_width = state->map->width * 32.0f - 32;
        float world_height = state->map->height * 32.0f - 32;
        if (new_x > world_width) new_x = world_width;
        if (new_y > world_height) new_y = world_height;
    }

    // Try full movement first
    if (!check_tree_collision(state, new_x, new_y)) {
        data->x = new_x;
        data->y = new_y;
    } else {
        // Try horizontal movement only
        if (!check_tree_collision(state, new_x, data->y)) {
            data->x = new_x;
        }
        // Try vertical movement only
        else if (!check_tree_collision(state, data->x, new_y)) {
            data->y = new_y;
        }
    }
    
    // Check enemy collisions
    check_enemy_collisions(self);
    
    state->collision_time += get_time_ms() - collision_start;
}

static void player_render(leo_Actor *self) {
    ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(self));
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    
    if (data->alive) {
        leo_Rectangle cam_bounds = get_camera_bounds(state->camera);
        if (is_in_camera_bounds(cam_bounds, data->x, data->y, 32)) {
            leo_DrawTextureRec(state->hero_texture, (leo_Rectangle){0, 0, 32, 32}, 
                             (leo_Vector2){data->x, data->y}, LEO_WHITE);
        }
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
    
    double collision_start = get_time_ms();
    
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
    if (new_y < data->origin_y - max_distance) new_y = data->origin_y - max_distance;
    if (new_y > data->origin_y + max_distance) new_y = data->origin_y + max_distance;
    
    // World bounds
    if (new_x < 0) new_x = 0;
    if (new_y < 0) new_y = 0;
    if (state->map) {
        float world_width = state->map->width * 32.0f - 32;
        float world_height = state->map->height * 32.0f - 32;
        if (new_x > world_width) new_x = world_width;
        if (new_y > world_height) new_y = world_height;
    }
    
    // Try full movement first
    if (!check_tree_collision(state, new_x, new_y)) {
        data->x = new_x;
        data->y = new_y;
    } else {
        // Try horizontal movement only
        if (!check_tree_collision(state, new_x, data->y)) {
            data->x = new_x;
        }
        // Try vertical movement only
        else if (!check_tree_collision(state, data->x, new_y)) {
            data->y = new_y;
        } else {
            // If completely blocked, change direction
            data->direction_x = (float)(rand() % 3 - 1);
            data->direction_y = (float)(rand() % 3 - 1);
            data->change_timer = 0.2f;
        }
    }
    
    state->collision_time += get_time_ms() - collision_start;
}

static void enemy_render(leo_Actor *self) {
    ZeldaDemoState *state = (ZeldaDemoState *)leo_actor_userdata(leo_actor_parent(self));
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    
    leo_Rectangle cam_bounds = get_camera_bounds(state->camera);
    if (is_in_camera_bounds(cam_bounds, data->x, data->y, 32)) {
        leo_DrawTextureRec(state->enemy_texture, (leo_Rectangle){0, 0, 32, 32}, 
                         (leo_Vector2){data->x, data->y}, LEO_WHITE);
    }
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

    // Initialize particles
    init_particles(state);

    return true;
}

/* ----------------------------------------------------------
   Update
   ---------------------------------------------------------- */
static void demo_update(leo_GameContext *ctx) {
    ZeldaDemoState *state = (ZeldaDemoState *)ctx->user_data;
    float dt = ctx->dt;
    
    // Measure frame time
    double current_time = get_time_ms();
    if (state->last_frame_time > 0) {
        state->frame_time = current_time - state->last_frame_time;
    }
    state->last_frame_time = current_time;
    
    // Reset collision timer
    state->collision_time = 0;
    
    double start_time = get_time_ms();

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

    // Update particles
    update_particles(state, dt);

    state->update_time = get_time_ms() - start_time;

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
    
    double render_start = get_time_ms();

    // Begin world render with camera
    leo_BeginMode2D(state->camera);

    double tile_start = get_time_ms();
    
    // Render Tiled map layers
    if (state->map) {
        leo_Rectangle cam_bounds = get_camera_bounds(state->camera);
        
        // Calculate visible tile range
        int start_x = (int)(cam_bounds.x / 32) - 1;
        int start_y = (int)(cam_bounds.y / 32) - 1;
        int end_x = (int)((cam_bounds.x + cam_bounds.width) / 32) + 1;
        int end_y = (int)((cam_bounds.y + cam_bounds.height) / 32) + 1;
        
        // Clamp to map bounds
        if (start_x < 0) start_x = 0;
        if (start_y < 0) start_y = 0;
        if (end_x >= state->map->width) end_x = state->map->width - 1;
        if (end_y >= state->map->height) end_y = state->map->height - 1;
        
        // Render dirt layer (GID 1)
        const leo_TiledTileLayer *dirtLayer = leo_tiled_find_tile_layer(state->map, "dirt-layer");
        if (dirtLayer) {
            for (int y = start_y; y <= end_y; y++) {
                for (int x = start_x; x <= end_x; x++) {
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
            for (int y = start_y; y <= end_y; y++) {
                for (int x = start_x; x <= end_x; x++) {
                    uint32_t gid = leo_tiled_get_gid(treeLayer, x, y);
                    if (gid == 2) { // Tree tile
                        leo_DrawTextureRec(state->tree_texture, (leo_Rectangle){0, 0, 32, 32}, 
                                         (leo_Vector2){x * 32.0f, y * 32.0f}, LEO_WHITE);
                    }
                }
            }
        }
    }
    
    state->tile_render_time = get_time_ms() - tile_start;
    
    double actor_start = get_time_ms();
    
    // Render all actors
    leo_actor_system_render(state->actor_system);
    
    state->actor_render_time = get_time_ms() - actor_start;

    // Render particles on top
    render_particles(state);

    leo_EndMode2D();

    state->render_time = get_time_ms() - render_start;

    // UI overlay
    leo_DrawFPS(20, 32);
    
    // Timing info
    char timing_text[256];
    sprintf(timing_text, "Frame: %.2fms (%.1f FPS)", state->frame_time, 1000.0 / state->frame_time);
    leo_DrawText(timing_text, 20, 60, 16, LEO_WHITE);
    
    sprintf(timing_text, "Update: %.2fms", state->update_time);
    leo_DrawText(timing_text, 20, 80, 16, LEO_GREEN);
    
    sprintf(timing_text, "Render: %.2fms", state->render_time);
    leo_DrawText(timing_text, 20, 100, 16, LEO_GREEN);
    
    sprintf(timing_text, "  Tiles: %.2fms", state->tile_render_time);
    leo_DrawText(timing_text, 20, 120, 16, LEO_YELLOW);
    
    sprintf(timing_text, "  Actors: %.2fms", state->actor_render_time);
    leo_DrawText(timing_text, 20, 140, 16, LEO_YELLOW);
    
    sprintf(timing_text, "Collision: %.2fms", state->collision_time);
    leo_DrawText(timing_text, 20, 160, 16, LEO_RED);
    
    // Death message
    if (state->player_actor) {
        PlayerData *player_data = (PlayerData *)leo_actor_userdata(state->player_actor);
        if (!player_data->alive) {
            leo_DrawText("YOU DIED! Press R to respawn", 20, 190, 20, LEO_RED);
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

