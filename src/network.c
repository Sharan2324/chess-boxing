#include "network.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool s_winsockInitialized = false;
static SOCKET s_listenSock = INVALID_SOCKET;
static SOCKET s_peerSock = INVALID_SOCKET;
static NetState s_state = NET_STATE_IDLE;
static char s_statusMsg[128] = "Idle";

#define RX_BUFFER_SIZE 2048
static uint8_t s_rxBuf[RX_BUFFER_SIZE];
static int s_rxLen = 0;

static uint16_t encodePlayerInput(const PlayerInput *in)
{
    uint16_t b = 0;
    if (in->left)       b |= (1 << 0);
    if (in->right)      b |= (1 << 1);
    if (in->up)         b |= (1 << 2);
    if (in->down)       b |= (1 << 3);
    if (in->lightPunch) b |= (1 << 4);
    if (in->heavyPunch) b |= (1 << 5);
    if (in->uppercut)   b |= (1 << 6);
    if (in->block)      b |= (1 << 7);
    if (in->dodge)      b |= (1 << 8);
    return b;
}

static void decodePlayerInput(uint16_t b, PlayerInput *out)
{
    out->left       = (b & (1 << 0)) != 0;
    out->right      = (b & (1 << 1)) != 0;
    out->up         = (b & (1 << 2)) != 0;
    out->down       = (b & (1 << 3)) != 0;
    out->lightPunch = (b & (1 << 4)) != 0;
    out->heavyPunch = (b & (1 << 5)) != 0;
    out->uppercut   = (b & (1 << 6)) != 0;
    out->block      = (b & (1 << 7)) != 0;
    out->dodge      = (b & (1 << 8)) != 0;
}

static bool setNonBlocking(SOCKET sock)
{
    u_long mode = 1;
    return (ioctlsocket(sock, FIONBIO, &mode) == 0);
}

bool net_init(void)
{
    if (s_winsockInitialized) return true;

    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        snprintf(s_statusMsg, sizeof(s_statusMsg), "WSAStartup failed: %d", res);
        return false;
    }

    s_winsockInitialized = true;
    s_state = NET_STATE_IDLE;
    snprintf(s_statusMsg, sizeof(s_statusMsg), "Ready");
    return true;
}

void net_cleanup(void)
{
    net_disconnect();
    if (s_winsockInitialized) {
        WSACleanup();
        s_winsockInitialized = false;
    }
}

void net_disconnect(void)
{
    if (s_peerSock != INVALID_SOCKET) {
        uint8_t disc = MSG_DISCONNECT;
        send(s_peerSock, (const char *)&disc, 1, 0);
        closesocket(s_peerSock);
        s_peerSock = INVALID_SOCKET;
    }
    if (s_listenSock != INVALID_SOCKET) {
        closesocket(s_listenSock);
        s_listenSock = INVALID_SOCKET;
    }
    s_state = NET_STATE_IDLE;
    s_rxLen = 0;
    snprintf(s_statusMsg, sizeof(s_statusMsg), "Disconnected");
}

NetState net_get_state(void)
{
    return s_state;
}

const char* net_get_status_message(void)
{
    return s_statusMsg;
}

void net_get_local_ip(char *outIp, int maxLen)
{
    if (!net_init()) {
        strncpy(outIp, "127.0.0.1", maxLen - 1);
        outIp[maxLen - 1] = '\0';
        return;
    }

    /* Probe route using a dummy UDP socket to public IP (no packets sent) */
    SOCKET probe = socket(AF_INET, SOCK_DGRAM, 0);
    if (probe != INVALID_SOCKET) {
        struct sockaddr_in serv;
        memset(&serv, 0, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = inet_addr("8.8.8.8");
        serv.sin_port = htons(53);

        if (connect(probe, (const struct sockaddr *)&serv, sizeof(serv)) == 0) {
            struct sockaddr_in localAddr;
            int nameLen = sizeof(localAddr);
            if (getsockname(probe, (struct sockaddr *)&localAddr, &nameLen) == 0) {
                const char *ip = inet_ntoa(localAddr.sin_addr);
                if (ip && strcmp(ip, "0.0.0.0") != 0) {
                    strncpy(outIp, ip, maxLen - 1);
                    outIp[maxLen - 1] = '\0';
                    closesocket(probe);
                    return;
                }
            }
        }
        closesocket(probe);
    }

    /* Fallback to gethostname */
    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent *he = gethostbyname(hostname);
        if (he && he->h_addr_list && he->h_addr_list[0]) {
            struct in_addr addr;
            memcpy(&addr, he->h_addr_list[0], sizeof(struct in_addr));
            const char *ip = inet_ntoa(addr);
            if (ip) {
                strncpy(outIp, ip, maxLen - 1);
                outIp[maxLen - 1] = '\0';
                return;
            }
        }
    }

    strncpy(outIp, "127.0.0.1", maxLen - 1);
    outIp[maxLen - 1] = '\0';
}

