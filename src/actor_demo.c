#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_LOG_ENTRIES 8

typedef struct {
    float x, y;
    float vx, vy;
    leo_Color color;
    float lifetime;
    float max_lifetime;
} ParticleData;

typedef struct {
    float x, y;
    float speed;
    int direction; // 0=right, 1=down, 2=left, 3=up
} EnemyData;

typedef struct {
    float x, y;
} PlayerData;

typedef struct {
    bool one_frame;
    leo_Actor *player_actor;
    int enemy_group;
    int particle_group;
    int enemy_count;
    int particle_count;
    char log_entries[MAX_LOG_ENTRIES][64];
    int log_count;
} ActorDemoState;

// Actor VTables
static bool player_init(leo_Actor *self);
static void player_update(leo_Actor *self, float dt);
static void player_render(leo_Actor *self);

static bool enemy_init(leo_Actor *self);
static void enemy_update(leo_Actor *self, float dt);
static void enemy_render(leo_Actor *self);
static void enemy_exit(leo_Actor *self);

static bool particle_init(leo_Actor *self);
static void particle_update(leo_Actor *self, float dt);
static void particle_render(leo_Actor *self);
static void particle_exit(leo_Actor *self);

static const leo_ActorVTable player_vtable = {
    .on_init = player_init,
    .on_update = player_update,
    .on_render = player_render,
    .on_exit = NULL
};

static const leo_ActorVTable enemy_vtable = {
    .on_init = enemy_init,
    .on_update = enemy_update,
    .on_render = enemy_render,
    .on_exit = enemy_exit
};

static const leo_ActorVTable particle_vtable = {
    .on_init = particle_init,
    .on_update = particle_update,
    .on_render = particle_render,
    .on_exit = particle_exit
};

// Helper to add log entry
static void add_log(ActorDemoState *state, const char *message) {
    snprintf(state->log_entries[state->log_count % MAX_LOG_ENTRIES], 64, "%s", message);
    state->log_count++;
}

// Player Actor Implementation
static bool player_init(leo_Actor *self) {
    PlayerData *data = malloc(sizeof(PlayerData));
    data->x = 400;
    data->y = 300;
    leo_actor_set_userdata(self, data);
    
    // Define and emit spawned signal
    leo_signal_define(leo_actor_emitter(self), "spawned");
    leo_signal_emit(leo_actor_emitter(self), "spawned");
    
    return true;
}

static void player_update(leo_Actor *self, float dt) {
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    
    // Movement
    if (leo_IsKeyDown(KEY_W)) data->y -= 200 * dt;
    if (leo_IsKeyDown(KEY_S)) data->y += 200 * dt;
    if (leo_IsKeyDown(KEY_A)) data->x -= 200 * dt;
    if (leo_IsKeyDown(KEY_D)) data->x += 200 * dt;
    
    // Keep in bounds
    if (data->x < 20) data->x = 20;
    if (data->x > 580) data->x = 580;
    if (data->y < 20) data->y = 20;
    if (data->y > 380) data->y = 380;
}

static void player_render(leo_Actor *self) {
    PlayerData *data = (PlayerData *)leo_actor_userdata(self);
    
    // Draw player as blue rectangle
    leo_DrawRectangle((int)data->x - 10, (int)data->y - 10, 20, 20, LEO_BLUE);
    
    // Draw direction indicator
    leo_DrawCircle((int)data->x, (int)data->y, 3, LEO_WHITE);
}

// Enemy Actor Implementation
static bool enemy_init(leo_Actor *self) {
    EnemyData *data = malloc(sizeof(EnemyData));
    data->x = 100 + (rand() % 400);
    data->y = 100 + (rand() % 200);
    data->speed = 50 + (rand() % 100);
    data->direction = rand() % 4;
    leo_actor_set_userdata(self, data);
    
    return true;
}

static void enemy_update(leo_Actor *self, float dt) {
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    
    // Move based on direction
    switch (data->direction) {
        case 0: data->x += data->speed * dt; break; // right
        case 1: data->y += data->speed * dt; break; // down
        case 2: data->x -= data->speed * dt; break; // left
        case 3: data->y -= data->speed * dt; break; // up
    }
    
    // Bounce off walls
    if (data->x <= 20 || data->x >= 580) {
        data->direction = (data->direction == 0) ? 2 : (data->direction == 2) ? 0 : data->direction;
    }
    if (data->y <= 20 || data->y >= 380) {
        data->direction = (data->direction == 1) ? 3 : (data->direction == 3) ? 1 : data->direction;
    }
    
    // Keep in bounds
    if (data->x < 20) data->x = 20;
    if (data->x > 580) data->x = 580;
    if (data->y < 20) data->y = 20;
    if (data->y > 380) data->y = 380;
}

