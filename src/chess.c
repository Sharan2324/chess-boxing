#include "chess.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void initChessBoard(GameState *game)
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            game->board[r][c] = ' ';
            game->legalMoves[r][c] = false;
        }
    }

    /* Black pieces (row 0 & 1) */
    game->board[0][0] = 'r';
    game->board[0][1] = 'n';
    game->board[0][2] = 'b';
    game->board[0][3] = 'q';
    game->board[0][4] = 'k';
    game->board[0][5] = 'b';
    game->board[0][6] = 'n';
    game->board[0][7] = 'r';
    for (c = 0; c < BOARD_SIZE; c++) {
        game->board[1][c] = 'p';
    }

    /* White pieces (row 7 & 6) */
    game->board[7][0] = 'R';
    game->board[7][1] = 'N';
    game->board[7][2] = 'B';
    game->board[7][3] = 'Q';
    game->board[7][4] = 'K';
    game->board[7][5] = 'B';
    game->board[7][6] = 'N';
    game->board[7][7] = 'R';
    for (c = 0; c < BOARD_SIZE; c++) {
        game->board[6][c] = 'P';
    }

    game->selectedRow = -1;
    game->selectedCol = -1;
    game->isDragging = false;
    game->dragStartRow = -1;
    game->dragStartCol = -1;
    game->dragCurrentX = 0;
    game->dragCurrentY = 0;

    game->currentPlayer = 0; /* White starts */
    game->isCheck[0] = false;
    game->isCheck[1] = false;
    game->isCheckmate[0] = false;
    game->isCheckmate[1] = false;

    game->whiteClock = CHESS_TOTAL_CLOCK;
    game->blackClock = CHESS_TOTAL_CLOCK;

    /* Castling flags */
    game->whiteKingMoved = false;
    game->blackKingMoved = false;
    game->whiteKingsideRookMoved = false;
    game->whiteQueensideRookMoved = false;
    game->blackKingsideRookMoved = false;
    game->blackQueensideRookMoved = false;

    /* En Passant */
    game->enPassantRow = -1;
    game->enPassantCol = -1;
    game->enPassantPawnRow = -1;
    game->enPassantPawnCol = -1;

    /* Promotion modal */
    game->isPromoting = false;
    game->promotionRow = -1;
    game->promotionCol = -1;
    game->promotionPawnPlayer = -1;

    /* Captured pieces */
    game->capturedCount[0] = 0;
    game->capturedCount[1] = 0;
    game->materialScore[0] = 0;
    game->materialScore[1] = 0;
    game->capturedPieces[0][0] = '\0';
    game->capturedPieces[1][0] = '\0';
}

char getPiece(const GameState *game, int row, int col)
{
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return ' ';
    }
    return game->board[row][col];
}

bool isWhitePiece(char p)
{
    return (p >= 'A' && p <= 'Z');
}

bool isBlackPiece(char p)
{
    return (p >= 'a' && p <= 'z');
}

bool isPlayersPiece(char p, int player)
{
    if (p == ' ') return false;
    if (player == 0) return isWhitePiece(p);
    if (player == 1) return isBlackPiece(p);
    return false;
}

bool mouseToBoardSquare(int mouseX, int mouseY, int *row, int *col)
{
    if (mouseX < BOARD_X || mouseX >= BOARD_X + BOARD_SIZE * SQUARE_SIZE ||
        mouseY < BOARD_Y || mouseY >= BOARD_Y + BOARD_SIZE * SQUARE_SIZE) {
        return false;
    }
    *col = (mouseX - BOARD_X) / SQUARE_SIZE;
    *row = (mouseY - BOARD_Y) / SQUARE_SIZE;
    return true;
}

static bool canMoveToSquare(const char board[BOARD_SIZE][BOARD_SIZE], int fromR, int fromC, int toR, int toC)
{
    char moving = board[fromR][fromC];
    char dest = board[toR][toC];

    if (dest == ' ') return true;
    if (isWhitePiece(moving) && isWhitePiece(dest)) return false;
    if (isBlackPiece(moving) && isBlackPiece(dest)) return false;
    return true;
}

