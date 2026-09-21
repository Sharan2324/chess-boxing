#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "game.h"

void drawText(SDL_Renderer *renderer, TTF_Font *font, const char *text, float x, float y, SDL_Color color, bool center);
void renderGame(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game);

bool isPointInRect(float px, float py, const SDL_FRect *r);
void getMenuButtonRect(int index, SDL_FRect *outRect);
void getDifficultyButtonRect(int index, SDL_FRect *outRect);
void getDifficultyBackRect(SDL_FRect *outRect);
void getNetSelectButtonRect(int index, SDL_FRect *outRect);
void getNetSelectBackRect(SDL_FRect *outRect);
void getNetHostBackRect(SDL_FRect *outRect);
void getNetJoinInputRect(SDL_FRect *outRect);
void getNetConnectButtonRect(SDL_FRect *outRect);
void getNetJoinBackRect(SDL_FRect *outRect);
void getPromotionButtonRect(int index, SDL_FRect *outRect);
void getHowToPlayBackRect(SDL_FRect *outRect);

#endif