static void enemy_render(leo_Actor *self) {
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    
    leo_Color color = leo_actor_is_effectively_paused(self) ? LEO_GRAY : LEO_RED;
    leo_DrawRectangle((int)data->x - 8, (int)data->y - 8, 16, 16, color);
}

static void enemy_exit(leo_Actor *self) {
    EnemyData *data = (EnemyData *)leo_actor_userdata(self);
    free(data);
}

// Particle Actor Implementation
static bool particle_init(leo_Actor *self) {
    ParticleData *data = malloc(sizeof(ParticleData));
    data->x = 400 + (rand() % 100 - 50);
    data->y = 300 + (rand() % 100 - 50);
    data->vx = (rand() % 200 - 100);
    data->vy = (rand() % 200 - 100);
    data->max_lifetime = 2.0f + (rand() % 100) / 100.0f;
    data->lifetime = data->max_lifetime;
    data->color = (leo_Color){255, 255, 0, 255}; // Yellow
    leo_actor_set_userdata(self, data);
    
    return true;
}

static void particle_update(leo_Actor *self, float dt) {
    ParticleData *data = (ParticleData *)leo_actor_userdata(self);
    
    data->x += data->vx * dt;
    data->y += data->vy * dt;
    data->lifetime -= dt;
    
    // Fade out over time
    float alpha = data->lifetime / data->max_lifetime;
    data->color.a = (unsigned char)(255 * alpha);
    
    // Kill when lifetime expires
    if (data->lifetime <= 0) {
        leo_actor_kill(self);
    }
}

static void particle_render(leo_Actor *self) {
    ParticleData *data = (ParticleData *)leo_actor_userdata(self);
    leo_DrawCircle((int)data->x, (int)data->y, 3, data->color);
}

static void particle_exit(leo_Actor *self) {
    ParticleData *data = (ParticleData *)leo_actor_userdata(self);
    free(data);
}

// Demo Implementation
static bool demo_setup(leo_GameContext *ctx) {
    ActorDemoState *state = (ActorDemoState *)ctx->user_data;
    
    // Get groups
    state->enemy_group = leo_actor_group_get_or_create(ctx->actors, "enemies");
    state->particle_group = leo_actor_group_get_or_create(ctx->actors, "particles");
    
    // Spawn player
    leo_ActorDesc player_desc = {
        .name = "player",
        .vtable = &player_vtable,
        .user_data = NULL,
        .groups = 0,
        .start_paused = false
    };
    state->player_actor = leo_actor_spawn(ctx->root, &player_desc);
    
    // Spawn initial enemies
    for (int i = 0; i < 3; i++) {
        leo_ActorDesc enemy_desc = {
            .name = "enemy",
            .vtable = &enemy_vtable,
            .user_data = NULL,
            .groups = (1ULL << state->enemy_group),
            .start_paused = false
        };
        leo_actor_spawn(ctx->root, &enemy_desc);
        state->enemy_count++;
    }
    
    state->log_count = 0;
    add_log(state, "Actor demo initialized");
    
    return true;
}

