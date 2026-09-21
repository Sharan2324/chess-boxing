#include "renderer.h"
#include "chess.h"
#include "boxing.h"
#include "network.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool isPointInRect(float px, float py, const SDL_FRect *r)
{
    return (px >= r->x && px <= r->x + r->w && py >= r->y && py <= r->y + r->h);
}

void getMenuButtonRect(int index, SDL_FRect *outRect)
{
    outRect->w = 360.0f;
    outRect->h = 46.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 330.0f + (float)index * 58.0f;
}

void getDifficultyButtonRect(int index, SDL_FRect *outRect)
{
    outRect->w = 540.0f;
    outRect->h = 64.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 330.0f + (float)index * 80.0f;
}

void getDifficultyBackRect(SDL_FRect *outRect)
{
    outRect->w = 240.0f;
    outRect->h = 46.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 600.0f;
}

void getNetSelectButtonRect(int index, SDL_FRect *outRect)
{
    outRect->w = 500.0f;
    outRect->h = 68.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 350.0f + (float)index * 90.0f;
}

void getNetSelectBackRect(SDL_FRect *outRect)
{
    outRect->w = 240.0f;
    outRect->h = 46.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 560.0f;
}

void getNetHostBackRect(SDL_FRect *outRect)
{
    outRect->w = 240.0f;
    outRect->h = 46.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 550.0f;
}

void getNetJoinInputRect(SDL_FRect *outRect)
{
    outRect->w = 380.0f;
    outRect->h = 50.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 360.0f;
}

void getNetConnectButtonRect(SDL_FRect *outRect)
{
    outRect->w = 260.0f;
    outRect->h = 50.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 435.0f;
}

void getNetJoinBackRect(SDL_FRect *outRect)
{
    outRect->w = 240.0f;
    outRect->h = 46.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 550.0f;
}

void getHowToPlayBackRect(SDL_FRect *outRect)
{
    outRect->w = 240.0f;
    outRect->h = 48.0f;
    outRect->x = (WINDOW_WIDTH - outRect->w) * 0.5f;
    outRect->y = 730.0f;
}

void getPromotionButtonRect(int index, SDL_FRect *outRect)
{
    /* 4 buttons: Queen, Rook, Bishop, Knight */
    float totalW = 4.0f * 90.0f + 3.0f * 14.0f;
    float startX = (WINDOW_WIDTH - totalW) * 0.5f;
    outRect->x = startX + (float)index * (90.0f + 14.0f);
    outRect->y = WINDOW_HEIGHT * 0.5f - 20.0f;
    outRect->w = 90.0f;
    outRect->h = 90.0f;
}

static const char* getPieceSymbol(char piece)
{
    switch (piece) {
        /* White pieces */
        case 'K': return "♔";
        case 'Q': return "♕";
        case 'R': return "♖";
        case 'B': return "♗";
        case 'N': return "♘";
        case 'P': return "♙";

        /* Black pieces */
        case 'k': return "♚";
        case 'q': return "♛";
        case 'r': return "♜";
        case 'b': return "♝";
        case 'n': return "♞";
        case 'p': return "♟";

        default: return "";
    }
}

void drawText(SDL_Renderer *renderer, TTF_Font *font, const char *text, float x, float y, SDL_Color color, bool center)
{
    if (!font || !text || text[0] == '\0') return;

    SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, color);
    if (!surf) return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_FRect dst;
        dst.w = (float)surf->w;
        dst.h = (float)surf->h;
        dst.x = center ? (x - dst.w * 0.5f) : x;
        dst.y = center ? (y - dst.h * 0.5f) : y;

        SDL_RenderTexture(renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(surf);
}

/* -------------------------------------------------------------
   MAIN MENU RENDERING
   ------------------------------------------------------------- */

static void renderMainMenu(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont)
{
    /* Background stadium lighting */
    SDL_FRect bg = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 12, 14, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    /* Decorative spotlights */
    SDL_FRect beam1 = { 150, 0, 260, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 35, 45, 65, 30);
    SDL_RenderFillRect(renderer, &beam1);

    SDL_FRect beam2 = { 590, 0, 260, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 35, 45, 65, 30);
    SDL_RenderFillRect(renderer, &beam2);

    /* Decorative chess pieces at top */
    drawText(renderer, chessFont, "♔", 320, 130, (SDL_Color){255, 255, 255, 180}, true);
    drawText(renderer, chessFont, "🥊", 500, 130, (SDL_Color){240, 60, 60, 220}, true);
    drawText(renderer, chessFont, "♚", 680, 130, (SDL_Color){180, 180, 180, 180}, true);

    /* Title */
    drawText(renderer, chessFont, "CHESSBOXING", WINDOW_WIDTH * 0.5f, 200, (SDL_Color){255, 215, 0, 255}, true);
    drawText(renderer, uiFont, "THE ULTIMATE TEST OF MIND AND MUSCLE", WINDOW_WIDTH * 0.5f, 250, (SDL_Color){200, 210, 225, 255}, true);

    /* Menu Buttons */
    const char *labels[5] = {
        "1. SINGLE PLAYER (VS AI)",
        "2. LOCAL 2 PLAYER",
        "3. NETWORK 2 PLAYER (LAN)",
        "4. HOW TO PLAY",
        "5. EXIT GAME"
    };

    for (int i = 0; i < 5; i++) {
        SDL_FRect btn;
        getMenuButtonRect(i, &btn);

        SDL_SetRenderDrawColor(renderer, 26, 32, 46, 240);
        SDL_RenderFillRect(renderer, &btn);

        if (i == 0) SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        else if (i == 1) SDL_SetRenderDrawColor(renderer, 90, 160, 255, 240);
        else if (i == 2) SDL_SetRenderDrawColor(renderer, 80, 220, 180, 240);
        else if (i == 3) SDL_SetRenderDrawColor(renderer, 180, 180, 200, 220);
        else SDL_SetRenderDrawColor(renderer, 220, 80, 80, 220);
        SDL_RenderRect(renderer, &btn);

        SDL_Color txtCol = (i == 0) ? (SDL_Color){255, 220, 80, 255} : (SDL_Color){230, 235, 245, 255};
        drawText(renderer, uiFont, labels[i], btn.x + btn.w * 0.5f, btn.y + btn.h * 0.5f, txtCol, true);
    }

    drawText(renderer, uiFont, "Click option or press 1 - 5 / ESC", WINDOW_WIDTH * 0.5f, 645, (SDL_Color){130, 140, 155, 255}, true);
}

static void renderDifficultySelect(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont)
{
    SDL_FRect bg = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 12, 14, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    drawText(renderer, chessFont, "SELECT AI DIFFICULTY", WINDOW_WIDTH * 0.5f, 180, (SDL_Color){255, 215, 0, 255}, true);
    drawText(renderer, uiFont, "CHOOSE YOUR OPPONENT'S SKILL LEVEL", WINDOW_WIDTH * 0.5f, 235, (SDL_Color){180, 200, 225, 255}, true);

    const char *titles[3] = {
        "1. ROOKIE (EASY)",
        "2. CONTENDER (MEDIUM)",
        "3. GRANDMASTER (HARD)"
    };
    const char *descs[3] = {
        "Random moves * Casual punches * Relaxed spacing",
        "1-ply tactics & captures * Tactical blocks & dodges * Balanced boxing",
        "3-ply Minimax + Alpha-Beta * Crouch duck combos * Elite spacing & counters"
    };

    for (int i = 0; i < 3; i++) {
        SDL_FRect btn;
        getDifficultyButtonRect(i, &btn);

        SDL_SetRenderDrawColor(renderer, 26, 32, 46, 240);
        SDL_RenderFillRect(renderer, &btn);

        if (i == 0) SDL_SetRenderDrawColor(renderer, 100, 220, 120, 240);
        else if (i == 1) SDL_SetRenderDrawColor(renderer, 255, 200, 60, 240);
        else SDL_SetRenderDrawColor(renderer, 255, 80, 80, 240);
        SDL_RenderRect(renderer, &btn);

        SDL_Color colTitle = (i == 0) ? (SDL_Color){120, 240, 140, 255} :
                             (i == 1) ? (SDL_Color){255, 215, 80, 255} : (SDL_Color){255, 110, 110, 255};
        drawText(renderer, uiFont, titles[i], btn.x + btn.w * 0.5f, btn.y + 18, colTitle, true);
        drawText(renderer, uiFont, descs[i], btn.x + btn.w * 0.5f, btn.y + 42, (SDL_Color){170, 185, 205, 255}, true);
    }

    /* Back Button */
    SDL_FRect backBtn;
    getDifficultyBackRect(&backBtn);
    SDL_SetRenderDrawColor(renderer, 24, 28, 38, 240);
    SDL_RenderFillRect(renderer, &backBtn);
    SDL_SetRenderDrawColor(renderer, 120, 130, 150, 220);
    SDL_RenderRect(renderer, &backBtn);
    drawText(renderer, uiFont, "BACK TO MENU (ESC)", backBtn.x + backBtn.w * 0.5f, backBtn.y + backBtn.h * 0.5f, (SDL_Color){200, 210, 220, 255}, true);
}

