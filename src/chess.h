#ifndef CHESS_H
#define CHESS_H

#include "game.h"
#include <stdbool.h>

void initChessBoard(GameState *game);
char getPiece(const GameState *game, int row, int col);
bool isWhitePiece(char p);
bool isBlackPiece(char p);
bool isPlayersPiece(char p, int player);

bool mouseToBoardSquare(int mouseX, int mouseY, int *row, int *col);

bool isSquareAttacked(const char board[BOARD_SIZE][BOARD_SIZE], int targetRow, int targetCol, int attackerPlayer);
bool findKing(const char board[BOARD_SIZE][BOARD_SIZE], int player, int *outRow, int *outCol);
bool isKingInCheck(const char board[BOARD_SIZE][BOARD_SIZE], int player);

bool isCastlingMove(const GameState *game, int fromR, int fromC, int toR, int toC);
bool isMoveMechanicallyLegal(const GameState *game, int fromR, int fromC, int toR, int toC);
bool isMoveLegal(const GameState *game, int fromR, int fromC, int toR, int toC);

bool hasAnyLegalMoves(const GameState *game, int player);
void calculateLegalMoves(GameState *game, int fromR, int fromC);
void clearLegalMoves(GameState *game);

bool executeChessMove(GameState *game, int fromR, int fromC, int toR, int toC);
void completePawnPromotion(GameState *game, char pieceChoice);
void updateCheckAndCheckmateStatus(GameState *game);

#endif