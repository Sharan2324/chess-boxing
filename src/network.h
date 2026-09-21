#ifndef NETWORK_H
#define NETWORK_H

#include "game.h"
#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_NET_PORT 5000

typedef enum {
    NET_STATE_IDLE,
    NET_STATE_LISTENING,
    NET_STATE_CONNECTING,
    NET_STATE_CONNECTED,
    NET_STATE_DISCONNECTED
} NetState;

typedef enum {
    MSG_NONE = 0,
    MSG_HANDSHAKE = 1,
    MSG_CHESS_MOVE = 2,
    MSG_PROMOTION = 3,
    MSG_BOXING_INPUT = 4,
    MSG_HOST_SYNC = 5,
    MSG_REMATCH = 6,
    MSG_DISCONNECT = 7
} NetMsgType;

#pragma pack(push, 1)
typedef struct {
    uint8_t type;
    float p1Health;
    float p2Health;
    float p1DisplayHealth;
    float p2DisplayHealth;
    float p1Stamina;
    float p2Stamina;
    float p1Guard;
    float p2Guard;
    float p1X, p1Y;
    float p2X, p2Y;
    float p1Vx, p1Vy;
    float p2Vx, p2Vy;
    int8_t p1Facing;
    int8_t p2Facing;
    uint8_t p1State;
    uint8_t p2State;
    float p1StateTimer;
    float p2StateTimer;
    float roundTimer;
    uint8_t currentRound;
    uint8_t phase;
    int8_t winner;
    uint8_t winReason;
} NetHostSyncPayload;
#pragma pack(pop)

typedef struct {
    bool hasHandshake;
    bool hasChessMove;
    int fromR, fromC, toR, toC;
    bool hasPromotion;
    char promotionPiece;
    bool hasRemoteInput;
    PlayerInput remoteInput;
    bool hasHostSync;
    NetHostSyncPayload hostSync;
    bool hasRematch;
    bool isDisconnected;
} NetEvents;

/*
    =========================================
    NETWORK LIFECYCLE & CONTROL
    =========================================
*/

bool net_init(void);
void net_cleanup(void);

bool net_start_host(int port);
bool net_start_client(const char *ip, int port);
void net_disconnect(void);

NetState net_get_state(void);
const char* net_get_status_message(void);
void net_get_local_ip(char *outIp, int maxLen);

/*
    =========================================
    PACKET POLLING & TRANSMISSION
    =========================================
*/

void net_poll(NetEvents *outEvents);

bool net_send_handshake(void);
bool net_send_chess_move(int fromR, int fromC, int toR, int toC);
bool net_send_promotion(char pieceChoice);
bool net_send_boxing_input(const PlayerInput *input);
bool net_send_host_sync(const GameState *game);
bool net_send_rematch(void);

#endif