static void renderNetSelect(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont)
{
    SDL_FRect bg = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 12, 14, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    drawText(renderer, chessFont, "NETWORK 2 PLAYER", WINDOW_WIDTH * 0.5f, 180, (SDL_Color){80, 220, 180, 255}, true);
    drawText(renderer, uiFont, "PLAY ACROSS TWO LAPTOPS ON THE SAME WI-FI / LAN", WINDOW_WIDTH * 0.5f, 235, (SDL_Color){180, 200, 225, 255}, true);

    const char *titles[2] = {
        "1. HOST MATCH (PLAYER 1 - WHITE)",
        "2. JOIN MATCH (PLAYER 2 - BLACK)"
    };
    const char *descs[2] = {
        "Start a server on your laptop and wait for Player 2 to connect",
        "Connect to a Host laptop using its local Wi-Fi / LAN IP address"
    };

    for (int i = 0; i < 2; i++) {
        SDL_FRect btn;
        getNetSelectButtonRect(i, &btn);

        SDL_SetRenderDrawColor(renderer, 26, 32, 46, 240);
        SDL_RenderFillRect(renderer, &btn);

        if (i == 0) SDL_SetRenderDrawColor(renderer, 80, 220, 180, 240);
        else SDL_SetRenderDrawColor(renderer, 90, 160, 255, 240);
        SDL_RenderRect(renderer, &btn);

        SDL_Color colTitle = (i == 0) ? (SDL_Color){100, 240, 200, 255} : (SDL_Color){120, 190, 255, 255};
        drawText(renderer, uiFont, titles[i], btn.x + btn.w * 0.5f, btn.y + 20, colTitle, true);
        drawText(renderer, uiFont, descs[i], btn.x + btn.w * 0.5f, btn.y + 44, (SDL_Color){170, 185, 205, 255}, true);
    }

    /* Back Button */
    SDL_FRect backBtn;
    getNetSelectBackRect(&backBtn);
    SDL_SetRenderDrawColor(renderer, 24, 28, 38, 240);
    SDL_RenderFillRect(renderer, &backBtn);
    SDL_SetRenderDrawColor(renderer, 120, 130, 150, 220);
    SDL_RenderRect(renderer, &backBtn);
    drawText(renderer, uiFont, "BACK TO MENU (ESC)", backBtn.x + backBtn.w * 0.5f, backBtn.y + backBtn.h * 0.5f, (SDL_Color){200, 210, 220, 255}, true);
}

static void renderNetHostWait(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game)
{
    SDL_FRect bg = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 12, 14, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    drawText(renderer, chessFont, "HOSTING LAN MATCH", WINDOW_WIDTH * 0.5f, 170, (SDL_Color){80, 220, 180, 255}, true);
    drawText(renderer, uiFont, "YOU ARE PLAYER 1 (WHITE)", WINDOW_WIDTH * 0.5f, 220, (SDL_Color){255, 215, 0, 255}, true);

    /* Info card */
    SDL_FRect card = { WINDOW_WIDTH * 0.5f - 260, 260, 520, 230 };
    SDL_SetRenderDrawColor(renderer, 22, 28, 40, 245);
    SDL_RenderFillRect(renderer, &card);
    SDL_SetRenderDrawColor(renderer, 80, 220, 180, 200);
    SDL_RenderRect(renderer, &card);

    char ipStr[128];
    snprintf(ipStr, sizeof(ipStr), "YOUR LAN IP:  %s", game->hostLocalIp);
    drawText(renderer, uiFont, ipStr, WINDOW_WIDTH * 0.5f, card.y + 40, (SDL_Color){255, 255, 255, 255}, true);

    drawText(renderer, uiFont, "PORT:  5000", WINDOW_WIDTH * 0.5f, card.y + 75, (SDL_Color){200, 220, 240, 255}, true);

    char statusStr[128];
    snprintf(statusStr, sizeof(statusStr), "STATUS:  %s", net_get_status_message());
    drawText(renderer, uiFont, statusStr, WINDOW_WIDTH * 0.5f, card.y + 115, (SDL_Color){255, 215, 80, 255}, true);

    drawText(renderer, uiFont, "Have Player 2 select 'Join Match' and enter this IP!", WINDOW_WIDTH * 0.5f, card.y + 165, (SDL_Color){160, 180, 200, 255}, true);

    /* Cancel Button */
    SDL_FRect cancelBtn;
    getNetHostBackRect(&cancelBtn);
    SDL_SetRenderDrawColor(renderer, 24, 28, 38, 240);
    SDL_RenderFillRect(renderer, &cancelBtn);
    SDL_SetRenderDrawColor(renderer, 220, 80, 80, 220);
    SDL_RenderRect(renderer, &cancelBtn);
    drawText(renderer, uiFont, "CANCEL (ESC)", cancelBtn.x + cancelBtn.w * 0.5f, cancelBtn.y + cancelBtn.h * 0.5f, (SDL_Color){255, 120, 120, 255}, true);
}

static void renderNetJoinInput(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game)
{
    SDL_FRect bg = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 12, 14, 20, 255);
    SDL_RenderFillRect(renderer, &bg);

    drawText(renderer, chessFont, "JOIN LAN MATCH", WINDOW_WIDTH * 0.5f, 170, (SDL_Color){90, 160, 255, 255}, true);
    drawText(renderer, uiFont, "YOU ARE PLAYER 2 (BLACK)", WINDOW_WIDTH * 0.5f, 220, (SDL_Color){255, 120, 120, 255}, true);

    drawText(renderer, uiFont, "Enter Host Laptop IP Address:", WINDOW_WIDTH * 0.5f, 310, (SDL_Color){220, 230, 245, 255}, true);

    /* Text Input Box */
    SDL_FRect inBox;
    getNetJoinInputRect(&inBox);
    SDL_SetRenderDrawColor(renderer, 16, 20, 30, 255);
    SDL_RenderFillRect(renderer, &inBox);
    SDL_SetRenderDrawColor(renderer, 90, 160, 255, 255);
    SDL_RenderRect(renderer, &inBox);

    /* Display entered text with cursor */
    char displayStr[128];
    bool cursorOn = ((SDL_GetTicks() / 500) % 2 == 0);
    snprintf(displayStr, sizeof(displayStr), "%s%s", game->netIpInput, cursorOn ? "|" : " ");
    drawText(renderer, uiFont, displayStr, inBox.x + inBox.w * 0.5f, inBox.y + inBox.h * 0.5f, (SDL_Color){255, 255, 255, 255}, true);

    /* Connect Button */
    SDL_FRect connBtn;
    getNetConnectButtonRect(&connBtn);
    SDL_SetRenderDrawColor(renderer, 24, 48, 80, 240);
    SDL_RenderFillRect(renderer, &connBtn);
    SDL_SetRenderDrawColor(renderer, 80, 180, 255, 255);
    SDL_RenderRect(renderer, &connBtn);
    drawText(renderer, uiFont, "CONNECT TO HOST", connBtn.x + connBtn.w * 0.5f, connBtn.y + connBtn.h * 0.5f, (SDL_Color){220, 240, 255, 255}, true);

    /* Status */
    char statusStr[128];
    snprintf(statusStr, sizeof(statusStr), "Status: %s", net_get_status_message());
    drawText(renderer, uiFont, statusStr, WINDOW_WIDTH * 0.5f, 500, (SDL_Color){255, 215, 80, 255}, true);

    /* Back Button */
    SDL_FRect backBtn;
    getNetJoinBackRect(&backBtn);
    SDL_SetRenderDrawColor(renderer, 24, 28, 38, 240);
    SDL_RenderFillRect(renderer, &backBtn);
    SDL_SetRenderDrawColor(renderer, 120, 130, 150, 220);
    SDL_RenderRect(renderer, &backBtn);
    drawText(renderer, uiFont, "BACK (ESC)", backBtn.x + backBtn.w * 0.5f, backBtn.y + backBtn.h * 0.5f, (SDL_Color){200, 210, 220, 255}, true);
}

