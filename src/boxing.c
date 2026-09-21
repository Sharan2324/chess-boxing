#include "boxing.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GRAVITY 1400.0f
#define JUMP_VELOCITY -560.0f
#define WALK_SPEED 260.0f
#define BLOCK_WALK_SPEED 80.0f
#define CROUCH_WALK_SPEED 50.0f

#define LIGHT_PUNCH_DURATION 0.22f
#define HEAVY_PUNCH_DURATION 0.42f
#define UPPERCUT_DURATION 0.34f
#define DODGE_DURATION 0.20f

void initBoxing(GameState *game)
{
    BoxingState *b = &game->boxing;

    for (int i = 0; i < 2; i++) {
        b->boxers[i].x = (i == 0) ? 320.0f : 680.0f;
        b->boxers[i].y = FLOOR_Y;
        b->boxers[i].vx = 0.0f;
        b->boxers[i].vy = 0.0f;
        b->boxers[i].facing = (i == 0) ? 1 : -1;
        b->boxers[i].health = BOXER_MAX_HEALTH;
        b->boxers[i].displayHealth = BOXER_MAX_HEALTH;

        b->boxers[i].stamina = 100.0f;
        b->boxers[i].maxStamina = 100.0f;
        b->boxers[i].isExhausted = false;
        b->boxers[i].exhaustionTimer = 0.0f;

        b->boxers[i].guardMeter = 100.0f;
        b->boxers[i].maxGuardMeter = 100.0f;
        b->boxers[i].isGuardBroken = false;
        b->boxers[i].guardBreakTimer = 0.0f;

        b->boxers[i].isDodging = false;
        b->boxers[i].dodgeTimer = 0.0f;
        b->boxers[i].dodgeDirection = 0;

        b->boxers[i].state = BOXER_IDLE;
        b->boxers[i].stateTimer = 0.0f;
        b->boxers[i].isGrounded = true;
        b->boxers[i].hasHit = false;
        b->boxers[i].damageMultiplier = 1.0f;

        memset(&b->prevInputs[i], 0, sizeof(PlayerInput));
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        b->particles[i].active = false;
    }

    b->shakeIntensity = 0.0f;
    b->shakeTimer = 0.0f;
}

void resetBoxingRound(GameState *game)
{
    BoxingState *b = &game->boxing;

    for (int i = 0; i < 2; i++) {
        b->boxers[i].x = (i == 0) ? 320.0f : 680.0f;
        b->boxers[i].y = FLOOR_Y;
        b->boxers[i].vx = 0.0f;
        b->boxers[i].vy = 0.0f;
        b->boxers[i].facing = (i == 0) ? 1 : -1;

        b->boxers[i].stamina = b->boxers[i].maxStamina;
        b->boxers[i].isExhausted = false;
        b->boxers[i].exhaustionTimer = 0.0f;

        b->boxers[i].guardMeter = b->boxers[i].maxGuardMeter;
        b->boxers[i].isGuardBroken = false;
        b->boxers[i].guardBreakTimer = 0.0f;

        b->boxers[i].isDodging = false;
        b->boxers[i].dodgeTimer = 0.0f;

        if (b->boxers[i].state != BOXER_KO) {
            b->boxers[i].state = BOXER_IDLE;
            b->boxers[i].stateTimer = 0.0f;
        }

        memset(&b->prevInputs[i], 0, sizeof(PlayerInput));
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        b->particles[i].active = false;
    }
}

void spawnHitParticles(BoxingState *box, float x, float y, int count, unsigned char r, unsigned char g, unsigned char b)
{
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (!box->particles[i].active) {
            box->particles[i].active = true;
            box->particles[i].x = x;
            box->particles[i].y = y;
            float angle = ((float)rand() / (float)RAND_MAX) * 6.28318f;
            float speed = 80.0f + ((float)rand() / (float)RAND_MAX) * 280.0f;
            box->particles[i].vx = cosf(angle) * speed;
            box->particles[i].vy = sinf(angle) * speed - 60.0f;
            box->particles[i].life = 0.0f;
            box->particles[i].maxLife = 0.25f + ((float)rand() / (float)RAND_MAX) * 0.25f;
            box->particles[i].r = r;
            box->particles[i].g = g;
            box->particles[i].b = b;
            spawned++;
        }
    }
}

