#include <leo/leo.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct
{
    leo_Vector2 position;
    leo_Vector2 velocity;
    float radius;
    leo_Color color;
} Ball;

typedef struct
{
    leo_Rectangle rect;
    leo_Vector2 velocity;
    leo_Color color;
} MovingRect;

typedef struct CollisionDemoState
{
    bool one_frame;
    bool fullscreen;

    // Bouncing balls
    Ball balls[3];

    // Moving rectangles
    MovingRect rects[2];

    // Static walls
    leo_Rectangle walls[4];

    // Screen dimensions
    int screenWidth;
    int screenHeight;
} CollisionDemoState;

static bool demo_setup(leo_GameContext *ctx)
{
    CollisionDemoState *state = (CollisionDemoState *)ctx->user_data;

    state->screenWidth = 800;
    state->screenHeight = 600;
    state->fullscreen = false;
    state->fullscreen = false;

    // Initialize balls
    state->balls[0] = (Ball){{100, 100}, {150, 120}, 25, LEO_RED};
    state->balls[1] = (Ball){{200, 200}, {-100, 80}, 30, LEO_BLUE};
    state->balls[2] = (Ball){{300, 150}, {80, -140}, 20, LEO_GREEN};

    // Initialize moving rectangles
    state->rects[0].rect = (leo_Rectangle){400, 100, 60, 40};
    state->rects[0].velocity = (leo_Vector2){50, 70};
    state->rects[0].color = LEO_PURPLE;

    state->rects[1].rect = (leo_Rectangle){500, 300, 80, 50};
    state->rects[1].velocity = (leo_Vector2){-60, -40};
    state->rects[1].color = LEO_ORANGE;

    // Initialize walls
    state->walls[0] = (leo_Rectangle){0, 0, state->screenWidth, 10};                        // top
    state->walls[1] = (leo_Rectangle){0, state->screenHeight - 10, state->screenWidth, 10}; // bottom
    state->walls[2] = (leo_Rectangle){0, 0, 10, state->screenHeight};                       // left
    state->walls[3] = (leo_Rectangle){state->screenWidth - 10, 0, 10, state->screenHeight}; // right

    return true;
}

static void demo_update(leo_GameContext *ctx)
{
    CollisionDemoState *state = (CollisionDemoState *)ctx->user_data;
    float deltaTime = leo_GetFrameTime();

    // Toggle fullscreen
    if (leo_IsKeyReleased(KEY_TAB))
    {
        state->fullscreen = !state->fullscreen;
        leo_SetFullscreen(state->fullscreen);
    }

    // Update balls
    for (int i = 0; i < 3; i++)
    {
        state->balls[i].position.x += state->balls[i].velocity.x * deltaTime;
        state->balls[i].position.y += state->balls[i].velocity.y * deltaTime;

        // Wall collisions
        for (int w = 0; w < 4; w++)
        {
            if (leo_CheckCollisionCircleRec(state->balls[i].position, state->balls[i].radius, state->walls[w]))
            {
                if (w < 2)
                    state->balls[i].velocity.y *= -1; // top/bottom
                else
                    state->balls[i].velocity.x *= -1; // left/right
            }
        }

        // Ball-ball collisions
        for (int j = i + 1; j < 3; j++)
        {
            if (leo_CheckCollisionCircles(state->balls[i].position, state->balls[i].radius, state->balls[j].position,
                                          state->balls[j].radius))
            {
                // Simple collision response - swap velocities
                leo_Vector2 temp = state->balls[i].velocity;
                state->balls[i].velocity = state->balls[j].velocity;
                state->balls[j].velocity = temp;
            }
        }
    }

    // Update rectangles
    for (int i = 0; i < 2; i++)
    {
        state->rects[i].rect.x += state->rects[i].velocity.x * deltaTime;
        state->rects[i].rect.y += state->rects[i].velocity.y * deltaTime;

        // Wall collisions
        for (int w = 0; w < 4; w++)
        {
            if (leo_CheckCollisionRecs(state->rects[i].rect, state->walls[w]))
            {
                if (w < 2)
                    state->rects[i].velocity.y *= -1; // top/bottom
                else
                    state->rects[i].velocity.x *= -1; // left/right
            }
        }

        // Rect-rect collisions
        for (int j = i + 1; j < 2; j++)
        {
            if (leo_CheckCollisionRecs(state->rects[i].rect, state->rects[j].rect))
            {
                // Swap velocities
                leo_Vector2 temp = state->rects[i].velocity;
                state->rects[i].velocity = state->rects[j].velocity;
                state->rects[j].velocity = temp;
            }
        }
    }

    if (state->one_frame)
    {
        ctx->request_quit = true; // Exit after one frame for testing
    }
}