/* -------------------------------------------------------------
   HOW TO PLAY SCREEN
   ------------------------------------------------------------- */

static void renderHowToPlay(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont)
{
    SDL_FRect bg = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 14, 16, 22, 255);
    SDL_RenderFillRect(renderer, &bg);

    drawText(renderer, chessFont, "HOW TO PLAY CHESSBOXING", WINDOW_WIDTH * 0.5f, 45, (SDL_Color){255, 215, 0, 255}, true);

    /* Card 1: Chess */
    SDL_FRect card1 = { 40, 95, 440, 380 };
    SDL_SetRenderDrawColor(renderer, 22, 26, 36, 240);
    SDL_RenderFillRect(renderer, &card1);
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 200);
    SDL_RenderRect(renderer, &card1);

    drawText(renderer, uiFont, "♟ CHESS ROUNDS (60s)", card1.x + 20, card1.y + 24, (SDL_Color){255, 220, 80, 255}, false);
    drawText(renderer, uiFont, "• Mouse: Drag & Drop or Click-to-Move.", card1.x + 20, card1.y + 60, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "• Full Rules: Castling (O-O, O-O-O) enabled!", card1.x + 20, card1.y + 90, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "• En Passant capture fully supported.", card1.x + 20, card1.y + 120, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "• Promotion: Choose Queen, Rook, Bishop, Knight.", card1.x + 20, card1.y + 150, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "• Total clock: 3:00 per player.", card1.x + 20, card1.y + 180, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "★ WIN CONDITION: CHECKMATE = INSTANT VICTORY!", card1.x + 20, card1.y + 220, (SDL_Color){100, 220, 255, 255}, false);

    /* Card 2: Boxing */
    SDL_FRect card2 = { 520, 95, 440, 380 };
    SDL_SetRenderDrawColor(renderer, 22, 26, 36, 240);
    SDL_RenderFillRect(renderer, &card2);
    SDL_SetRenderDrawColor(renderer, 240, 70, 70, 200);
    SDL_RenderRect(renderer, &card2);

    drawText(renderer, uiFont, "🥊 BOXING ROUNDS (45s)", card2.x + 20, card2.y + 24, (SDL_Color){255, 100, 100, 255}, false);
    drawText(renderer, uiFont, "Player 1 (White):", card2.x + 20, card2.y + 60, (SDL_Color){255, 215, 0, 255}, false);
    drawText(renderer, uiFont, "  A/D: Move | W: Jump | S: Crouch | Q: Dodge", card2.x + 20, card2.y + 85, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "  F: Light | G: Heavy | V: Uppercut | H: Block", card2.x + 20, card2.y + 110, (SDL_Color){220, 220, 230, 255}, false);

    drawText(renderer, uiFont, "Player 2 (Black):", card2.x + 20, card2.y + 145, (SDL_Color){255, 120, 120, 255}, false);
    drawText(renderer, uiFont, "  Arrows: Move/Jump/Crouch | /: Dodge", card2.x + 20, card2.y + 170, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "  K: Light | L: Heavy | .: Uppercut | ;: Block", card2.x + 20, card2.y + 195, (SDL_Color){220, 220, 230, 255}, false);

    drawText(renderer, uiFont, "• Crouch ducks high punches! Uppercuts hit crouch!", card2.x + 20, card2.y + 230, (SDL_Color){255, 200, 100, 255}, false);
    drawText(renderer, uiFont, "• Heavy punches can trigger GUARD BREAK on blocks!", card2.x + 20, card2.y + 260, (SDL_Color){255, 150, 220, 255}, false);
    drawText(renderer, uiFont, "★ WIN CONDITION: KNOCKOUT (0 HP) = INSTANT VICTORY!", card2.x + 20, card2.y + 300, (SDL_Color){255, 60, 60, 255}, false);

    /* Card 3: Chess <-> Boxing Synergy */
    SDL_FRect card3 = { 40, 500, 920, 195 };
    SDL_SetRenderDrawColor(renderer, 20, 24, 34, 240);
    SDL_RenderFillRect(renderer, &card3);
    SDL_SetRenderDrawColor(renderer, 100, 180, 255, 200);
    SDL_RenderRect(renderer, &card3);

    drawText(renderer, uiFont, "🔥 CHESS ↔ BOXING ADVANTAGE SYNERGY", card3.x + 20, card3.y + 24, (SDL_Color){100, 220, 255, 255}, false);
    drawText(renderer, uiFont, "• Capturing pieces earns Material: Pawn=1, Knight/Bishop=3, Rook=5, Queen=9.", card3.x + 20, card3.y + 60, (SDL_Color){220, 220, 230, 255}, false);
    drawText(renderer, uiFont, "• Material Advantage gives your Boxer +3.5% PUNCH DAMAGE per point (up to +35%)!", card3.x + 20, card3.y + 90, (SDL_Color){255, 215, 0, 255}, false);
    drawText(renderer, uiFont, "• Material Advantage also grants +3 MAX STAMINA per point (up to 130 Max Stamina).", card3.x + 20, card3.y + 120, (SDL_Color){150, 255, 150, 255}, false);
    drawText(renderer, uiFont, "• Play aggressively on the board to gain decisive punching power in the ring!", card3.x + 20, card3.y + 150, (SDL_Color){255, 180, 100, 255}, false);

    /* Back Button */
    SDL_FRect backBtn;
    getHowToPlayBackRect(&backBtn);
    SDL_SetRenderDrawColor(renderer, 35, 42, 58, 240);
    SDL_RenderFillRect(renderer, &backBtn);
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderRect(renderer, &backBtn);
    drawText(renderer, uiFont, "[ BACK TO MENU ]", backBtn.x + backBtn.w * 0.5f, backBtn.y + backBtn.h * 0.5f, (SDL_Color){255, 215, 0, 255}, true);
}

/* -------------------------------------------------------------
   CHESS RENDERING
   ------------------------------------------------------------- */

