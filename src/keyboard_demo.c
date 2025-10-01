#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_KEYS_TO_SHOW 16
#define MAX_CHAR_BUFFER 64

typedef struct
{
    int key;
    const char *name;
    int x, y;
} KeyDisplay;

typedef struct KeyboardDemoState
{
    bool one_frame;
    char char_buffer[MAX_CHAR_BUFFER];
    int char_count;
    int last_key_pressed;
    KeyDisplay keys[MAX_KEYS_TO_SHOW];
} KeyboardDemoState;

static bool demo_setup(leo_GameContext *ctx)
{
    KeyboardDemoState *state = (KeyboardDemoState *)ctx->user_data;

    // Initialize character buffer
    memset(state->char_buffer, 0, sizeof(state->char_buffer));
    state->char_count = 0;
    state->last_key_pressed = 0;

    // Setup key display grid
    KeyDisplay keys[] = {
        {KEY_W, "W", 100, 150},          {KEY_A, "A", 50, 200},          {KEY_S, "S", 100, 200},
        {KEY_D, "D", 150, 200},          {KEY_UP, "↑", 300, 150},        {KEY_LEFT, "←", 250, 200},
        {KEY_DOWN, "↓", 300, 200},       {KEY_RIGHT, "→", 350, 200},     {KEY_SPACE, "SPACE", 200, 250},
        {KEY_RETURN, "ENTER", 300, 250}, {KEY_LSHIFT, "SHIFT", 50, 300}, {KEY_LCTRL, "CTRL", 150, 300},
        {KEY_Q, "Q", 50, 150},           {KEY_E, "E", 150, 150},         {KEY_TAB, "TAB", 400, 150},
        {KEY_ESCAPE, "ESC", 400, 100}};

    memcpy(state->keys, keys, sizeof(keys));

    return true;
}

static void demo_update(leo_GameContext *ctx)
{
    KeyboardDemoState *state = (KeyboardDemoState *)ctx->user_data;

    if (state->one_frame)
    {
        leo_GameQuit(ctx);
        return;
    }

    // Handle character input
    int ch = leo_GetCharPressed();
    if (ch > 0 && state->char_count < MAX_CHAR_BUFFER - 1)
    {
        state->char_buffer[state->char_count++] = (char)ch;
        state->char_buffer[state->char_count] = '\0';
    }

    // Handle backspace for character buffer
    if (leo_IsKeyPressed(KEY_BACKSPACE) && state->char_count > 0)
    {
        state->char_buffer[--state->char_count] = '\0';
    }

    // Clear buffer with C key
    if (leo_IsKeyPressed(KEY_C))
    {
        memset(state->char_buffer, 0, sizeof(state->char_buffer));
        state->char_count = 0;
    }

    // Get any key pressed for demonstration
    state->last_key_pressed = leo_GetKeyPressed();
}

