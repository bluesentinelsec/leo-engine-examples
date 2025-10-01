#include <leo/leo.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_LOG_ENTRIES 10
#define MAX_LOG_LENGTH 64

typedef struct
{
    leo_SignalEmitter emitter;
    int x, y;
    leo_Color color;
    const char *name;
    bool active;
    float flash_timer;
} SignalNode;

typedef struct
{
    bool one_frame;
    SignalNode button_node;
    SignalNode timer_node;
    SignalNode counter_node;
    SignalNode logger_node;

    int counter_value;
    float timer_accumulator;
    bool timer_paused;
    bool connections_active;

    char log_entries[MAX_LOG_ENTRIES][MAX_LOG_LENGTH];
    int log_count;
} SignalDemoState;

// Signal callback functions
static void on_button_clicked(void *owner, void *user_data, va_list ap)
{
    SignalDemoState *state = (SignalDemoState *)user_data;
    const char *message = va_arg(ap, const char *);

    // Add to log
    snprintf(state->log_entries[state->log_count % MAX_LOG_ENTRIES], MAX_LOG_LENGTH, "Button: %s", message);
    state->log_count++;

    // Increment counter and emit if threshold reached
    state->counter_value++;
    state->counter_node.flash_timer = 0.3f; // Flash counter when incremented

    if (state->counter_value >= 5)
    {
        leo_signal_emit(&state->counter_node.emitter, "threshold", "Counter reached 5!");
        state->counter_value = 0;
    }
}

static void on_timer_timeout(void *owner, void *user_data, va_list ap)
{
    SignalDemoState *state = (SignalDemoState *)user_data;
    float elapsed = va_arg(ap, double); // va_arg promotes float to double

    snprintf(state->log_entries[state->log_count % MAX_LOG_ENTRIES], MAX_LOG_LENGTH, "Timer: %.1fs elapsed", elapsed);
    state->log_count++;

    // Flash logger when receiving timer signal
    state->logger_node.flash_timer = 0.3f;
}

static void on_counter_threshold(void *owner, void *user_data, va_list ap)
{
    SignalDemoState *state = (SignalDemoState *)user_data;
    const char *message = va_arg(ap, const char *);

    snprintf(state->log_entries[state->log_count % MAX_LOG_ENTRIES], MAX_LOG_LENGTH, "Counter: %s", message);
    state->log_count++;

    // Flash the logger node when counter threshold is reached
    state->logger_node.flash_timer = 0.5f;
}

static void on_logger_message(void *owner, void *user_data, va_list ap)
{
    SignalDemoState *state = (SignalDemoState *)user_data;

    // Flash the logger node
    state->logger_node.flash_timer = 0.4f;

    // The logger receives the same parameters as the original callbacks
    // For timer: float elapsed time
    // For counter: const char* message
    // We'll just flash without processing parameters to avoid conflicts
}

static bool demo_setup(leo_GameContext *ctx)
{
    SignalDemoState *state = (SignalDemoState *)ctx->user_data;

    // Initialize signal nodes
    state->button_node = (SignalNode){.x = 150, .y = 150, .color = LEO_BLUE, .name = "Button"};
    state->timer_node = (SignalNode){.x = 400, .y = 150, .color = LEO_GREEN, .name = "Timer"};
    state->counter_node = (SignalNode){.x = 150, .y = 300, .color = LEO_ORANGE, .name = "Counter"};
    state->logger_node = (SignalNode){.x = 400, .y = 300, .color = LEO_PURPLE, .name = "Logger"};

    // Initialize signal emitters
    leo_signal_emitter_init(&state->button_node.emitter, &state->button_node);
    leo_signal_emitter_init(&state->timer_node.emitter, &state->timer_node);
    leo_signal_emitter_init(&state->counter_node.emitter, &state->counter_node);
    leo_signal_emitter_init(&state->logger_node.emitter, &state->logger_node);

    // Define signals
    leo_signal_define(&state->button_node.emitter, "clicked");
    leo_signal_define(&state->timer_node.emitter, "timeout");
    leo_signal_define(&state->counter_node.emitter, "threshold");
    leo_signal_define(&state->logger_node.emitter, "message");

    // Initial state
    state->counter_value = 0;
    state->timer_accumulator = 0.0f;
    state->timer_paused = false;
    state->connections_active = true;
    state->log_count = 0;

    // Connect signals initially
    leo_signal_connect(&state->button_node.emitter, "clicked", on_button_clicked, state);
    leo_signal_connect(&state->timer_node.emitter, "timeout", on_timer_timeout, state);
    leo_signal_connect(&state->counter_node.emitter, "threshold", on_counter_threshold, state);

    // Connect Logger to receive signals (this was missing!)
    leo_signal_connect(&state->timer_node.emitter, "timeout", on_logger_message, state);
    leo_signal_connect(&state->counter_node.emitter, "threshold", on_logger_message, state);

    // Add initial log entry
    snprintf(state->log_entries[0], MAX_LOG_LENGTH, "Signal demo initialized");
    state->log_count = 1;

    return true;
}