static void renderChessBoardAndPieces(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game)
{
    /* Board border frame */
    SDL_FRect frameRect = { BOARD_X - 16, BOARD_Y - 16, BOARD_SIZE * SQUARE_SIZE + 32, BOARD_SIZE * SQUARE_SIZE + 32 };
    SDL_SetRenderDrawColor(renderer, 20, 22, 28, 255);
    SDL_RenderFillRect(renderer, &frameRect);

    SDL_FRect innerFrame = { BOARD_X - 6, BOARD_Y - 6, BOARD_SIZE * SQUARE_SIZE + 12, BOARD_SIZE * SQUARE_SIZE + 12 };
    SDL_SetRenderDrawColor(renderer, 90, 70, 50, 255);
    SDL_RenderFillRect(renderer, &innerFrame);

    /* Coordinates */
    char coordBuf[4];
    SDL_Color coordColor = { 180, 170, 160, 255 };
    for (int i = 0; i < BOARD_SIZE; i++) {
        coordBuf[0] = 'A' + i; coordBuf[1] = '\0';
        drawText(renderer, uiFont, coordBuf, BOARD_X + i * SQUARE_SIZE + SQUARE_SIZE * 0.5f, BOARD_Y + BOARD_SIZE * SQUARE_SIZE + 10, coordColor, true);

        coordBuf[0] = '8' - i; coordBuf[1] = '\0';
        drawText(renderer, uiFont, coordBuf, BOARD_X - 12, BOARD_Y + i * SQUARE_SIZE + SQUARE_SIZE * 0.5f, coordColor, true);
    }

    /* Squares & Pieces */
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            SDL_FRect sq = {
                BOARD_X + col * SQUARE_SIZE,
                BOARD_Y + row * SQUARE_SIZE,
                SQUARE_SIZE,
                SQUARE_SIZE
            };

            if ((row + col) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 240, 218, 181, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
            }
            SDL_RenderFillRect(renderer, &sq);

            /* Selected square highlight */
            if (row == game->selectedRow && col == game->selectedCol) {
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 150);
                SDL_RenderFillRect(renderer, &sq);
            }

            /* King in Check Highlight */
            char pieceAtSq = game->board[row][col];
            if ((pieceAtSq == 'K' && game->isCheck[0]) || (pieceAtSq == 'k' && game->isCheck[1])) {
                SDL_SetRenderDrawColor(renderer, 230, 40, 40, 180);
                SDL_RenderFillRect(renderer, &sq);
            }

            /* Legal Move Destination Highlights */
            if (game->legalMoves[row][col]) {
                if (pieceAtSq == ' ' && !(row == game->enPassantRow && col == game->enPassantCol)) {
                    SDL_FRect dot = { sq.x + SQUARE_SIZE * 0.38f, sq.y + SQUARE_SIZE * 0.38f, SQUARE_SIZE * 0.24f, SQUARE_SIZE * 0.24f };
                    SDL_SetRenderDrawColor(renderer, 30, 180, 60, 190);
                    SDL_RenderFillRect(renderer, &dot);
                } else {
                    /* Capture target frame */
                    SDL_SetRenderDrawColor(renderer, 220, 50, 50, 130);
                    SDL_RenderFillRect(renderer, &sq);
                    SDL_SetRenderDrawColor(renderer, 255, 40, 40, 220);
                    SDL_RenderRect(renderer, &sq);
                }
            }

            /* Piece on square */
            if (pieceAtSq != ' ') {
                if (game->isDragging && row == game->dragStartRow && col == game->dragStartCol) {
                    SDL_FRect faintSq = { sq.x + 10, sq.y + 10, 60, 60 };
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40);
                    SDL_RenderFillRect(renderer, &faintSq);
                } else {
                    const char *sym = getPieceSymbol(pieceAtSq);
                    SDL_Color colText = isWhitePiece(pieceAtSq) ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){20, 20, 20, 255};
                    SDL_Color shadowCol = {40, 40, 40, 100};
                    drawText(renderer, chessFont, sym, sq.x + SQUARE_SIZE * 0.5f + 2, sq.y + SQUARE_SIZE * 0.5f + 2, shadowCol, true);
                    drawText(renderer, chessFont, sym, sq.x + SQUARE_SIZE * 0.5f, sq.y + SQUARE_SIZE * 0.5f, colText, true);
                }
            }
        }
    }

    /* Dragged Piece floating above cursor */
    if (game->isDragging && game->dragStartRow >= 0 && game->dragStartCol >= 0) {
        char draggedPiece = game->board[game->dragStartRow][game->dragStartCol];
        if (draggedPiece != ' ') {
            const char *sym = getPieceSymbol(draggedPiece);
            SDL_Color colText = isWhitePiece(draggedPiece) ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){20, 20, 20, 255};
            SDL_Color shadowCol = {10, 10, 10, 120};
            drawText(renderer, chessFont, sym, (float)game->dragCurrentX + 6, (float)game->dragCurrentY + 6, shadowCol, true);
            drawText(renderer, chessFont, sym, (float)game->dragCurrentX, (float)game->dragCurrentY, colText, true);
        }
    }
}

static void renderChessHUD(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game)
{
    char buf[128];

    /* Top Bar Panel */
    SDL_FRect topBar = { 0, 0, WINDOW_WIDTH, 85 };
    SDL_SetRenderDrawColor(renderer, 22, 24, 30, 255);
    SDL_RenderFillRect(renderer, &topBar);

    /* Player 1 Card (Left) */
    int wMins = (int)game->whiteClock / 60;
    int wSecs = (int)game->whiteClock % 60;
    snprintf(buf, sizeof(buf), "P1 [WHITE]  %02d:%02d", wMins, wSecs);
    SDL_Color p1Col = (game->currentPlayer == 0) ? (SDL_Color){255, 220, 100, 255} : (SDL_Color){180, 180, 180, 255};
    drawText(renderer, uiFont, buf, 30, 18, p1Col, false);

    snprintf(buf, sizeof(buf), "HP: %.0f  |  Material: %d pts", game->boxing.boxers[0].health, game->materialScore[0]);
    drawText(renderer, uiFont, buf, 30, 44, (SDL_Color){160, 220, 160, 255}, false);

    /* Captured pieces tally for P1 */
    if (game->capturedCount[0] > 0) {
        char p1Caps[64] = "";
        for (int i = 0; i < game->capturedCount[0] && i < 12; i++) {
            strcat(p1Caps, getPieceSymbol(game->capturedPieces[0][i]));
            strcat(p1Caps, " ");
        }
        drawText(renderer, chessFont, p1Caps, 30, 68, (SDL_Color){230, 230, 230, 255}, false);
    }

    /* Player 2 Card (Right) */
    int bMins = (int)game->blackClock / 60;
    int bSecs = (int)game->blackClock % 60;
    snprintf(buf, sizeof(buf), "%02d:%02d  P2 [BLACK]", bMins, bSecs);
    SDL_Color p2Col = (game->currentPlayer == 1) ? (SDL_Color){255, 220, 100, 255} : (SDL_Color){180, 180, 180, 255};
    drawText(renderer, uiFont, buf, WINDOW_WIDTH - 240, 18, p2Col, false);

    snprintf(buf, sizeof(buf), "HP: %.0f  |  Material: %d pts", game->boxing.boxers[1].health, game->materialScore[1]);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH - 240, 44, (SDL_Color){160, 220, 160, 255}, false);

    /* Captured pieces tally for P2 */
    if (game->capturedCount[1] > 0) {
        char p2Caps[64] = "";
        for (int i = 0; i < game->capturedCount[1] && i < 12; i++) {
            strcat(p2Caps, getPieceSymbol(game->capturedPieces[1][i]));
            strcat(p2Caps, " ");
        }
        drawText(renderer, chessFont, p2Caps, WINDOW_WIDTH - 240, 68, (SDL_Color){230, 230, 230, 255}, false);
    }

    /* Center: Round & Round Timer */
    int rMins = (int)game->roundTimer / 60;
    int rSecs = (int)game->roundTimer % 60;
    snprintf(buf, sizeof(buf), "ROUND %d : CHESS  (%02d:%02d)", game->currentRound, rMins, rSecs);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, 18, (SDL_Color){240, 240, 240, 255}, true);

    if (game->mode == MODE_SINGLE_PLAYER) {
        if (game->currentPlayer == 1) {
            bool blink = ((SDL_GetTicks() / 400) % 2 == 0);
            drawText(renderer, uiFont, blink ? ">> AI IS THINKING... <<" : "   AI IS THINKING...   ", WINDOW_WIDTH * 0.5f, 44, (SDL_Color){255, 140, 60, 255}, true);
        } else {
            drawText(renderer, uiFont, ">> YOUR TURN (WHITE) <<", WINDOW_WIDTH * 0.5f, 44, (SDL_Color){255, 215, 0, 255}, true);
        }
    } else if (game->mode == MODE_NETWORK_MULTIPLAYER) {
        if (game->currentPlayer == game->localPlayerIndex) {
            drawText(renderer, uiFont, ">> YOUR TURN TO MOVE <<", WINDOW_WIDTH * 0.5f, 44, (SDL_Color){100, 240, 150, 255}, true);
        } else {
            drawText(renderer, uiFont, ">> OPPONENT'S TURN <<", WINDOW_WIDTH * 0.5f, 44, (SDL_Color){240, 180, 80, 255}, true);
        }
    } else {
        if (game->currentPlayer == 0) {
            drawText(renderer, uiFont, ">> WHITE TO MOVE <<", WINDOW_WIDTH * 0.5f, 44, (SDL_Color){255, 215, 0, 255}, true);
        } else {
            drawText(renderer, uiFont, ">> BLACK TO MOVE <<", WINDOW_WIDTH * 0.5f, 44, (SDL_Color){255, 120, 120, 255}, true);
        }
    }

    /* Check Alert */
    if (game->isCheck[0]) {
        drawText(renderer, uiFont, "⚠️ WHITE IS IN CHECK!", WINDOW_WIDTH * 0.5f, BOARD_Y - 20, (SDL_Color){255, 60, 60, 255}, true);
    } else if (game->isCheck[1]) {
        drawText(renderer, uiFont, "⚠️ BLACK IS IN CHECK!", WINDOW_WIDTH * 0.5f, BOARD_Y - 20, (SDL_Color){255, 60, 60, 255}, true);
    }

    /* Bottom Info */
    drawText(renderer, uiFont, "Castling & En Passant enabled  |  Checkmate = Instant Victory", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 22, (SDL_Color){140, 140, 150, 255}, true);
}