static void demo_render_ui(leo_GameContext *ctx)
{
    CollisionDemoState *state = (CollisionDemoState *)ctx->user_data;
    leo_Vector2 mousePos = leo_GetMousePosition();

    // Draw walls
    for (int i = 0; i < 4; i++)
    {
        leo_DrawRectangle((int)state->walls[i].x, (int)state->walls[i].y, (int)state->walls[i].width,
                          (int)state->walls[i].height, LEO_GRAY);
    }

    // Draw balls
    for (int i = 0; i < 3; i++)
    {
        leo_DrawCircle((int)state->balls[i].position.x, (int)state->balls[i].position.y, state->balls[i].radius,
                       state->balls[i].color);
    }

    // Draw rectangles
    for (int i = 0; i < 2; i++)
    {
        leo_DrawRectangle((int)state->rects[i].rect.x, (int)state->rects[i].rect.y, (int)state->rects[i].rect.width,
                          (int)state->rects[i].rect.height, state->rects[i].color);
    }

    // Mouse interaction - highlight shapes under cursor
    for (int i = 0; i < 3; i++)
    {
        if (leo_CheckCollisionPointCircle(mousePos, state->balls[i].position, state->balls[i].radius))
        {
            // Draw outline around highlighted ball
            leo_DrawCircle((int)state->balls[i].position.x, (int)state->balls[i].position.y, state->balls[i].radius + 5,
                           LEO_WHITE);
            leo_DrawCircle((int)state->balls[i].position.x, (int)state->balls[i].position.y, state->balls[i].radius + 3,
                           LEO_BLACK);
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (leo_CheckCollisionPointRec(mousePos, state->rects[i].rect))
        {
            // Draw outline around highlighted rectangle
            leo_Rectangle outline = {state->rects[i].rect.x - 3, state->rects[i].rect.y - 3,
                                     state->rects[i].rect.width + 6, state->rects[i].rect.height + 6};
            leo_DrawRectangle((int)outline.x, (int)outline.y, (int)outline.width, (int)outline.height, LEO_WHITE);
        }
    }

    // Draw instructions
    leo_DrawText("2D Collision Demo", 10, 10, 20, LEO_WHITE);
    leo_DrawText("Balls and rectangles bounce off walls and each other", 10, 35, 16, LEO_WHITE);
    leo_DrawText("Move mouse over shapes to highlight them", 10, 55, 16, LEO_WHITE);
    leo_DrawText("Tab: Toggle Fullscreen", 10, 75, 16, LEO_WHITE);
}

static void demo_shutdown(leo_GameContext *ctx)
{
    // No cleanup needed for this demo
    (void)ctx;
}

bool CollisionDemo(bool oneFrame)
{
    CollisionDemoState state = {
        .one_frame = oneFrame,
    };

    leo_GameConfig cfg = {
        .window_width = 800,
        .window_height = 600,
        .window_title = "Leo Engine - 2D Collision Demo",
        .target_fps = 60,
        .logical_width = 800,
        .logical_height = 600,
        .presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX,
        .scale_mode = LEO_SCALE_LINEAR,
        .clear_color = (leo_Color){30, 30, 30, 255},
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