void readKeyboardInput(const bool *keys, PlayerInput *p1Input, PlayerInput *p2Input)
{
    if (p1Input) {
        memset(p1Input, 0, sizeof(PlayerInput));
        if (keys) {
            p1Input->left = keys[SDL_SCANCODE_A];
            p1Input->right = keys[SDL_SCANCODE_D];
            p1Input->up = keys[SDL_SCANCODE_W];
            p1Input->down = keys[SDL_SCANCODE_S];
            p1Input->lightPunch = keys[SDL_SCANCODE_F];
            p1Input->heavyPunch = keys[SDL_SCANCODE_G];
            p1Input->uppercut = keys[SDL_SCANCODE_V];
            p1Input->block = keys[SDL_SCANCODE_H];
            p1Input->dodge = keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_LSHIFT];
        }
    }

    if (p2Input) {
        memset(p2Input, 0, sizeof(PlayerInput));
        if (keys) {
            p2Input->left = keys[SDL_SCANCODE_LEFT];
            p2Input->right = keys[SDL_SCANCODE_RIGHT];
            p2Input->up = keys[SDL_SCANCODE_UP];
            p2Input->down = keys[SDL_SCANCODE_DOWN];
            p2Input->lightPunch = keys[SDL_SCANCODE_K];
            p2Input->heavyPunch = keys[SDL_SCANCODE_L];
            p2Input->uppercut = keys[SDL_SCANCODE_PERIOD];
            p2Input->block = keys[SDL_SCANCODE_SEMICOLON];
            p2Input->dodge = keys[SDL_SCANCODE_SLASH] || keys[SDL_SCANCODE_RSHIFT];
        }
    }
}

static void updateBoxerPhysics(Boxer *b, float dt)
{
    if (!b->isGrounded) {
        b->vy += GRAVITY * dt;
    }

    b->x += b->vx * dt;
    b->y += b->vy * dt;

    if (b->y >= FLOOR_Y) {
        b->y = FLOOR_Y;
        b->vy = 0.0f;
        b->isGrounded = true;
    } else {
        b->isGrounded = false;
    }

    if (b->x < RING_MIN_X) {
        b->x = RING_MIN_X;
        b->vx = 0.0f;
    }
    if (b->x > RING_MAX_X) {
        b->x = RING_MAX_X;
        b->vx = 0.0f;
    }

    if (b->isGrounded && b->state != BOXER_WALK && b->state != BOXER_CROUCH && b->state != BOXER_DODGE) {
        b->vx *= 0.70f;
        if (fabsf(b->vx) < 5.0f) b->vx = 0.0f;
    }
}

