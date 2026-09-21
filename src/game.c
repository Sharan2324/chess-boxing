#include "game.h"
#include "chess.h"
#include "boxing.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

int getPieceValue(char p)
{
    char lower = (char)tolower(p);
    switch (lower) {
        case 'p': return 1;
        case 'n': return 3;
        case 'b': return 3;
        case 'r': return 5;
        case 'q': return 9;
        default:  return 0;
    }
}

void updateAdvantageBuffs(GameState *game)
{
    int diff = game->materialScore[0] - game->materialScore[1];

    if (diff > 0) {
        /* Player 1 (White) advantage */
        float bonus = (float)diff * 0.035f;
        if (bonus > 0.35f) bonus = 0.35f;
        game->boxing.boxers[0].damageMultiplier = 1.0f + bonus;
        game->boxing.boxers[0].maxStamina = 100.0f + (float)diff * 3.0f;
        if (game->boxing.boxers[0].maxStamina > 130.0f) game->boxing.boxers[0].maxStamina = 130.0f;

        game->boxing.boxers[1].damageMultiplier = 1.0f;
        game->boxing.boxers[1].maxStamina = 100.0f;
    } else if (diff < 0) {
        /* Player 2 (Black) advantage */
        float bonus = (float)(-diff) * 0.035f;
        if (bonus > 0.35f) bonus = 0.35f;
        game->boxing.boxers[1].damageMultiplier = 1.0f + bonus;
        game->boxing.boxers[1].maxStamina = 100.0f + (float)(-diff) * 3.0f;
        if (game->boxing.boxers[1].maxStamina > 130.0f) game->boxing.boxers[1].maxStamina = 130.0f;

        game->boxing.boxers[0].damageMultiplier = 1.0f;
        game->boxing.boxers[0].maxStamina = 100.0f;
    } else {
        /* Equal material */
        game->boxing.boxers[0].damageMultiplier = 1.0f;
        game->boxing.boxers[0].maxStamina = 100.0f;
        game->boxing.boxers[1].damageMultiplier = 1.0f;
        game->boxing.boxers[1].maxStamina = 100.0f;
    }
}

void initializeGame(GameState *game)
{
    game->phase = PHASE_MENU;
    game->mode = MODE_LOCAL_MULTIPLAYER;
    game->aiDifficulty = AI_MEDIUM;
    game->netRole = NET_ROLE_NONE;
    game->localPlayerIndex = 0;
    game->currentRound = 1;
    game->roundTimer = CHESS_ROUND_DURATION;
    game->transitionTimer = 0.0f;
    game->nextPhase = PHASE_BOXING;
    game->winner = -1;
    game->winReason = WIN_NONE;

    game->aiThinkingTimer = 0.0f;
    game->aiMoveReady = false;
    game->aiPendingFromR = -1;
    game->aiPendingFromC = -1;
    game->aiPendingToR = -1;
    game->aiPendingToC = -1;

    strcpy(game->netIpInput, "127.0.0.1");
    game->netIpLen = (int)strlen(game->netIpInput);
    strcpy(game->hostLocalIp, "127.0.0.1");
    game->netStatusTimer = 0.0f;
    game->healthRecoveredAfterSecondChess = false;

    initChessBoard(game);
    initBoxing(game);
    updateAdvantageBuffs(game);
}

void startMatchWithMode(GameState *game, GameMode mode, AIDifficulty diff, NetRole role, int localIdx)
{
    game->mode = mode;
    game->aiDifficulty = diff;
    game->netRole = role;
    game->localPlayerIndex = localIdx;

    game->phase = PHASE_CHESS;
    game->currentRound = 1;
    game->roundTimer = CHESS_ROUND_DURATION;
    game->transitionTimer = 0.0f;
    game->nextPhase = PHASE_BOXING;
    game->winner = -1;
    game->winReason = WIN_NONE;
    game->healthRecoveredAfterSecondChess = false;

    game->aiThinkingTimer = 0.0f;
    game->aiMoveReady = false;

    initChessBoard(game);
    initBoxing(game);
    updateAdvantageBuffs(game);

    printf("Match started! Mode: %d | Local Player: P%d | Round 1: CHESS\n", mode + 1, localIdx + 1);
}

void resetForRematch(GameState *game)
{
    printf("Starting a new match from rematch!\n");
    startMatchWithMode(game, game->mode, game->aiDifficulty, game->netRole, game->localPlayerIndex);
}

