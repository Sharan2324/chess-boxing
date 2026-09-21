#include "ai.h"
#include "chess.h"
#include "boxing.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

/*
    =========================================
    CHESS AI IMPLEMENTATION
    =========================================
*/

typedef struct {
    int fromR, fromC;
    int toR, toC;
    int score;
} ChessMove;

#define MAX_LEGAL_MOVES 256

/* Piece-Square Tables (oriented for Black, row 0 is Black back rank, row 7 is White) */
static const int pawnTable[8][8] = {
    {  0,   0,   0,   0,   0,   0,   0,   0 },
    {  5,  10,  10, -20, -20,  10,  10,   5 },
    {  5,  -5, -10,   0,   0, -10,  -5,   5 },
    {  0,   0,   0,  20,  20,   0,   0,   0 },
    {  5,   5,  10,  25,  25,  10,   5,   5 },
    { 10,  10,  20,  30,  30,  20,  10,  10 },
    { 50,  50,  50,  50,  50,  50,  50,  50 },
    {  0,   0,   0,   0,   0,   0,   0,   0 }
};

static const int knightTable[8][8] = {
    { -50, -40, -30, -30, -30, -30, -40, -50 },
    { -40, -20,   0,   5,   5,   0, -20, -40 },
    { -30,   5,  10,  15,  15,  10,   5, -30 },
    { -30,   0,  15,  20,  20,  15,   0, -30 },
    { -30,   5,  15,  20,  20,  15,   5, -30 },
    { -30,   0,  10,  15,  15,  10,   0, -30 },
    { -40, -20,   0,   0,   0,   0, -20, -40 },
    { -50, -40, -30, -30, -30, -30, -40, -50 }
};

static const int bishopTable[8][8] = {
    { -20, -10, -10, -10, -10, -10, -10, -20 },
    { -10,   5,   0,   0,   0,   0,   5, -10 },
    { -10,  10,  10,  10,  10,  10,  10, -10 },
    { -10,   0,  10,  10,  10,  10,   0, -10 },
    { -10,   5,   5,  10,  10,   5,   5, -10 },
    { -10,   0,   5,  10,  10,   5,   0, -10 },
    { -10,   0,   0,   0,   0,   0,   0, -10 },
    { -20, -10, -10, -10, -10, -10, -10, -20 }
};

static const int kingTable[8][8] = {
    {  20,  30,  10,   0,   0,  10,  30,  20 },
    {  20,  20,   0,   0,   0,   0,  20,  20 },
    { -10, -20, -20, -20, -20, -20, -20, -10 },
    { -20, -30, -30, -40, -40, -30, -30, -20 },
    { -30, -40, -40, -50, -50, -40, -40, -30 },
    { -30, -40, -40, -50, -50, -40, -40, -30 },
    { -30, -40, -40, -50, -50, -40, -40, -30 },
    { -30, -40, -40, -50, -50, -40, -40, -30 }
};

static int getPositionalScore(char piece, int r, int c, int player)
{
    /* For White (player 0), invert row for table lookup */
    int tableRow = (player == 1) ? r : (7 - r);
    char upper = (char)toupper(piece);

    switch (upper) {
        case 'P': return pawnTable[tableRow][c];
        case 'N': return knightTable[tableRow][c];
        case 'B': return bishopTable[tableRow][c];
        case 'K': return kingTable[tableRow][c];
        case 'R': return 0;
        case 'Q': return 0;
        default: return 0;
    }
}

/* Generates all legal moves for a given state and player */
static int generateLegalMoves(const GameState *state, int player, ChessMove outMoves[MAX_LEGAL_MOVES])
{
    int count = 0;
    for (int fr = 0; fr < BOARD_SIZE; fr++) {
        for (int fc = 0; fc < BOARD_SIZE; fc++) {
            char p = state->board[fr][fc];
            if (p == ' ' || !isPlayersPiece(p, player)) continue;

            for (int tr = 0; tr < BOARD_SIZE; tr++) {
                for (int tc = 0; tc < BOARD_SIZE; tc++) {
                    if (isMoveLegal(state, fr, fc, tr, tc)) {
                        if (count < MAX_LEGAL_MOVES) {
                            outMoves[count].fromR = fr;
                            outMoves[count].fromC = fc;
                            outMoves[count].toR = tr;
                            outMoves[count].toC = tc;
                            outMoves[count].score = 0;
                            count++;
                        }
                    }
                }
            }
        }
    }
    return count;
}