static void demo_update(leo_GameContext *ctx) {
    ActorDemoState *state = (ActorDemoState *)ctx->user_data;
    
    if (state->one_frame) {
        leo_GameQuit(ctx);
        return;
    }
    
    // Spawn particles
    if (leo_IsKeyPressed(KEY_SPACE)) {
        for (int i = 0; i < 5; i++) {
            leo_ActorDesc particle_desc = {
                .name = "particle",
                .vtable = &particle_vtable,
                .user_data = NULL,
                .groups = (1ULL << state->particle_group),
                .start_paused = false
            };
            leo_actor_spawn(ctx->root, &particle_desc);
            state->particle_count++;
        }
        add_log(state, "Spawned particles");
    }
    
    // Toggle enemy pause
    if (leo_IsKeyPressed(KEY_P)) {
        static bool enemies_paused = false;
        enemies_paused = !enemies_paused;
        
        // This would require a helper function to pause all in group
        // For now, we'll use the global pause as demonstration
        add_log(state, enemies_paused ? "Enemies paused" : "Enemies unpaused");
    }
    
    // Global pause toggle
    if (leo_IsKeyPressed(KEY_G)) {
        bool paused = leo_actor_system_is_paused(ctx->actors);
        leo_actor_system_set_paused(ctx->actors, !paused);
        add_log(state, paused ? "Global unpause" : "Global pause");
    }
    
    // Spawn new enemy
    if (leo_IsKeyPressed(KEY_E)) {
        leo_ActorDesc enemy_desc = {
            .name = "enemy",
            .vtable = &enemy_vtable,
            .user_data = NULL,
            .groups = (1ULL << state->enemy_group),
            .start_paused = false
        };
        leo_actor_spawn(ctx->root, &enemy_desc);
        state->enemy_count++;
        add_log(state, "Spawned enemy");
    }
}

// Helper to count actors in group
static void count_actor(leo_Actor *a, void *user) {
    int *count = (int *)user;
    (*count)++;
}

static void demo_render_ui(leo_GameContext *ctx) {
    ActorDemoState *state = (ActorDemoState *)ctx->user_data;
    
    // Title
    leo_DrawText("Actor System Demo", 10, 10, 24, LEO_WHITE);
    
    // Count actors in groups
    int enemy_count = 0, particle_count = 0;
    leo_actor_for_each_in_group(ctx->actors, state->enemy_group, count_actor, &enemy_count);
    leo_actor_for_each_in_group(ctx->actors, state->particle_group, count_actor, &particle_count);
    
    // Status info
    char info[128];
    snprintf(info, sizeof(info), "Enemies: %d", enemy_count);
    leo_DrawText(info, 600, 50, 16, LEO_RED);
    
    snprintf(info, sizeof(info), "Particles: %d", particle_count);
    leo_DrawText(info, 600, 70, 16, LEO_YELLOW);
    
    snprintf(info, sizeof(info), "Global Pause: %s", 
             leo_actor_system_is_paused(ctx->actors) ? "ON" : "OFF");
    leo_DrawText(info, 600, 90, 16, LEO_WHITE);
    
    // Controls
    leo_DrawText("Controls:", 600, 130, 16, LEO_YELLOW);
    leo_DrawText("WASD - Move Player", 600, 150, 12, LEO_GRAY);
    leo_DrawText("SPACE - Spawn Particles", 600, 170, 12, LEO_GRAY);
    leo_DrawText("E - Spawn Enemy", 600, 190, 12, LEO_GRAY);
    leo_DrawText("G - Global Pause", 600, 210, 12, LEO_GRAY);
    leo_DrawText("ESC - Exit", 600, 230, 12, LEO_GRAY);
    
    // Legend
    leo_DrawText("Legend:", 600, 270, 16, LEO_YELLOW);
    leo_DrawRectangle(600, 290, 15, 15, LEO_BLUE);
    leo_DrawText("Player", 620, 292, 12, LEO_WHITE);
    leo_DrawRectangle(600, 310, 15, 15, LEO_RED);
    leo_DrawText("Enemy", 620, 312, 12, LEO_WHITE);
    leo_DrawCircle(607, 337, 5, LEO_YELLOW);
    leo_DrawText("Particle", 620, 332, 12, LEO_WHITE);
    
    // Activity log
    leo_DrawText("Activity Log:", 600, 370, 14, LEO_YELLOW);
    for (int i = 0; i < MAX_LOG_ENTRIES && i < state->log_count; i++) {
        int log_idx = (state->log_count - 1 - i) % MAX_LOG_ENTRIES;
        leo_DrawText(state->log_entries[log_idx], 600, 390 + i * 14, 10, LEO_WHITE);
    }
}

static void demo_shutdown(leo_GameContext *ctx) {
    printf("✅ Actor Demo shutdown complete\n");
}

bool ActorDemo(bool oneFrame) {
    ActorDemoState state = {0};
    state.one_frame = oneFrame;
    
    leo_GameConfig config = {
        .window_width = 900,
        .window_height = 600,
        .window_title = "Leo Engine - Actor System Demo",
        .target_fps = 60,
        .logical_width = 900,
        .logical_height = 600,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = {20, 20, 30, 255},
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
