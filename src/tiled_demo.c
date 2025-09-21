#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define TILE_SIZE 32

typedef struct {
    float x, y;
    leo_Texture2D texture;
} DirtData;

typedef struct {
    float x, y;
    leo_Texture2D texture;
    leo_Rectangle collision_box;
} TreeData;

typedef struct {
    float x, y;
    leo_Texture2D texture;
    float speed;
    bool alive;
} PlayerData;

typedef struct {
    float x, y;
    leo_Texture2D texture;
    leo_Rectangle collision_box;
} EnemyData;

typedef struct {
    bool one_frame;
    leo_Actor *player_actor;
    
    // Tiled map data
    leo_TiledMap *map;
    leo_Texture2D tileset_texture;
} TiledDemoState;

// Actor VTables
static bool dirt_init(leo_Actor *self);
static void dirt_render(leo_Actor *self);

static bool tree_init(leo_Actor *self);
static void tree_render(leo_Actor *self);
static void tree_exit(leo_Actor *self);

static bool player_init(leo_Actor *self);
static void player_update(leo_Actor *self, float dt);
static void player_render(leo_Actor *self);
static void player_exit(leo_Actor *self);

static bool enemy_init(leo_Actor *self);
static void enemy_render(leo_Actor *self);
static void enemy_exit(leo_Actor *self);

static const leo_ActorVTable dirt_vtable = {
    .on_init = dirt_init,
    .on_update = NULL,
    .on_render = dirt_render,
    .on_exit = NULL
};

static const leo_ActorVTable tree_vtable = {
    .on_init = tree_init,
    .on_update = NULL,
    .on_render = tree_render,
    .on_exit = tree_exit
};

static const leo_ActorVTable player_vtable = {
    .on_init = player_init,
    .on_update = player_update,
    .on_render = player_render,
    .on_exit = player_exit
};

static const leo_ActorVTable enemy_vtable = {
    .on_init = enemy_init,
    .on_update = NULL,
    .on_render = enemy_render,
    .on_exit = enemy_exit
};

// Helper function to get collision rectangle for actor
static leo_Rectangle get_actor_rect(float x, float y) {
    return (leo_Rectangle){x, y, TILE_SIZE, TILE_SIZE};
}