/* Applies a move to a GameState copy without console printing or timers */
static void applyMoveCopy(const GameState *src, GameState *dst, const ChessMove *m)
{
    *dst = *src;

    char moving = dst->board[m->fromR][m->fromC];
    int curr = dst->currentPlayer;

    /* Check Castling */
    if (isCastlingMove(dst, m->fromR, m->fromC, m->toR, m->toC)) {
        int row = m->fromR;
        if (m->toC == 6) {
            dst->board[row][6] = moving;
            dst->board[row][4] = ' ';
            dst->board[row][5] = dst->board[row][7];
            dst->board[row][7] = ' ';
            if (curr == 0) { dst->whiteKingMoved = true; dst->whiteKingsideRookMoved = true; }
            else { dst->blackKingMoved = true; dst->blackKingsideRookMoved = true; }
        } else if (m->toC == 2) {
            dst->board[row][2] = moving;
            dst->board[row][4] = ' ';
            dst->board[row][3] = dst->board[row][0];
            dst->board[row][0] = ' ';
            if (curr == 0) { dst->whiteKingMoved = true; dst->whiteQueensideRookMoved = true; }
            else { dst->blackKingMoved = true; dst->blackQueensideRookMoved = true; }
        }
    }
    else {
        /* En Passant */
        if ((moving == 'P' || moving == 'p') && m->toR == dst->enPassantRow && m->toC == dst->enPassantCol) {
            dst->board[dst->enPassantPawnRow][dst->enPassantPawnCol] = ' ';
        }

        dst->board[m->toR][m->toC] = moving;
        dst->board[m->fromR][m->fromC] = ' ';

        if (moving == 'K') dst->whiteKingMoved = true;
        if (moving == 'k') dst->blackKingMoved = true;
        if (moving == 'R' && m->fromR == 7 && m->fromC == 7) dst->whiteKingsideRookMoved = true;
        if (moving == 'R' && m->fromR == 7 && m->fromC == 0) dst->whiteQueensideRookMoved = true;
        if (moving == 'r' && m->fromR == 0 && m->fromC == 7) dst->blackKingsideRookMoved = true;
        if (moving == 'r' && m->fromR == 0 && m->fromC == 0) dst->blackQueensideRookMoved = true;
    }

    /* En Passant target square */
    if ((moving == 'P' || moving == 'p') && abs(m->toR - m->fromR) == 2) {
        int dir = (moving == 'P') ? -1 : 1;
        dst->enPassantRow = m->fromR + dir;
        dst->enPassantCol = m->fromC;
        dst->enPassantPawnRow = m->toR;
        dst->enPassantPawnCol = m->toC;
    } else {
        dst->enPassantRow = -1;
        dst->enPassantCol = -1;
        dst->enPassantPawnRow = -1;
        dst->enPassantPawnCol = -1;
    }

    /* Auto-promote to Queen */
    if (moving == 'P' && m->toR == 0) dst->board[m->toR][m->toC] = 'Q';
    if (moving == 'p' && m->toR == 7) dst->board[m->toR][m->toC] = 'q';

    /* Switch turn and check status */
    dst->currentPlayer = 1 - curr;
    dst->isCheck[0] = isKingInCheck(dst->board, 0);
    dst->isCheck[1] = isKingInCheck(dst->board, 1);
}

static int getChessPieceValue(char p)
{
    char lower = (char)tolower(p);
    switch (lower) {
        case 'p': return 100;
        case 'n': return 320;
        case 'b': return 330;
        case 'r': return 500;
        case 'q': return 900;
        case 'k': return 20000;
        default:  return 0;
    }
}

