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

enum ACTIONS {
    NOACT,
    CLOSE_WINDOW,
    NEW_GAME
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

typedef struct Ball_t {
    Vector2 center;
    int radius;
    Vector2 velocity;
    int racketCollsionCount;
} Ball_t;

#define RACKET_WIDTH 20

typedef struct Racket_t {
    Rectangle hitBox;
    Rectangle drawingBox;
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
bool CheckCollisionRayLeftRacket(Vector2 from, Vector2 to, Racket_t racket);
bool CheckCollisionRayRightRacket(Vector2 from, Vector2 to, Racket_t racket);
int drawEndScreen(int playerScore, int botScore);
bool isCursorOnButton(Rectangle button);
bool isButtonPresed(Rectangle button);
void showButton(Rectangle button, char* text, Color color, Color colorForCursorOnButton, Color TextColor);
int drawStartScreen(int* difficulty);

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "THE PONG");
    SetTargetFPS(TARGET_FPS);

    Ball = initBall();
    LeftRacket = initRacket(Field.x + LINE_THICK + 10);
    RightRacket = initRacket(Field.x + FIELD_WIDTH - LINE_THICK - RACKET_WIDTH - 10);

    int maxScore = 5;
    int gameOverCode;
    int playerScore = 0;
    int botScore = 0;
    int difficulty;
    bool isGameGoing = false;
    bool shouldWait = false;

    while (!WindowShouldClose()) {
        if (!isGameGoing) {
            if (drawStartScreen(&difficulty) == CLOSE_WINDOW)
                break;

            isGameGoing = true;

            BeginDrawing();
            ClearBackground(SKYBLUE);

            drawHeader(playerScore, botScore);
            drawGame();

            EndDrawing();

            WaitTime(1);
        }

        BeginDrawing();
        ClearBackground(SKYBLUE);

        drawHeader(playerScore, botScore);
        drawGame();

        gameOverCode = isGameOver();
        if (gameOverCode) {
            shouldWait = true;

            if (gameOverCode == PLAYER_WON)
                playerScore++;
            else
                botScore++;

            EndDrawing();

            if (playerScore >= maxScore || botScore >= maxScore) {
                int NextAction = drawEndScreen(playerScore, botScore);

                if (NextAction == CLOSE_WINDOW)
                    break;

                playerScore = 0;
                botScore = 0;

                if (drawStartScreen(&difficulty) == CLOSE_WINDOW)
                    break;
            }

            Ball = initBall();
            LeftRacket = initRacket(Field.x + LINE_THICK + 10);
            RightRacket = initRacket(Field.x + FIELD_WIDTH - LINE_THICK - RACKET_WIDTH - 10);
            LastCountToUpdateBallSpeed = 0;

            continue;
        }

        updatePlayerRacket();
        updateBotRacket(difficulty);
        updateBall();

        EndDrawing();

        if (shouldWait) {
            WaitTime(1);
            shouldWait = false;
        }
    }

    CloseWindow();

    return 0;
}

void drawGame() {
    DrawRectangle(10, HEADHER_HEIGHT + 10, SCREEN_WIDTH - 20, FIELD_HEIGHT, GRAY);
    DrawRectangleLinesEx(Field, LINE_THICK, BLACK);

    DrawCircle(Ball.center.x, Ball.center.y, Ball.radius, WHITE);

    drawRacket(LeftRacket);
    drawRacket(RightRacket);
}

void drawHeader(int playerScore, int botScore) {
    const int textFontSize = 40;
    const int scoreFontSize = 50;

    DrawText("SCORE",
        (SCREEN_WIDTH - MeasureText("SCORE", textFontSize)) / 2,
        textFontSize,
        textFontSize,
        WHITE
    );

    char score[6];

    sprintf_s(score, 6, "%1d : %1d", playerScore, botScore);

    DrawText(score,
        (SCREEN_WIDTH - MeasureText(score, scoreFontSize)) / 2,
        textFontSize + scoreFontSize,
        scoreFontSize,
        RED
    );
}

Ball_t initBall() {
    srand(time(NULL));

    Ball_t ball = { .radius = 10 };

    ball.center = (Vector2){ Field.x + Field.width / 2, Field.y + Field.height / 2 };

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

    Ball.center.x += Ball.velocity.x / TARGET_FPS;
    Ball.center.y += Ball.velocity.y / TARGET_FPS;
}