bool net_start_host(int port)
{
    if (!net_init()) return false;
    net_disconnect();

    s_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_listenSock == INVALID_SOCKET) {
        snprintf(s_statusMsg, sizeof(s_statusMsg), "Host socket creation failed");
        return false;
    }

    int opt = 1;
    setsockopt(s_listenSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    setNonBlocking(s_listenSock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((u_short)port);

    if (bind(s_listenSock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        snprintf(s_statusMsg, sizeof(s_statusMsg), "Bind failed on port %d", port);
        closesocket(s_listenSock);
        s_listenSock = INVALID_SOCKET;
        return false;
    }

    if (listen(s_listenSock, 1) == SOCKET_ERROR) {
        snprintf(s_statusMsg, sizeof(s_statusMsg), "Listen failed on port %d", port);
        closesocket(s_listenSock);
        s_listenSock = INVALID_SOCKET;
        return false;
    }

    s_state = NET_STATE_LISTENING;
    snprintf(s_statusMsg, sizeof(s_statusMsg), "Waiting for client on port %d...", port);
    return true;
}

bool net_start_client(const char *ip, int port)
{
    if (!net_init()) return false;
    net_disconnect();

    s_peerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_peerSock == INVALID_SOCKET) {
        snprintf(s_statusMsg, sizeof(s_statusMsg), "Client socket creation failed");
        return false;
    }

    setNonBlocking(s_peerSock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons((u_short)port);

    int res = connect(s_peerSock, (struct sockaddr *)&addr, sizeof(addr));
    if (res == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
            snprintf(s_statusMsg, sizeof(s_statusMsg), "Connect error (%d)", err);
            closesocket(s_peerSock);
            s_peerSock = INVALID_SOCKET;
            return false;
        }
    }

    s_state = NET_STATE_CONNECTING;
    snprintf(s_statusMsg, sizeof(s_statusMsg), "Connecting to %s:%d...", ip, port);
    return true;
}

void net_poll(NetEvents *outEvents)
{
    memset(outEvents, 0, sizeof(NetEvents));

    /* 1. If HOST is listening for incoming client connection */
    if (s_state == NET_STATE_LISTENING) {
        SOCKET client = accept(s_listenSock, NULL, NULL);
        if (client != INVALID_SOCKET) {
            s_peerSock = client;
            setNonBlocking(s_peerSock);

            /* Disable Nagle's algorithm for instant low-latency LAN response */
            int nodelay = 1;
            setsockopt(s_peerSock, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(nodelay));

            s_state = NET_STATE_CONNECTED;
            snprintf(s_statusMsg, sizeof(s_statusMsg), "Client connected!");
            net_send_handshake();
        }
        return;
    }

    /* 2. If CLIENT is in the process of connecting */
    if (s_state == NET_STATE_CONNECTING) {
        fd_set writeFds, errFds;
        FD_ZERO(&writeFds);
        FD_ZERO(&errFds);
        FD_SET(s_peerSock, &writeFds);
        FD_SET(s_peerSock, &errFds);

        struct timeval tv = { 0, 0 };
        int sel = select(0, NULL, &writeFds, &errFds, &tv);

        if (sel > 0) {
            if (FD_ISSET(s_peerSock, &writeFds)) {
                int err = 0;
                int len = sizeof(err);
                getsockopt(s_peerSock, SOL_SOCKET, SO_ERROR, (char *)&err, &len);

                if (err == 0) {
                    int nodelay = 1;
                    setsockopt(s_peerSock, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(nodelay));

                    s_state = NET_STATE_CONNECTED;
                    snprintf(s_statusMsg, sizeof(s_statusMsg), "Connected to Host!");
                    net_send_handshake();
                } else {
                    s_state = NET_STATE_DISCONNECTED;
                    snprintf(s_statusMsg, sizeof(s_statusMsg), "Connection refused / host unreachable");
                    outEvents->isDisconnected = true;
                    closesocket(s_peerSock);
                    s_peerSock = INVALID_SOCKET;
                }
            } else if (FD_ISSET(s_peerSock, &errFds)) {
                s_state = NET_STATE_DISCONNECTED;
                snprintf(s_statusMsg, sizeof(s_statusMsg), "Connection failed");
                outEvents->isDisconnected = true;
                closesocket(s_peerSock);
                s_peerSock = INVALID_SOCKET;
            }
        }
        return;
    }

    /* 3. If CONNECTED, non-blockingly receive incoming packets */
    if (s_state == NET_STATE_CONNECTED) {
        int space = RX_BUFFER_SIZE - s_rxLen;
        if (space > 0) {
            int n = recv(s_peerSock, (char *)s_rxBuf + s_rxLen, space, 0);
            if (n > 0) {
                s_rxLen += n;
            } else if (n == 0) {
                /* Peer closed gracefully */
                s_state = NET_STATE_DISCONNECTED;
                snprintf(s_statusMsg, sizeof(s_statusMsg), "Opponent disconnected");
                outEvents->isDisconnected = true;
                closesocket(s_peerSock);
                s_peerSock = INVALID_SOCKET;
                return;
            } else {
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK) {
                    s_state = NET_STATE_DISCONNECTED;
                    snprintf(s_statusMsg, sizeof(s_statusMsg), "Socket error: %d", err);
                    outEvents->isDisconnected = true;
                    closesocket(s_peerSock);
                    s_peerSock = INVALID_SOCKET;
                    return;
                }
            }
        }

        /* Parse complete packets from the stream */
        while (s_rxLen > 0) {
            uint8_t type = s_rxBuf[0];
            int packetSize = 0;

            switch (type) {
                case MSG_HANDSHAKE:
                case MSG_REMATCH:
                case MSG_DISCONNECT:
                    packetSize = 1;
                    break;
                case MSG_CHESS_MOVE:
                    packetSize = 5;
                    break;
                case MSG_PROMOTION:
                    packetSize = 2;
                    break;
                case MSG_BOXING_INPUT:
                    packetSize = 3;
                    break;
                case MSG_HOST_SYNC:
                    packetSize = (int)sizeof(NetHostSyncPayload);
                    break;
                default:
                    /* Unrecognized message byte, drop 1 byte to realign */
                    memmove(s_rxBuf, s_rxBuf + 1, s_rxLen - 1);
                    s_rxLen--;
                    continue;
            }

            if (s_rxLen < packetSize) {
                /* Incomplete packet, wait for more data */
                break;
            }

            /* Process complete packet */
            if (type == MSG_HANDSHAKE) {
                outEvents->hasHandshake = true;
            } else if (type == MSG_CHESS_MOVE) {
                outEvents->hasChessMove = true;
                outEvents->fromR = (int)s_rxBuf[1];
                outEvents->fromC = (int)s_rxBuf[2];
                outEvents->toR   = (int)s_rxBuf[3];
                outEvents->toC   = (int)s_rxBuf[4];
            } else if (type == MSG_PROMOTION) {
                outEvents->hasPromotion = true;
                outEvents->promotionPiece = (char)s_rxBuf[1];
            } else if (type == MSG_BOXING_INPUT) {
                uint16_t bits = 0;
                memcpy(&bits, s_rxBuf + 1, sizeof(uint16_t));
                outEvents->hasRemoteInput = true;
                decodePlayerInput(bits, &outEvents->remoteInput);
            } else if (type == MSG_HOST_SYNC) {
                outEvents->hasHostSync = true;
                memcpy(&outEvents->hostSync, s_rxBuf, sizeof(NetHostSyncPayload));
            } else if (type == MSG_REMATCH) {
                outEvents->hasRematch = true;
            } else if (type == MSG_DISCONNECT) {
                outEvents->isDisconnected = true;
                s_state = NET_STATE_DISCONNECTED;
                snprintf(s_statusMsg, sizeof(s_statusMsg), "Opponent left the match");
            }

            /* Shift remaining bytes */
            memmove(s_rxBuf, s_rxBuf + packetSize, s_rxLen - packetSize);
            s_rxLen -= packetSize;
        }
    }
}