/* Static board evaluation from the perspective of aiPlayer */
static int evaluateBoard(const GameState *state, int aiPlayer)
{
    int score = 0;
    int oppPlayer = 1 - aiPlayer;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            char p = state->board[r][c];
            if (p == ' ') continue;

            int pieceVal = getChessPieceValue(p);
            int pieceOwner = isWhitePiece(p) ? 0 : 1;
            int posVal = getPositionalScore(p, r, c, pieceOwner);

            if (pieceOwner == aiPlayer) {
                score += (pieceVal + posVal);
            } else {
                score -= (pieceVal + posVal);
            }
        }
    }

    /* Bonus if opponent is in check */
    if (state->isCheck[oppPlayer]) {
        score += 40;
    }
    /* Penalty if AI is in check */
    if (state->isCheck[aiPlayer]) {
        score -= 40;
    }

    return score;
}

/* Minimax with Alpha-Beta Pruning */
static int minimax(GameState *state, int depth, int alpha, int beta, bool isMaximizing, int aiPlayer)
{
    if (depth == 0) {
        return evaluateBoard(state, aiPlayer);
    }

    int currPlayer = state->currentPlayer;
    ChessMove moves[MAX_LEGAL_MOVES];
    int count = generateLegalMoves(state, currPlayer, moves);

    if (count == 0) {
        if (state->isCheck[currPlayer]) {
            /* Checkmate: large penalty if current is AI, large reward if opponent */
            return isMaximizing ? (-90000 - depth * 100) : (90000 + depth * 100);
        }
        return 0; /* Stalemate */
    }

    /* Move ordering: captures first */
    for (int i = 0; i < count; i++) {
        char target = state->board[moves[i].toR][moves[i].toC];
        if (target != ' ') {
            moves[i].score = getChessPieceValue(target) * 10 - getChessPieceValue(state->board[moves[i].fromR][moves[i].fromC]);
        } else {
            moves[i].score = 0;
        }
    }

    /* Simple selection sort for move ordering */
    for (int i = 0; i < count - 1; i++) {
        int bestIdx = i;
        for (int j = i + 1; j < count; j++) {
            if (moves[j].score > moves[bestIdx].score) {
                bestIdx = j;
            }
        }
        if (bestIdx != i) {
            ChessMove temp = moves[i];
            moves[i] = moves[bestIdx];
            moves[bestIdx] = temp;
        }
    }

    if (isMaximizing) {
        int maxEval = -999999;
        for (int i = 0; i < count; i++) {
            GameState nextState;
            applyMoveCopy(state, &nextState, &moves[i]);
            int eval = minimax(&nextState, depth - 1, alpha, beta, false, aiPlayer);
            if (eval > maxEval) maxEval = eval;
            if (eval > alpha) alpha = eval;
            if (beta <= alpha) break; /* Beta cutoff */
        }
        return maxEval;
    } else {
        int minEval = 999999;
        for (int i = 0; i < count; i++) {
            GameState nextState;
            applyMoveCopy(state, &nextState, &moves[i]);
            int eval = minimax(&nextState, depth - 1, alpha, beta, true, aiPlayer);
            if (eval < minEval) minEval = eval;
            if (eval < beta) beta = eval;
            if (beta <= alpha) break; /* Alpha cutoff */
        }
        return minEval;
    }
}

