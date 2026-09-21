#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "game.h"
#include "chess.h"
#include "boxing.h"
#include "renderer.h"
#include "ai.h"
#include "network.h"

static void getLocalNetworkInput(const bool *keys, PlayerInput *out)
{
    PlayerInput p1, p2;
    readKeyboardInput(keys, &p1, &p2);
    out->left = p1.left || p2.left;
    out->right = p1.right || p2.right;
    out->up = p1.up || p2.up;
    out->down = p1.down || p2.down;
    out->lightPunch = p1.lightPunch || p2.lightPunch;
    out->heavyPunch = p1.heavyPunch || p2.heavyPunch;
    out->uppercut = p1.uppercut || p2.uppercut;
    out->block = p1.block || p2.block;
    out->dodge = p1.dodge || p2.dodge;
}

static void applyHostSync(GameState *game, const NetHostSyncPayload *sync)
{
    Boxer *b1 = &game->boxing.boxers[0];
    Boxer *b2 = &game->boxing.boxers[1];

    b1->health = sync->p1Health;
    b2->health = sync->p2Health;
    b1->displayHealth = sync->p1DisplayHealth;
    b2->displayHealth = sync->p2DisplayHealth;
    b1->stamina = sync->p1Stamina;
    b2->stamina = sync->p2Stamina;
    b1->guardMeter = sync->p1Guard;
    b2->guardMeter = sync->p2Guard;
    b1->x = sync->p1X;
    b1->y = sync->p1Y;
    b2->x = sync->p2X;
    b2->y = sync->p2Y;
    b1->vx = sync->p1Vx;
    b1->vy = sync->p1Vy;
    b2->vx = sync->p2Vx;
    b2->vy = sync->p2Vy;
    b1->facing = sync->p1Facing;
    b2->facing = sync->p2Facing;
    b1->state = (BoxerState)sync->p1State;
    b2->state = (BoxerState)sync->p2State;
    b1->stateTimer = sync->p1StateTimer;
    b2->stateTimer = sync->p2StateTimer;

    game->roundTimer = sync->roundTimer;
    game->currentRound = sync->currentRound;

    if (game->phase != (MatchPhase)sync->phase) {
        game->phase = (MatchPhase)sync->phase;
    }
    if (sync->winner != -1) {
        game->winner = sync->winner;
        game->winReason = (WinReason)sync->winReason;
        game->phase = PHASE_GAME_OVER;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    if (!TTF_Init()) {
        printf("TTF_Init failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (!net_init()) {
        printf("Winsock initialization failed: %s\n", net_get_status_message());
    }

    SDL_Window *window = SDL_CreateWindow(
        "Chess Boxing",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );

    if (window == NULL) {
        printf("Window creation failed: %s\n", SDL_GetError());
        net_cleanup();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        net_cleanup();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* Load fonts */
    TTF_Font *chessFont = TTF_OpenFont("assets/fonts/seguisym.ttf", 60);
    TTF_Font *uiFont = TTF_OpenFont("assets/fonts/seguisym.ttf", 19);

    if (chessFont == NULL || uiFont == NULL) {
        printf("Font loading failed: %s\n", SDL_GetError());
        if (chessFont) TTF_CloseFont(chessFont);
        if (uiFont) TTF_CloseFont(uiFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        net_cleanup();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    GameState game;
    initializeGame(&game);

    bool running = true;
    SDL_Event event;
    Uint64 lastTick = SDL_GetTicks();

    while (running) {
        Uint64 currentTick = SDL_GetTicks();
        float dt = (float)(currentTick - lastTick) / 1000.0f;
        if (dt > 0.05f) dt = 0.05f;
        lastTick = currentTick;

        const bool *keys = SDL_GetKeyboardState(NULL);

        /*
            =========================================
            NON-BLOCKING NETWORK POLLING
            =========================================
        */
        NetEvents netEvents;
        net_poll(&netEvents);

        /* Check for LAN Host connection established */
        if (game.phase == PHASE_NET_HOST_WAIT && net_get_state() == NET_STATE_CONNECTED) {
            startMatchWithMode(&game, MODE_NETWORK_MULTIPLAYER, AI_MEDIUM, NET_ROLE_HOST, 0);
        }
        /* Check for LAN Client connection established */
        else if (game.phase == PHASE_NET_CONNECTING && net_get_state() == NET_STATE_CONNECTED) {
            startMatchWithMode(&game, MODE_NETWORK_MULTIPLAYER, AI_MEDIUM, NET_ROLE_CLIENT, 1);
        }

        /* Check for unexpected opponent disconnect during active match */
        if (netEvents.isDisconnected && (game.phase == PHASE_CHESS || game.phase == PHASE_BOXING || game.phase == PHASE_TRANSITION)) {
            game.phase = PHASE_GAME_OVER;
            game.winReason = WIN_DISCONNECT;
            game.winner = game.localPlayerIndex;
            printf("Opponent disconnected! Match ended.\n");
        }

        /* Rematch message from peer */
        if (netEvents.hasRematch && game.phase == PHASE_GAME_OVER) {
            resetForRematch(&game);
        }

        /* Chess move from remote peer */
        if (netEvents.hasChessMove && game.phase == PHASE_CHESS) {
            executeChessMove(&game, netEvents.fromR, netEvents.fromC, netEvents.toR, netEvents.toC);
        }

        /* Pawn promotion from remote peer */
        if (netEvents.hasPromotion && game.phase == PHASE_CHESS && game.isPromoting) {
            completePawnPromotion(&game, netEvents.promotionPiece);
        }

        /* Host sync packet received by Client */
        if (netEvents.hasHostSync && game.mode == MODE_NETWORK_MULTIPLAYER && game.netRole == NET_ROLE_CLIENT) {
            applyHostSync(&game, &netEvents.hostSync);
        }

        /*
            =========================================
            EVENT PROCESSING
            =========================================
        */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }

            /* -------------------------------------------------------------
               MAIN MENU INPUTS
               ------------------------------------------------------------- */
            if (game.phase == PHASE_MENU) {
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.scancode == SDL_SCANCODE_1) {
                        game.phase = PHASE_DIFFICULTY_SELECT;
                    } else if (event.key.scancode == SDL_SCANCODE_2 || event.key.scancode == SDL_SCANCODE_RETURN || event.key.scancode == SDL_SCANCODE_SPACE) {
                        startMatchWithMode(&game, MODE_LOCAL_MULTIPLAYER, AI_MEDIUM, NET_ROLE_NONE, 0);
                    } else if (event.key.scancode == SDL_SCANCODE_3) {
                        game.phase = PHASE_NET_SELECT;
                    } else if (event.key.scancode == SDL_SCANCODE_4) {
                        game.phase = PHASE_HOW_TO_PLAY;
                    } else if (event.key.scancode == SDL_SCANCODE_5 || event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        running = false;
                        break;
                    }
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                    float mx = event.button.x;
                    float my = event.button.y;

                    SDL_FRect btns[5];
                    for (int i = 0; i < 5; i++) getMenuButtonRect(i, &btns[i]);

                    if (isPointInRect(mx, my, &btns[0])) {
                        game.phase = PHASE_DIFFICULTY_SELECT;
                    } else if (isPointInRect(mx, my, &btns[1])) {
                        startMatchWithMode(&game, MODE_LOCAL_MULTIPLAYER, AI_MEDIUM, NET_ROLE_NONE, 0);
                    } else if (isPointInRect(mx, my, &btns[2])) {
                        game.phase = PHASE_NET_SELECT;
                    } else if (isPointInRect(mx, my, &btns[3])) {
                        game.phase = PHASE_HOW_TO_PLAY;
                    } else if (isPointInRect(mx, my, &btns[4])) {
                        running = false;
                        break;
                    }
                }
                continue;
            }

            /* -------------------------------------------------------------
               DIFFICULTY SELECT INPUTS
               ------------------------------------------------------------- */
            if (game.phase == PHASE_DIFFICULTY_SELECT) {
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.scancode == SDL_SCANCODE_1) {
                        startMatchWithMode(&game, MODE_SINGLE_PLAYER, AI_EASY, NET_ROLE_NONE, 0);
                        ai_reset();
                    } else if (event.key.scancode == SDL_SCANCODE_2) {
                        startMatchWithMode(&game, MODE_SINGLE_PLAYER, AI_MEDIUM, NET_ROLE_NONE, 0);
                        ai_reset();
                    } else if (event.key.scancode == SDL_SCANCODE_3) {
                        startMatchWithMode(&game, MODE_SINGLE_PLAYER, AI_HARD, NET_ROLE_NONE, 0);
                        ai_reset();
                    } else if (event.key.scancode == SDL_SCANCODE_ESCAPE || event.key.scancode == SDL_SCANCODE_BACKSPACE) {
                        game.phase = PHASE_MENU;
                    }
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                    float mx = event.button.x;
                    float my = event.button.y;

                    SDL_FRect dBtns[3], backBtn;
                    for (int i = 0; i < 3; i++) getDifficultyButtonRect(i, &dBtns[i]);
                    getDifficultyBackRect(&backBtn);

                    if (isPointInRect(mx, my, &dBtns[0])) {
                        startMatchWithMode(&game, MODE_SINGLE_PLAYER, AI_EASY, NET_ROLE_NONE, 0);
                        ai_reset();
                    } else if (isPointInRect(mx, my, &dBtns[1])) {
                        startMatchWithMode(&game, MODE_SINGLE_PLAYER, AI_MEDIUM, NET_ROLE_NONE, 0);
                        ai_reset();
                    } else if (isPointInRect(mx, my, &dBtns[2])) {
                        startMatchWithMode(&game, MODE_SINGLE_PLAYER, AI_HARD, NET_ROLE_NONE, 0);
                        ai_reset();
                    } else if (isPointInRect(mx, my, &backBtn)) {
                        game.phase = PHASE_MENU;
                    }
                }
                continue;
            }

            /* -------------------------------------------------------------
               NETWORK SELECT INPUTS (HOST OR JOIN)
               ------------------------------------------------------------- */
            if (game.phase == PHASE_NET_SELECT) {
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.scancode == SDL_SCANCODE_1) {
                        net_get_local_ip(game.hostLocalIp, sizeof(game.hostLocalIp));
                        net_start_host(DEFAULT_NET_PORT);
                        game.phase = PHASE_NET_HOST_WAIT;
                    } else if (event.key.scancode == SDL_SCANCODE_2) {
                        SDL_StartTextInput(window);
                        game.phase = PHASE_NET_JOIN_INPUT;
                    } else if (event.key.scancode == SDL_SCANCODE_ESCAPE || event.key.scancode == SDL_SCANCODE_BACKSPACE) {
                        game.phase = PHASE_MENU;
                    }
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                    float mx = event.button.x;
                    float my = event.button.y;

                    SDL_FRect nBtns[2], backBtn;
                    for (int i = 0; i < 2; i++) getNetSelectButtonRect(i, &nBtns[i]);
                    getNetSelectBackRect(&backBtn);

                    if (isPointInRect(mx, my, &nBtns[0])) {
                        net_get_local_ip(game.hostLocalIp, sizeof(game.hostLocalIp));
                        net_start_host(DEFAULT_NET_PORT);
                        game.phase = PHASE_NET_HOST_WAIT;
                    } else if (isPointInRect(mx, my, &nBtns[1])) {
                        SDL_StartTextInput(window);
                        game.phase = PHASE_NET_JOIN_INPUT;
                    } else if (isPointInRect(mx, my, &backBtn)) {
                        game.phase = PHASE_MENU;
                    }
                }
                continue;
            }

            /* -------------------------------------------------------------
               NETWORK HOST WAITING
               ------------------------------------------------------------- */
            if (game.phase == PHASE_NET_HOST_WAIT) {
                if (event.type == SDL_EVENT_KEY_DOWN && (event.key.scancode == SDL_SCANCODE_ESCAPE || event.key.scancode == SDL_SCANCODE_BACKSPACE)) {
                    net_disconnect();
                    game.phase = PHASE_NET_SELECT;
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                    SDL_FRect cancelBtn;
                    getNetHostBackRect(&cancelBtn);
                    if (isPointInRect(event.button.x, event.button.y, &cancelBtn)) {
                        net_disconnect();
                        game.phase = PHASE_NET_SELECT;
                    }
                }
                continue;
            }

            /* -------------------------------------------------------------
               NETWORK JOIN INPUT BOX & CONNECTING
               ------------------------------------------------------------- */
            if (game.phase == PHASE_NET_JOIN_INPUT || game.phase == PHASE_NET_CONNECTING) {
                if (event.type == SDL_EVENT_TEXT_INPUT) {
                    if (game.phase == PHASE_NET_JOIN_INPUT) {
                        const char *newChars = event.text.text;
                        for (int i = 0; newChars[i] != '\0' && game.netIpLen < 31; i++) {
                            char c = newChars[i];
                            if ((c >= '0' && c <= '9') || c == '.') {
                                game.netIpInput[game.netIpLen++] = c;
                                game.netIpInput[game.netIpLen] = '\0';
                            }
                        }
                    }
                } else if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.scancode == SDL_SCANCODE_BACKSPACE) {
                        if (game.netIpLen > 0) {
                            game.netIpInput[--game.netIpLen] = '\0';
                        }
                    } else if (event.key.scancode == SDL_SCANCODE_RETURN || event.key.scancode == SDL_SCANCODE_KP_ENTER) {
                        if (game.netIpLen > 0) {
                            SDL_StopTextInput(window);
                            net_start_client(game.netIpInput, DEFAULT_NET_PORT);
                            game.phase = PHASE_NET_CONNECTING;
                        }
                    } else if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        SDL_StopTextInput(window);
                        net_disconnect();
                        game.phase = PHASE_NET_SELECT;
                    }
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                    float mx = event.button.x;
                    float my = event.button.y;

                    SDL_FRect connBtn, backBtn;
                    getNetConnectButtonRect(&connBtn);
                    getNetJoinBackRect(&backBtn);

                    if (isPointInRect(mx, my, &connBtn)) {
                        if (game.netIpLen > 0) {
                            SDL_StopTextInput(window);
                            net_start_client(game.netIpInput, DEFAULT_NET_PORT);
                            game.phase = PHASE_NET_CONNECTING;
                        }
                    } else if (isPointInRect(mx, my, &backBtn)) {
                        SDL_StopTextInput(window);
                        net_disconnect();
                        game.phase = PHASE_NET_SELECT;
                    }
                }
                continue;
            }

            /* -------------------------------------------------------------
               HOW TO PLAY INPUTS
               ------------------------------------------------------------- */
            if (game.phase == PHASE_HOW_TO_PLAY) {
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE || event.key.scancode == SDL_SCANCODE_BACKSPACE || event.key.scancode == SDL_SCANCODE_RETURN) {
                        game.phase = PHASE_MENU;
                    }
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                    SDL_FRect backBtn;
                    getHowToPlayBackRect(&backBtn);
                    if (isPointInRect(event.button.x, event.button.y, &backBtn)) {
                        game.phase = PHASE_MENU;
                    }
                }
                continue;
            }

            /* -------------------------------------------------------------
               GAME OVER INPUTS
               ------------------------------------------------------------- */
            if (game.phase == PHASE_GAME_OVER) {
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.scancode == SDL_SCANCODE_SPACE) {
                        if (game.mode == MODE_NETWORK_MULTIPLAYER) {
                            net_send_rematch();
                        }
                        resetForRematch(&game);
                        if (game.mode == MODE_SINGLE_PLAYER) ai_reset();
                        continue;
                    } else if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        if (game.mode == MODE_NETWORK_MULTIPLAYER) {
                            net_disconnect();
                        }
                        game.phase = PHASE_MENU;
                        continue;
                    }
                }
            }

            /* -------------------------------------------------------------
               CHESS INPUTS & PROMOTION
               ------------------------------------------------------------- */
            if (game.phase == PHASE_CHESS) {
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    if (game.mode == MODE_NETWORK_MULTIPLAYER) net_disconnect();
                    game.phase = PHASE_MENU;
                    continue;
                }

                /* Active Player check: prevent moving out of turn */
                bool canAct = true;
                if (game.mode == MODE_SINGLE_PLAYER && game.currentPlayer != 0) {
                    canAct = false;
                } else if (game.mode == MODE_NETWORK_MULTIPLAYER && game.currentPlayer != game.localPlayerIndex) {
                    canAct = false;
                }

                if (game.isPromoting && canAct) {
                    char choice = '\0';
                    if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.scancode == SDL_SCANCODE_Q) choice = 'Q';
                        else if (event.key.scancode == SDL_SCANCODE_R) choice = 'R';
                        else if (event.key.scancode == SDL_SCANCODE_B) choice = 'B';
                        else if (event.key.scancode == SDL_SCANCODE_N) choice = 'N';
                    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                        float mx = event.button.x;
                        float my = event.button.y;
                        SDL_FRect pBtn;

                        getPromotionButtonRect(0, &pBtn);
                        if (isPointInRect(mx, my, &pBtn)) choice = 'Q';
                        getPromotionButtonRect(1, &pBtn);
                        if (isPointInRect(mx, my, &pBtn)) choice = 'R';
                        getPromotionButtonRect(2, &pBtn);
                        if (isPointInRect(mx, my, &pBtn)) choice = 'B';
                        getPromotionButtonRect(3, &pBtn);
                        if (isPointInRect(mx, my, &pBtn)) choice = 'N';
                    }

                    if (choice != '\0') {
                        completePawnPromotion(&game, choice);
                        if (game.mode == MODE_NETWORK_MULTIPLAYER) {
                            net_send_promotion(choice);
                        }
                    }
                    continue;
                }

                /* Standard Chess Drag / Click Input */
                if (canAct && !game.isPromoting) {
                    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
                        int r, c;
                        if (mouseToBoardSquare((int)event.button.x, (int)event.button.y, &r, &c)) {
                            char clickedPiece = getPiece(&game, r, c);

                            if (isPlayersPiece(clickedPiece, game.currentPlayer)) {
                                game.selectedRow = r;
                                game.selectedCol = c;
                                game.isDragging = true;
                                game.dragStartRow = r;
                                game.dragStartCol = c;
                                game.dragCurrentX = (int)event.button.x;
                                game.dragCurrentY = (int)event.button.y;
                                calculateLegalMoves(&game, r, c);
                            } else if (game.selectedRow >= 0 && game.selectedCol >= 0) {
                                int fromR = game.selectedRow;
                                int fromC = game.selectedCol;
                                if (executeChessMove(&game, fromR, fromC, r, c)) {
                                    if (game.mode == MODE_NETWORK_MULTIPLAYER) {
                                        net_send_chess_move(fromR, fromC, r, c);
                                    }
                                }
                                game.selectedRow = -1;
                                game.selectedCol = -1;
                                clearLegalMoves(&game);
                            }
                        } else {
                            game.selectedRow = -1;
                            game.selectedCol = -1;
                            game.isDragging = false;
                            clearLegalMoves(&game);
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        if (game.isDragging) {
                            game.dragCurrentX = (int)event.motion.x;
                            game.dragCurrentY = (int)event.motion.y;
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                        if (game.isDragging) {
                            int dropR, dropC;
                            if (mouseToBoardSquare((int)event.button.x, (int)event.button.y, &dropR, &dropC)) {
                                if (dropR != game.dragStartRow || dropC != game.dragStartCol) {
                                    int fromR = game.dragStartRow;
                                    int fromC = game.dragStartCol;
                                    if (executeChessMove(&game, fromR, fromC, dropR, dropC)) {
                                        if (game.mode == MODE_NETWORK_MULTIPLAYER) {
                                            net_send_chess_move(fromR, fromC, dropR, dropC);
                                        }
                                    }
                                }
                            }
                            game.isDragging = false;
                        }
                    }
                }
            }

            /* -------------------------------------------------------------
               BOXING ESCAPE INPUT
               ------------------------------------------------------------- */
            if (game.phase == PHASE_BOXING) {
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    if (game.mode == MODE_NETWORK_MULTIPLAYER) net_disconnect();
                    game.phase = PHASE_MENU;
                    continue;
                }
            }
        }

        /*
            =========================================
            CHESS AI EVALUATION & MOVE EXECUTION
            =========================================
        */
        if (game.phase == PHASE_CHESS && game.mode == MODE_SINGLE_PLAYER && game.currentPlayer == 1 && !game.isPromoting) {
            game.aiThinkingTimer += dt;
            float targetThinkingTime = (game.aiDifficulty == AI_EASY) ? 0.45f :
                                       (game.aiDifficulty == AI_MEDIUM) ? 0.65f : 0.85f;

            if (game.aiThinkingTimer >= targetThinkingTime) {
                game.aiThinkingTimer = 0.0f;
                int fr, fc, tr, tc;
                if (ai_get_chess_move(&game, game.aiDifficulty, &fr, &fc, &tr, &tc)) {
                    executeChessMove(&game, fr, fc, tr, tc);
                    if (game.isPromoting && game.promotionPawnPlayer == 1) {
                        completePawnPromotion(&game, 'Q');
                    }
                }
            }
        }

        /*
            =========================================
            BOXING INPUT ROUTING & PHYSICS
            =========================================
        */
        if (game.phase == PHASE_BOXING || game.phase == PHASE_GAME_OVER) {
            PlayerInput inputs[2];
            memset(inputs, 0, sizeof(inputs));

            if (game.mode == MODE_LOCAL_MULTIPLAYER) {
                readKeyboardInput(keys, &inputs[0], &inputs[1]);
            } else if (game.mode == MODE_SINGLE_PLAYER) {
                PlayerInput p1Key, dummy;
                readKeyboardInput(keys, &p1Key, &dummy);
                inputs[0] = p1Key;
                ai_update_boxing(&game, 1, game.aiDifficulty, &inputs[1], dt);
            } else if (game.mode == MODE_NETWORK_MULTIPLAYER) {
                PlayerInput localKey;
                getLocalNetworkInput(keys, &localKey);
                net_send_boxing_input(&localKey);

                if (game.netRole == NET_ROLE_HOST) {
                    inputs[0] = localKey;
                    inputs[1] = netEvents.hasRemoteInput ? netEvents.remoteInput : game.boxing.prevInputs[1];
                } else {
                    inputs[1] = localKey;
                    inputs[0] = netEvents.hasRemoteInput ? netEvents.remoteInput : game.boxing.prevInputs[0];
                }
            }

            updateBoxing(&game, inputs, dt);
        }

        /* Update match timers and round transitions */
        updateMatch(&game, dt);

        /* Host periodically synchronizes client state */
        if (game.mode == MODE_NETWORK_MULTIPLAYER && game.netRole == NET_ROLE_HOST) {
            if (game.phase == PHASE_BOXING || game.phase == PHASE_TRANSITION || game.phase == PHASE_GAME_OVER) {
                net_send_host_sync(&game);
            }
        }

        /* Render entire game frame */
        renderGame(renderer, chessFont, uiFont, &game);
        SDL_RenderPresent(renderer);

        /* Frame pacing ~60 FPS */
        SDL_Delay(16);
    }

    /* Cleanup */
    net_cleanup();
    TTF_CloseFont(chessFont);
    TTF_CloseFont(uiFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}