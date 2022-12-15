#include <cmath>
#include "tractor.h"
#include "utils.h"

const int totalRainbowColors = 6;
Color rainbowColors[totalRainbowColors] {
    {230, 55, 55, 255},
    {230, 143, 55, 255},
    {230, 224, 55, 255},
    {90, 230, 55, 255},
    {55, 140, 230, 255},
    {99, 55, 230, 255}
};

void Tractor::Init(int sw, Camera2D cam, int startY) {
    idleAnimationTimer = 0;
    runningAnimationTimer = 0;
    cartDesiredDis = 22;
    flipTimer = 0;
    rainbowTimer = 0;

    ySquish = 0;
    
    facingRight = true;
    isMoving = false;
    isLongWagon = false;
    isRainbow = false;
    color = Color {255, 255, 255, 255};
    
    smokeParticleTimer = 0;
    smokeParticlces.clear();

    momentum = {0, 0};
    rect = Rectangle {cam.offset.x + (GetScreenWidth() / cam.zoom) / 2 - 16, (float) startY - 32, 32, 32};
    hitbox = Rectangle {3, 11, 27, 21};
    cartX = rect.x + rect.width / 2 - (facingRight ? cartDesiredDis : -cartDesiredDis);
}

void Tractor::Update(Camera2D cam, GameData &game, bool canMove, float customSpeed) {
    float speed;
    if (customSpeed == -1)
        speed = game.speedUpgrade.values[game.speedUpgrade.unlocked];
    else
        speed = customSpeed;

    float acceleration = 0.25 * speed;
    float deceleration = 0.125 * speed;
    
    if (canMove) {
        isMoving = false;

        bool keyRight = (IsKeyDown(KEY_D) && !IsKeyDown(KEY_A)) || (IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT));
        bool keyLeft = (IsKeyDown(KEY_A) && !IsKeyDown(KEY_D)) || (IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT));

        // Movement
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !(keyRight || keyLeft)) {
            int mx = GetMousePosition().x;
            if ((mx > (rect.x + rect.width / 2 + 8) * cam.zoom || mx > GetScreenWidth() - 128) && mx > 128) {
                keyRight = true;
            } else if (mx < (rect.x + rect.width / 2 - 8) * cam.zoom || mx < 128) {
                keyLeft = true;
            }
        }

        if (keyRight) {
            if (momentum.x < speed) {
                momentum.x += acceleration;
                if (momentum.x > speed) {
                    momentum.x = speed;
                }
            }
            if (!facingRight) flipTimer = 10;
            facingRight = true;
            isMoving = true;
        } else if (keyLeft) {
            if (momentum.x > -speed) {
                momentum.x -= acceleration;
                if (momentum.x < -speed) {
                    momentum.x = -speed;
                }
            }
            if (facingRight) flipTimer = 10;
            facingRight = false;
            isMoving = true;
        } else {
            if (abs(momentum.x) < deceleration) {
                momentum.x = 0;
            } else {
                momentum.x -= momentum.x > 0 ? deceleration : -deceleration;
            }
        }

        // Squish
        if (IsKeyDown(KEY_W) && !IsKeyDown(KEY_S)) {
            if (ySquish > -5) {
                ySquish -= 0.75;
            } else {
                ySquish = -5;
            }
        } else if (IsKeyDown(KEY_S) && !IsKeyDown(KEY_W)) {
            if (ySquish < 5) {
                ySquish += 0.75;
            } else {
                ySquish = 5;
            }
        } else {
            ySquish = Diminish(ySquish, 0.75);
        }

        rect.x += momentum.x;
    }

    if (facingRight) {
        if (rect.x + hitbox.x + hitbox.width > cam.offset.x + GetScreenWidth() / cam.zoom) {
            rect.x = (cam.offset.x + GetScreenWidth() / cam.zoom) - hitbox.width - hitbox.x;
            if (momentum.x >= speed) momentum.x = -momentum.x;
        } 
    } else {
        if (rect.x + hitbox.x < cam.offset.x) {
            rect.x = cam.offset.x - hitbox.x;
            if (momentum.x <= -speed) momentum.x = abs(momentum.x);
        }
    }

    int desiredPos = rect.x + rect.width / 2 - (facingRight ? cartDesiredDis : -cartDesiredDis);

    float cartSpeed = speed / 2;
    if (isMoving) {
         if ((cartX > desiredPos && facingRight) || (cartX < desiredPos && !facingRight)) {
            cartSpeed = speed + 0.5;
        } else if (std::abs(cartX - desiredPos) > 15) {
            cartSpeed = std::abs(cartX - desiredPos) > 20 ? speed : speed - 0.2;
        } else {
            cartSpeed = 0.5;
        }
    }

    if (abs(cartX - desiredPos) < cartSpeed) {
        cartX = desiredPos;
    } else {
        if (cartX < desiredPos) {
            cartX += cartSpeed;
        } else if (cartX > desiredPos) {
            cartX -= cartSpeed;
        }
    }

    // if (IsKeyPressed(KEY_SPACE)) {
    //     color = Color {(unsigned char) GetRandomValue(0, 255), (unsigned char) GetRandomValue(0, 255), (unsigned char) GetRandomValue(0, 255), 255};
    //     print((int) color.r ___ (int) color.g ___ (int) color.b)
    // }
}

