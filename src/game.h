#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

/*
    =========================================
    WINDOW & BOARD CONFIGURATION
    =========================================
*/

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800

#define BOARD_SIZE 8
#define SQUARE_SIZE 80
#define BOARD_X 180
#define BOARD_Y 90

/*
    =========================================
    MATCH PHASES & GAME MODES
    =========================================
*/

typedef enum {
    MODE_SINGLE_PLAYER,
    MODE_LOCAL_MULTIPLAYER,
    MODE_NETWORK_MULTIPLAYER
} GameMode;

typedef enum {
    AI_EASY,
    AI_MEDIUM,
    AI_HARD
} AIDifficulty;

typedef enum {
    NET_ROLE_NONE,
    NET_ROLE_HOST,
    NET_ROLE_CLIENT
} NetRole;

typedef enum {
    PHASE_MENU,
    PHASE_DIFFICULTY_SELECT,
    PHASE_NET_SELECT,
    PHASE_NET_HOST_WAIT,
    PHASE_NET_JOIN_INPUT,
    PHASE_NET_CONNECTING,
    PHASE_HOW_TO_PLAY,
    PHASE_CHESS,
    PHASE_BOXING,
    PHASE_TRANSITION,
    PHASE_GAME_OVER
} MatchPhase;

typedef enum {
    WIN_NONE,
    WIN_CHECKMATE,
    WIN_KO,
    WIN_TIMEOUT,
    WIN_DISCONNECT
} WinReason;

#define CHESS_ROUND_DURATION 60.0f     /* 60 seconds per chess round */
#define BOXING_ROUND_DURATION 45.0f    /* 45 seconds per boxing round */
#define CHESS_TOTAL_CLOCK 180.0f       /* 3 minutes per player total chess clock */
#define TRANSITION_DURATION 3.0f       /* 3.0 seconds for round transition banner */

#define BOXER_MAX_HEALTH 1000.0f
#define BOXER_MIN_HEALTH_AFTER_SECOND_CHESS 500.0f

/*
    =========================================
    GENERIC PLAYER INPUT ABSTRACTION
    =========================================
*/

typedef struct {
    bool left;
    bool right;
    bool up;         /* Jump */
    bool down;       /* Crouch */
    bool lightPunch;
    bool heavyPunch;
    bool uppercut;
    bool block;
    bool dodge;
} PlayerInput;

/*
    =========================================
    BOXING ENGINE TYPES
    =========================================
*/

typedef enum {
    BOXER_IDLE,
    BOXER_WALK,
    BOXER_CROUCH,
    BOXER_JUMP,
    BOXER_PUNCH_LIGHT,
    BOXER_PUNCH_HEAVY,
    BOXER_UPPERCUT,
    BOXER_BLOCK,
    BOXER_DODGE,
    BOXER_HIT_STUN,
    BOXER_GUARD_BROKEN,
    BOXER_KO
} BoxerState;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    int facing;             /* 1 = right, -1 = left */
    float health;           /* 0.0 to 100.0 */
    float displayHealth;    /* Smooth lagging damage bar */

    /* Stamina System */
    float stamina;          /* Current stamina */
    float maxStamina;       /* Max stamina (buffed by chess advantage) */
    bool isExhausted;
    float exhaustionTimer;

    /* Guard System */
    float guardMeter;       /* 0.0 to 100.0 */
    float maxGuardMeter;
    bool isGuardBroken;
    float guardBreakTimer;

    /* Dodge / Slip System */
    bool isDodging;
    float dodgeTimer;
    int dodgeDirection;

    BoxerState state;
    float stateTimer;       /* Seconds spent in current state */
    bool isGrounded;
    bool hasHit;            /* Flag to prevent multi-hit in single attack */

    /* Damage multiplier from chess material advantage */
    float damageMultiplier;

} Boxer;

#define MAX_PARTICLES 64

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float maxLife;
    unsigned char r, g, b;
    bool active;
} Particle;

typedef struct {
    Boxer boxers[2];        /* 0 = Player 1, 1 = Player 2 */
    PlayerInput prevInputs[2];
    Particle particles[MAX_PARTICLES];
    float shakeIntensity;
    float shakeTimer;
} BoxingState;

/*
    =========================================
    UNIFIED GAME STATE
    =========================================
*/

typedef struct {
    /* Match mode & configuration */
    GameMode mode;
    AIDifficulty aiDifficulty;
    NetRole netRole;
    int localPlayerIndex;   /* 0 = Player 1, 1 = Player 2 */

    /* AI State */
    float aiThinkingTimer;
    bool aiMoveReady;
    int aiPendingFromR, aiPendingFromC;
    int aiPendingToR, aiPendingToC;

    /* Network State UI buffers */
    char netIpInput[64];
    int netIpLen;
    char hostLocalIp[64];
    float netStatusTimer;

    /* Match management */
    MatchPhase phase;
    int currentRound;
    float roundTimer;
    float transitionTimer;
    MatchPhase nextPhase;
    int winner;             /* -1 = ongoing, 0 = Player 1, 1 = Player 2 */
    WinReason winReason;

    /* Chess State */
    char board[BOARD_SIZE][BOARD_SIZE];
    int selectedRow;
    int selectedCol;
    bool isDragging;
    int dragStartRow;
    int dragStartCol;
    int dragCurrentX;
    int dragCurrentY;
    bool legalMoves[BOARD_SIZE][BOARD_SIZE];

    int currentPlayer;      /* 0 = White (P1), 1 = Black (P2) */
    bool isCheck[2];        /* Index 0 = White in check, 1 = Black in check */
    bool isCheckmate[2];    /* Index 0 = White checkmated, 1 = Black checkmated */
    float whiteClock;       /* Countdown for White */
    float blackClock;       /* Countdown for Black */

    /* Castling Rights */
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteKingsideRookMoved;
    bool whiteQueensideRookMoved;
    bool blackKingsideRookMoved;
    bool blackQueensideRookMoved;

    /* En Passant State */
    int enPassantRow;       /* Target square row (-1 if none) */
    int enPassantCol;       /* Target square col (-1 if none) */
    int enPassantPawnRow;   /* Location of actual pawn to remove */
    int enPassantPawnCol;

    /* Pawn Promotion Modal State */
    bool isPromoting;
    int promotionRow;
    int promotionCol;
    int promotionPawnPlayer;

    /* Captured Pieces & Material Tracking */
    char capturedPieces[2][32];
    int capturedCount[2];
    int materialScore[2];   /* White total captured score vs Black total captured score */

    /* Boxing State */
    BoxingState boxing;
    bool healthRecoveredAfterSecondChess;

} GameState;

/*
    =========================================
    FUNCTION PROTOTYPES
    =========================================
*/

void initializeGame(GameState *game);
void startMatchWithMode(GameState *game, GameMode mode, AIDifficulty diff, NetRole role, int localIdx);
void resetForRematch(GameState *game);
void updateMatch(GameState *game, float dt);
void startTransition(GameState *game, MatchPhase nextPhase);
void updateAdvantageBuffs(GameState *game);
void recoverBoxingHealthAfterSecondChess(GameState *game);
int getPieceValue(char p);

#endif