int getCollisions() {
    int collisions = 0;

    if (Ball.center.y - Ball.radius <= Field.y)
        collisions |= TOP;
    else if (Ball.center.y + Ball.radius >= Field.y + Field.height)
        collisions |= BOTTOM;

    if (Ball.center.x - Ball.radius < LeftRacket.drawingBox.x + RACKET_WIDTH / 2)
        collisions |= LEFT_WALL;
    else if (Ball.center.x + Ball.radius > RightRacket.drawingBox.x + RACKET_WIDTH / 2)
        collisions |= RIGHT_WALL;

    Vector2 nextFrameBallCenter = { Ball.center.x + Ball.velocity.x / TARGET_FPS, Ball.center.y + Ball.velocity.y / TARGET_FPS };

    if (Ball.velocity.x < 0 && CheckCollisionRayLeftRacket(Ball.center, nextFrameBallCenter, LeftRacket)) {
        collisions |= LEFT_RACKET;
    }
    else if (Ball.velocity.x > 0 && CheckCollisionRayRightRacket(Ball.center, nextFrameBallCenter, RightRacket))
        collisions |= RIGHT_RACKET;

    return collisions;
}

Racket_t initRacket(int x) {
    Racket_t racket;

    const int baseRacketHeight = 120;

    racket.drawingBox = (Rectangle){
        x,
        Field.y + (FIELD_HEIGHT - baseRacketHeight) / 2,
        RACKET_WIDTH,
        baseRacketHeight
    };

    racket.hitBox = (Rectangle){
        x - Ball.radius,
        Field.y + (FIELD_HEIGHT - baseRacketHeight) / 2 - Ball.radius,
        RACKET_WIDTH + 2 * Ball.radius,
        baseRacketHeight + 2 * Ball.radius
    };

    racket.velocity = (Vector2){ 0, 0 };

    return racket;
}

void drawRacket(Racket_t racket) {
    DrawRectangle(
        racket.drawingBox.x,
        racket.drawingBox.y,
        racket.drawingBox.width,
        racket.drawingBox.height,
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

    if (canMoveRacket(LeftRacket)) {
        LeftRacket.drawingBox.y += step;
        LeftRacket.hitBox.y += step;
    }
}

void updateBotRacket(int level) {
    const int velocityFactor = 5 - level;

    float YCenterRacket = RightRacket.drawingBox.y + RightRacket.drawingBox.height / 2;

    if (YCenterRacket < Ball.center.y)
        RightRacket.velocity.y = RACKET_VELOCITY / velocityFactor;
    else if (YCenterRacket > Ball.center.y)
        RightRacket.velocity.y = -RACKET_VELOCITY / velocityFactor;

    if (canMoveRacket(RightRacket)) {
        RightRacket.drawingBox.y += RightRacket.velocity.y / TARGET_FPS;
        RightRacket.hitBox.y += RightRacket.velocity.y / TARGET_FPS;
    }
}

bool canMoveRacket(Racket_t racket) {
    int step = racket.velocity.y / TARGET_FPS;

    if ((racket.velocity.y < 0 && racket.drawingBox.y + step >= Field.y + Ball.radius) ||
        (racket.velocity.y > 0 && racket.drawingBox.y + racket.drawingBox.height + step <= Field.y + FIELD_HEIGHT - Ball.radius)) {

        return true;
    }

    return false;
}

// Racket that ball collide with
void updateBallYVelocity(Racket_t racket) {
    const float maxChangeInVelocity = 200;
    const float maxDiff = racket.hitBox.height / 2;

    float RacketCenterY = racket.hitBox.y + racket.hitBox.height / 2;

    float diff = RacketCenterY - Ball.center.y;

    Ball.velocity.y -= (maxChangeInVelocity / maxDiff) * diff + racket.velocity.y / 10;
}

bool CheckCollisionRayLeftRacket(Vector2 from, Vector2 to, Racket_t racket) {
    float x = racket.hitBox.x + racket.hitBox.width;

    Vector2 pos;

    return CheckCollisionLines(
        (Vector2) {
        x, racket.hitBox.y
    },
        (Vector2) {
        x, racket.hitBox.y + racket.hitBox.height
    },
        (Vector2) {
        from.x, from.y
    },
        (Vector2) {
        to.x, to.y
    },
        & pos
    );
}

bool CheckCollisionRayRightRacket(Vector2 from, Vector2 to, Racket_t racket) {
    float x = racket.hitBox.x;

    Vector2 pos;

    return CheckCollisionLines(
        (Vector2) {
        x, racket.hitBox.y
    },
        (Vector2) {
        x, racket.hitBox.y + racket.hitBox.height
    },
        (Vector2) {
        from.x, from.y
    },
        (Vector2) {
        to.x, to.y
    },
        &pos
    );
}

int drawEndScreen(int playerScore, int botScore) {
    int exitCode = NOACT;

    const int fontSize = 60;

    char message[32];
    char score[32];
    Color scoreColor;

    if (playerScore > botScore) {
        sprintf_s(message, 32, "You WON!");
        scoreColor = DARKGREEN;
    }
    else {
        sprintf_s(message, 32, "You Lost!");
        scoreColor = RED;
    }
    sprintf_s(score, 32, "%d : %d", playerScore, botScore);

    Rectangle buttonForAgain = { 100, 500, 150, 50 };
    Rectangle buttonForExit = { SCREEN_WIDTH - 100 - 150, 500, 150, 50 };

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(SKYBLUE);

        DrawText(message, (FIELD_WIDTH - MeasureText(message, fontSize)) / 2, 100, fontSize, YELLOW);
        DrawText(score, (FIELD_WIDTH - MeasureText(score, fontSize)) / 2, 200, fontSize, scoreColor);

        showButton(buttonForAgain, "Again", WHITE, LIGHTGRAY, BLACK);
        showButton(buttonForExit, "Exit", WHITE, LIGHTGRAY, BLACK);

        if (isButtonPresed(buttonForAgain)) {
            exitCode = NEW_GAME;
            EndDrawing();
            break;
        }
        if (isButtonPresed(buttonForExit)) {
            exitCode = CLOSE_WINDOW;
            EndDrawing();
            break;
        }

        EndDrawing();
    }

    return exitCode;
}