void Tractor::Draw(Camera2D cam, bool updateAnimations) {
    Texture2D &cartTexture = GetTexture(Textures::cart);
    Rectangle cartDest = Rectangle {cartX - cartTexture.width / 2, rect.y + 10, (float) cartTexture.width, 32};
    Rectangle cartWheelDest = {cartDest.x + 16, cartDest.y + 9, 16, 16};
    
    Vector2 lineStart = {(float) (facingRight ? cartX + 8 : cartX - 8), rect.y + 23};
    Vector2 lineEnd = {facingRight ? rect.x + 8 : rect.x + rect.width - 8, rect.y + 22};
    
    if (lineStart.x > lineEnd.x) {
        Vector2 temp = lineStart;
        lineStart = lineEnd;
        lineEnd = temp;
    }

    if (isMoving) {
        if (runningAnimationTimer % 15 < 2 && std::abs(rect.x + rect.width / 2 - cartX) > 24) {
            cartDest.y -= 1;
        }

        if (GetRandomValue(0, 3) == 0 && updateAnimations) {
            cartWheelDest.y -= 1;
        }
    }

    Vector2 wheelLargePos = facingRight ? Vector2 {2, 18} : Vector2 {14, 18};
    Vector2 wheelSmallPos = facingRight ? Vector2 {17, 20} : Vector2 {-1, 20};
    int waitDur = 20;
    int moveDur = 8;
    Rectangle tractorBackDest = rect;
    Rectangle tractorFrontDest = rect;
    Rectangle wheelLargeDest = {rect.x + wheelLargePos.x , rect.y + wheelLargePos.y, 16, 16};
    Rectangle wheelSmallDest = {rect.x + wheelSmallPos.x , rect.y + wheelSmallPos.y, 16, 16};

    if (isMoving) {
        idleAnimationTimer = 0;

        if (updateAnimations)
            runningAnimationTimer++;

        if (runningAnimationTimer % (moveDur * 2) < moveDur) {
            tractorBackDest.y -= 1;
            tractorFrontDest.y -= 1;
        }

        if (runningAnimationTimer % (waitDur * 2 + moveDur * 2) < waitDur + moveDur) {
            tractorBackDest.x += 1;
            tractorFrontDest.x += 1;
            tractorBackDest.width -= 2;
            tractorFrontDest.width -= 2;
        }

        if ((runningAnimationTimer + 10) % waitDur < moveDur || runningAnimationTimer % (moveDur * 2) < moveDur) {
            tractorBackDest.height += 1;
            tractorFrontDest.height += 1;
        }

        if (GetRandomValue(0, 2) == 0 && updateAnimations) {
            if (GetRandomValue(0, 2) == 0) {
                wheelLargeDest.y -= 1;
            } else {
                wheelSmallDest.y -= 1;
            }
        }

    } else {
        runningAnimationTimer = 0;

        if (updateAnimations)
            idleAnimationTimer++;

        if (idleAnimationTimer > moveDur * 2 + waitDur * 2) {
            idleAnimationTimer = 0;
        }

        if (idleAnimationTimer <= moveDur) {
            tractorBackDest.y -= 1;
            tractorFrontDest.y -= 1;
        } else if (idleAnimationTimer <= moveDur + waitDur) {
            tractorBackDest.y -= 2;
            tractorFrontDest.y -= 2;
        } else if (idleAnimationTimer <= moveDur * 2 + waitDur) {
            tractorBackDest.y -= 1;
            tractorFrontDest.y -= 1;
        }

        if (idleAnimationTimer <= moveDur * 2 || idleAnimationTimer > moveDur + waitDur * 2) {
            tractorBackDest.x += 1;
            tractorFrontDest.x += 1;
            tractorBackDest.width -= 2;
            tractorFrontDest.width -= 2;
        }

        if (idleAnimationTimer > moveDur * 1.5 && idleAnimationTimer < waitDur) {
            tractorBackDest.height += 1;
            tractorFrontDest.height += 1;
        }
    }

    if (flipTimer) {
        if (facingRight) {
            wheelSmallDest.x -= (float) flipTimer / 10 * 6;
            tractorBackDest.width = rect.width - (float) flipTimer / 10 * 6;
            tractorFrontDest.width = rect.width - (float) flipTimer / 10 * 6;
        } else {
            wheelSmallDest.x += (float) flipTimer / 10 * 6;
            tractorBackDest.x = rect.x + (float) flipTimer / 10 * 6;
            tractorFrontDest.x = rect.x + (float) flipTimer / 10 * 6;
            tractorBackDest.width = rect.width - (float) flipTimer / 10 * 6;
            tractorFrontDest.width = rect.width - (float) flipTimer / 10 * 6;
        }
        flipTimer--;
    }

    Color tintColor = color;
    if (isRainbow) {
        rainbowTimer++;
        if (rainbowTimer >= totalRainbowColors * 20)
            rainbowTimer = 0;

        int currentColor = rainbowTimer / 20;
        tintColor = Morph((float) (rainbowTimer - currentColor) / ((currentColor + 1) * 20), rainbowColors[currentColor], rainbowColors[(currentColor + 1) % totalRainbowColors]);
    }

    tractorBackDest.height -= ySquish;
    tractorFrontDest.height -= ySquish;
    tractorBackDest.y += ySquish;
    tractorFrontDest.y += ySquish;

    Texture2D halo = GetTexture(Textures::halo);
    Vector2 haloPos = {rect.x + rect.width / 2 - halo.width / 2, rect.y + rect.height / 2 - halo.height / 2};
    DrawTexture(halo, haloPos.x, haloPos.y, Color {252, 121, 20, 30});

    DrawRectangle(lineStart.x, lineStart.y, lineEnd.x - lineStart.x, 2, Color {95, 52, 54, 255});
    DrawTexturePro(GetTexture(Textures::wheelSmall), {0, 0, 16, 16}, cartWheelDest, {0, 0}, 0, WHITE);
    DrawTexturePro(GetTexture(Textures::cart), {0, (float) (isLongWagon ? 32 : 0), (float) cartTexture.width, 32}, cartDest, {0, 0}, 0, WHITE);

    DrawTexturePro(GetTexture(Textures::tractor), Rectangle {0, 64, (float) (facingRight ? 32 : -32), 32}, tractorBackDest, {0, 0}, 0, WHITE);
    DrawTexturePro(GetTexture(Textures::wheelLarge), Rectangle {0, 0, (float) (facingRight ? 16 : -16), 16}, wheelLargeDest, {0, 0}, 0, WHITE);
    DrawTexturePro(GetTexture(Textures::wheelSmall), Rectangle {0, 0, (float) (facingRight ? 16 : -16), 16}, wheelSmallDest, {0, 0}, 0, WHITE);
    DrawTexturePro(GetTexture(Textures::tractor), Rectangle {0, 0, (float) (facingRight ? 32 : -32), 32}, tractorFrontDest, {0, 0}, 0, tintColor);
    DrawTexturePro(GetTexture(Textures::tractor), Rectangle {0, 32, (float) (facingRight ? 32 : -32), 32}, tractorFrontDest, {0, 0}, 0, WHITE);
}