bool ai_get_chess_move(const GameState *game, AIDifficulty diff,
                       int *fromR, int *fromC, int *toR, int *toC)
{
    int player = game->currentPlayer;
    ChessMove moves[MAX_LEGAL_MOVES];
    int count = generateLegalMoves(game, player, moves);

    if (count == 0) return false;

    /* -------------------------------------------------------------
       EASY AI: Random Legal Move
       ------------------------------------------------------------- */
    if (diff == AI_EASY) {
        int choice = rand() % count;
        *fromR = moves[choice].fromR;
        *fromC = moves[choice].fromC;
        *toR = moves[choice].toR;
        *toC = moves[choice].toC;
        return true;
    }

    /* -------------------------------------------------------------
       MEDIUM AI: 1-Ply Heuristic (Captures, Center, Checks)
       ------------------------------------------------------------- */
    if (diff == AI_MEDIUM) {
        int bestScore = -999999;
        int bestIdx = 0;

        for (int i = 0; i < count; i++) {
            GameState nextState;
            applyMoveCopy(game, &nextState, &moves[i]);

            int score = evaluateBoard(&nextState, player);

            /* Slight random noise to prevent identical games */
            score += (rand() % 15) - 7;

            if (score > bestScore) {
                bestScore = score;
                bestIdx = i;
            }
        }

        *fromR = moves[bestIdx].fromR;
        *fromC = moves[bestIdx].fromC;
        *toR = moves[bestIdx].toR;
        *toC = moves[bestIdx].toC;
        return true;
    }

    /* -------------------------------------------------------------
       HARD AI: Minimax with Alpha-Beta (Depth 3)
       ------------------------------------------------------------- */
    int bestScore = -999999;
    int bestIdx = 0;
    int alpha = -999999;
    int beta = 999999;

    for (int i = 0; i < count; i++) {
        GameState nextState;
        applyMoveCopy(game, &nextState, &moves[i]);

        int score = minimax(&nextState, 2, alpha, beta, false, player);

        if (score > bestScore) {
            bestScore = score;
            bestIdx = i;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    *fromR = moves[bestIdx].fromR;
    *fromC = moves[bestIdx].fromC;
    *toR = moves[bestIdx].toR;
    *toC = moves[bestIdx].toC;
    return true;
}

/*
    =========================================
    BOXING AI IMPLEMENTATION
    =========================================
*/

static float s_boxingActionTimer = 0.0f;
static float s_boxingActionCooldown = 0.35f;
static int s_comboCounter = 0;

void ai_reset(void)
{
    s_boxingActionTimer = 0.0f;
    s_boxingActionCooldown = 0.35f;
    s_comboCounter = 0;
}

void ai_update_boxing(const GameState *game, int aiIdx, AIDifficulty diff,
                      PlayerInput *outInput, float dt)
{
    memset(outInput, 0, sizeof(PlayerInput));

    const BoxingState *box = &game->boxing;
    const Boxer *self = &box->boxers[aiIdx];
    int oppIdx = 1 - aiIdx;
    const Boxer *opp = &box->boxers[oppIdx];

    if (self->state == BOXER_KO || opp->state == BOXER_KO) return;

    s_boxingActionTimer += dt;

    float dist = fabsf(self->x - opp->x);
    bool selfIsLeft = (self->x < opp->x);
    bool oppIsAttacking = (opp->state == BOXER_PUNCH_LIGHT ||
                           opp->state == BOXER_PUNCH_HEAVY ||
                           opp->state == BOXER_UPPERCUT);
    bool oppIsCrouching = (opp->state == BOXER_CROUCH);
    bool oppIsStunned = (opp->state == BOXER_HIT_STUN || opp->state == BOXER_GUARD_BROKEN);

    /* -------------------------------------------------------------
       EASY BOXING AI
       - Simple approach and light punch spam with wide gaps
       - Rarely blocks, never dodges
       ------------------------------------------------------------- */
    if (diff == AI_EASY) {
        if (dist > 75.0f) {
            if (selfIsLeft) outInput->right = true;
            else outInput->left = true;
        } else {
            /* In punching range */
            if (s_boxingActionTimer >= s_boxingActionCooldown) {
                s_boxingActionTimer = 0.0f;
                s_boxingActionCooldown = 0.45f + ((float)(rand() % 30) / 100.0f);

                int roll = rand() % 100;
                if (roll < 75) {
                    outInput->lightPunch = true;
                } else {
                    outInput->heavyPunch = true;
                }
            }
        }

        /* 15% chance to block if opponent attacks */
        if (oppIsAttacking && dist < 85.0f && (rand() % 100 < 15)) {
            outInput->block = true;
        }
        return;
    }

    /* -------------------------------------------------------------
       MEDIUM BOXING AI
       - Paces stamina, backs up when tired
       - Blocks and dodges with reasonable reactions
       - Punishes crouching opponents with uppercuts
       ------------------------------------------------------------- */
    if (diff == AI_MEDIUM) {
        /* If exhausted or low stamina, back away */
        if (self->isExhausted || self->stamina < 25.0f) {
            if (selfIsLeft) outInput->left = true;
            else outInput->right = true;
            outInput->block = true;
            return;
        }

        /* Defensive reaction */
        if (oppIsAttacking && dist < 90.0f) {
            int defRoll = rand() % 100;
            if (defRoll < 45) {
                outInput->block = true;
                return;
            } else if (defRoll < 65 && self->stamina >= 15.0f) {
                outInput->dodge = true;
                return;
            }
        }

        /* Movement: close distance if far */
        if (dist > 80.0f) {
            if (selfIsLeft) outInput->right = true;
            else outInput->left = true;
        } else if (dist < 50.0f) {
            /* Too close, slight retreat */
            if (selfIsLeft) outInput->left = true;
            else outInput->right = true;
        }

        /* Offensive decision */
        if (dist <= 85.0f && s_boxingActionTimer >= s_boxingActionCooldown) {
            s_boxingActionTimer = 0.0f;
            s_boxingActionCooldown = 0.28f + ((float)(rand() % 20) / 100.0f);

            if (oppIsCrouching) {
                outInput->uppercut = true;
            } else if (oppIsStunned) {
                outInput->heavyPunch = true;
            } else {
                int attRoll = rand() % 100;
                if (attRoll < 55) outInput->lightPunch = true;
                else if (attRoll < 85) outInput->heavyPunch = true;
                else outInput->uppercut = true;
            }
        }
        return;
    }

    /* -------------------------------------------------------------
       HARD BOXING AI
       - Elite spacing and rhythm
       - Ducks high punches (crouch under light/heavy)
       - Dodges or blocks uppercuts
       - Punishes whiffed punches and broken guards
       - Executes fast combinations
       ------------------------------------------------------------- */
    /* Check health & stamina preservation */
    if (self->isExhausted || self->stamina < 28.0f || self->guardMeter < 30.0f) {
        /* Tactical retreat and guard */
        if (selfIsLeft) outInput->left = true;
        else outInput->right = true;
        outInput->block = true;
        return;
    }

    /* Frame-tight defensive reaction */
    if (oppIsAttacking && dist < 95.0f) {
        if (opp->state == BOXER_PUNCH_LIGHT || opp->state == BOXER_PUNCH_HEAVY) {
            /* High punch! Duck under it or slip */
            int duckRoll = rand() % 100;
            if (duckRoll < 50) {
                outInput->down = true; /* Crouch under punch! */
                return;
            } else if (duckRoll < 80 && self->stamina >= 15.0f) {
                outInput->dodge = true;
                return;
            } else {
                outInput->block = true;
                return;
            }
        } else if (opp->state == BOXER_UPPERCUT) {
            /* Uppercut hits crouch! So block or dodge */
            int dodgeRoll = rand() % 100;
            if (dodgeRoll < 50 && self->stamina >= 15.0f) {
                outInput->dodge = true;
            } else {
                outInput->block = true;
            }
            return;
        }
    }

    /* Optimal Spacing: maintain 65px - 78px */
    if (dist > 78.0f) {
        if (selfIsLeft) outInput->right = true;
        else outInput->left = true;
    } else if (dist < 56.0f) {
        if (selfIsLeft) outInput->left = true;
        else outInput->right = true;
    }

    /* Offensive Punish / Combos */
    if (dist <= 86.0f && s_boxingActionTimer >= s_boxingActionCooldown) {
        s_boxingActionTimer = 0.0f;

        if (oppIsStunned) {
            /* Unload heaviest damage */
            outInput->heavyPunch = true;
            s_boxingActionCooldown = 0.16f;
        } else if (oppIsCrouching) {
            /* Anti-crouch uppercut */
            outInput->uppercut = true;
            s_boxingActionCooldown = 0.20f;
        } else if (opp->state == BOXER_BLOCK && opp->guardMeter < 45.0f) {
            /* Heavy punch to trigger Guard Break */
            outInput->heavyPunch = true;
            s_boxingActionCooldown = 0.22f;
        } else {
            /* Combo flow */
            s_comboCounter = (s_comboCounter + 1) % 4;
            switch (s_comboCounter) {
                case 0:
                case 1:
                    outInput->lightPunch = true;
                    s_boxingActionCooldown = 0.15f;
                    break;
                case 2:
                    outInput->heavyPunch = true;
                    s_boxingActionCooldown = 0.24f;
                    break;
                case 3:
                    outInput->uppercut = true;
                    s_boxingActionCooldown = 0.22f;
                    break;
            }
        }
    }
}
