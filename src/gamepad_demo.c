#include <leo/leo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct
{
    bool one_frame;
} GamepadDemoState;

static bool demo_setup(leo_GameContext *ctx)
{
    return true;
}

static void demo_update(leo_GameContext *ctx)
{
    GamepadDemoState *state = (GamepadDemoState *)ctx->user_data;

    if (state->one_frame)
    {
        leo_GameQuit(ctx);
        return;
    }
}

static void draw_button(int x, int y, int size, const char *label, bool pressed)
{
    leo_Color color = pressed ? LEO_GREEN : LEO_GRAY;
    leo_DrawCircle(x, y, size, color);
    leo_DrawText(label, x - 8, y - 6, 12, LEO_BLACK);
}

static void draw_dpad_button(int x, int y, int w, int h, const char *label, bool pressed)
{
    leo_Color color = pressed ? LEO_GREEN : LEO_GRAY;
    leo_DrawRectangle(x, y, w, h, color);
    leo_DrawText(label, x + 2, y + 2, 10, LEO_BLACK);
}

static void draw_trigger_bar(int x, int y, int width, const char *label, float value)
{
    leo_DrawRectangle(x, y, width, 15, LEO_GRAY);
    leo_DrawRectangle(x, y, (int)(value * width), 15, LEO_ORANGE);
    leo_DrawText(label, x, y - 15, 12, LEO_WHITE);
}

static void draw_stick(int x, int y, int radius, const char *label, leo_Vector2 stick, bool pressed)
{
    leo_Color base_color = pressed ? LEO_YELLOW : LEO_GRAY;
    leo_DrawCircle(x, y, radius, base_color);

    int stick_x = x + (int)(stick.x * (radius - 5));
    int stick_y = y + (int)(stick.y * (radius - 5));
    leo_DrawCircle(stick_x, stick_y, 5, LEO_WHITE);

    leo_DrawText(label, x - 15, y + radius + 5, 12, LEO_WHITE);
}

static void demo_render_ui(leo_GameContext *ctx)
{
    leo_ClearBackground(30, 30, 40, 255);

    // Title
    leo_DrawText("Xbox Controller Demo", 10, 10, 24, LEO_WHITE);

    if (!leo_IsGamepadAvailable(0))
    {
        leo_DrawText("No gamepad connected", 10, 50, 18, LEO_RED);
        return;
    }

    const char *name = leo_GetGamepadName(0);
    char info[128];
    snprintf(info, sizeof(info), "Controller: %s", name ? name : "Unknown");
    leo_DrawText(info, 10, 40, 16, LEO_YELLOW);

    // Face buttons (A, B, X, Y)
    draw_button(600, 200, 20, "A", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_FACE_DOWN));
    draw_button(650, 150, 20, "B", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_FACE_RIGHT));
    draw_button(550, 150, 20, "X", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_FACE_LEFT));
    draw_button(600, 100, 20, "Y", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_FACE_UP));

    // D-pad
    draw_dpad_button(150, 120, 20, 15, "U", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_DPAD_UP));
    draw_dpad_button(150, 165, 20, 15, "D", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_DPAD_DOWN));
    draw_dpad_button(130, 140, 15, 20, "L", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_DPAD_LEFT));
    draw_dpad_button(175, 140, 15, 20, "R", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_DPAD_RIGHT));

    // Bumpers
    draw_button(200, 80, 15, "LB", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_LEFT_BUMPER));
    draw_button(500, 80, 15, "RB", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_RIGHT_BUMPER));

    // Triggers
    float lt = leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_LEFT_TRIGGER);
    float rt = leo_GetGamepadAxisMovement(0, LEO_GAMEPAD_AXIS_RIGHT_TRIGGER);
    draw_trigger_bar(200, 50, 80, "LT", lt);
    draw_trigger_bar(500, 50, 80, "RT", rt);

    // Analog sticks
    leo_Vector2 left_stick = leo_GetGamepadStick(0, LEO_GAMEPAD_STICK_LEFT);
    leo_Vector2 right_stick = leo_GetGamepadStick(0, LEO_GAMEPAD_STICK_RIGHT);
    bool left_pressed = leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_LEFT_STICK);
    bool right_pressed = leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_RIGHT_STICK);

    draw_stick(250, 250, 30, "Left", left_stick, left_pressed);
    draw_stick(550, 250, 30, "Right", right_stick, right_pressed);

    // Center buttons
    draw_button(350, 120, 12, "Back", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_BACK));
    draw_button(400, 120, 12, "Start", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_START));
    draw_button(375, 80, 10, "Guide", leo_IsGamepadButtonDown(0, LEO_GAMEPAD_BUTTON_GUIDE));

    // Stick values display
    snprintf(info, sizeof(info), "Left Stick: (%.2f, %.2f)", left_stick.x, left_stick.y);
    leo_DrawText(info, 10, 350, 14, LEO_WHITE);

    snprintf(info, sizeof(info), "Right Stick: (%.2f, %.2f)", right_stick.x, right_stick.y);
    leo_DrawText(info, 10, 370, 14, LEO_WHITE);

    snprintf(info, sizeof(info), "Triggers: LT=%.2f RT=%.2f", lt, rt);
    leo_DrawText(info, 10, 390, 14, LEO_WHITE);

    // Last button pressed
    int last_button = leo_GetGamepadButtonPressed();
    if (last_button >= 0)
    {
        snprintf(info, sizeof(info), "Last Button: %d", last_button);
        leo_DrawText(info, 10, 410, 14, LEO_GREEN);
    }

    leo_DrawText("Press ESC to exit", 10, 450, 16, LEO_GRAY);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    // Cleanup if needed
}

bool GamepadDemo(bool oneFrame)
{
    GamepadDemoState state = {0};
    state.one_frame = oneFrame;

    leo_GameConfig config = {0};
    config.window_width = 800;
    config.window_height = 500;
    config.window_title = "Leo Engine - Xbox Controller Demo";
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