static bool isPawnMove(const GameState *game, int fromR, int fromC, int toR, int toC)
{
    char piece = game->board[fromR][fromC];
    char dest = game->board[toR][toC];
    int dir = (piece == 'P') ? -1 : 1;
    int startRow = (piece == 'P') ? 6 : 1;

    /* 1 square forward */
    if (toC == fromC && toR == fromR + dir) {
        return (dest == ' ');
    }

    /* 2 squares forward from start */
    if (toC == fromC && fromR == startRow && toR == fromR + 2 * dir) {
        return (dest == ' ' && game->board[fromR + dir][fromC] == ' ');
    }

    /* Diagonal capture (standard) */
    if (abs(toC - fromC) == 1 && toR == fromR + dir) {
        if (dest != ' ') {
            int player = isWhitePiece(piece) ? 0 : 1;
            return !isPlayersPiece(dest, player);
        }

        /* En Passant capture */
        if (toR == game->enPassantRow && toC == game->enPassantCol) {
            return true;
        }
    }

    return false;
}

static bool isKnightMove(const char board[BOARD_SIZE][BOARD_SIZE], int fromR, int fromC, int toR, int toC)
{
    int dr = abs(toR - fromR);
    int dc = abs(toC - fromC);
    if (!((dr == 2 && dc == 1) || (dr == 1 && dc == 2))) return false;
    return canMoveToSquare(board, fromR, fromC, toR, toC);
}

static bool isBishopMove(const char board[BOARD_SIZE][BOARD_SIZE], int fromR, int fromC, int toR, int toC)
{
    int dr = abs(toR - fromR);
    int dc = abs(toC - fromC);
    if (dr != dc || dr == 0) return false;

    int rStep = (toR > fromR) ? 1 : -1;
    int cStep = (toC > fromC) ? 1 : -1;
    int r = fromR + rStep;
    int c = fromC + cStep;

    while (r != toR && c != toC) {
        if (board[r][c] != ' ') return false;
        r += rStep;
        c += cStep;
    }

    return canMoveToSquare(board, fromR, fromC, toR, toC);
}

static bool isRookMove(const char board[BOARD_SIZE][BOARD_SIZE], int fromR, int fromC, int toR, int toC)
{
    if (fromR != toR && fromC != toC) return false;
    if (fromR == toR && fromC == toC) return false;

    if (fromC == toC) {
        int step = (toR > fromR) ? 1 : -1;
        int r = fromR + step;
        while (r != toR) {
            if (board[r][fromC] != ' ') return false;
            r += step;
        }
    } else {
        int step = (toC > fromC) ? 1 : -1;
        int c = fromC + step;
        while (c != toC) {
            if (board[fromR][c] != ' ') return false;
            c += step;
        }
    }

    return canMoveToSquare(board, fromR, fromC, toR, toC);
}

static bool isQueenMove(const char board[BOARD_SIZE][BOARD_SIZE], int fromR, int fromC, int toR, int toC)
{
    return isBishopMove(board, fromR, fromC, toR, toC) || isRookMove(board, fromR, fromC, toR, toC);
}

static bool isStandardKingMove(const char board[BOARD_SIZE][BOARD_SIZE], int fromR, int fromC, int toR, int toC)
{
    int dr = abs(toR - fromR);
    int dc = abs(toC - fromC);
    if (dr == 0 && dc == 0) return false;
    if (dr > 1 || dc > 1) return false;
    return canMoveToSquare(board, fromR, fromC, toR, toC);
}

bool isCastlingMove(const GameState *game, int fromR, int fromC, int toR, int toC)
{
    char piece = game->board[fromR][fromC];
    if (piece != 'K' && piece != 'k') return false;

    int player = (piece == 'K') ? 0 : 1;
    int startRow = (player == 0) ? 7 : 0;

    if (fromR != startRow || fromC != 4 || toR != startRow) {
        return false;
    }

    /* King cannot castle out of check */
    if (isKingInCheck(game->board, player)) {
        return false;
    }

    int enemy = 1 - player;

    /* Kingside Castling (O-O): King moves to (startRow, 6) */
    if (toC == 6) {
        if (player == 0 && (game->whiteKingMoved || game->whiteKingsideRookMoved)) return false;
        if (player == 1 && (game->blackKingMoved || game->blackKingsideRookMoved)) return false;

        char rookPiece = (player == 0) ? 'R' : 'r';
        if (game->board[startRow][7] != rookPiece) return false;

        /* Squares between king and rook must be empty */
        if (game->board[startRow][5] != ' ' || game->board[startRow][6] != ' ') {
            return false;
        }

        /* Squares transit and destination must not be attacked */
        if (isSquareAttacked(game->board, startRow, 5, enemy) ||
            isSquareAttacked(game->board, startRow, 6, enemy)) {
            return false;
        }

        return true;
    }

    /* Queenside Castling (O-O-O): King moves to (startRow, 2) */
    if (toC == 2) {
        if (player == 0 && (game->whiteKingMoved || game->whiteQueensideRookMoved)) return false;
        if (player == 1 && (game->blackKingMoved || game->blackQueensideRookMoved)) return false;

        char rookPiece = (player == 0) ? 'R' : 'r';
        if (game->board[startRow][0] != rookPiece) return false;

        /* Squares between king and rook must be empty */
        if (game->board[startRow][1] != ' ' || game->board[startRow][2] != ' ' || game->board[startRow][3] != ' ') {
            return false;
        }

        /* Transit and destination squares must not be attacked */
        if (isSquareAttacked(game->board, startRow, 3, enemy) ||
            isSquareAttacked(game->board, startRow, 2, enemy)) {
            return false;
        }

        return true;
    }

    return false;
}

