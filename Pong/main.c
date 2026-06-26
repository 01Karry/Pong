//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include "raylib.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

enum COLLISIONS {
    NONE = 0,
    TOP = 1,
    BOTTOM = 1 << 1,
    LEFT_WALL = 1 << 2,
    RIGHT_WALL = 1 << 3,
    LEFT_RACKET = 1 << 4,
    RIGHT_RACKET = 1 << 5
};

enum GAMEOVER {
    PLAYER_WON = 1,
    BOT_WON
};

#define TARGET_FPS 60

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 700

#define HEADHER_HEIGHT 150

#define OFFSET_FOR_FIELD 10
#define LINE_THICK 5

#define FIELD_HEIGHT 430
#define FIELD_WIDTH SCREEN_WIDTH - 2 * OFFSET_FOR_FIELD

#define RACKET_VELOCITY 500

Vector2 FieldOrigin = { OFFSET_FOR_FIELD, HEADHER_HEIGHT + OFFSET_FOR_FIELD };

typedef struct Ball_t {
    Rectangle hitBox;
    int radius;
    Vector2 velocity;
    int racketCollsionCount;
} Ball_t;

#define RACKET_WIDTH 20

typedef struct Racket_t {
    Rectangle hitBox;
    Vector2 velocity;
} Racket_t;

Rectangle Field = { OFFSET_FOR_FIELD, HEADHER_HEIGHT + OFFSET_FOR_FIELD, FIELD_WIDTH, FIELD_HEIGHT };
Ball_t Ball;
Racket_t LeftRacket, RightRacket;

int LastCountToUpdateBallSpeed = 0;

void drawGame();
void drawHeader(int playerScore, int botScore);
Ball_t initBall();
void updateBall();
int getCollisions();
Racket_t initRacket(int x);
void drawRacket(Racket_t racket);
int isGameOver();
void updatePlayerRacket();
void updateBotRacket(int level);
bool canMoveRacket(Racket_t racket);
void updateBallYVelocity(Racket_t racket);

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "THE PONG");
    SetTargetFPS(TARGET_FPS);

    Ball = initBall();
    LeftRacket = initRacket(FieldOrigin.x + LINE_THICK + 10);
    RightRacket = initRacket(FieldOrigin.x + FIELD_WIDTH - LINE_THICK - RACKET_WIDTH - 10);

    int gameOverCode;
    int playerScore = 0;
    int botScore = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(SKYBLUE);

        drawHeader(playerScore, botScore);
        drawGame();

        gameOverCode = isGameOver();
        if (gameOverCode) {
            if (gameOverCode == PLAYER_WON)
                playerScore++;
            else
                botScore++;

            EndDrawing();

            Ball = initBall();
            LeftRacket = initRacket(FieldOrigin.x + LINE_THICK + 10);
            RightRacket = initRacket(FieldOrigin.x + FIELD_WIDTH - LINE_THICK - RACKET_WIDTH - 10);
            LastCountToUpdateBallSpeed = 0;

            continue;
        }

        updatePlayerRacket();
        updateBotRacket(0);
        updateBall();

        EndDrawing();
    }


    CloseWindow();

    return 0;
}

void drawGame() {
    DrawRectangle(10, HEADHER_HEIGHT + 10, SCREEN_WIDTH - 20, FIELD_HEIGHT, GRAY);
    DrawRectangleLinesEx(Field, LINE_THICK, BLACK);

    DrawCircle(Ball.hitBox.x + Ball.radius, Ball.hitBox.y + Ball.radius, Ball.radius, WHITE);

    drawRacket(LeftRacket);
    drawRacket(RightRacket);
}

void drawHeader(int playerScore, int botScore) {
    const int textFontSize = 40;
    const int scoreFontSize = 50;

    DrawText("SCORE",
        SCREEN_WIDTH / 2 - (textFontSize * strlen("SCORE") / 2),
        textFontSize,
        textFontSize,
        WHITE
    );

    char score[6];

    sprintf_s(score, 6, "%1d : %1d", playerScore, botScore);

    DrawText(score,
        SCREEN_WIDTH / 2 - (textFontSize * strlen("SCORE") / 2) + 15,
        textFontSize + scoreFontSize,
        scoreFontSize,
        RED
    );
}

Ball_t initBall() {
    srand(time(NULL));

    Ball_t ball = { .radius = 10 };

    Vector2 FieldCenter = { Field.x + Field.width / 2, Field.y + Field.height / 2 };

    ball.hitBox = (Rectangle){ FieldCenter.x - ball.radius, FieldCenter.y - ball.radius, 2 * ball.radius, 2 * ball.radius };

    const int BaseVelocity = 500;

    int xVelocityDir = rand() % 2 == 1 ? 1 : -1;
    int yVelocityDir = rand() % 2 == 1 ? 1 : -1;
    int yVelocity = rand() % BaseVelocity;

    ball.velocity = (Vector2){ xVelocityDir * BaseVelocity, yVelocityDir * yVelocity };

    ball.racketCollsionCount = 0;

    return ball;
}