static void demo_update(leo_GameContext *ctx)
{
    SignalDemoState *state = (SignalDemoState *)ctx->user_data;

    if (state->one_frame)
    {
        leo_GameQuit(ctx);
        return;
    }

    // Update flash timers
    if (state->button_node.flash_timer > 0)
        state->button_node.flash_timer -= ctx->dt;
    if (state->timer_node.flash_timer > 0)
        state->timer_node.flash_timer -= ctx->dt;
    if (state->counter_node.flash_timer > 0)
        state->counter_node.flash_timer -= ctx->dt;
    if (state->logger_node.flash_timer > 0)
        state->logger_node.flash_timer -= ctx->dt;

    // Manual button triggers
    if (leo_IsKeyPressed(KEY_1))
    {
        leo_signal_emit(&state->button_node.emitter, "clicked", "Manual trigger");
        state->button_node.flash_timer = 0.3f;
    }

    // Timer system
    if (leo_IsKeyPressed(KEY_SPACE))
    {
        state->timer_paused = !state->timer_paused;
    }

    if (!state->timer_paused)
    {
        state->timer_accumulator += ctx->dt;
        if (state->timer_accumulator >= 2.0f)
        {
            leo_signal_emit(&state->timer_node.emitter, "timeout", state->timer_accumulator);
            state->timer_node.flash_timer = 0.3f;
            state->timer_accumulator = 0.0f;
        }
    }

    // Toggle connections
    if (leo_IsKeyPressed(KEY_C))
    {
        if (state->connections_active)
        {
            leo_signal_disconnect_all(&state->button_node.emitter, "clicked");
            leo_signal_disconnect_all(&state->timer_node.emitter, "timeout");
            leo_signal_disconnect_all(&state->counter_node.emitter, "threshold");
            state->connections_active = false;

            snprintf(state->log_entries[state->log_count % MAX_LOG_ENTRIES], MAX_LOG_LENGTH,
                     "All connections disabled");
            state->log_count++;
        }
        else
        {
            leo_signal_connect(&state->button_node.emitter, "clicked", on_button_clicked, state);
            leo_signal_connect(&state->timer_node.emitter, "timeout", on_timer_timeout, state);
            leo_signal_connect(&state->counter_node.emitter, "threshold", on_counter_threshold, state);

            // Reconnect Logger connections
            leo_signal_connect(&state->timer_node.emitter, "timeout", on_logger_message, state);
            leo_signal_connect(&state->counter_node.emitter, "threshold", on_logger_message, state);

            state->connections_active = true;

            snprintf(state->log_entries[state->log_count % MAX_LOG_ENTRIES], MAX_LOG_LENGTH, "All connections enabled");
            state->log_count++;
        }
    }

    // Reset counter
    if (leo_IsKeyPressed(KEY_R))
    {
        state->counter_value = 0;
        state->log_count = 0;
        snprintf(state->log_entries[0], MAX_LOG_LENGTH, "Demo reset");
        state->log_count = 1;
    }
}