void updateBoxing(GameState *game, const PlayerInput inputs[2], float dt)
{
    BoxingState *box = &game->boxing;

    /* Update screen shake */
    if (box->shakeTimer > 0.0f) {
        box->shakeTimer -= dt;
        if (box->shakeTimer <= 0.0f) {
            box->shakeIntensity = 0.0f;
        }
    }

    /* Update particles */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (box->particles[i].active) {
            box->particles[i].life += dt;
            if (box->particles[i].life >= box->particles[i].maxLife) {
                box->particles[i].active = false;
            } else {
                box->particles[i].x += box->particles[i].vx * dt;
                box->particles[i].y += box->particles[i].vy * dt;
                box->particles[i].vy += GRAVITY * 0.4f * dt;
            }
        }
    }

    /* Smooth lagging damage bars */
    for (int i = 0; i < 2; i++) {
        if (box->boxers[i].displayHealth > box->boxers[i].health) {
            box->boxers[i].displayHealth -= (BOXER_MAX_HEALTH * 0.25f) * dt;
            if (box->boxers[i].displayHealth < box->boxers[i].health) {
                box->boxers[i].displayHealth = box->boxers[i].health;
            }
        } else if (box->boxers[i].displayHealth < box->boxers[i].health) {
            box->boxers[i].displayHealth = box->boxers[i].health;
        }
    }

    /* If Game Over, update physics and return */
    if (game->phase == PHASE_GAME_OVER) {
        for (int i = 0; i < 2; i++) {
            updateBoxerPhysics(&box->boxers[i], dt);
        }
        return;
    }

    /* Auto face each other unless knocked out */
    if (box->boxers[0].state != BOXER_KO && box->boxers[1].state != BOXER_KO) {
        if (box->boxers[0].x < box->boxers[1].x) {
            box->boxers[0].facing = 1;
            box->boxers[1].facing = -1;
        } else {
            box->boxers[0].facing = -1;
            box->boxers[1].facing = 1;
        }
    }

    /* -------------------------------------------------------------
       PROCESS INPUTS & COMBAT STATE FOR BOTH BOXERS
       ------------------------------------------------------------- */
    for (int i = 0; i < 2; i++) {
        Boxer *b = &box->boxers[i];
        const PlayerInput *in = &inputs[i];
        const PlayerInput *prevIn = &box->prevInputs[i];

        if (b->state == BOXER_KO) continue;

        /* Stamina & guard break recovery updates */
        if (b->isExhausted) {
            b->exhaustionTimer -= dt;
            if (b->exhaustionTimer <= 0.0f) {
                b->isExhausted = false;
            }
        }

        if (b->isGuardBroken) {
            b->guardBreakTimer -= dt;
            if (b->guardBreakTimer <= 0.0f) {
                b->isGuardBroken = false;
                b->guardMeter = 40.0f;
                b->state = BOXER_IDLE;
            }
        } else if (b->state != BOXER_BLOCK) {
            b->guardMeter += 25.0f * dt;
            if (b->guardMeter > b->maxGuardMeter) b->guardMeter = b->maxGuardMeter;
        }

        /* Stamina recharge when neutral */
        if (b->state != BOXER_BLOCK && b->state != BOXER_PUNCH_LIGHT &&
            b->state != BOXER_PUNCH_HEAVY && b->state != BOXER_UPPERCUT && b->state != BOXER_DODGE) {
            b->stamina += 22.0f * dt;
            if (b->stamina > b->maxStamina) b->stamina = b->maxStamina;
        } else if (b->state == BOXER_BLOCK) {
            b->stamina -= 10.0f * dt;
            if (b->stamina <= 0.0f) {
                b->stamina = 0.0f;
                b->isExhausted = true;
                b->exhaustionTimer = 1.2f;
                b->state = BOXER_IDLE;
            }
        }

        /* If stunned or guard broken, cannot take input actions */
        if (b->state == BOXER_HIT_STUN || b->state == BOXER_GUARD_BROKEN) {
            continue;
        }

        bool canAct = (b->state != BOXER_PUNCH_LIGHT && b->state != BOXER_PUNCH_HEAVY &&
                       b->state != BOXER_UPPERCUT && b->state != BOXER_DODGE);

        /* Edge-triggered attack actions */
        if (canAct && !b->isExhausted) {
            if (in->lightPunch && !prevIn->lightPunch && b->stamina >= 8.0f) {
                b->state = BOXER_PUNCH_LIGHT;
                b->stateTimer = 0.0f;
                b->hasHit = false;
                b->stamina -= 8.0f;
                canAct = false;
            }
            else if (in->heavyPunch && !prevIn->heavyPunch && b->stamina >= 22.0f) {
                b->state = BOXER_PUNCH_HEAVY;
                b->stateTimer = 0.0f;
                b->hasHit = false;
                b->stamina -= 22.0f;
                canAct = false;
            }
            else if (in->uppercut && !prevIn->uppercut && b->stamina >= 18.0f) {
                b->state = BOXER_UPPERCUT;
                b->stateTimer = 0.0f;
                b->hasHit = false;
                b->stamina -= 18.0f;
                canAct = false;
            }
            else if (in->dodge && !prevIn->dodge && b->stamina >= 15.0f && b->isGrounded) {
                b->state = BOXER_DODGE;
                b->stateTimer = 0.0f;
                b->isDodging = true;
                b->dodgeTimer = DODGE_DURATION;
                b->vx = -b->facing * 420.0f;
                b->stamina -= 15.0f;
                spawnHitParticles(box, b->x, b->y - 40.0f, 6, 180, 220, 255);
                canAct = false;
            }
        }

        /* Continuous defensive & movement states */
        if (canAct) {
            if (in->down && b->isGrounded) {
                b->state = BOXER_CROUCH;
            } else if (b->state == BOXER_CROUCH && !in->down) {
                b->state = BOXER_IDLE;
            }

            if (in->block && b->isGrounded && b->state != BOXER_CROUCH && !b->isExhausted) {
                b->state = BOXER_BLOCK;
            } else if (b->state == BOXER_BLOCK && !in->block) {
                b->state = BOXER_IDLE;
            }

            if (in->up && b->isGrounded && b->state != BOXER_BLOCK && b->state != BOXER_CROUCH && !b->isExhausted) {
                if (b->stamina >= 10.0f) {
                    b->vy = JUMP_VELOCITY;
                    b->isGrounded = false;
                    b->stamina -= 10.0f;
                }
            }

            /* Horizontal Movement */
            float speed = WALK_SPEED;
            if (b->state == BOXER_BLOCK) speed = BLOCK_WALK_SPEED;
            else if (b->state == BOXER_CROUCH) speed = CROUCH_WALK_SPEED;
            if (b->isExhausted) speed *= 0.55f;

            float moveDir = 0.0f;
            if (in->left) moveDir -= 1.0f;
            if (in->right) moveDir += 1.0f;

            if (moveDir != 0.0f) {
                b->vx = moveDir * speed;
                if (b->state == BOXER_IDLE && b->isGrounded) {
                    b->state = BOXER_WALK;
                }
            } else if (b->state == BOXER_WALK) {
                b->state = BOXER_IDLE;
            }
        }
    }

    /* Update states, timers, and animations */
    for (int i = 0; i < 2; i++) {
        Boxer *b = &box->boxers[i];
        b->stateTimer += dt;

        if (b->state == BOXER_DODGE) {
            b->dodgeTimer -= dt;
            if (b->dodgeTimer <= 0.0f) {
                b->isDodging = false;
                b->state = BOXER_IDLE;
                b->stateTimer = 0.0f;
            }
        } else if (b->state == BOXER_PUNCH_LIGHT) {
            if (b->stateTimer >= LIGHT_PUNCH_DURATION) {
                b->state = BOXER_IDLE;
                b->stateTimer = 0.0f;
            }
        } else if (b->state == BOXER_PUNCH_HEAVY) {
            if (b->stateTimer >= HEAVY_PUNCH_DURATION) {
                b->state = BOXER_IDLE;
                b->stateTimer = 0.0f;
            }
        } else if (b->state == BOXER_UPPERCUT) {
            if (b->stateTimer >= UPPERCUT_DURATION) {
                b->state = BOXER_IDLE;
                b->stateTimer = 0.0f;
            }
        } else if (b->state == BOXER_HIT_STUN) {
            if (b->stateTimer >= 0.22f) {
                b->state = BOXER_IDLE;
                b->stateTimer = 0.0f;
            }
        }

        updateBoxerPhysics(b, dt);
    }

    /* Pushing collision between boxers */
    float dist = box->boxers[1].x - box->boxers[0].x;
    float minDist = 52.0f;
    if (dist > 0 && dist < minDist) {
        float push = (minDist - dist) * 0.5f;
        box->boxers[0].x -= push;
        box->boxers[1].x += push;
    } else if (dist < 0 && dist > -minDist) {
        float push = (minDist + dist) * 0.5f;
        box->boxers[0].x += push;
        box->boxers[1].x -= push;
    }

    /* -------------------------------------------------------------
       COMBAT HITBOX DETECTION & RESOLUTION
       ------------------------------------------------------------- */
    for (int attackerIdx = 0; attackerIdx < 2; attackerIdx++) {
        int defenderIdx = 1 - attackerIdx;
        Boxer *att = &box->boxers[attackerIdx];
        Boxer *def = &box->boxers[defenderIdx];

        if (def->state == BOXER_KO) continue;
        if (def->isDodging) continue;

        bool isActiveFrame = false;
        bool isHeavy = false;
        bool isUppercut = false;
        float reach = 0.0f;
        float baseDmg = 0.0f;
        float punchY = att->y - 85.0f;

        if (att->state == BOXER_PUNCH_LIGHT) {
            if (att->stateTimer >= 0.05f && att->stateTimer <= 0.15f) {
                isActiveFrame = true;
                reach = 72.0f;
                baseDmg = 7.0f;
                punchY = att->y - 85.0f;
            }
        } else if (att->state == BOXER_PUNCH_HEAVY) {
            if (att->stateTimer >= 0.14f && att->stateTimer <= 0.28f) {
                isActiveFrame = true;
                isHeavy = true;
                reach = 94.0f;
                baseDmg = 18.0f;
                punchY = att->y - 88.0f;
            }
        } else if (att->state == BOXER_UPPERCUT) {
            if (att->stateTimer >= 0.08f && att->stateTimer <= 0.22f) {
                isActiveFrame = true;
                isUppercut = true;
                reach = 65.0f;
                baseDmg = 14.0f;
                punchY = att->y - 65.0f;
            }
        }

        if (isActiveFrame && !att->hasHit) {
            float punchX = att->x + att->facing * reach;

            float defH = (def->state == BOXER_CROUCH) ? BOXER_CROUCH_HEIGHT : BOXER_HEIGHT;
            float defLeft = def->x - BOXER_WIDTH * 0.5f;
            float defRight = def->x + BOXER_WIDTH * 0.5f;
            float defTop = def->y - defH;
            float defBottom = def->y;

            bool crouchAvoidsHighPunch = (def->state == BOXER_CROUCH && !isUppercut);

            bool hitX = (punchX >= defLeft && punchX <= defRight) ||
                        (att->facing > 0 && att->x < def->x && punchX >= defLeft) ||
                        (att->facing < 0 && att->x > def->x && punchX <= defRight);
            bool hitY = (punchY >= defTop && punchY <= defBottom);

            if (hitX && hitY && !crouchAvoidsHighPunch) {
                att->hasHit = true;
                float finalDmg = baseDmg * att->damageMultiplier;

                bool isBlocked = (def->state == BOXER_BLOCK) && (def->facing != att->facing) && !def->isGuardBroken;

                if (isBlocked) {
                    float guardLoss = isHeavy ? 40.0f : (isUppercut ? 30.0f : 15.0f);
                    def->guardMeter -= guardLoss;

                    float chipDmg = isHeavy ? 4.0f : 1.0f;
                    chipDmg *= att->damageMultiplier;
                    def->health -= chipDmg;
                    if (def->health < 0.0f) def->health = 0.0f;

                    def->stamina -= 8.0f;
                    if (def->stamina < 0.0f) def->stamina = 0.0f;

                    def->vx = att->facing * (isHeavy ? 190.0f : 70.0f);
                    spawnHitParticles(box, punchX, punchY, 8, 100, 220, 255);

                    if (def->guardMeter <= 0.0f) {
                        def->guardMeter = 0.0f;
                        def->isGuardBroken = true;
                        def->guardBreakTimer = 0.65f;
                        def->state = BOXER_GUARD_BROKEN;
                        box->shakeIntensity = 8.0f;
                        box->shakeTimer = 0.20f;
                        spawnHitParticles(box, def->x, def->y - 70.0f, 24, 255, 100, 255);
                        printf("GUARD BREAK! Player %d guard shattered!\n", defenderIdx + 1);
                    } else {
                        printf("Player %d BLOCKED! (%.1f chip dmg, Guard: %.0f/100)\n",
                               defenderIdx + 1, chipDmg, def->guardMeter);
                    }
                } else {
                    def->health -= finalDmg;
                    if (def->health < 0.0f) def->health = 0.0f;

                    def->state = BOXER_HIT_STUN;
                    def->stateTimer = 0.0f;

                    if (isUppercut) {
                        def->vy = -340.0f;
                        def->vx = att->facing * 160.0f;
                        box->shakeIntensity = 10.0f;
                        box->shakeTimer = 0.22f;
                        spawnHitParticles(box, punchX, punchY, 18, 255, 140, 40);
                        printf("UPPERCUT LANDED! Player %d dealt %.1f dmg\n", attackerIdx + 1, finalDmg);
                    } else if (isHeavy) {
                        def->vy = -180.0f;
                        def->vx = att->facing * 440.0f;
                        box->shakeIntensity = 12.0f;
                        box->shakeTimer = 0.25f;
                        spawnHitParticles(box, punchX, punchY, 22, 255, 60, 40);
                        printf("CRITICAL HIT! Player %d landed HEAVY PUNCH (%.1f dmg)\n", attackerIdx + 1, finalDmg);
                    } else {
                        def->vx = att->facing * 240.0f;
                        box->shakeIntensity = 4.0f;
                        box->shakeTimer = 0.12f;
                        spawnHitParticles(box, punchX, punchY, 10, 255, 200, 50);
                        printf("Hit! Player %d landed light punch (%.1f dmg)\n", attackerIdx + 1, finalDmg);
                    }
                }

                if (def->health <= 0.0f) {
                    def->health = 0.0f;
                    def->state = BOXER_KO;
                    def->stateTimer = 0.0f;
                    def->vx = -def->facing * 260.0f;
                    def->vy = -380.0f;
                    box->shakeIntensity = 18.0f;
                    box->shakeTimer = 0.45f;
                    spawnHitParticles(box, def->x, def->y - 60.0f, 32, 255, 30, 30);

                    game->winner = attackerIdx;
                    game->winReason = WIN_KO;
                    game->phase = PHASE_GAME_OVER;
                    printf("KNOCKOUT! Player %d wins by KO!\n", attackerIdx + 1);
                }
            }
        }
    }

    /* Store inputs as prevInputs for edge-triggered actions next frame */
    for (int i = 0; i < 2; i++) {
        box->prevInputs[i] = inputs[i];
    }
}
