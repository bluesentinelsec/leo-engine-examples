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
    leo_Actor *dirt_actor;
    leo_Actor *tree_actor;
    leo_Actor *player_actor;
    leo_Actor *enemy_actor;
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
    
    // For now, we'll do a simple collision check - in a full game you'd iterate through all tree actors
    TreeData *tree_data = NULL;
    leo_Actor *parent = leo_actor_parent(self);
    if (parent) {
        // Find tree actor by name (simplified approach)
        leo_Actor *tree_actor = leo_actor_find_child_by_name(parent, "tree");
        if (tree_actor) {
            tree_data = (TreeData *)leo_actor_userdata(tree_actor);
            if (tree_data && leo_CheckCollisionRecs(player_rect, tree_data->collision_box)) {
                // Collision with tree - don't move
                return;
            }
        }
    }
    
    // Check collision with enemy
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
    
    // Create actors
    leo_ActorDesc dirt_desc = {
        .name = "dirt",
        .vtable = &dirt_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    state->dirt_actor = leo_actor_spawn(ctx->root, &dirt_desc);
    
    leo_ActorDesc tree_desc = {
        .name = "tree",
        .vtable = &tree_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    state->tree_actor = leo_actor_spawn(ctx->root, &tree_desc);
    
    leo_ActorDesc player_desc = {
        .name = "player",
        .vtable = &player_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    state->player_actor = leo_actor_spawn(ctx->root, &player_desc);
    
    leo_ActorDesc enemy_desc = {
        .name = "enemy",
        .vtable = &enemy_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    state->enemy_actor = leo_actor_spawn(ctx->root, &enemy_desc);
    
    printf("✅ Tiled Demo initialized with 4 actors\n");
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
        if (player_data) {
            player_data->alive = true;
            player_data->x = 32;
            player_data->y = 32;
            printf("🔄 Game reset\n");
        }
    }
}

static void demo_render_ui(leo_GameContext *ctx) {
    TiledDemoState *state = (TiledDemoState *)ctx->user_data;
    
    // Title
    leo_DrawText("Tiled Map Demo", 10, 10, 16, LEO_WHITE);
    
    // FPS counter
    leo_DrawFPS(10, 30);
    
    // Controls
    leo_DrawText("WASD/Arrows: Move", 10, 50, 8, LEO_GRAY);
    leo_DrawText("R: Reset", 10, 60, 8, LEO_GRAY);
    
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