static void draw_signal_node(SignalNode *node)
{
    leo_Color draw_color = node->color;

    // Flash effect
    if (node->flash_timer > 0)
    {
        draw_color = LEO_WHITE;
    }

    // Draw node circle
    leo_DrawCircle(node->x, node->y, 30, draw_color);

    // Draw name
    leo_DrawText(node->name, node->x - 25, node->y + 40, 12, LEO_WHITE);
}

static void draw_connection_line(SignalNode *from, SignalNode *to, bool active)
{
    leo_Color line_color = active ? LEO_GREEN : LEO_GRAY;
    leo_DrawLine(from->x, from->y, to->x, to->y, line_color);
}

static void demo_render_ui(leo_GameContext *ctx)
{
    SignalDemoState *state = (SignalDemoState *)ctx->user_data;

    // Title
    leo_DrawText("Signal System Demo", 10, 10, 24, LEO_WHITE);

    // Draw connection lines
    if (state->connections_active)
    {
        draw_connection_line(&state->button_node, &state->counter_node, true);
        draw_connection_line(&state->timer_node, &state->logger_node, true);
        draw_connection_line(&state->counter_node, &state->logger_node, true);
    }

    // Draw signal nodes
    draw_signal_node(&state->button_node);
    draw_signal_node(&state->timer_node);
    draw_signal_node(&state->counter_node);
    draw_signal_node(&state->logger_node);

    // Status info
    char info[128];
    snprintf(info, sizeof(info), "Counter: %d/5", state->counter_value);
    leo_DrawText(info, 10, 50, 16, LEO_WHITE);

    snprintf(info, sizeof(info), "Timer: %.1fs %s", state->timer_accumulator, state->timer_paused ? "(PAUSED)" : "");
    leo_DrawText(info, 10, 70, 16, LEO_WHITE);

    snprintf(info, sizeof(info), "Connections: %s", state->connections_active ? "ACTIVE" : "DISABLED");
    leo_DrawText(info, 10, 90, 16, state->connections_active ? LEO_GREEN : LEO_RED);

    // Controls
    leo_DrawText("Controls:", 10, 130, 16, LEO_YELLOW);
    leo_DrawText("1 - Trigger Button", 10, 150, 14, LEO_GRAY);
    leo_DrawText("SPACE - Pause/Resume Timer", 10, 170, 14, LEO_GRAY);
    leo_DrawText("C - Toggle Connections", 10, 190, 14, LEO_GRAY);
    leo_DrawText("R - Reset", 10, 210, 14, LEO_GRAY);
    leo_DrawText("ESC - Exit", 10, 230, 14, LEO_GRAY);

    // Signal log
    leo_DrawText("Signal Log:", 500, 50, 16, LEO_YELLOW);
    int start_idx = (state->log_count > MAX_LOG_ENTRIES) ? state->log_count - MAX_LOG_ENTRIES : 0;

    for (int i = 0; i < MAX_LOG_ENTRIES && i < state->log_count; i++)
    {
        int log_idx = (start_idx + i) % MAX_LOG_ENTRIES;
        leo_DrawText(state->log_entries[log_idx], 500, 70 + i * 16, 12, LEO_WHITE);
    }
}

static void demo_shutdown(leo_GameContext *ctx)
{
    SignalDemoState *state = (SignalDemoState *)ctx->user_data;

    // Cleanup signal emitters
    leo_signal_emitter_free(&state->button_node.emitter);
    leo_signal_emitter_free(&state->timer_node.emitter);
    leo_signal_emitter_free(&state->counter_node.emitter);
    leo_signal_emitter_free(&state->logger_node.emitter);

    printf("✅ Signal Demo shutdown complete\n");
}

bool SignalDemo(bool oneFrame)
{
    SignalDemoState state = {0};
    state.one_frame = oneFrame;

    leo_GameConfig config = {
        .window_width = 900,
        .window_height = 500,
        .window_title = "Leo Engine - Signal System Demo",
        .target_fps = 60,
        .clear_color = {30, 30, 50, 255},
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