bool isMoveMechanicallyLegal(const GameState *game, int fromR, int fromC, int toR, int toC)
{
    if (fromR < 0 || fromR >= BOARD_SIZE || fromC < 0 || fromC >= BOARD_SIZE) return false;
    if (toR < 0 || toR >= BOARD_SIZE || toC < 0 || toC >= BOARD_SIZE) return false;
    if (fromR == toR && fromC == toC) return false;

    char piece = game->board[fromR][fromC];
    if (piece == ' ') return false;

    switch (piece) {
        case 'P':
        case 'p':
            return isPawnMove(game, fromR, fromC, toR, toC);
        case 'N':
        case 'n':
            return isKnightMove(game->board, fromR, fromC, toR, toC);
        case 'B':
        case 'b':
            return isBishopMove(game->board, fromR, fromC, toR, toC);
        case 'R':
        case 'r':
            return isRookMove(game->board, fromR, fromC, toR, toC);
        case 'Q':
        case 'q':
            return isQueenMove(game->board, fromR, fromC, toR, toC);
        case 'K':
        case 'k':
            return isStandardKingMove(game->board, fromR, fromC, toR, toC) || isCastlingMove(game, fromR, fromC, toR, toC);
        default:
            return false;
    }
}

bool isSquareAttacked(const char board[BOARD_SIZE][BOARD_SIZE], int targetRow, int targetCol, int attackerPlayer)
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            char p = board[r][c];
            if (!isPlayersPiece(p, attackerPlayer)) continue;

            /* Pawn attacks diagonally */
            if (p == 'P' || p == 'p') {
                int dir = (p == 'P') ? -1 : 1;
                if (targetRow == r + dir && (targetCol == c - 1 || targetCol == c + 1)) {
                    return true;
                }
            } else if (p == 'N' || p == 'n') {
                if (isKnightMove(board, r, c, targetRow, targetCol)) return true;
            } else if (p == 'B' || p == 'b') {
                if (isBishopMove(board, r, c, targetRow, targetCol)) return true;
            } else if (p == 'R' || p == 'r') {
                if (isRookMove(board, r, c, targetRow, targetCol)) return true;
            } else if (p == 'Q' || p == 'q') {
                if (isQueenMove(board, r, c, targetRow, targetCol)) return true;
            } else if (p == 'K' || p == 'k') {
                if (isStandardKingMove(board, r, c, targetRow, targetCol)) return true;
            }
        }
    }
    return false;
}

bool findKing(const char board[BOARD_SIZE][BOARD_SIZE], int player, int *outRow, int *outCol)
{
    char targetKing = (player == 0) ? 'K' : 'k';
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (board[r][c] == targetKing) {
                *outRow = r;
                *outCol = c;
                return true;
            }
        }
    }
    return false;
}

bool isKingInCheck(const char board[BOARD_SIZE][BOARD_SIZE], int player)
{
    int kingR, kingC;
    if (!findKing(board, player, &kingR, &kingC)) {
        return false;
    }
    return isSquareAttacked(board, kingR, kingC, 1 - player);
}

bool isMoveLegal(const GameState *game, int fromR, int fromC, int toR, int toC)
{
    char piece = game->board[fromR][fromC];
    if (!isPlayersPiece(piece, game->currentPlayer)) return false;

    if (!isMoveMechanicallyLegal(game, fromR, fromC, toR, toC)) return false;

    /* Castling already verified king safety */
    if (isCastlingMove(game, fromR, fromC, toR, toC)) {
        return true;
    }

    /* Simulate move on a copy of the board */
    char tempBoard[BOARD_SIZE][BOARD_SIZE];
    memcpy(tempBoard, game->board, sizeof(tempBoard));

    /* Check for en passant capture in simulation */
    if ((piece == 'P' || piece == 'p') && toR == game->enPassantRow && toC == game->enPassantCol) {
        tempBoard[game->enPassantPawnRow][game->enPassantPawnCol] = ' ';
    }

    tempBoard[toR][toC] = tempBoard[fromR][fromC];
    tempBoard[fromR][fromC] = ' ';

    /* Handle pawn promotion in simulation */
    if (piece == 'P' && toR == 0) tempBoard[toR][toC] = 'Q';
    if (piece == 'p' && toR == 7) tempBoard[toR][toC] = 'q';

    /* Move is legal only if own king is not in check afterwards */
    return !isKingInCheck(tempBoard, game->currentPlayer);
}