static void demo_render_ui(leo_GameContext *ctx)
{
    KeyboardDemoState *state = (KeyboardDemoState *)ctx->user_data;

    // Clear background
    leo_ClearBackground(30, 30, 40, 255);

    // Title and instructions
    leo_DrawText("Keyboard API Demo", 10, 10, 24, LEO_WHITE);
    leo_DrawText("Press keys to see their states. Type to test character input.", 10, 40, 16, LEO_GRAY);
    leo_DrawText("C = Clear text buffer, Backspace = Delete char, ESC = Exit", 10, 60, 16, LEO_GRAY);

    // Legend
    leo_DrawText("Legend:", 500, 100, 18, LEO_WHITE);
    leo_DrawRectangle(500, 125, 20, 20, LEO_GREEN);
    leo_DrawText("Pressed", 530, 130, 14, LEO_WHITE);
    leo_DrawRectangle(500, 150, 20, 20, LEO_BLUE);
    leo_DrawText("Held", 530, 155, 14, LEO_WHITE);
    leo_DrawRectangle(500, 175, 20, 20, LEO_RED);
    leo_DrawText("Released", 530, 180, 14, LEO_WHITE);
    leo_DrawRectangle(500, 200, 20, 20, LEO_GRAY);
    leo_DrawText("Up", 530, 205, 14, LEO_WHITE);

    // Draw key states
    for (int i = 0; i < MAX_KEYS_TO_SHOW; i++)
    {
        KeyDisplay *key = &state->keys[i];
        leo_Color color = LEO_GRAY;

        if (leo_IsKeyPressed(key->key))
        {
            color = LEO_GREEN; // Just pressed
        }
        else if (leo_IsKeyDown(key->key))
        {
            color = LEO_BLUE; // Held down
        }
        else if (leo_IsKeyReleased(key->key))
        {
            color = LEO_RED; // Just released
        }

        // Draw key indicator
        leo_DrawRectangle(key->x, key->y, 40, 30, color);
        leo_DrawText(key->name, key->x + 2, key->y + 8, 12, LEO_BLACK);
    }

    // Character input display
    leo_DrawText("Character Input:", 10, 350, 18, LEO_WHITE);
    leo_Color dark_gray = {64, 64, 64, 255};
    leo_DrawRectangle(10, 375, 500, 30, dark_gray);
    leo_DrawText(state->char_buffer, 15, 382, 16, LEO_WHITE);

    // Key pressed display
    char key_info[128];
    if (state->last_key_pressed > 0)
    {
        snprintf(key_info, sizeof(key_info), "Last Key Pressed: %d", state->last_key_pressed);
    }
    else
    {
        strcpy(key_info, "Last Key Pressed: None");
    }
    leo_DrawText(key_info, 10, 420, 16, LEO_YELLOW);

    // Real-time key state info
    leo_DrawText("Real-time Key States:", 10, 450, 16, LEO_WHITE);

    // Show some specific key states
    char state_info[256];
    snprintf(state_info, sizeof(state_info),
             "WASD: W=%d A=%d S=%d D=%d | Arrows: ↑=%d ←=%d ↓=%d →=%d | Space=%d Enter=%d", leo_IsKeyDown(KEY_W),
             leo_IsKeyDown(KEY_A), leo_IsKeyDown(KEY_S), leo_IsKeyDown(KEY_D), leo_IsKeyDown(KEY_UP),
             leo_IsKeyDown(KEY_LEFT), leo_IsKeyDown(KEY_DOWN), leo_IsKeyDown(KEY_RIGHT), leo_IsKeyDown(KEY_SPACE),
             leo_IsKeyDown(KEY_RETURN));
    leo_DrawText(state_info, 10, 470, 12, LEO_GRAY);

    // Show pressed/released states for demonstration
    if (leo_IsKeyPressed(KEY_SPACE))
    {
        leo_DrawText("SPACE PRESSED!", 10, 500, 20, LEO_GREEN);
    }
    if (leo_IsKeyReleased(KEY_SPACE))
    {
        leo_DrawText("SPACE RELEASED!", 10, 500, 20, LEO_RED);
    }
}

static void demo_shutdown(leo_GameContext *ctx)
{
    // Cleanup if needed
}

bool KeyboardDemo(bool oneFrame)
{
    KeyboardDemoState state = {0};
    state.one_frame = oneFrame;

    leo_GameConfig config = {0};
    config.window_width = 800;
    config.window_height = 600;
    config.window_title = "Leo Engine - Keyboard Demo";
    config.target_fps = 60;
    config.clear_color = LEO_BLACK;
    config.user_data = &state;

    leo_GameCallbacks callbacks = {0};
    callbacks.on_setup = demo_setup;
    callbacks.on_update = demo_update;
    callbacks.on_render_ui = demo_render_ui;
    callbacks.on_shutdown = demo_shutdown;

    return leo_GameRun(&config, &callbacks) == 0;
}