static bool sendRaw(const void *data, int len)
{
    if (s_state != NET_STATE_CONNECTED || s_peerSock == INVALID_SOCKET) {
        return false;
    }
    int sent = send(s_peerSock, (const char *)data, len, 0);
    return (sent == len);
}

bool net_send_handshake(void)
{
    uint8_t msg = MSG_HANDSHAKE;
    return sendRaw(&msg, 1);
}

bool net_send_chess_move(int fromR, int fromC, int toR, int toC)
{
    uint8_t packet[5];
    packet[0] = MSG_CHESS_MOVE;
    packet[1] = (uint8_t)fromR;
    packet[2] = (uint8_t)fromC;
    packet[3] = (uint8_t)toR;
    packet[4] = (uint8_t)toC;
    return sendRaw(packet, sizeof(packet));
}

bool net_send_promotion(char pieceChoice)
{
    uint8_t packet[2];
    packet[0] = MSG_PROMOTION;
    packet[1] = (uint8_t)pieceChoice;
    return sendRaw(packet, sizeof(packet));
}

bool net_send_boxing_input(const PlayerInput *input)
{
    uint8_t packet[3];
    packet[0] = MSG_BOXING_INPUT;
    uint16_t bits = encodePlayerInput(input);
    memcpy(packet + 1, &bits, sizeof(uint16_t));
    return sendRaw(packet, sizeof(packet));
}