static void renderPromotionModal(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game)
{
    /* Dim background */
    SDL_FRect dim = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &dim);

    /* Modal box */
    SDL_FRect box = { WINDOW_WIDTH * 0.5f - 240, WINDOW_HEIGHT * 0.5f - 110, 480, 220 };
    SDL_SetRenderDrawColor(renderer, 24, 28, 38, 255);
    SDL_RenderFillRect(renderer, &box);

    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderRect(renderer, &box);

    drawText(renderer, uiFont, "PAWN PROMOTION — CHOOSE YOUR PIECE", WINDOW_WIDTH * 0.5f, box.y + 24, (SDL_Color){255, 215, 0, 255}, true);

    const char *symbolsWhite[4] = { "♕", "♖", "♗", "♘" };
    const char *symbolsBlack[4] = { "♛", "♜", "♝", "♞" };
    const char *labels[4] = { "Queen", "Rook", "Bishop", "Knight" };

    for (int i = 0; i < 4; i++) {
        SDL_FRect btn;
        getPromotionButtonRect(i, &btn);

        SDL_SetRenderDrawColor(renderer, 38, 44, 60, 255);
        SDL_RenderFillRect(renderer, &btn);

        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 220);
        SDL_RenderRect(renderer, &btn);

        const char *sym = (game->promotionPawnPlayer == 0) ? symbolsWhite[i] : symbolsBlack[i];
        drawText(renderer, chessFont, sym, btn.x + btn.w * 0.5f, btn.y + btn.h * 0.42f, (SDL_Color){255, 255, 255, 255}, true);
        drawText(renderer, uiFont, labels[i], btn.x + btn.w * 0.5f, btn.y + btn.h * 0.78f, (SDL_Color){220, 220, 220, 255}, true);
    }
}

/* -------------------------------------------------------------
   BOXING RENDERING
   ------------------------------------------------------------- */

static void renderBoxingRing(SDL_Renderer *renderer)
{
    SDL_FRect bg = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 14, 16, 22, 255);
    SDL_RenderFillRect(renderer, &bg);

    /* Spotlights */
    SDL_FRect lightBeam1 = { 200, 0, 250, 620 };
    SDL_SetRenderDrawColor(renderer, 40, 48, 65, 40);
    SDL_RenderFillRect(renderer, &lightBeam1);

    SDL_FRect lightBeam2 = { 550, 0, 250, 620 };
    SDL_SetRenderDrawColor(renderer, 40, 48, 65, 40);
    SDL_RenderFillRect(renderer, &lightBeam2);

    /* Apron */
    SDL_FRect apron = { 80, 620, 840, 140 };
    SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
    SDL_RenderFillRect(renderer, &apron);

    /* Canvas */
    SDL_FRect canvas = { 100, 615, 800, 25 };
    SDL_SetRenderDrawColor(renderer, 48, 56, 70, 255);
    SDL_RenderFillRect(renderer, &canvas);

    SDL_FRect canvasTop = { 100, 612, 800, 6 };
    SDL_SetRenderDrawColor(renderer, 70, 80, 100, 255);
    SDL_RenderFillRect(renderer, &canvasTop);

    /* Posts */
    SDL_FRect postLeft = { 120, 400, 18, 220 };
    SDL_SetRenderDrawColor(renderer, 50, 100, 220, 255);
    SDL_RenderFillRect(renderer, &postLeft);

    SDL_FRect postRight = { 862, 400, 18, 220 };
    SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
    SDL_RenderFillRect(renderer, &postRight);

    /* Ropes */
    float ropeY[3] = { 440.0f, 495.0f, 550.0f };
    for (int r = 0; r < 3; r++) {
        SDL_FRect rope = { 130, ropeY[r], 740, 6 };
        if (r == 0) SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
        else if (r == 1) SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        else SDL_SetRenderDrawColor(renderer, 60, 100, 220, 255);
        SDL_RenderFillRect(renderer, &rope);
    }
}

