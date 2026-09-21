# ♟️🥊 Chess Boxing

A hybrid chess and boxing game developed in C using SDL3.

The game combines strategic chess gameplay with real-time boxing combat. 
Chess performance affects boxing advantages, creating a match where both
strategy and combat skill matter.

## Features

- ♟️ Full chess gameplay
- ♙ Legal move validation
- ♚ Check and checkmate
- ♜ Castling
- ♟ En passant
- 👑 Pawn promotion
- ⏱️ Chess timers
- 🤖 Single-player AI
- 👥 Local 2-player
- 🌐 LAN multiplayer
- 🥊 Real-time boxing combat
- 👊 Light punch
- 💥 Heavy punch
- ⬆️ Uppercut
- 🛡️ Blocking
- 💨 Dodge
- 🧎 Crouching
- 💪 Stamina system
- 🛡️ Guard break
- 💥 Knockback
- ❤️ Health system
- 🏆 KO system
- 🔄 Chess/boxing round transitions
- ♟️ Chess material advantage affecting boxing
- 🔌 TCP-based LAN multiplayer

## Technologies

- C
- SDL3
- SDL3_ttf
- Winsock
- MinGW GCC

## Game Modes

### Single Player
Play against the computer with different AI difficulty levels.

### Local Multiplayer
Two players can play on the same computer.

### LAN Multiplayer
Two players can play from separate computers connected to the same local network.

## Controls

### Player 1

| Action | Key |
|---|---|
| Move Left | A |
| Move Right | D |
| Jump | W |
| Crouch | S |
| Dodge | Q / Left Shift |
| Light Punch | F |
| Heavy Punch | G |
| Uppercut | V |
| Block | H |

### Player 2

| Action | Key |
|---|---|
| Move Left | Numpad 4 |
| Move Right | Numpad 6 |
| Jump | Numpad 8 |
| Crouch | Numpad 5 |
| Dodge | Numpad 0 |
| Light Punch | Numpad 1 |
| Heavy Punch | Numpad 2 |
| Uppercut | Numpad 3 |
| Block | Numpad 7 |

## Building

Compile using GCC with SDL3 and SDL3_ttf.

Example:

gcc src/main.c src/game.c src/chess.c src/boxing.c src/renderer.c src/ai.c src/network.c -I... -L... -lSDL3_ttf -lSDL3 -lws2_32 -o chessboxing.exe

## Project Status

Current version is a playable prototype with:

- Chess
- Boxing
- AI
- Local multiplayer
- LAN multiplayer

Further gameplay features and polish will be added in future versions.

## Future Plans

- Improved boxing combat
- More advanced AI
- Sound effects and music
- Improved character animations
- Customizable controls
- Additional game modes
- Gameplay balancing
- Visual polish
