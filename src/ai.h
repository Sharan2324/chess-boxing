#ifndef AI_H
#define AI_H

#include "game.h"
#include <stdbool.h>

/*
    =========================================
    CHESS AI INTERFACE
    =========================================
*/

/*
    Calculates the best move for the current player based on AIDifficulty.
    Returns true if a legal move was found, populating fromR, fromC, toR, toC.
    Returns false if no legal moves exist (checkmate / stalemate).
*/
bool ai_get_chess_move(const GameState *game, AIDifficulty diff,
                       int *fromR, int *fromC, int *toR, int *toC);

/*
    =========================================
    BOXING AI INTERFACE
    =========================================
*/

/*
    Updates AI decision state machine and outputs a PlayerInput struct.
    aiIdx: Index of the boxer controlled by AI (typically 1 for Player 2 / Black).
    diff: Difficulty setting (Easy, Medium, Hard).
    outInput: Populated with controls for the current frame.
*/
void ai_update_boxing(const GameState *game, int aiIdx, AIDifficulty diff,
                      PlayerInput *outInput, float dt);

/*
    Resets AI internal timers and state (called at the start of a round).
*/
void ai_reset(void);

#endif