static void renderBoxer(SDL_Renderer *renderer, const Boxer *b, int index)
{
    float x = b->x;
    float y = b->y;
    int f = b->facing;

    /* Knocked out */
    if (b->state == BOXER_KO) {
        SDL_FRect fallenTorso = { x - 50, y - 25, 100, 22 };
        SDL_SetRenderDrawColor(renderer, (index == 0) ? 220 : 60, (index == 0) ? 220 : 60, (index == 0) ? 230 : 70, 255);
        SDL_RenderFillRect(renderer, &fallenTorso);

        SDL_FRect fallenHead = { x - 70, y - 28, 24, 24 };
        SDL_SetRenderDrawColor(renderer, 235, 180, 140, 255);
        SDL_RenderFillRect(renderer, &fallenHead);

        SDL_FRect fallenGlove = { x + 52, y - 22, 20, 20 };
        SDL_SetRenderDrawColor(renderer, (index == 0) ? 240 : 220, (index == 0) ? 240 : 40, (index == 0) ? 240 : 40, 255);
        SDL_RenderFillRect(renderer, &fallenGlove);
        return;
    }

    /* Animation offset */
    float bobY = 0.0f;
    if (b->state == BOXER_WALK) {
        bobY = sinf(b->stateTimer * 16.0f) * 4.0f;
    } else if (b->state == BOXER_IDLE) {
        bobY = sinf(b->stateTimer * 4.0f) * 2.0f;
    }

    /* Crouch height lowering */
    float crouchDrop = (b->state == BOXER_CROUCH) ? 55.0f : 0.0f;

    float headY = y - 120 + bobY + crouchDrop;
    float torsoY = y - 90 + bobY + crouchDrop * 0.7f;
    float hipsY = y - 48 + bobY + crouchDrop * 0.3f;

    /* Dodge ghost trail */
    if (b->isDodging) {
        SDL_FRect ghost = { x - f * 20 - 20, torsoY, 40, 44 };
        SDL_SetRenderDrawColor(renderer, 150, 200, 255, 70);
        SDL_RenderFillRect(renderer, &ghost);
    }

    /* Feet shadow */
    SDL_FRect feetShadow = { x - 35, y - 4, 70, 8 };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 90);
    SDL_RenderFillRect(renderer, &feetShadow);

    /* Legs & Boots */
    float legH = (y - hipsY - 14);
    if (legH < 10.0f) legH = 10.0f;
    SDL_FRect legL = { x - 18, hipsY + 14, 14, legH };
    SDL_FRect legR = { x + 4, hipsY + 14, 14, legH };
    SDL_SetRenderDrawColor(renderer, 220, 175, 135, 255);
    SDL_RenderFillRect(renderer, &legL);
    SDL_RenderFillRect(renderer, &legR);

    SDL_FRect bootL = { x - 20, y - 14, 18, 14 };
    SDL_FRect bootR = { x + 2, y - 14, 18, 14 };
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderFillRect(renderer, &bootL);
    SDL_RenderFillRect(renderer, &bootR);

    /* Trunks */
    SDL_FRect shorts = { x - 24, hipsY, 48, (b->state == BOXER_CROUCH) ? 18.0f : 22.0f };
    if (index == 0) SDL_SetRenderDrawColor(renderer, 245, 245, 250, 255);
    else SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
    SDL_RenderFillRect(renderer, &shorts);

    SDL_FRect trim = { x - 24, hipsY, 48, 4 };
    if (index == 0) SDL_SetRenderDrawColor(renderer, 220, 180, 40, 255);
    else SDL_SetRenderDrawColor(renderer, 220, 40, 40, 255);
    SDL_RenderFillRect(renderer, &trim);

    /* Torso */
    SDL_FRect torso = { x - 22, torsoY, 44, 44 };
    SDL_SetRenderDrawColor(renderer, 230, 180, 140, 255);
    SDL_RenderFillRect(renderer, &torso);

    /* Hit Stun */
    if (b->state == BOXER_HIT_STUN) {
        x -= f * 8.0f;
    }

    /* Head */
    SDL_FRect head = { x - 14, headY, 28, 28 };
    SDL_SetRenderDrawColor(renderer, 235, 185, 145, 255);
    SDL_RenderFillRect(renderer, &head);

    SDL_FRect hair = { x - 15, headY - 2, 30, 10 };
    if (index == 0) SDL_SetRenderDrawColor(renderer, 210, 160, 50, 255);
    else SDL_SetRenderDrawColor(renderer, 40, 30, 25, 255);
    SDL_RenderFillRect(renderer, &hair);

    /* Guard Broken dizzy stars */
    if (b->state == BOXER_GUARD_BROKEN) {
        SDL_FRect star1 = { head.x - 6, head.y - 12, 8, 8 };
        SDL_FRect star2 = { head.x + 12, head.y - 16, 8, 8 };
        SDL_FRect star3 = { head.x + 28, head.y - 12, 8, 8 };
        SDL_SetRenderDrawColor(renderer, 255, 220, 50, 255);
        SDL_RenderFillRect(renderer, &star1);
        SDL_RenderFillRect(renderer, &star2);
        SDL_RenderFillRect(renderer, &star3);
    }

    /* Gloves */
    SDL_Color gloveCol = (index == 0) ? (SDL_Color){245, 245, 250, 255} : (SDL_Color){220, 40, 40, 255};

    if (b->state == BOXER_BLOCK) {
        SDL_FRect g1 = { x + f * 10, headY + 2, 20, 22 };
        SDL_FRect g2 = { x + f * 22, headY - 2, 20, 22 };
        SDL_SetRenderDrawColor(renderer, gloveCol.r, gloveCol.g, gloveCol.b, 255);
        SDL_RenderFillRect(renderer, &g1);
        SDL_RenderFillRect(renderer, &g2);

        /* Blue shield */
        SDL_FRect aura = { x + f * 8, headY - 10, 38, 55 };
        SDL_SetRenderDrawColor(renderer, 80, 190, 255, 90);
        SDL_RenderFillRect(renderer, &aura);
    }
    else if (b->state == BOXER_UPPERCUT) {
        /* Rising vertical punch */
        float rise = (b->stateTimer <= 0.16f) ? (b->stateTimer / 0.16f) : (1.0f - (b->stateTimer - 0.16f) / 0.18f);
        float gloveY = torsoY - rise * 50.0f;
        SDL_FRect uppercutGlove = { x + f * 28, gloveY, 26, 26 };
        SDL_SetRenderDrawColor(renderer, gloveCol.r, gloveCol.g, gloveCol.b, 255);
        SDL_RenderFillRect(renderer, &uppercutGlove);

        /* Motion flash */
        SDL_FRect trail = { uppercutGlove.x - 4, uppercutGlove.y + 10, 34, 28 };
        SDL_SetRenderDrawColor(renderer, 255, 140, 40, 140);
        SDL_RenderFillRect(renderer, &trail);
    }
    else if (b->state == BOXER_PUNCH_LIGHT) {
        SDL_FRect rearGlove = { x - f * 6, torsoY + 4, 18, 18 };
        SDL_SetRenderDrawColor(renderer, gloveCol.r, gloveCol.g, gloveCol.b, 255);
        SDL_RenderFillRect(renderer, &rearGlove);

        float ext = (b->stateTimer <= 0.11f) ? (b->stateTimer / 0.11f) : (1.0f - (b->stateTimer - 0.11f) / 0.11f);
        float armLen = 16.0f + ext * 48.0f;
        SDL_FRect leadArm = { (f > 0) ? x + 10 : (x + 10 - armLen), torsoY + 6, armLen, 12 };
        SDL_SetRenderDrawColor(renderer, 220, 175, 135, 255);
        SDL_RenderFillRect(renderer, &leadArm);

        SDL_FRect punchGlove = { x + f * (10 + armLen), torsoY + 2, 22, 20 };
        SDL_SetRenderDrawColor(renderer, gloveCol.r, gloveCol.g, gloveCol.b, 255);
        SDL_RenderFillRect(renderer, &punchGlove);
    }
    else if (b->state == BOXER_PUNCH_HEAVY) {
        float armLen = 0.0f;
        if (b->stateTimer < 0.14f) {
            armLen = -14.0f;
        } else {
            float ext = (b->stateTimer - 0.14f) / (0.42f - 0.14f);
            armLen = 65.0f * (1.0f - fabsf(ext - 0.4f) * 1.5f);
            if (armLen < 15.0f) armLen = 15.0f;
        }

        SDL_FRect leadArm = { (f > 0) ? x + 10 : (x + 10 - armLen), torsoY + 4, armLen, 16 };
        SDL_SetRenderDrawColor(renderer, 220, 175, 135, 255);
        SDL_RenderFillRect(renderer, &leadArm);

        SDL_FRect heavyGlove = { x + f * (10 + armLen), torsoY - 2, 28, 26 };
        SDL_SetRenderDrawColor(renderer, gloveCol.r, gloveCol.g, gloveCol.b, 255);
        SDL_RenderFillRect(renderer, &heavyGlove);

        if (b->stateTimer >= 0.14f && b->stateTimer <= 0.28f) {
            SDL_FRect glow = { heavyGlove.x - 4, heavyGlove.y - 4, 36, 34 };
            SDL_SetRenderDrawColor(renderer, 255, 220, 50, 120);
            SDL_RenderFillRect(renderer, &glow);
        }
    }
    else if (b->state == BOXER_CROUCH) {
        /* Crouched guard */
        SDL_FRect g1 = { x + f * 10, headY + 12, 18, 18 };
        SDL_FRect g2 = { x + f * 20, headY + 16, 18, 18 };
        SDL_SetRenderDrawColor(renderer, gloveCol.r, gloveCol.g, gloveCol.b, 255);
        SDL_RenderFillRect(renderer, &g1);
        SDL_RenderFillRect(renderer, &g2);
    }
    else {
        /* Ready stance */
        SDL_FRect g1 = { x + f * 12, torsoY + 4, 18, 18 };
        SDL_FRect g2 = { x + f * 24, torsoY + 10, 18, 18 };
        SDL_SetRenderDrawColor(renderer, gloveCol.r, gloveCol.g, gloveCol.b, 255);
        SDL_RenderFillRect(renderer, &g1);
        SDL_RenderFillRect(renderer, &g2);
    }
}