void updateBall() {
    int collisions = getCollisions();

    if (collisions & TOP || collisions & BOTTOM)
        Ball.velocity.y *= -1;

    if (collisions & LEFT_RACKET) {
        Ball.velocity.x *= -1;
        updateBallYVelocity(LeftRacket);
        ++Ball.racketCollsionCount;
    }
    if (collisions & RIGHT_RACKET) {
        Ball.velocity.x *= -1;
        updateBallYVelocity(RightRacket);
        ++Ball.racketCollsionCount;
    }

    if (Ball.racketCollsionCount % 5 == 0 && Ball.racketCollsionCount != LastCountToUpdateBallSpeed) {
        Ball.velocity.x *= 1.3;
        LastCountToUpdateBallSpeed = Ball.racketCollsionCount;
    }

    Ball.hitBox.x += Ball.velocity.x / TARGET_FPS;
    Ball.hitBox.y += Ball.velocity.y / TARGET_FPS;
}

int getCollisions() {
    int collisions = 0;

    if (Ball.hitBox.y <= Field.y)
        collisions |= TOP;
    else if (Ball.hitBox.y + 2 * Ball.radius >= Field.y + Field.height)
        collisions |= BOTTOM;

    if (Ball.hitBox.x < LeftRacket.hitBox.x + RACKET_WIDTH / 2)
        collisions |= LEFT_WALL;
    else if (Ball.hitBox.x + 2 * Ball.radius > RightRacket.hitBox.x + RACKET_WIDTH / 2)
        collisions |= RIGHT_WALL;

    Rectangle nextFrameBallHitbox = { Ball.hitBox.x + Ball.velocity.x / TARGET_FPS, Ball.hitBox.y + Ball.velocity.y / TARGET_FPS, Ball.hitBox.width, Ball.hitBox.height };
    if (CheckCollisionRecs(nextFrameBallHitbox, LeftRacket.hitBox) && Ball.velocity.x < 0)
        collisions |= LEFT_RACKET;
    else if (CheckCollisionRecs(nextFrameBallHitbox, RightRacket.hitBox) && Ball.velocity.x > 0)
        collisions |= RIGHT_RACKET;

    return collisions;
}

Racket_t initRacket(int x) {
    Racket_t racket;

    const int baseRacketHeight = 120;

    racket.hitBox = (Rectangle){
        x,
        FieldOrigin.y + (FIELD_HEIGHT - baseRacketHeight) / 2,
        RACKET_WIDTH,
        baseRacketHeight
    };

    racket.velocity = (Vector2){ 0, 0 };

    return racket;
}

void drawRacket(Racket_t racket) {
    DrawRectangle(
        racket.hitBox.x,
        racket.hitBox.y,
        racket.hitBox.width,
        racket.hitBox.height,
        DARKPURPLE
    );
}

int isGameOver() {
    int collisions = getCollisions();

    if (collisions & LEFT_WALL)
        return BOT_WON;
    if (collisions & RIGHT_WALL)
        return PLAYER_WON;

    return 0;
}

void updatePlayerRacket() {
    if (IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN))
        LeftRacket.velocity.y = -RACKET_VELOCITY;
    else if (IsKeyDown(KEY_DOWN) && !IsKeyDown(KEY_UP))
        LeftRacket.velocity.y = RACKET_VELOCITY;
    else
        LeftRacket.velocity.y = 0;

    int step = LeftRacket.velocity.y / TARGET_FPS;

    if (canMoveRacket(LeftRacket))
        LeftRacket.hitBox.y += step;
}

void updateBotRacket(int level) {
    const int velocityFactor = 5 - level;

    float YCenterRacket = RightRacket.hitBox.y + RightRacket.hitBox.height / 2;

    if (YCenterRacket < Ball.hitBox.y + Ball.radius)
        RightRacket.velocity.y = RACKET_VELOCITY / velocityFactor;
    else if (YCenterRacket > Ball.hitBox.y + Ball.radius)
        RightRacket.velocity.y = -RACKET_VELOCITY / velocityFactor;

    if (canMoveRacket(RightRacket))
        RightRacket.hitBox.y += RightRacket.velocity.y / TARGET_FPS;
}

bool canMoveRacket(Racket_t racket) {
    int step = racket.velocity.y / TARGET_FPS;

    if ((racket.velocity.y < 0 && racket.hitBox.y + step >= FieldOrigin.y + Ball.radius) ||
        (racket.velocity.y > 0 && racket.hitBox.y + racket.hitBox.height + step <= FieldOrigin.y + FIELD_HEIGHT - Ball.radius)) {

        return true;
    }

    return false;
}

// Racket that ball collide with
void updateBallYVelocity(Racket_t racket) {
    const float maxChangeInVelocity = 200;
    const float maxDiff = racket.hitBox.height / 2 + Ball.radius;

    float RacketCenterY = racket.hitBox.y + racket.hitBox.height / 2;
    float BallCenterY = Ball.hitBox.y + Ball.radius;

    float diff = RacketCenterY - BallCenterY;

    Ball.velocity.y -= (maxChangeInVelocity / maxDiff) * diff + racket.velocity.y / 10;
}