void Tractor::DrawParticles(Camera2D cam, bool updateParticles) {
    if (updateParticles)
        smokeParticleTimer--;

    if (smokeParticleTimer < 0) {
        smokeParticleTimer = 30;
        
        for (int i = GetRandomValue(8, 14); i > 0; i--) {
            SmokeParticle particle;
            particle.lifetime = GetRandomValue(45, 50);
            particle.timer = 0;
            particle.pos = Vector2 {rect.x + (facingRight ? 12 : 20) + GetRandomValue(-4, 4), rect.y + 6 + GetRandomValue(-4, 4)};
            particle.vel = Vector2 {0, -1.3};
            unsigned char greyness = (unsigned char) GetRandomValue(60, 120);
            particle.color = Color {greyness, greyness, greyness, 255};

            smokeParticlces.push_back(particle);
        }
    }

    for (int index = smokeParticlces.size() - 1; index >= 0; index--) {
        SmokeParticle &particle = smokeParticlces.at(index);
        if (updateParticles)
            particle.timer += 1;
        
        if (particle.timer > particle.lifetime) {
            smokeParticlces.erase(smokeParticlces.begin() + index);
            continue;
        }
    
        
        if (updateParticles) {
            particle.pos.x += particle.vel.x;
            particle.pos.y += particle.vel.y;
            particle.vel.y = Diminish(particle.vel.y, 0.025);
        }

        particle.color.a = (unsigned char) ((1 - ((float) particle.timer / (float) particle.lifetime)) * 100);
        
        float zoomLevel = cam.zoom / 4;
        Vector2 relative = toScreenPos(Vector2 {particle.pos.x, particle.pos.y}, cam);
        Texture2D smokeTexture = GetTexture(Textures::smoke);
        DrawTextureEx(smokeTexture, Vector2 {
            relative.x - (smokeTexture.width * zoomLevel) / 2, relative.y - (smokeTexture.height * zoomLevel) / 2}, 0, zoomLevel, 
            particle.color);
    }
}

Rectangle Tractor::GetTractorRect() {
    return {rect.x + hitbox.x, rect.y + hitbox.y, hitbox.width, hitbox.height};
}

Rectangle Tractor::GetCartRect() {
    Texture2D &cartTexture = GetTexture(Textures::cart);
    if (isLongWagon) {
        return {cartX - cartTexture.width / 2 + 5, rect.y + 17, 38, 4};
    }

    return {cartX - cartTexture.width / 2 + 10, rect.y + 17, 28, 4};
}