void recoverBoxingHealthAfterSecondChess(GameState *game)
{
    if (game->healthRecoveredAfterSecondChess) {
        return;
    }
    game->healthRecoveredAfterSecondChess = true;

    for (int i = 0; i < 2; i++) {
        Boxer *b = &game->boxing.boxers[i];
        if (b->health < BOXER_MIN_HEALTH_AFTER_SECOND_CHESS) {
            printf("Player %d health recovered from %.1f to %.1f HP after Round 2 Chess!\n",
                   i + 1, b->health, BOXER_MIN_HEALTH_AFTER_SECOND_CHESS);
            b->health = BOXER_MIN_HEALTH_AFTER_SECOND_CHESS;
            if (b->displayHealth < BOXER_MIN_HEALTH_AFTER_SECOND_CHESS) {
                b->displayHealth = BOXER_MIN_HEALTH_AFTER_SECOND_CHESS;
            }
        } else {
            printf("Player %d health already at %.1f HP (>= %.1f), preserved.\n",
                   i + 1, b->health, BOXER_MIN_HEALTH_AFTER_SECOND_CHESS);
        }
    }
}

void startTransition(GameState *game, MatchPhase nextPhase)
{
    game->phase = PHASE_TRANSITION;
    game->nextPhase = nextPhase;
    game->transitionTimer = TRANSITION_DURATION;

    /* Clear any chess selection / drag */
    game->selectedRow = -1;
    game->selectedCol = -1;
    game->isDragging = false;
    clearLegalMoves(game);

    updateAdvantageBuffs(game);

    /*
        Special Health Recovery after the SECOND CHESS ROUND:
        Match sequence: Round 1 (Chess 1) -> Round 2 (Boxing 1) -> Round 3 (Chess 2) -> Round 4 (Boxing 2).
        When Round 3 (second chess round) finishes and transitions into boxing:
        Recover each player's health to at least 50% (BOXER_MIN_HEALTH_AFTER_SECOND_CHESS).
        In network mode, only the HOST is authoritative for triggering health recovery.
    */
    if (game->currentRound == 3 && nextPhase == PHASE_BOXING && !game->healthRecoveredAfterSecondChess) {
        if (game->mode != MODE_NETWORK_MULTIPLAYER || game->netRole == NET_ROLE_HOST) {
            recoverBoxingHealthAfterSecondChess(game);
        }
        game->healthRecoveredAfterSecondChess = true;
    }

    printf("Round transition started: next phase is %s\n",
           (nextPhase == PHASE_BOXING) ? "BOXING" : "CHESS");
}

void updateMatch(GameState *game, float dt)
{
    if (game->phase == PHASE_MENU ||
        game->phase == PHASE_DIFFICULTY_SELECT ||
        game->phase == PHASE_NET_SELECT ||
        game->phase == PHASE_NET_HOST_WAIT ||
        game->phase == PHASE_NET_JOIN_INPUT ||
        game->phase == PHASE_NET_CONNECTING ||
        game->phase == PHASE_HOW_TO_PLAY) {
        return;
    }

    if (game->phase == PHASE_GAME_OVER) {
        return;
    }

    /* -------------------------------------------------------------
       TRANSITION PHASE
       ------------------------------------------------------------- */
    if (game->phase == PHASE_TRANSITION) {
        game->transitionTimer -= dt;
        if (game->transitionTimer <= 0.0f) {
            game->phase = game->nextPhase;
            game->currentRound++;

            if (game->phase == PHASE_CHESS) {
                game->roundTimer = CHESS_ROUND_DURATION;
                printf("Entering Round %d: CHESS!\n", game->currentRound);
                updateCheckAndCheckmateStatus(game);
            } else if (game->phase == PHASE_BOXING) {
                game->roundTimer = BOXING_ROUND_DURATION;
                printf("Entering Round %d: BOXING!\n", game->currentRound);
                updateAdvantageBuffs(game);
                resetBoxingRound(game);
            }
        }
        return;
    }

    /* -------------------------------------------------------------
       CHESS PHASE CLOCKS
       ------------------------------------------------------------- */
    if (game->phase == PHASE_CHESS) {
        /* If pawn promotion dialog is open, pause timers */
        if (game->isPromoting) {
            return;
        }

        /* Decrement active player's chess clock */
        if (game->currentPlayer == 0) {
            game->whiteClock -= dt;
            if (game->whiteClock <= 0.0f) {
                game->whiteClock = 0.0f;
                game->winner = 1;
                game->winReason = WIN_TIMEOUT;
                game->phase = PHASE_GAME_OVER;
                printf("TIME OUT! Black wins on chess time!\n");
                return;
            }
        } else {
            game->blackClock -= dt;
            if (game->blackClock <= 0.0f) {
                game->blackClock = 0.0f;
                game->winner = 0;
                game->winReason = WIN_TIMEOUT;
                game->phase = PHASE_GAME_OVER;
                printf("TIME OUT! White wins on chess time!\n");
                return;
            }
        }

        /* Decrement chess round timer */
        game->roundTimer -= dt;
        if (game->roundTimer <= 0.0f) {
            startTransition(game, PHASE_BOXING);
        }
    }

    /* -------------------------------------------------------------
       BOXING PHASE ROUND TIMER
       ------------------------------------------------------------- */
    else if (game->phase == PHASE_BOXING) {
        game->roundTimer -= dt;
        if (game->roundTimer <= 0.0f) {
            startTransition(game, PHASE_CHESS);
        }
    }
}