static void renderBoxingHUD(SDL_Renderer *renderer, TTF_Font *uiFont, const GameState *game)
{
    const BoxingState *b = &game->boxing;
    char buf[96];

    /* Top HUD Panel */
    SDL_FRect hudPanel = { 0, 0, WINDOW_WIDTH, 115 };
    SDL_SetRenderDrawColor(renderer, 18, 20, 26, 230);
    SDL_RenderFillRect(renderer, &hudPanel);

    /* ---------------- PLAYER 1 HUD (LEFT) ---------------- */
    /* Health Bar */
    SDL_FRect p1BarBg = { 40, 28, 360, 22 };
    SDL_SetRenderDrawColor(renderer, 40, 45, 55, 255);
    SDL_RenderFillRect(renderer, &p1BarBg);

    float p1DispW = 360.0f * (b->boxers[0].displayHealth / BOXER_MAX_HEALTH);
    if (p1DispW > 360.0f) p1DispW = 360.0f;
    if (p1DispW < 0.0f) p1DispW = 0.0f;
    SDL_FRect p1DispBar = { 40, 28, p1DispW, 22 };
    SDL_SetRenderDrawColor(renderer, 220, 200, 60, 255);
    SDL_RenderFillRect(renderer, &p1DispBar);

    float p1CurW = 360.0f * (b->boxers[0].health / BOXER_MAX_HEALTH);
    if (p1CurW > 360.0f) p1CurW = 360.0f;
    if (p1CurW < 0.0f) p1CurW = 0.0f;
    SDL_FRect p1CurBar = { 40, 28, p1CurW, 22 };
    if (b->boxers[0].health > BOXER_MAX_HEALTH * 0.40f) SDL_SetRenderDrawColor(renderer, 50, 200, 80, 255);
    else SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
    SDL_RenderFillRect(renderer, &p1CurBar);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderRect(renderer, &p1BarBg);

    /* Stamina Bar */
    SDL_FRect p1StamBg = { 40, 54, 360, 12 };
    SDL_SetRenderDrawColor(renderer, 30, 35, 45, 255);
    SDL_RenderFillRect(renderer, &p1StamBg);
    float p1StamW = 360.0f * (b->boxers[0].stamina / b->boxers[0].maxStamina);
    SDL_FRect p1StamBar = { 40, 54, p1StamW, 12 };
    if (b->boxers[0].isExhausted) SDL_SetRenderDrawColor(renderer, 220, 80, 80, 255);
    else SDL_SetRenderDrawColor(renderer, 80, 200, 240, 255);
    SDL_RenderFillRect(renderer, &p1StamBar);

    /* Labels */
    snprintf(buf, sizeof(buf), "PLAYER 1 [WHITE]  HP: %.0f/%.0f   STAM: %.0f/%.0f",
             b->boxers[0].health, BOXER_MAX_HEALTH, b->boxers[0].stamina, b->boxers[0].maxStamina);
    drawText(renderer, uiFont, buf, 42, 10, (SDL_Color){240, 240, 240, 255}, false);

    snprintf(buf, sizeof(buf), "Buff: %.2fx DMG | Guard: %.0f%%", b->boxers[0].damageMultiplier, b->boxers[0].guardMeter);
    drawText(renderer, uiFont, buf, 42, 72, (SDL_Color){255, 215, 80, 255}, false);

    /* ---------------- PLAYER 2 HUD (RIGHT) ---------------- */
    /* Health Bar */
    SDL_FRect p2BarBg = { WINDOW_WIDTH - 400, 28, 360, 22 };
    SDL_SetRenderDrawColor(renderer, 40, 45, 55, 255);
    SDL_RenderFillRect(renderer, &p2BarBg);

    float p2DispW = 360.0f * (b->boxers[1].displayHealth / BOXER_MAX_HEALTH);
    if (p2DispW > 360.0f) p2DispW = 360.0f;
    if (p2DispW < 0.0f) p2DispW = 0.0f;
    SDL_FRect p2DispBar = { WINDOW_WIDTH - 400 + (360.0f - p2DispW), 28, p2DispW, 22 };
    SDL_SetRenderDrawColor(renderer, 220, 200, 60, 255);
    SDL_RenderFillRect(renderer, &p2DispBar);

    float p2CurW = 360.0f * (b->boxers[1].health / BOXER_MAX_HEALTH);
    if (p2CurW > 360.0f) p2CurW = 360.0f;
    if (p2CurW < 0.0f) p2CurW = 0.0f;
    SDL_FRect p2CurBar = { WINDOW_WIDTH - 400 + (360.0f - p2CurW), 28, p2CurW, 22 };
    if (b->boxers[1].health > BOXER_MAX_HEALTH * 0.40f) SDL_SetRenderDrawColor(renderer, 50, 200, 80, 255);
    else SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
    SDL_RenderFillRect(renderer, &p2CurBar);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderRect(renderer, &p2BarBg);

    /* Stamina Bar */
    SDL_FRect p2StamBg = { WINDOW_WIDTH - 400, 54, 360, 12 };
    SDL_SetRenderDrawColor(renderer, 30, 35, 45, 255);
    SDL_RenderFillRect(renderer, &p2StamBg);
    float p2StamW = 360.0f * (b->boxers[1].stamina / b->boxers[1].maxStamina);
    SDL_FRect p2StamBar = { WINDOW_WIDTH - 400 + (360.0f - p2StamW), 54, p2StamW, 12 };
    if (b->boxers[1].isExhausted) SDL_SetRenderDrawColor(renderer, 220, 80, 80, 255);
    else SDL_SetRenderDrawColor(renderer, 80, 200, 240, 255);
    SDL_RenderFillRect(renderer, &p2StamBar);

    /* Labels */
    snprintf(buf, sizeof(buf), "STAM: %.0f/%.0f   HP: %.0f/%.0f  [BLACK] PLAYER 2",
             b->boxers[1].stamina, b->boxers[1].maxStamina, b->boxers[1].health, BOXER_MAX_HEALTH);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH - 400, 10, (SDL_Color){240, 240, 240, 255}, false);

    snprintf(buf, sizeof(buf), "Guard: %.0f%% | Buff: %.2fx DMG", b->boxers[1].guardMeter, b->boxers[1].damageMultiplier);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH - 400, 72, (SDL_Color){255, 215, 80, 255}, false);

    /* Center: Round & Countdown Timer */
    int secs = (int)game->roundTimer;
    snprintf(buf, sizeof(buf), "ROUND %d : BOXING", game->currentRound);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, 18, (SDL_Color){255, 215, 0, 255}, true);

    snprintf(buf, sizeof(buf), "%02d", secs);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, 48, (SDL_Color){255, 255, 255, 255}, true);

    /* Bottom cheat sheet */
    if (game->mode == MODE_SINGLE_PLAYER) {
        drawText(renderer, uiFont, "YOU (P1): A/D: Move | W: Jump | S: Crouch | F: Light | G: Heavy | V: Uppercut | H: Block | Q: Dodge",
                 WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 36, (SDL_Color){180, 195, 215, 255}, true);
        drawText(renderer, uiFont, "OPPONENT: COMPUTER AI BOXER",
                 WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 16, (SDL_Color){255, 140, 60, 255}, true);
    } else if (game->mode == MODE_NETWORK_MULTIPLAYER) {
        drawText(renderer, uiFont, "YOU: A/D: Move | W: Jump | S: Crouch | F: Light | G: Heavy | V: Uppercut | H: Block | Q: Dodge",
                 WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 36, (SDL_Color){180, 195, 215, 255}, true);
        if (game->localPlayerIndex == 0) {
            drawText(renderer, uiFont, "LAN MATCH: YOU ARE PLAYER 1 (WHITE)  |  OPPONENT: PLAYER 2 (BLACK)",
                     WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 16, (SDL_Color){100, 240, 180, 255}, true);
        } else {
            drawText(renderer, uiFont, "LAN MATCH: YOU ARE PLAYER 2 (BLACK)  |  OPPONENT: PLAYER 1 (WHITE)",
                     WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 16, (SDL_Color){120, 180, 255, 255}, true);
        }
    } else {
        drawText(renderer, uiFont, "P1: A/D: Move | W: Jump | S: Crouch | F: Light | G: Heavy | V: Uppercut | H: Block | Q: Dodge",
                 WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 36, (SDL_Color){180, 195, 215, 255}, true);
        drawText(renderer, uiFont, "P2: Arrows: Move/Jump/Crouch | K: Light | L: Heavy | .: Uppercut | ;: Block | /: Dodge",
                 WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT - 16, (SDL_Color){180, 195, 215, 255}, true);
    }
}

/* -------------------------------------------------------------
   TRANSITION & GAME OVER OVERLAYS
   ------------------------------------------------------------- */

