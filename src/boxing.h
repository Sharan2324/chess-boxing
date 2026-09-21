#ifndef BOXING_H
#define BOXING_H

#include "game.h"
#include <SDL3/SDL.h>

#define RING_MIN_X 140.0f
#define RING_MAX_X 860.0f
#define FLOOR_Y 620.0f

#define BOXER_WIDTH 60.0f
#define BOXER_HEIGHT 130.0f
#define BOXER_CROUCH_HEIGHT 75.0f

void initBoxing(GameState *game);
void resetBoxingRound(GameState *game);
void readKeyboardInput(const bool *keys, PlayerInput *p1Input, PlayerInput *p2Input);
void updateBoxing(GameState *game, const PlayerInput inputs[2], float dt);
void spawnHitParticles(BoxingState *box, float x, float y, int count, unsigned char r, unsigned char g, unsigned char b);

#endif