bool isCursorOnButton(Rectangle button) {
    Vector2 mouse = GetMousePosition();

    if (mouse.x >= button.x && mouse.x <= button.x + button.width &&
        mouse.y >= button.y && mouse.y <= button.y + button.height)
        return true;

    return false;
}

bool isButtonPresed(Rectangle button) {
    Vector2 mouse = GetMousePosition();

    if (isCursorOnButton(button) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return true;

    return false;
}

void showButton(Rectangle button, char* text, Color color, Color colorForCursorOnButton, Color TextColor) {
    const int textFontSize = 40;

    Vector2 textSize = MeasureTextEx(GetFontDefault(), text, textFontSize, 1);
    Vector2 textPos = { button.x + (button.width - textSize.x) / 2,
                       button.y + (button.height - textSize.y) / 2 };

    if (isCursorOnButton(button))
        color = colorForCursorOnButton;

    DrawRectangle(button.x, button.y, button.width, button.height, color);
    DrawTextEx(GetFontDefault(), text, textPos, textFontSize, 1, TextColor);
}

int drawStartScreen(int* difficulty) {
    int exitCode = CLOSE_WINDOW;

    const int fontSize = 80;

    char* gameName = "The Pong";

    const int diffFontSize = 50;

    char difficulties[5][32] = {
        "Easy", "Normal", "Hard", "Extreme", "Imposible"
    };


    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(SKYBLUE);

        DrawText(gameName, (FIELD_WIDTH - MeasureText(gameName, fontSize)) / 2, 50, fontSize, WHITE);

        int buttonStartPosY = 150;

        for (int i = 0; i < 5; i++) {
            Vector2 buttonSize = MeasureTextEx(GetFontDefault(), difficulties[i], diffFontSize, 1);
            int buttonStartPosX = (FIELD_WIDTH - buttonSize.x) / 2;

            Rectangle buttonsForDiff = (Rectangle){buttonStartPosX, buttonStartPosY, buttonSize.x, buttonSize.y};

            if (isButtonPresed(buttonsForDiff)) {
                *difficulty = i;
                exitCode = NEW_GAME;
                EndDrawing();
                return exitCode;
            }
            else if (isCursorOnButton(buttonsForDiff))
                showButton(
                    buttonsForDiff,
                    difficulties[i], SKYBLUE, SKYBLUE, GRAY
                );
            else
                showButton(
                    buttonsForDiff,
                    difficulties[i], SKYBLUE, SKYBLUE, WHITE
                );

            buttonStartPosY += buttonSize.y + 40;
        }
        // Exit Button
        Vector2 exitButtonSize = MeasureTextEx(GetFontDefault(), "Exit", diffFontSize, 1);
        Rectangle exitButton = {
                (FIELD_WIDTH - exitButtonSize.x) / 2,
                buttonStartPosY + 20,
                exitButtonSize.x,
                exitButtonSize.y
        };

        showButton(
            exitButton,
            "Exit", SKYBLUE, SKYBLUE, RED
        );

        EndDrawing();

        if (isButtonPresed(exitButton)) {
            exitCode = CLOSE_WINDOW;
            break;
        }
    }

    return exitCode;
}