static void renderTransitionOverlay(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game)
{
    SDL_FRect dim = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 205);
    SDL_RenderFillRect(renderer, &dim);

    SDL_FRect banner = { 0, WINDOW_HEIGHT * 0.5f - 145, WINDOW_WIDTH, 290 };
    SDL_SetRenderDrawColor(renderer, 22, 26, 36, 250);
    SDL_RenderFillRect(renderer, &banner);

    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_FRect goldTop = { 0, WINDOW_HEIGHT * 0.5f - 145, WINDOW_WIDTH, 4 };
    SDL_FRect goldBot = { 0, WINDOW_HEIGHT * 0.5f + 141, WINDOW_WIDTH, 4 };
    SDL_RenderFillRect(renderer, &goldTop);
    SDL_RenderFillRect(renderer, &goldBot);

    char buf[128];
    snprintf(buf, sizeof(buf), "ROUND %d COMPLETE!", game->currentRound);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 115, (SDL_Color){220, 220, 220, 255}, true);

    /* Captured piece tallies & synergy breakdown */
    snprintf(buf, sizeof(buf), "White Material: %d pts   vs   Black Material: %d pts",
             game->materialScore[0], game->materialScore[1]);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 75, (SDL_Color){255, 220, 100, 255}, true);

    int diff = game->materialScore[0] - game->materialScore[1];
    if (diff > 0) {
        snprintf(buf, sizeof(buf), "★ P1 (WHITE) ADVANTAGE +%d -> BOXING DAMAGE: %.0f%% | MAX STAMINA: %.0f",
                 diff, game->boxing.boxers[0].damageMultiplier * 100.0f, game->boxing.boxers[0].maxStamina);
        drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 40, (SDL_Color){130, 255, 140, 255}, true);
    } else if (diff < 0) {
        snprintf(buf, sizeof(buf), "★ P2 (BLACK) ADVANTAGE +%d -> BOXING DAMAGE: %.0f%% | MAX STAMINA: %.0f",
                 -diff, game->boxing.boxers[1].damageMultiplier * 100.0f, game->boxing.boxers[1].maxStamina);
        drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 40, (SDL_Color){255, 130, 130, 255}, true);
    } else {
        drawText(renderer, uiFont, "★ CHESS MATERIAL TIED — EQUAL BOXING BUFFS", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 40, (SDL_Color){200, 200, 200, 255}, true);
    }

    if (game->nextPhase == PHASE_BOXING) {
        drawText(renderer, chessFont, "🥊 NOW ENTERING THE RING! 🥊", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f + 10, (SDL_Color){255, 70, 70, 255}, true);
    } else {
        drawText(renderer, chessFont, "♟ RETURN TO THE CHESSBOARD! ♟", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f + 10, (SDL_Color){100, 200, 255, 255}, true);
    }

    snprintf(buf, sizeof(buf), "Next round starts in %.1f seconds...", game->transitionTimer);
    drawText(renderer, uiFont, buf, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f + 70, (SDL_Color){255, 215, 0, 255}, true);
}

static void renderGameOverOverlay(SDL_Renderer *renderer, TTF_Font *uiFont, const GameState *game)
{
    SDL_FRect dim = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 210);
    SDL_RenderFillRect(renderer, &dim);

    SDL_FRect modal = { WINDOW_WIDTH * 0.5f - 280, WINDOW_HEIGHT * 0.5f - 140, 560, 280 };
    SDL_SetRenderDrawColor(renderer, 20, 22, 30, 255);
    SDL_RenderFillRect(renderer, &modal);

    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderRect(renderer, &modal);

    if (game->mode == MODE_SINGLE_PLAYER) {
        if (game->winner == 0) {
            drawText(renderer, uiFont, "VICTORY! YOU DEFEATED THE COMPUTER!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 80, (SDL_Color){255, 215, 0, 255}, true);
        } else if (game->winner == 1) {
            drawText(renderer, uiFont, "DEFEAT! THE COMPUTER DEFEATED YOU!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 80, (SDL_Color){255, 90, 90, 255}, true);
        } else {
            drawText(renderer, uiFont, "MATCH ENDED IN A DRAW!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 80, (SDL_Color){200, 200, 200, 255}, true);
        }
    } else {
        if (game->winner == 0) {
            drawText(renderer, uiFont, "VICTORY FOR PLAYER 1 [WHITE]!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 80, (SDL_Color){255, 215, 0, 255}, true);
        } else if (game->winner == 1) {
            drawText(renderer, uiFont, "VICTORY FOR PLAYER 2 [BLACK]!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 80, (SDL_Color){255, 215, 0, 255}, true);
        } else {
            drawText(renderer, uiFont, "MATCH ENDED IN A DRAW!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 80, (SDL_Color){200, 200, 200, 255}, true);
        }
    }

    /* Win Reason */
    if (game->winReason == WIN_CHECKMATE) {
        drawText(renderer, uiFont, "INSTANT WIN BY CHECKMATE!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 30, (SDL_Color){100, 220, 255, 255}, true);
    } else if (game->winReason == WIN_KO) {
        drawText(renderer, uiFont, "INSTANT WIN BY KNOCKOUT (K.O.)!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 30, (SDL_Color){255, 60, 60, 255}, true);
    } else if (game->winReason == WIN_TIMEOUT) {
        drawText(renderer, uiFont, "VICTORY ON CHESS TIME!", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 30, (SDL_Color){255, 180, 50, 255}, true);
    } else if (game->winReason == WIN_DISCONNECT) {
        drawText(renderer, uiFont, "OPPONENT DISCONNECTED FROM MATCH", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f - 30, (SDL_Color){255, 120, 120, 255}, true);
    }

    drawText(renderer, uiFont, "Press [SPACE] for Rematch", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f + 40, (SDL_Color){240, 240, 240, 255}, true);
    drawText(renderer, uiFont, "Press [ESC] to Return to Menu", WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f + 75, (SDL_Color){160, 160, 170, 255}, true);
}

/* -------------------------------------------------------------
   MAIN UNIFIED RENDER FUNCTION
   ------------------------------------------------------------- */

void renderGame(SDL_Renderer *renderer, TTF_Font *chessFont, TTF_Font *uiFont, const GameState *game)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (game->phase == PHASE_MENU) {
        renderMainMenu(renderer, chessFont, uiFont);
        return;
    }

    if (game->phase == PHASE_DIFFICULTY_SELECT) {
        renderDifficultySelect(renderer, chessFont, uiFont);
        return;
    }

    if (game->phase == PHASE_NET_SELECT) {
        renderNetSelect(renderer, chessFont, uiFont);
        return;
    }

    if (game->phase == PHASE_NET_HOST_WAIT) {
        renderNetHostWait(renderer, chessFont, uiFont, game);
        return;
    }

    if (game->phase == PHASE_NET_JOIN_INPUT || game->phase == PHASE_NET_CONNECTING) {
        renderNetJoinInput(renderer, chessFont, uiFont, game);
        return;
    }

    if (game->phase == PHASE_HOW_TO_PLAY) {
        renderHowToPlay(renderer, chessFont, uiFont);
        return;
    }

    if (game->phase == PHASE_CHESS || (game->phase == PHASE_TRANSITION && game->nextPhase == PHASE_BOXING) || (game->phase == PHASE_GAME_OVER && game->winReason != WIN_KO)) {
        SDL_SetRenderDrawColor(renderer, 28, 30, 36, 255);
        SDL_RenderClear(renderer);

        renderChessBoardAndPieces(renderer, chessFont, uiFont, game);
        renderChessHUD(renderer, chessFont, uiFont, game);

        if (game->isPromoting) {
            renderPromotionModal(renderer, chessFont, uiFont, game);
        }
    } else {
        renderBoxingRing(renderer);

        renderBoxer(renderer, &game->boxing.boxers[0], 0);
        renderBoxer(renderer, &game->boxing.boxers[1], 1);

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (game->boxing.particles[i].active) {
                const Particle *p = &game->boxing.particles[i];
                SDL_FRect pr = { p->x - 2, p->y - 2, 5, 5 };
                SDL_SetRenderDrawColor(renderer, p->r, p->g, p->b, 255);
                SDL_RenderFillRect(renderer, &pr);
            }
        }

        renderBoxingHUD(renderer, uiFont, game);
    }

    if (game->phase == PHASE_TRANSITION) {
        renderTransitionOverlay(renderer, chessFont, uiFont, game);
    }

    if (game->phase == PHASE_GAME_OVER) {
        renderGameOverOverlay(renderer, uiFont, game);
    }
}