bool net_send_host_sync(const GameState *game)
{
    NetHostSyncPayload payload;
    memset(&payload, 0, sizeof(payload));
    payload.type = MSG_HOST_SYNC;

    const Boxer *b1 = &game->boxing.boxers[0];
    const Boxer *b2 = &game->boxing.boxers[1];

    payload.p1Health = b1->health;
    payload.p2Health = b2->health;
    payload.p1DisplayHealth = b1->displayHealth;
    payload.p2DisplayHealth = b2->displayHealth;
    payload.p1Stamina = b1->stamina;
    payload.p2Stamina = b2->stamina;
    payload.p1Guard = b1->guardMeter;
    payload.p2Guard = b2->guardMeter;
    payload.p1X = b1->x;
    payload.p1Y = b1->y;
    payload.p2X = b2->x;
    payload.p2Y = b2->y;
    payload.p1Vx = b1->vx;
    payload.p1Vy = b1->vy;
    payload.p2Vx = b2->vx;
    payload.p2Vy = b2->vy;
    payload.p1Facing = (int8_t)b1->facing;
    payload.p2Facing = (int8_t)b2->facing;
    payload.p1State = (uint8_t)b1->state;
    payload.p2State = (uint8_t)b2->state;
    payload.p1StateTimer = b1->stateTimer;
    payload.p2StateTimer = b2->stateTimer;

    payload.roundTimer = game->roundTimer;
    payload.currentRound = (uint8_t)game->currentRound;
    payload.phase = (uint8_t)game->phase;
    payload.winner = (int8_t)game->winner;
    payload.winReason = (uint8_t)game->winReason;

    return sendRaw(&payload, sizeof(payload));
}

bool net_send_rematch(void)
{
    uint8_t msg = MSG_REMATCH;
    return sendRaw(&msg, 1);
}