// Actor spawning functions
static leo_Actor* spawn_tree_actor(leo_GameContext *ctx, float x, float y) {
    leo_ActorDesc tree_desc = {
        .name = "tree",
        .vtable = &tree_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    leo_Actor *tree = leo_actor_spawn(ctx->root, &tree_desc);
    if (tree) {
        TreeData *data = (TreeData *)leo_actor_userdata(tree);
        if (data) {
            data->x = x;
            data->y = y;
            data->collision_box = get_actor_rect(x, y);
        }
    }
    return tree;
}

static leo_Actor* spawn_enemy_actor(leo_GameContext *ctx, float x, float y) {
    leo_ActorDesc enemy_desc = {
        .name = "enemy",
        .vtable = &enemy_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    leo_Actor *enemy = leo_actor_spawn(ctx->root, &enemy_desc);
    if (enemy) {
        EnemyData *data = (EnemyData *)leo_actor_userdata(enemy);
        if (data) {
            data->x = x;
            data->y = y;
            data->collision_box = get_actor_rect(x, y);
        }
    }
    return enemy;
}

static leo_Actor* spawn_player_actor(leo_GameContext *ctx, float x, float y) {
    leo_ActorDesc player_desc = {
        .name = "player",
        .vtable = &player_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    leo_Actor *player = leo_actor_spawn(ctx->root, &player_desc);
    if (player) {
        PlayerData *data = (PlayerData *)leo_actor_userdata(player);
        if (data) {
            data->x = x;
            data->y = y;
        }
    }
    return player;
}

// Create world from Tiled map data
static void create_world_from_map(leo_GameContext *ctx, leo_TiledMap *map) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    printf("Creating world from Tiled map...\n");
    
    // Process tree layer (tile-based obstacles)
    const leo_TiledTileLayer *tree_layer = leo_tiled_find_tile_layer(map, "tree-layer");
    if (tree_layer) {
        int tree_count = 0;
        for (int y = 0; y < tree_layer->height; y++) {
            for (int x = 0; x < tree_layer->width; x++) {
                uint32_t gid = leo_tiled_get_gid(tree_layer, x, y);
                if (gid == 2) { // Tree tile
                    spawn_tree_actor(ctx, x * 32, y * 32);
                    tree_count++;
                }
            }
        }
        printf("  Created %d trees from tile layer\n", tree_count);
    }
    
    // Process enemy objects
    const leo_TiledObjectLayer *enemy_layer = leo_tiled_find_object_layer(map, "enemies");
    if (enemy_layer) {
        int enemy_count = 0;
        for (int i = 0; i < enemy_layer->object_count; i++) {
            const leo_TiledObject *obj = &enemy_layer->objects[i];
            if (obj->type && strcmp(obj->type, "enemy") == 0) {
                spawn_enemy_actor(ctx, (float)obj->x, (float)obj->y);
                enemy_count++;
            }
        }
        printf("  Created %d enemies from object layer\n", enemy_count);
    }
    
    // Process player spawn
    const leo_TiledObjectLayer *player_layer = leo_tiled_find_object_layer(map, "player");
    if (player_layer && player_layer->object_count > 0) {
        const leo_TiledObject *obj = &player_layer->objects[0];
        if (obj->type && strcmp(obj->type, "player") == 0) {
            state->player_actor = spawn_player_actor(ctx, (float)obj->x, (float)obj->y);
            printf("  Created player at (%.0f, %.0f)\n", obj->x, obj->y);
        }
    }
}

// Dirt Actor Implementation
static bool dirt_init(leo_Actor *self) {
    DirtData *data = malloc(sizeof(DirtData));
    data->x = 64;
    data->y = 64;
    data->texture = leo_LoadTexture("images/dirt_32x32.png");
    
    if (data->texture._handle == NULL) {
        printf("❌ Failed to load dirt texture\n");
        free(data);
        return false;
    }
    
    leo_actor_set_userdata(self, data);
    return true;
}

static void dirt_render(leo_Actor *self) {
    DirtData *data = (DirtData *)leo_actor_userdata(self);
    leo_Rectangle src = {0, 0, TILE_SIZE, TILE_SIZE};
    leo_Vector2 pos = {data->x, data->y};
    leo_DrawTextureRec(data->texture, src, pos, LEO_WHITE);
}

// Tree Actor Implementation
static bool tree_init(leo_Actor *self) {
    TreeData *data = malloc(sizeof(TreeData));
    data->x = 128;
    data->y = 64;
    data->texture = leo_LoadTexture("images/tree_32x32.png");
    data->collision_box = get_actor_rect(data->x, data->y);
    
    if (data->texture._handle == NULL) {
        printf("❌ Failed to load tree texture\n");
        free(data);
        return false;
    }
    
    leo_actor_set_userdata(self, data);
    return true;
}

static void tree_render(leo_Actor *self) {
    TreeData *data = (TreeData *)leo_actor_userdata(self);
    leo_Rectangle src = {0, 0, TILE_SIZE, TILE_SIZE};
    leo_Vector2 pos = {data->x, data->y};
    leo_DrawTextureRec(data->texture, src, pos, LEO_WHITE);
}

static void tree_exit(leo_Actor *self) {
    TreeData *data = (TreeData *)leo_actor_userdata(self);
    leo_UnloadTexture(&data->texture);
    free(data);
}

// Player Actor Implementation
static bool player_init(leo_Actor *self) {
    PlayerData *data = malloc(sizeof(PlayerData));
    data->x = 32;
    data->y = 32;
    data->speed = 80.0f; // pixels per second
    data->alive = true;
    data->texture = leo_LoadTexture("images/hero_32x32.png");
    
    if (data->texture._handle == NULL) {
        printf("❌ Failed to load player texture\n");
        free(data);
        return false;
    }
    
    leo_actor_set_userdata(self, data);
    return true;
}

static void player_update(leo_Actor *self, float dt) {
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    
    if (!data->alive) return;
    
    float new_x = data->x;
    float new_y = data->y;
    
    // NES Zelda-style movement (4-directional, grid-like)
    if (leo_IsKeyDown(KEY_W) || leo_IsKeyDown(KEY_UP)) {
        new_y -= data->speed * dt;
    }
    if (leo_IsKeyDown(KEY_S) || leo_IsKeyDown(KEY_DOWN)) {
        new_y += data->speed * dt;
    }
    if (leo_IsKeyDown(KEY_A) || leo_IsKeyDown(KEY_LEFT)) {
        new_x -= data->speed * dt;
    }
    if (leo_IsKeyDown(KEY_D) || leo_IsKeyDown(KEY_RIGHT)) {
        new_x += data->speed * dt;
    }
    
    // Keep player in bounds
    if (new_x < 0) new_x = 0;
    if (new_x > 320 - TILE_SIZE) new_x = 320 - TILE_SIZE;
    if (new_y < 0) new_y = 0;
    if (new_y > 200 - TILE_SIZE) new_y = 200 - TILE_SIZE;
    
    // Check collision with tree (get tree actor from parent system)
    leo_Rectangle player_rect = get_actor_rect(new_x, new_y);
    
    // Check collision with all tree actors
    leo_Actor *parent = leo_actor_parent(self);
    if (parent) {
        // Simple collision check - iterate through children to find trees
        // In a full game, you'd use spatial partitioning or groups for efficiency
        bool collision = false;
        
        // This is a simplified approach - in practice you'd use actor groups or spatial queries
        // For now, we'll check against the first tree we find by name
        leo_Actor *tree_actor = leo_actor_find_child_by_name(parent, "tree");
        while (tree_actor && !collision) {
            TreeData *tree_data = (TreeData *)leo_actor_userdata(tree_actor);
            if (tree_data && leo_CheckCollisionRecs(player_rect, tree_data->collision_box)) {
                collision = true;
            }
            // Note: This only checks the first tree. In a full implementation,
            // you'd iterate through all tree actors or use a spatial system
            break;
        }
        
        if (collision) {
            return; // Don't move if collision detected
        }
    }
    
    // Check collision with enemy (simplified - checks first enemy found)
    leo_Actor *enemy_actor = leo_actor_find_child_by_name(parent, "enemy");
    if (enemy_actor) {
        EnemyData *enemy_data = (EnemyData *)leo_actor_userdata(enemy_actor);
        if (enemy_data && leo_CheckCollisionRecs(player_rect, enemy_data->collision_box)) {
            // Player dies on enemy collision
            data->alive = false;
            printf("💀 Player killed by enemy!\n");
            return;
        }
    }
    
    // Update position if no collision
    data->x = new_x;
    data->y = new_y;
}

static void player_render(leo_Actor *self) {
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    
    leo_Rectangle src = {0, 0, TILE_SIZE, TILE_SIZE};
    leo_Vector2 pos = {data->x, data->y};
    
    // Tint red if dead
    leo_Color tint = data->alive ? LEO_WHITE : LEO_RED;
    leo_DrawTextureRec(data->texture, src, pos, tint);
}

static void player_exit(leo_Actor *self) {
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    leo_UnloadTexture(&data->texture);
    free(data);
}

// Enemy Actor Implementation
static bool enemy_init(leo_Actor *self) {
    EnemyData *data = malloc(sizeof(EnemyData));
    data->x = 192;
    data->y = 128;
    data->texture = leo_LoadTexture("images/enemy_32x32.png");
    data->collision_box = get_actor_rect(data->x, data->y);
    
    if (data->texture._handle == NULL) {
        printf("❌ Failed to load enemy texture\n");
        free(data);
        return false;
    }
    
    leo_actor_set_userdata(self, data);
    return true;
}

static void enemy_render(leo_Actor *self) {
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    leo_Rectangle src = {0, 0, TILE_SIZE, TILE_SIZE};
    leo_Vector2 pos = {data->x, data->y};
    leo_DrawTextureRec(data->texture, src, pos, LEO_WHITE);
}

static void enemy_exit(leo_Actor *self) {
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    leo_UnloadTexture(&data->texture);
    free(data);
}

// Demo Implementation
static bool demo_setup(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    // Mount VFS
    if (!leo_MountResourcePack("resources.leopack", "password", 1)) {
        printf("❌ Unable to mount resources.leopack\n");
        return false;
    }
    printf("✅ Successfully mounted resources.leopack\n");
    
    // Load Tiled map
    leo_TiledLoadOptions tiled_opts = {0};
    tiled_opts.allow_compression = 1;
    
    state->map = leo_tiled_load("maps/demo.json", &tiled_opts);
    if (!state->map) {
        printf("❌ Failed to load Tiled map: %s\n", leo_GetError());
        return false;
    }
    
    // Debug: Print map info
    printf("✅ Loaded Tiled map:\n");
    printf("  Size: %dx%d tiles\n", state->map->width, state->map->height);
    printf("  Tile size: %dx%d pixels\n", state->map->tilewidth, state->map->tileheight);
    printf("  Orientation: %s\n", state->map->orientation ? state->map->orientation : "NULL");
    printf("  Render order: %s\n", state->map->renderorder ? state->map->renderorder : "NULL");
    printf("  Tilesets: %d\n", state->map->tileset_count);
    printf("  Layers: %d\n", state->map->layer_count);
    
    // Debug: Print tileset info
    for (int i = 0; i < state->map->tileset_count; i++) {
        const leo_TiledTileset *ts = &state->map->tilesets[i];
        printf("  Tileset %d:\n", i);
        printf("    Name: %s\n", ts->name ? ts->name : "NULL");
        printf("    First GID: %d\n", ts->first_gid);
        printf("    Tile size: %dx%d\n", ts->tilewidth, ts->tileheight);
        printf("    Columns: %d, Count: %d\n", ts->columns, ts->tilecount);
        printf("    Image: %s\n", ts->image ? ts->image : "NULL");
    }
    
    // Debug: Print layer info
    for (int i = 0; i < state->map->layer_count; i++) {
        const leo_TiledLayer *layer = &state->map->layers[i];
        printf("  Layer %d: Type %d\n", i, layer->type);
        
        if (layer->type == LEO_TILED_LAYER_TILE) {
            const leo_TiledTileLayer *tl = &layer->as.tile;
            printf("    Tile Layer: %s (%dx%d)\n", 
                   tl->name ? tl->name : "NULL", tl->width, tl->height);
            printf("    GID count: %zu\n", tl->gids_count);
            
            // Sample first few tiles
            printf("    First 10 GIDs: ");
            for (int j = 0; j < 10 && j < (int)tl->gids_count; j++) {
                printf("%u ", tl->gids[j]);
            }
            printf("\n");
        } else if (layer->type == LEO_TILED_LAYER_OBJECT) {
            const leo_TiledObjectLayer *ol = &layer->as.object;
            printf("    Object Layer: %s (%d objects)\n", 
                   ol->name ? ol->name : "NULL", ol->object_count);
        }
    }
    
    // Try to load tileset texture (if available)
    if (state->map->tileset_count > 0 && state->map->tilesets[0].image) {
        state->tileset_texture = leo_LoadTexture(state->map->tilesets[0].image);
        if (state->tileset_texture._handle) {
            printf("✅ Loaded tileset texture: %dx%d\n", 
                   state->tileset_texture.width, state->tileset_texture.height);
        } else {
            printf("❌ Failed to load tileset texture: %s\n", state->map->tilesets[0].image);
        }
    }
    
    // Create world from map data
    create_world_from_map(ctx, state->map);
    
    printf("✅ Tiled Demo initialized with map-based world\n");
    return true;
}

static void demo_update(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    if (state->one_frame) {
        leo_GameQuit(ctx);
        return;
    }
    
    // Reset game on R key
    if (leo_IsKeyPressed(KEY_R)) {
        PlayerData *player_data = (PlayerData *)leo_actor_userdata(state->player_actor);
        if (player_data && state->map) {
            player_data->alive = true;
            
            // Reset to map spawn position
            const leo_TiledObjectLayer *player_layer = leo_tiled_find_object_layer(state->map, "player");
            if (player_layer && player_layer->object_count > 0) {
                const leo_TiledObject *obj = &player_layer->objects[0];
                player_data->x = (float)obj->x;
                player_data->y = (float)obj->y;
                printf("🔄 Player reset to spawn position (%.0f, %.0f)\n", obj->x, obj->y);
            } else {
                // Fallback to default position
                player_data->x = 32;
                player_data->y = 32;
                printf("🔄 Player reset to default position\n");
            }
        }
    }
}

static void demo_render_ui(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    // Title
    leo_DrawText("Tiled Map Demo", 10, 10, 16, LEO_WHITE);
    
    // FPS counter
    leo_DrawFPS(10, 30);
    
    // Map info
    if (state->map) {
        char info[128];
        snprintf(info, sizeof(info), "Map: %dx%d (%dx%d tiles)", 
                 state->map->width * state->map->tilewidth,
                 state->map->height * state->map->tileheight,
                 state->map->width, state->map->height);
        leo_DrawText(info, 10, 50, 8, LEO_YELLOW);
        
        snprintf(info, sizeof(info), "Tilesets: %d, Layers: %d", 
                 state->map->tileset_count, state->map->layer_count);
        leo_DrawText(info, 10, 60, 8, LEO_YELLOW);
        
        // Show tileset texture status
        if (state->tileset_texture._handle) {
            snprintf(info, sizeof(info), "Tileset: %dx%d loaded", 
                     state->tileset_texture.width, state->tileset_texture.height);
            leo_DrawText(info, 10, 70, 8, LEO_GREEN);
        } else {
            leo_DrawText("Tileset: Not loaded", 10, 70, 8, LEO_RED);
        }
    } else {
        leo_DrawText("Map: Not loaded", 10, 50, 8, LEO_RED);
    }
    
    // Controls
    leo_DrawText("WASD/Arrows: Move", 10, 90, 8, LEO_GRAY);
    leo_DrawText("R: Reset", 10, 100, 8, LEO_GRAY);
    
    // Player status
    if (state->player_actor) {
        PlayerData *player_data = (PlayerData *)leo_actor_userdata(state->player_actor);
        if (player_data && !player_data->alive) {
            leo_DrawText("GAME OVER", 120, 90, 16, LEO_RED);
            leo_DrawText("Press R to restart", 100, 110, 8, LEO_YELLOW);
        }
    }
}

static void demo_shutdown(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    // Cleanup Tiled map
    if (state->map) {
        leo_tiled_free(state->map);
        state->map = NULL;
    }
    
    // Cleanup tileset texture
    if (state->tileset_texture._handle) {
        leo_UnloadTexture(&state->tileset_texture);
    }
    
    printf("✅ Tiled Demo shutdown complete\n");
}

bool TiledDemo(bool oneFrame) {
    TiledDemoState state = {0};
    state.one_frame = oneFrame;
    
    leo_GameConfig config = {
        .window_width = 960,  // 320 * 3 for nice scaling
        .window_height = 600, // 200 * 3 for nice scaling
        .window_title = "Leo Engine - Tiled Map Demo",
        .target_fps = 60,
        .logical_width = 320,
        .logical_height = 200,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_NEAREST, // Pixel-perfect for retro look
        .clear_color = {32, 32, 64, 255}, // Dark blue retro background
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