bool hasAnyLegalMoves(const GameState *game, int player)
{
    GameState tempState = *game;
    tempState.currentPlayer = player;

    int fr, fc, tr, tc;
    for (fr = 0; fr < BOARD_SIZE; fr++) {
        for (fc = 0; fc < BOARD_SIZE; fc++) {
            char p = tempState.board[fr][fc];
            if (!isPlayersPiece(p, player)) continue;

            for (tr = 0; tr < BOARD_SIZE; tr++) {
                for (tc = 0; tc < BOARD_SIZE; tc++) {
                    if (isMoveLegal(&tempState, fr, fc, tr, tc)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void calculateLegalMoves(GameState *game, int fromR, int fromC)
{
    clearLegalMoves(game);
    if (fromR < 0 || fromR >= BOARD_SIZE || fromC < 0 || fromC >= BOARD_SIZE) return;

    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            game->legalMoves[r][c] = isMoveLegal(game, fromR, fromC, r, c);
        }
    }
}

void clearLegalMoves(GameState *game)
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            game->legalMoves[r][c] = false;
        }
    }
}

void updateCheckAndCheckmateStatus(GameState *game)
{
    game->isCheck[0] = isKingInCheck(game->board, 0);
    game->isCheck[1] = isKingInCheck(game->board, 1);

    int curr = game->currentPlayer;
    if (!hasAnyLegalMoves(game, curr)) {
        if (game->isCheck[curr]) {
            game->isCheckmate[curr] = true;
            game->winner = 1 - curr;
            game->winReason = WIN_CHECKMATE;
            game->phase = PHASE_GAME_OVER;
            printf("CHECKMATE! Player %d wins the match!\n", game->winner + 1);
        } else {
            /* Stalemate: decider by higher material or boxing HP */
            printf("STALEMATE! Ending match.\n");
            if (game->materialScore[0] > game->materialScore[1]) {
                game->winner = 0;
            } else if (game->materialScore[1] > game->materialScore[0]) {
                game->winner = 1;
            } else {
                game->winner = (game->boxing.boxers[0].health >= game->boxing.boxers[1].health) ? 0 : 1;
            }
            game->winReason = WIN_CHECKMATE;
            game->phase = PHASE_GAME_OVER;
        }
    }
}

static void recordCapturedPiece(GameState *game, int captor, char piece)
{
    if (piece == ' ') return;
    if (game->capturedCount[captor] < 31) {
        int idx = game->capturedCount[captor];
        game->capturedPieces[captor][idx] = piece;
        game->capturedPieces[captor][idx + 1] = '\0';
        game->capturedCount[captor]++;
    }
    game->materialScore[captor] += getPieceValue(piece);
    printf("Player %d captured %c! (Value: %d, Total Material: %d)\n",
           captor + 1, piece, getPieceValue(piece), game->materialScore[captor]);
}

void completePawnPromotion(GameState *game, char pieceChoice)
{
    if (!game->isPromoting || game->promotionRow < 0 || game->promotionCol < 0) return;

    char chosen = (game->promotionPawnPlayer == 0) ? (char)toupper(pieceChoice) : (char)tolower(pieceChoice);
    game->board[game->promotionRow][game->promotionCol] = chosen;
    printf("Pawn promoted to %c!\n", chosen);

    game->isPromoting = false;
    game->promotionRow = -1;
    game->promotionCol = -1;
    game->promotionPawnPlayer = -1;

    /* Switch turn now */
    game->currentPlayer = 1 - game->currentPlayer;
    updateCheckAndCheckmateStatus(game);
}

bool executeChessMove(GameState *game, int fromR, int fromC, int toR, int toC)
{
    if (game->isPromoting) return false;

    if (!isMoveLegal(game, fromR, fromC, toR, toC)) {
        return false;
    }

    char moving = game->board[fromR][fromC];
    char captured = game->board[toR][toC];
    int curr = game->currentPlayer;

    /* Handle Castling move execution */
    if (isCastlingMove(game, fromR, fromC, toR, toC)) {
        int row = fromR;
        if (toC == 6) {
            /* Kingside O-O */
            game->board[row][6] = moving;
            game->board[row][4] = ' ';
            game->board[row][5] = game->board[row][7];
            game->board[row][7] = ' ';
            if (curr == 0) {
                game->whiteKingMoved = true;
                game->whiteKingsideRookMoved = true;
            } else {
                game->blackKingMoved = true;
                game->blackKingsideRookMoved = true;
            }
            printf("Player %d performed Kingside Castling (O-O)!\n", curr + 1);
        } else if (toC == 2) {
            /* Queenside O-O-O */
            game->board[row][2] = moving;
            game->board[row][4] = ' ';
            game->board[row][3] = game->board[row][0];
            game->board[row][0] = ' ';
            if (curr == 0) {
                game->whiteKingMoved = true;
                game->whiteQueensideRookMoved = true;
            } else {
                game->blackKingMoved = true;
                game->blackQueensideRookMoved = true;
            }
            printf("Player %d performed Queenside Castling (O-O-O)!\n", curr + 1);
        }
    }
    else {
        /* Standard or En Passant move */

        /* En Passant capture execution */
        if ((moving == 'P' || moving == 'p') && toR == game->enPassantRow && toC == game->enPassantCol) {
            char epCaptured = game->board[game->enPassantPawnRow][game->enPassantPawnCol];
            recordCapturedPiece(game, curr, epCaptured);
            game->board[game->enPassantPawnRow][game->enPassantPawnCol] = ' ';
            printf("En Passant capture!\n");
        } else if (captured != ' ') {
            recordCapturedPiece(game, curr, captured);
        }

        /* Move piece */
        game->board[toR][toC] = moving;
        game->board[fromR][fromC] = ' ';

        /* Track Rook / King movement to invalidate castling */
        if (moving == 'K') game->whiteKingMoved = true;
        if (moving == 'k') game->blackKingMoved = true;
        if (moving == 'R' && fromR == 7 && fromC == 7) game->whiteKingsideRookMoved = true;
        if (moving == 'R' && fromR == 7 && fromC == 0) game->whiteQueensideRookMoved = true;
        if (moving == 'r' && fromR == 0 && fromC == 7) game->blackKingsideRookMoved = true;
        if (moving == 'r' && fromR == 0 && fromC == 0) game->blackQueensideRookMoved = true;

        /* Also check if rook was captured on original square */
        if (toR == 7 && toC == 7) game->whiteKingsideRookMoved = true;
        if (toR == 7 && toC == 0) game->whiteQueensideRookMoved = true;
        if (toR == 0 && toC == 7) game->blackKingsideRookMoved = true;
        if (toR == 0 && toC == 0) game->blackQueensideRookMoved = true;
    }

    /* Check if move opens en passant for next turn (2-square pawn jump) */
    if ((moving == 'P' || moving == 'p') && abs(toR - fromR) == 2) {
        int dir = (moving == 'P') ? -1 : 1;
        game->enPassantRow = fromR + dir;
        game->enPassantCol = fromC;
        game->enPassantPawnRow = toR;
        game->enPassantPawnCol = toC;
    } else {
        game->enPassantRow = -1;
        game->enPassantCol = -1;
        game->enPassantPawnRow = -1;
        game->enPassantPawnCol = -1;
    }

    /* Clear selection & drag */
    game->selectedRow = -1;
    game->selectedCol = -1;
    game->isDragging = false;
    game->dragStartRow = -1;
    game->dragStartCol = -1;
    clearLegalMoves(game);

    /* Check for Pawn Promotion */
    if (moving == 'P' && toR == 0) {
        game->isPromoting = true;
        game->promotionRow = toR;
        game->promotionCol = toC;
        game->promotionPawnPlayer = 0;
        printf("White Pawn promotion triggered! Select piece.\n");
        return true; /* Wait for player to choose piece */
    } else if (moving == 'p' && toR == 7) {
        game->isPromoting = true;
        game->promotionRow = toR;
        game->promotionCol = toC;
        game->promotionPawnPlayer = 1;
        printf("Black Pawn promotion triggered! Select piece.\n");
        return true; /* Wait for player to choose piece */
    }

    /* Switch turn */
    game->currentPlayer = 1 - game->currentPlayer;

    /* Update check and checkmate status for the new turn */
    updateCheckAndCheckmateStatus(game);

    return true;
}