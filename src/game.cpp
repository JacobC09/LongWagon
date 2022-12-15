#include <time.h>
#include "game.h"
#include "tractor.h"
#include "shop.h"
#include "easing.h"
#include "web.h"
#include "base.h"

ApplicationStates appState = Loading;

std::map<Textures, const char*> texturesToLoad;
std::map<Textures, Texture2D> loadedTextures;
std::map<Sounds, const char*> soundsToLoad;
std::map<Sounds, Sound> loadedSounds;
std::map<Fonts, JakeFont> loadedFonts;

int sw, sh;
int gameTime;
int pauseBtnSwitchTimer;
int pauseBtnTimer;
int coinShaketimer;
int nextItemTime;
int currentHealth;
int startCoins;
int nextItemDuration;
bool inMagnetMode;
bool inLightningMode;

int titleHoveredIndex;
float titleArrowY;
InterpolationFunction titleArrowAnim = {InterpolationFunction::EaseInOut, 15};

int gameOverAnimTimer;
int gameOverCoins;
bool gameOver;

int menuInAnim;
int menuHoveredIndex;
float menuArrowY;
bool menuOpen = false;
InterpolationFunction menuArrowAnim = {InterpolationFunction::EaseInOut, 15};

float initialItemVel = 0.8;
const int groundStartY = 9 * tileHeight;

GameData game;
Color playAgainColor;
Camera2D cam;
Tractor trac;
Animation coinAnimation;
RenderTexture2D target;

std::map<Shaders, Shader> shaders;
Shaders nextShader = Shaders::None;

std::vector<FallingItem> fallingItems;
std::vector<Particle*> particles;
std::vector<Particle*> explosionParticles;
std::vector<Transition*> transitions;
std::vector<Effect> effects;

void InitGame();
void EndGame();
void UpdateGame();
void UpdateItems(bool countCoins=false, bool moveItems=false);
void UpdateEffects();
void OnInCart(FallingItem &item, Rectangle cartRect);
void OnHitGround(FallingItem &item, Rectangle cartRect);
void SpawnNewItems();

void InitTitleScreen();
void UpdateTitleScreen();
void SpawnTitleScreenItems(bool randomY=false);

void InitGameOverScreen();
void UpdateGameOverScreen();
void InitMenu();
void UpdateMenu();

void UpdateTransitions();
void UpdateScreenSize();

void LoadShaders();
void PreloadAssets();
void UnloadAssets();
bool IsDoneLoadingAssets();

int GetPointValueFromId(int id, bool onGround=false);
float GetVelFromCoins(int coins);
bool InShaderMode();

int main() {
    InitWindow(1280, 800, "Long Wagon - Jake");
    InitAudioDevice();
    SetTraceLogLevel(LOG_ERROR);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    PreloadAssets(); 
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    
    int id = GetRandomValue(fruitIds.x, fruitIds.y);
    Image iconImage = LoadImage("resources/img/items.png");
    Image iconSource = ImageFromImage(iconImage, GetSourceRect(id, {(float) iconImage.width, (float) iconImage.height}, 16, 16));
    SetWindowIcon(iconSource);

    game = GameData {};
    coinAnimation = Animation({0, 1, 2, 3, 4}, {120, 6, 6, 6, 6}, true);

    while (!WindowShouldClose()) {
        switch (appState)
        {
        case ApplicationStates::Loading:
            if (IsDoneLoadingAssets()) {
                InitTitleScreen();
                appState = ApplicationStates::TitleScreen;

                if (GetRandomValue(0, 100) == 0)
                    PlaySound(GetSound(Sounds::ExtraLongWagonVoice));
                else
                    PlaySound(GetSound(Sounds::LongWagonVoice));
            }
            break;
        case ApplicationStates::GameOver:
            UpdateGameOverScreen();
            break;
        case ApplicationStates::TitleScreen:
            UpdateTitleScreen();
            break;
        case ApplicationStates::Running:
            UpdateGame();
            break;
        }

        if (InShaderMode()) {
            BeginDrawing();
                BeginShaderMode(shaders[game.selectedShader]);
                    DrawTextureRec(target.texture, {0, 0, (float)target.texture.width, (float)-target.texture.height}, {0, 0}, WHITE);
                EndShaderMode();
            EndDrawing();
        }

        if (nextShader != game.selectedShader) {
            game.selectedShader = nextShader;
        }
    }

    UnloadAssets();
}

/* ------------- Main Game ------------- */

void InitGame() {
    cam.zoom = 4;
    cam.offset = {0, 0};
    gameTime = 0;
    gameOver = false;
    pauseBtnTimer = 0;
    coinShaketimer = 0;
    currentHealth = game.healthUpgrade.values[game.healthUpgrade.unlocked];
    inMagnetMode = false;
    inLightningMode = false;

    UpdateScreenSize();
    trac.Init(sw, cam, groundStartY);
    nextItemTime = GetRandomValue(0, 120);
    coinAnimation.Reset();
    fallingItems.clear();
    effects.clear();
    clearHeapVector(particles);
    clearHeapVector(explosionParticles);

    menuOpen = false;
    menuInAnim = 0;
}

void EndGame() {
    submitScore(game.inGameCoins);
    game.coins += game.inGameCoins;
    game.inGameCoins = 0;
}

void UpdateGame() {
    // DEBUG NOT FINAL
    if (IsKeyDown(KEY_J) && IsKeyDown(KEY_A) && IsKeyDown(KEY_K) && IsKeyPressed(KEY_E)) {
        game.inGameCoins += 100;
    }

    if (currentHealth <= 0 && !gameOver) {
        gameOver = true;
        trac.isMoving = false;
        transitions.push_back(new FadeTransition("fade-gameover", 120, negativeColor));
    }

    gameTime++;
    UpdateScreenSize();
    
    if (IsKeyPressed(KEY_ESCAPE) && !gameOver) {
        if (menuOpen) {
            if (isShopOpen())
                setShopStatus(false);
            else
                menuOpen = false;
        } else {
            InitMenu();
            menuOpen = true;
        }
    }

    if (InShaderMode()) BeginTextureMode(target); else BeginDrawing();
    
        ClearBackground(BLACK);

        Texture2D &mapTexture = GetTexture(game.selectedMap);
        DrawTexturePro(mapTexture, {0, 0, (float) mapTexture.width, (float) mapTexture.height}, {0, 0, (float) GetScreenWidth(), (float) GetScreenHeight()}, {0, 0}, 0, WHITE);

        if (!menuOpen) {
            trac.Update(cam, game, !gameOver, inLightningMode ? game.speedUpgrade.values[game.speedUpgrade.unlocked] + 2 : -1);
        }

        UpdateItems(!gameOver, !menuOpen);
        
        if (!gameOver && !menuOpen) {
            SpawnNewItems();
        }

        // Draw Particles
        for (int index = (signed) particles.size() - 1; index > -1; index--) {
            particles[index]->Draw(cam);
            if (particles[index]->isDead) {
                free(particles[index]);
                particles.erase(particles.begin() + index);
            }
        }

        // Draw Tractor
        BeginMode2D(cam);
            trac.color = game.colors[game.selectedColor];
            trac.Draw(cam, !menuOpen);
        EndMode2D();

        trac.DrawParticles(cam, !menuOpen);
        
        // Draw Explosion Particles
        for (int index = (signed) explosionParticles.size() - 1; index > -1; index--) {
            explosionParticles[index]->Draw(cam);
            if (explosionParticles[index]->isDead) {
                free(explosionParticles[index]);
                explosionParticles.erase(explosionParticles.begin() + index);
            }
        }

        // UI
        Rectangle heartStartDest = {16, 12, 48, 48};
        Texture2D itemsTexture = GetTexture(Textures::items);
        
        for (int index = 0; index < game.healthUpgrade.values[game.healthUpgrade.unlocked]; index++) {
            if (!((index + 1) % 2)) {
                int id = index < currentHealth ? heartFullId : (index == currentHealth ? heartHalfFullId : heartEmptyId);
                DrawTexturePro(itemsTexture, GetSourceRect(id, {(float) itemsTexture.width, (float) itemsTexture.height}, 16, 16), 
                {heartStartDest.x + index / 2 * heartStartDest.width , heartStartDest.y, heartStartDest.width, heartStartDest.height}, {0, 0}, 0, WHITE);
            } else if (index == game.healthUpgrade.values[game.healthUpgrade.unlocked] - 1) {
                DrawTexturePro(itemsTexture, GetSourceRect(currentHealth == game.healthUpgrade.values[game.healthUpgrade.unlocked] ? heartHalfFullId : heartEmptyId, {(float) itemsTexture.width, (float) itemsTexture.height}, 16, 16), 
                {heartStartDest.x + index / 2 * heartStartDest.width, heartStartDest.y, heartStartDest.width, heartStartDest.height}, {0, 0}, 0, WHITE);
            }
        }

        DrawCoins(Vector2 {16, 64}, game.inGameCoins, !menuOpen);
        
        if (menuOpen || menuInAnim) {
            UpdateMenu();
        } else {
            Rectangle pauseButtonDest = {(float) GetScreenWidth() - 16 - 56, 16, 56, 56};
            DrawTexturePro(GetTexture(Textures::pauseButtons), {0, 0, 8, 8}, pauseButtonDest, {0, 0}, 0, WHITE);
            DrawTexturePro(GetTexture(Textures::pauseButtons), {0, 8, 8, 8}, pauseButtonDest, {0, 0}, 0, ColorAlpha(WHITE, (float) pauseBtnTimer / 10));

            if (CheckCollisionPointRec(GetMousePosition(), pauseButtonDest)) {
                pauseBtnTimer = max((float) pauseBtnTimer + 1, 10);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !gameOver) {
                    InitMenu();
                    menuOpen = true;
                }
            } else {
                pauseBtnTimer = min((float) pauseBtnTimer - 1, 0);
            }
        }

        UpdateTransitions();
        UpdateEffects();

    if (InShaderMode()) EndTextureMode(); else EndDrawing();

    if (gameOver && isTransitionFinished("fade-gameover")) {
        appState = ApplicationStates::GameOver;
        InitGameOverScreen();
    }
}

void UpdateEffects() {
    trac.isLongWagon = false;
    trac.isRainbow = false;
    inMagnetMode = false;
    inLightningMode = false;

    for (int index = effects.size() - 1; index > -1; index--) {
        Effect &effect = effects[index];
        effect.increment();

        switch (effect.type)
        {
        case EffectType::LongWagon:
            trac.isLongWagon = true;
            break;

        case EffectType::Magnet:
            inMagnetMode = true;
            break;

        case EffectType::Lightning:
            inLightningMode = true;
            trac.isRainbow = true;
            break;
        
        default:
            break;
        }

        if (effect.finished()) {
            effects.erase(effects.begin() + index);
        }
    }
}

void UpdateItems(bool countCoins, bool moveItems) {
    Rectangle cartRect = trac.GetCartRect();
    Texture2D itemsTexture = GetTexture(Textures::items);

    for (int index = fallingItems.size() - 1; index > -1; index--) {
        FallingItem &item = fallingItems[index];
        float gravity = min(item.initalYVel / 26, 0.03);
        unsigned char opacity = 255;

        if (moveItems) {
            if (item.insideCart) {
                item.pos.y += item.yVel;
                if (item.pos.x < cartRect.x) item.pos.x = cartRect.x;
                if (item.pos.x + itemTileSize >= cartRect.x + cartRect.width) item.pos.x = (cartRect.x + cartRect.width) - itemTileSize - 1;

                Rectangle dest = {item.pos.x, item.pos.y - itemTileSize, itemTileSize, itemTileSize};
                Rectangle source = GetSourceRect(item.id, {(float) itemsTexture.width, (float) itemsTexture.height}, tileWidth, tileHeight);
                if (cartRect.y + cartRect.height < dest.y + dest.height) {
                    source.height = dest.height = (cartRect.y + cartRect.height) - dest.y;
                    if (source.height <= 0) {
                        fallingItems.erase(fallingItems.begin() + index);
                        continue;
                    }
                }

                DrawTexturePro(itemsTexture, source, toScreenPos(dest, cam), {0, 0}, 0, Color {255, 255, 255, opacity});
                continue;
            }

            if (item.pos.y >= groundStartY) {
                item.pos.y = groundStartY;
                
                if (!item.hasHitGround) {
                    item.yVel = -(item.yVel / 4 > initialItemVel ? item.yVel / 4 : initialItemVel);
                    item.hasHitGround = true;
                    
                    if (countCoins)
                        OnHitGround(item, cartRect);
            
                } else {
                    item.yVel = -item.yVel + gravity * 10;
                }

                if (std::abs(item.yVel) < gravity) {
                    item.yVel = 0;
                }
            }

            if (item.yVel != 0) {
                item.pos.y += item.yVel; 

                if (item.yVel > gravity && item.yVel < 0) {
                    item.yVel = 0;
                } else if (item.yVel < item.initalYVel) {
                    item.yVel += gravity;
                }


                if (!item.hasHitGround) {
                    Rectangle collisionRect = Rectangle {item.pos.x + 1, item.pos.y - itemTileSize, itemTileSize - 2, itemTileSize};

                    item.pos.x += item.xVel;
                    float difference = (item.pos.x + itemTileSize / 2) - (cartRect.x + cartRect.width / 2);
                    if (std::abs(difference) <= 2 && !inMagnetMode)
                        item.xVel = Diminish(item.xVel, 0.01);
                    else
                        item.xVel = Diminish(item.xVel, 0.005);

                    if (inMagnetMode) {
                        if (difference < 2) {
                            item.xVel = max(item.xVel + 0.015, 0.6);
                        } else if (difference > 2) {
                            item.xVel = min(item.xVel - 0.015, -0.6);
                        }
                    }

                    if (!CheckCollisionRecs({collisionRect.x, collisionRect.y - item.yVel, collisionRect.width, collisionRect.height}, cartRect)) {
                        if (CheckCollisionRecs(collisionRect, cartRect)) {
                            if (cartRect.x <= collisionRect.x && collisionRect.x + collisionRect.width < cartRect.x + cartRect.width) {
                                item.insideCart = true;

                                if (countCoins)
                                    OnInCart(item, cartRect);

                            } else {
                                item.yVel = -item.yVel;
                            }
                        }
                    }
                }

            } else {
                item.pos.x = std::floor(item.pos.x);
                item.pos.y = std::floor(item.pos.y);

                item.lifetime++;
                if (item.lifetime > 3600) {
                    fallingItems.erase(fallingItems.begin() + index);
                    continue;
                } else if (item.lifetime > 3600 - 60) {
                    opacity = 255 - (255 / 60) * (item.lifetime - (3600 - 60));
                }
            }
        }

        Rectangle dest = toScreenPos(Rectangle {item.pos.x, item.pos.y - itemTileSize, itemTileSize, itemTileSize}, cam);
        DrawTexturePro(itemsTexture, GetSourceRect(item.id, {(float) itemsTexture.width, (float) itemsTexture.height}, tileWidth, tileHeight), dest, {0, 0}, 0, Color {255, 255, 255, opacity});
    }
}

void OnInCart(FallingItem &item, Rectangle cartRect) {
    int amount = GetPointValueFromId(item.id, false);

    if ((inLightningMode && amount > 0) || !inLightningMode) {
        game.inGameCoins += amount;
    }
    
    if (item.id == heartId) {
        if (currentHealth == game.healthUpgrade.values[game.healthUpgrade.unlocked]) {
            particles.push_back(new ScoreParticle(amount, Vector2 {item.pos.x + itemTileSize / 2, cartRect.y - itemTileSize}, false));
        } else {
            currentHealth += 2;
            particles.push_back(new ScoreParticle(2, Vector2 {item.pos.x + itemTileSize / 2, cartRect.y - itemTileSize}, true));
            if (currentHealth > game.healthUpgrade.values[game.healthUpgrade.unlocked]) 
                currentHealth = game.healthUpgrade.values[game.healthUpgrade.unlocked];
        }
    } else if (item.id == longWagonId) {
        effects.push_back(Effect {EffectType::LongWagon, 1600});
        if (trac.isLongWagon)
            PlaySound(GetSound(Sounds::ExtraLongWagonVoice));
        else
            PlaySound(GetSound(Sounds::LongWagonVoice));
    } else if (item.id == lightningId) {
        effects.push_back(Effect {EffectType::Lightning, 900});
        PlaySound(GetSound(Sounds::SpeedVoice));
    } else if (item.id == magnetId) {
        effects.push_back(Effect {EffectType::Magnet, 1600});
        PlaySound(GetSound(Sounds::MagnetVoice));
    } else {
        particles.push_back(new ScoreParticle(amount, Vector2 {item.pos.x + itemTileSize / 2, cartRect.y - itemTileSize}, false));
    }

    if (amount < 0 && !inLightningMode) {
        currentHealth -= 2;
        if (currentHealth < 0) 
            currentHealth = 0;
        
        if (item.id == bombId || item.id == dynomiteId) {
            PlaySound(GetSound(Sounds::BoomVoice));
            explosionParticles.push_back(new ExplosionParticle({item.pos.x + itemTileSize / 2, cartRect.y - itemTileSize / 2}));
        }
    }

    if (game.inGameCoins < 0) 
        game.inGameCoins = 0;
}

void OnHitGround(FallingItem &item, Rectangle cartRect) {
    int amount = GetPointValueFromId(item.id, true);
    game.inGameCoins += amount;

    if (amount != 0) particles.push_back(new ScoreParticle(amount, Vector2 {item.pos.x + itemTileSize / 2, cartRect.y - itemTileSize}, false));

    if (amount < 0) {
        currentHealth -= 1;
        if (currentHealth < 0) currentHealth = 0;
    }
    if (game.inGameCoins < 0) game.inGameCoins = 0;
}

void SpawnNewItems() {
    if (nextItemTime <= 0) {
        float velocity = GetVelFromCoins(game.inGameCoins);
        int id = GetRandomValue(fruitIds.x, fruitIds.y);

        float luck = game.luckUpgrade.values[game.luckUpgrade.unlocked];
        if (!GetRandomValue(0, 14 - (luck))) 
            id = diamondId;

        if (!GetRandomValue(0, 6 + std::ceil(luck / 2)))
            id = GetRandomValue(rottenFruitIds.x, rottenFruitIds.y + 1);

        if (!GetRandomValue(0, 6 + std::ceil(luck / 2))) 
            id = !GetRandomValue(0, 1) ? bombId : dynomiteId;

        if (!GetRandomValue(0, 26 - luck) && game.inGameCoins > 0) {
            id = GetRandomValue(0, 1) == 0 ? magnetId : longWagonId;
        }

        if (!GetRandomValue(0, 28 - luck)) 
            id = lightningId;

        if ((!GetRandomValue(0, 30 - luck) || (currentHealth == 1 && !GetRandomValue(0, 12))) && game.inGameCoins > 0) 
            id = heartId;

        // Check if id is the same
        if (fallingItems.size() > 0) {
            if (fallingItems[-1].id == id) {
                if (GetRandomValue(0, 3) != 0) {
                    nextItemTime = 0;
                    return;
                }
            }
        }
        
        float x = (float) GetRandomValue(itemTileSize / 2, (sw - itemTileSize - itemTileSize / 2));
        if (fallingItems.size() > 0) {
            int cartCenter = trac.GetCartRect().x + trac.GetCartRect().width / 2;
            if (nextItemDuration < 20)
                x = max(min(GetRandomValue(cartCenter - 16, cartCenter + 16), 0), (sw - itemTileSize - itemTileSize / 2));
            else if (nextItemDuration < 60)
                x = max(min(GetRandomValue(cartCenter - 32, cartCenter + 32), 0), (sw - itemTileSize - itemTileSize / 2));
            else if (nextItemDuration < 100)
                x = max(min(GetRandomValue(cartCenter - 112, cartCenter + 112), 0), (sw - itemTileSize - itemTileSize / 2));
        }


        fallingItems.push_back(FallingItem {
            Vector2 {x, 0}, 
            id, velocity
        });

        if (!GetRandomValue(0, 4))
            nextItemDuration = GetRandomValue(60, 100);
        if (!GetRandomValue(0, 12))
            nextItemDuration = GetRandomValue(20, 60);
        if (!GetRandomValue(0, 16))
            nextItemDuration = GetRandomValue(5, 20);
        else
            nextItemDuration = GetRandomValue(100 + luck * 4 - ((int) max((float) game.inGameCoins / 20, 60)), 200 - ((int) max((float) game.inGameCoins / 6, 120)));
        nextItemTime = nextItemDuration;

    } else {
        nextItemTime--;
    }
}

int GetPointValueFromId(int id, bool onGround) {
    int amount = 0;
    if (fruitIds.x <= id && id <= fruitIds.y) {
        amount = onGround? -10 : 10;
    } else if (id == diamondId) {
        amount = onGround? -50 : 75;
    } else if (id == heartId) {
        amount = onGround? 0 : 50;
    } else if (rottenFruitIds.x <= id && id <= rottenFruitIds.y) {
        amount = onGround? 0 : -20;
    } else if (id == bombId || id == dynomiteId) {
        amount = onGround? 0 : -100;
    }

    return amount;
}

float GetVelFromCoins(int coins) {
    float itemVel = initialItemVel;
    if (coins >= 1500) {
        itemVel = 2.5;
    } else if (coins >= 1000) {
        itemVel = 2.15;
    } else if (coins >= 850) {
        itemVel = 1.9;
    } else if (coins >= 600) {
        itemVel = 1.7;
    } else if (coins >= 450) {
        itemVel = 1.5;
    } else if (coins >= 300) {
        itemVel = 1.35;
    } else if (coins >= 200) {
        itemVel = 1.20;
    } else if (coins >= 150) {
        itemVel = 1.05;
    } else if (coins >= 100) {
        itemVel = 0.95;
    } else if (coins >= 50) {
        itemVel = 0.85;
    } 
    return itemVel;
}

bool InShaderMode() {
    return game.selectedShader != Shaders::None;
}

/* ----------- Title Screen ----------- */

void InitTitleScreen() {
    titleArrowY = -1;
    titleHoveredIndex = -1;
    titleArrowAnim.reset();
    
    nextItemTime = 0;
    fallingItems.clear();
    for (int index = GetRandomValue(3, 5); index > 0; index--) {
        SpawnTitleScreenItems(true);
    }
}

void UpdateTitleScreen() {
    JakeFont &font = GetFont(Fonts::normal);
    if (InShaderMode()) BeginTextureMode(target); else BeginDrawing();
        ClearBackground(BLACK);

        Texture2D &titleScreenBgText1 = GetTexture(Textures::titleScreenBg1);
        DrawTexturePro(titleScreenBgText1, {0, 0, (float) titleScreenBgText1.width, (float) titleScreenBgText1.height}, {0, 0, (float) GetScreenWidth(), (float) GetScreenHeight()}, {0, 0}, 0, WHITE);

        if (!isShopOpen()) {
            Texture2D &itemsTexture = GetTexture(Textures::items);
            for (int index = (signed) fallingItems.size() - 1; index > -1; index--) {
                FallingItem &item = fallingItems[index];
                Rectangle source = GetSourceRect(item.id, {(float) itemsTexture.width, (float) itemsTexture.height}, tileWidth, tileHeight);
                Rectangle dest = {item.pos.x, item.pos.y, tileWidth * 4, tileHeight * 4};
                DrawTexturePro(itemsTexture, source, dest, {0, 0}, 0, WHITE);

                item.pos.y += item.yVel;

                if (item.hasHitGround) {
                    item.yVel = max(item.yVel + 0.3, 8);
                }
                if (item.pos.y > GetScreenHeight()) {
                    fallingItems.erase(fallingItems.begin() + index);
                }
                if (CheckCollisionPointRec(GetMousePosition(), dest) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    item.hasHitGround = true;
                    item.yVel = -5;
                }
            }

            nextItemTime--;
            if (nextItemTime <= 0) SpawnTitleScreenItems(false);
        }

        Texture2D titleScreenBgText2 = GetTexture(Textures::titleScreenBg2);
        DrawTexturePro(titleScreenBgText2, {0, 0, (float) titleScreenBgText2.width, (float) titleScreenBgText2.height}, {0, 0, (float) GetScreenWidth(), (float) GetScreenHeight()}, {0, 0}, 0, WHITE);
        
        Texture2D &titleTexture = GetTexture(Textures::title);
        DrawTexturePro(titleTexture, {0, 0, (float) titleTexture.width, (float) titleTexture.height}, 
            {(float) GetScreenWidth() / 2 - titleTexture.width * 5 / 2, 85, (float) titleTexture.width * 5, (float) titleTexture.height * 5}, {0, 0}, 0, WHITE);

        float startY = (float) GetScreenHeight() / 2 + 24;
        int optionFontSize = 7;
        std::string optionNames[2] = {"Start", "Shop"};

        bool anyHovered = false;
        for (int index = 0; index < 2; index++) {
            int textWidth = font.Measure(optionNames[index]) * optionFontSize;
            Rectangle dest = {(float) (GetScreenWidth() - textWidth) / 2, startY + (8 + font.height * optionFontSize) * index, (float) textWidth, font.height * optionFontSize};

            DrawTextCentered(dest, font, optionNames[index], optionFontSize, Color {0, 149, 201, 255});
            if (CheckCollisionPointRec(GetMousePosition(), dest) && !isShopOpen()) {
                DrawTextCentered(dest, font, optionNames[index], optionFontSize, Color {7, 168, 247, 255});
                if (titleHoveredIndex == -1)
                    titleArrowY = -1;
                anyHovered = true;
                titleHoveredIndex = index;
                titleArrowAnim.increment();
            }

            // Move Arrow to desired position
            if (titleHoveredIndex == index) {
                int desiredArrowY = dest.y + dest.height / 2;
                float x = std::abs((desiredArrowY - titleArrowY) / dest.height);
                float speed = ceil(min(max(0.2 + 5 * (pow(3 * x, 2) - 2 * pow(x, 3)), 5), 0) * 10) / 10;  // I'm not going to try to explain this

                if (titleArrowY == -1) titleArrowY = desiredArrowY;
                titleArrowY = desiredArrowY + Diminish(titleArrowY - desiredArrowY, speed);
            }

            // Draw Arrow
            if (titleArrowY != -1 && titleArrowAnim.value()) {
                int arrowMargin = 134 - 16 * titleArrowAnim.value();
                font.Render(">", {(float) GetScreenWidth() / 2 - arrowMargin - font.Measure(">") * 5, titleArrowY - font.height * 2.5f}, 5, ColorAlpha(Cwhite, titleArrowAnim.value()));
                font.Render("<", {(float) GetScreenWidth() / 2 + arrowMargin, titleArrowY - font.height * 2.5f}, 5, ColorAlpha(Cwhite, titleArrowAnim.value()));
            }
        }

        if (isShopOpen())
            UpdateShop(game);

        // Animation
        if (!anyHovered)
            titleArrowAnim.decrement();
        else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (titleHoveredIndex == 0)  {
                appState = ApplicationStates::Running;
                transitions.push_back(new BoxTransition("title-screen-to-game", 40, true));
                InitGame();
            } else if (titleHoveredIndex == 1) {
                setShopStatus(true);
            }
        }

        UpdateTransitions();

    if (InShaderMode()) EndTextureMode(); else EndDrawing();
}

void SpawnTitleScreenItems(bool randomY) {
    nextItemTime = GetRandomValue(20, 100);
    Vector2 pos;
    
    for (int tries = 0; tries < 10; tries++) {
        pos = Vector2 {(float) GetRandomValue(128, GetScreenWidth() - 192), (float) (!randomY ? -64 : GetRandomValue(0, GetScreenHeight() - 64))};

        bool isAllowed = true;
        for (FallingItem &item : fallingItems) {
            if (Distance(item.pos, pos) < 350) {
                isAllowed = false;
                break;
            }
        }

        if (!isAllowed) continue;

        int id;
        while (true) {
            id = GetRandomValue(fruitIds.x, fruitIds.y);
            if (fallingItems.size() > 0) {
                if (fallingItems[-1].id == id) continue; 
            }
            break;
        }

        fallingItems.push_back(FallingItem {pos, id, 3});
        return;
    }

    nextItemTime = 20;
}

/* ------------- Game Over ------------ */

void InitGameOverScreen() {
    gameOverAnimTimer = 0;
    playAgainColor = Color {170, 170, 170, 0};
    gameOverCoins = game.coins;
    EndGame();
}

void UpdateGameOverScreen() {
    JakeFont &font = GetFont(Fonts::normal);

    std::string gameOverText = "Game Over";
    int gameOverSize = 8;
    int perLetterTime = 15;
    int letterOffset = 8;
    int length = font.Measure(gameOverText) * gameOverSize;
    Vector2 gameOverStart = {(float) (GetScreenWidth() - length) / 2, (float) GetScreenHeight() / 2 - 100};

    std::string playAgainText = "Play Again";
    int playAgainSize = 5;
    int playAgainWidth = font.Measure(playAgainText) * playAgainSize;
    Vector2 playAgainPos = {(float) (GetScreenWidth() - playAgainWidth) / 2, (float) GetScreenHeight() / 2};

    if (InShaderMode()) BeginTextureMode(target); else BeginDrawing();
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), negativeColor);

        int animationIndex = 0;        
        for (int charIndex = 0; charIndex < (signed) gameOverText.size(); charIndex++) {
            if (gameOverText[charIndex] == ' ') continue;
            if (gameOverAnimTimer < animationIndex * letterOffset) continue;
            
            float yoffset = 24 - max(24 * ((float) (gameOverAnimTimer - animationIndex * letterOffset) / perLetterTime), 24);
            float alpha = max(255 * ((float) (gameOverAnimTimer - animationIndex * letterOffset) / perLetterTime), 255);

            font.Render(std::string(1, gameOverText[charIndex]), Vector2 {gameOverStart.x + font.Measure(gameOverText.substr(0, charIndex)) * gameOverSize, gameOverStart.y + yoffset}, gameOverSize, {255, 255, 255, (unsigned char) alpha});
            animationIndex++;
        }

        if (gameOverAnimTimer > 90) {
            if (playAgainColor.a < 255 - 30) {
                playAgainColor.a += 30;  
            } else {
                playAgainColor.a = 255;  
            }

            std::string playAgainText = "Play Again";
            font.Render("Play Again", playAgainPos, playAgainSize, playAgainColor);

            if (CheckCollisionPointRec(GetMousePosition(), {playAgainPos.x, playAgainPos.y, (float) playAgainWidth, (float) font.texture.height * playAgainSize})) {
                if (playAgainColor.r < 255) {
                    playAgainColor.r += 5; playAgainColor.g += 5; playAgainColor.b += 5;
                    if (playAgainColor.r > 255) {
                        playAgainColor.r = 255; playAgainColor.g = 255; playAgainColor.b = 255;
                    }
                }
                DrawRectangle(playAgainPos.x, playAgainPos.y + font.texture.height * playAgainSize + playAgainSize, playAgainWidth, playAgainSize, playAgainColor);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    transitions.push_back(new BoxTransition("game-over-to-title-screen", 40, true));
                    appState = ApplicationStates::TitleScreen;
                    InitTitleScreen();
                }
            
            } else {
                if (playAgainColor.r > 55) {
                    playAgainColor.r -= 5; playAgainColor.g -= 5; playAgainColor.b -= 5;
                    if (playAgainColor.r < 200) {
                        playAgainColor.r = 200; playAgainColor.g = 200; playAgainColor.b = 200;
                    }
                }
            }

            if (gameOverCoins < game.coins && gameOverAnimTimer > 95) {
                coinShaketimer = 10;
                gameOverCoins += 7;
            }

            Vector2 coinPos = {16, 16};
            if (coinShaketimer) {
                coinPos.y -= GetRandomValue(-1, 1) * 4;
                coinPos.x -= GetRandomValue(-1, 1) * 4;
            }
            if (coinShaketimer)
                coinShaketimer--;

            DrawCoins(coinPos, gameOverCoins);
        }

        gameOverAnimTimer++;
        UpdateTransitions();

    if (InShaderMode()) EndTextureMode(); else EndDrawing();
}

/* --------------- Menu --------------- */

void InitMenu() {
    menuInAnim = 0;
    menuArrowY = -1;
    menuHoveredIndex = -1;
    menuArrowAnim.reset();
}

void UpdateMenu() {
    int y;
    if (menuOpen) {
        if (menuInAnim < 40)
            menuInAnim++;
        y = EaseCubicOut(menuInAnim, 40, 0, GetScreenHeight());
    } else {
        if (menuInAnim)
            menuInAnim = min(menuInAnim - 2, 0);
        y = EaseLinearNone(menuInAnim, 40, 0, GetScreenHeight());
    }

    DrawRectangle(0, 0, GetScreenWidth(), y, Color {0, 0, 0, (unsigned char) EaseCubicOut(menuInAnim, 40, 100, 90)});
    DrawLine(0, y + 1, GetScreenWidth(), y + 1, CgreyDarkDark);

    int sourceX = 8 * (int) (pauseBtnSwitchTimer/5);
    if (sourceX == 8 && !menuOpen)
        sourceX = 24;

    Rectangle pauseButtonDest = {(float) GetScreenWidth() - 16 - 56, 16, 56, 56};
    DrawTexturePro(GetTexture(Textures::pauseButtons), {(float) sourceX, 0, 8, 8}, pauseButtonDest, {0, 0}, 0, WHITE);
    DrawTexturePro(GetTexture(Textures::pauseButtons), {(float) sourceX, 8, 8, 8}, pauseButtonDest, {0, 0}, 0, ColorAlpha(WHITE, (float) pauseBtnTimer / 10));

    if (CheckCollisionPointRec(GetMousePosition(), pauseButtonDest) && !isShopOpen()) {
        pauseBtnTimer = max((float) pauseBtnTimer + 1, 10);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            menuOpen = false;
        }
    } else {
        pauseBtnTimer = min((float) pauseBtnTimer - 1, 0);
    }

    if (menuOpen) {
        if (menuOpen && pauseBtnSwitchTimer < 10)
            pauseBtnSwitchTimer++;
    } else if (pauseBtnSwitchTimer)
        pauseBtnSwitchTimer--;
    
    int yoffset = -GetScreenHeight() + y;

    JakeFont &font = GetFont(Fonts::normal);
    DrawTextCentered({0, (float) 135 + yoffset, (float) GetScreenWidth(), font.height * 14}, font, "Menu", 14, Cwhite);

    int optionFontSize = 6;
    std::string optionNames[5] = {"Shop", "Colors", "Background", "Effects", "Exit"};

    bool anyHovered = false;
    for (int index = 0; index < 5; index++) {
        int textWidth = font.Measure(optionNames[index]) * optionFontSize;
        Rectangle dest = {(float) (GetScreenWidth() - textWidth) / 2, 210 + yoffset + (8 + font.height * optionFontSize) * (index + 1), (float) textWidth, font.height * optionFontSize};

        DrawTextCentered(dest, font, optionNames[index], optionFontSize, index == 4 ? negativeColor : Cwhite);
        if (CheckCollisionPointRec(GetMousePosition(), dest) && !isShopOpen()) {
            DrawTextCentered(dest, font, optionNames[index], optionFontSize, index == 4 ? Color {189, 7, 19, 255} : Color {160, 160, 160, 255});
            if (menuHoveredIndex == -1)
                menuArrowY = -1;
            anyHovered = true;
            menuHoveredIndex = index;
            menuArrowAnim.increment();
        }

        // Move Arrow to desired position
        if (menuHoveredIndex == index) {
            int desiredArrowY = dest.y + dest.height / 2;
            float x = std::abs((desiredArrowY - menuArrowY) / dest.height);
            float speed = ceil(min(max(0.2 + 5 * (pow(3 * x, 2) - 2 * pow(x, 3)), 5), 0) * 10) / 10;  // I'm not going to try to explain this

            if (menuArrowY == -1) menuArrowY = desiredArrowY;
            menuArrowY = desiredArrowY + Diminish(menuArrowY - desiredArrowY, speed);
        }

        // Draw Arrow
        if (menuArrowY != -1 && menuArrowAnim.value()) {
            int arrowMargin = 176 - 16 * menuArrowAnim.value();
            font.Render(">", {(float) GetScreenWidth() / 2 - arrowMargin - font.Measure(">") * 4, menuArrowY - font.height * 2.0f}, 4, ColorAlpha(Cwhite, menuArrowAnim.value()));
            font.Render("<", {(float) GetScreenWidth() / 2 + arrowMargin, menuArrowY - font.height * 2.0f}, 4, ColorAlpha(Cwhite, menuArrowAnim.value()));
        }
    }

    if (isShopOpen())
        UpdateShop(game, 40);

    // Animation
    if (!anyHovered)
        menuArrowAnim.decrement();
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (menuHoveredIndex == 0)  {
            InitShop();
            setShopStatus(true);
            setShopPanel(Panel::Upgrades);
        } else if (menuHoveredIndex == 1) {
            InitShop();
            setShopStatus(true);
            setShopPanel(Panel::Colors);
        } else if (menuHoveredIndex == 2) {
            InitShop();
            setShopStatus(true);
            setShopPanel(Panel::Background);
        } else if (menuHoveredIndex == 3) {
            InitShop();
            setShopStatus(true);
            setShopPanel(Panel::Effects);
        } else if (menuHoveredIndex == 4) {
            InitTitleScreen();
            appState = ApplicationStates::TitleScreen;

            // Leave the game and reset the coins
            game.inGameCoins = 0;
        }
    }
}

/* ------------- Functions ------------ */

void DrawCoins(Vector2 startPos, int numOfCoins, bool updateAnimation) {
    if (updateAnimation)
        coinAnimation.Update();
    
    std::string text = std::string("Coins: ") + std::to_string(numOfCoins);
    DrawTexturePro(GetTexture(Textures::coinsheet), {(float) coinAnimation.Get() * 16, 0, 16, 16}, {startPos.x, startPos.y, 48, 48}, {0, 0}, 0, WHITE);
    GetFont(Fonts::normal).Render(text, {startPos.x + 58, startPos.y}, 4, Color {248, 183, 57, 255});
}

void UpdateTransitions() {
    for (int index = (signed) transitions.size() - 1; index > -1; index--) {
        if (transitions[index]->isFinished) {
            free(transitions[index]);
            transitions.erase(transitions.begin() + index);
        } else {
            transitions[index]->Draw();
        }
    }
}

void UpdateScreenSize() {
    sh = GetScreenHeight() / cam.zoom;
    sw = GetScreenWidth() / cam.zoom;
}

void SetNextShader(Shaders shader) {
    nextShader = shader;
}

GameData &GetGameData() {
    return game;
}

/* -------------- Classes ------------- */

void ExplosionParticle::Draw(Camera2D cam) {
    int frameDuration = 5;
    Texture2D &explosionTexture = GetTexture(Textures::explosion);
    
    Vector2 relativeCenter = toScreenPos(center, cam);
    DrawTexturePro(explosionTexture, {(float) 45 * (timer / frameDuration), 0, 45, 45}, 
        {relativeCenter.x - (45 / 2) * cam.zoom, relativeCenter.y - (45 / 2) * cam.zoom, 45 * cam.zoom, 45 * cam.zoom}, {0, 0}, 0, WHITE);

    timer += 1;
    if (timer >= 7 * frameDuration)
        isDead = true;
}

ScoreParticle::ScoreParticle(int score, Vector2 position, bool isHp) {
    timer = 0;
    lifetime = 60;
    posative = score > 0;
    text = posative ? (std::string("+") + std::to_string(score)) : std::to_string(score);
    if (isHp) text += std::string(" hp"); 

    pos = {position.x - GetFont(Fonts::normal).Measure(text) / 2, position.y};
}

void ScoreParticle::Draw(Camera2D cam) {
    timer++;
    pos.y -= 0.065;
    Color color = posative ? positiveColor : negativeColor;
    GetFont(Fonts::normal).Render(text, toScreenPos(Vector2 {pos.x - 0.5f, pos.y + 0.5f}, cam), cam.zoom, {0, 0, 0, 45});
    GetFont(Fonts::normal).Render(text, toScreenPos(pos, cam), cam.zoom, color);
    
    if (timer > lifetime) {
        isDead = true;
    }
}

Transition::Transition(const char* _name, int _totalDuration, bool _isReversed) {
    timer = 0;
    name = _name;
    totalDuration = _totalDuration;
    isReversed = _isReversed;
}

void BoxTransition::Draw() {
    int maxHeight = GetScreenHeight() / 2;
    int height = max(((float) timer / (totalDuration - 10)) * maxHeight, maxHeight);
    if (isReversed) height = (GetScreenHeight() / 2) - height;

    DrawRectangle(0, 0, GetScreenWidth(), height, BLACK);
    DrawRectangle(0, GetScreenHeight() - height, GetScreenWidth(), height, BLACK);

    timer++;
    if (timer > totalDuration) {
        isFinished = true;
    }
}

FadeTransition::FadeTransition(const char* _name, int _totalDuration, Color _color, bool _isReversed) {
    timer = 0;
    name = _name;
    totalDuration = _totalDuration;
    color = _color;
    isReversed = _isReversed;
}

void FadeTransition::Draw() {
    int alpha = ((float) timer / totalDuration) * 255;
    if (isReversed) alpha = 255 - alpha;
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color {color.r, color.g, color.b, (unsigned char) alpha});

    timer++;
    if (timer > totalDuration) {
        isFinished = true;
    }
}

Animation::Animation(std::vector<int> frameVector, int frameDur, bool repeating) {
    frames = frameVector;
    isReapting = repeating;

    for (int i = 0; i < (signed) frameVector.size(); i++) {
        frameDurations.push_back(frameDur);
    }

    Reset();
}

Animation::Animation(std::vector<int> frameVector, std::vector<int> frameDurs, bool repeating) {
    frames = frameVector;
    isReapting = repeating;
    frameDurations = frameDurs;

    Reset();
}

void Animation::Update() {
    timer += 1;

    int sum = 0;
    for (int index = 0; index < (signed) frameDurations.size(); index++) sum += frameDurations[index];
    if (timer >= sum) {
        timer = 0;
    }
}

void Animation::Reset() {
    isFinished = false;
    timer = 0;
}

int Animation::Get() {
    int value = 0;
    for (int index = 0; index < (signed) frameDurations.size(); index++) {
        value += frameDurations[index];
        if (timer < value) {
            return frames[index];
        }
    }
    return frames[-1];
}

std::string GameData::toString() {
    std::string stringRepresentation = IntToBase(coins, totalChars) + "-" 
        + IntToBase(speedUpgrade.unlocked, totalChars) + "-"
        + IntToBase(healthUpgrade.unlocked, totalChars) + "-"
        + IntToBase(luckUpgrade.unlocked, totalChars) + "-"
        + IntToBase(selectedColor, totalChars) + "-"
        + IntToBase((int) selectedMap, totalChars) + "-"
        + IntToBase((int) selectedShader, totalChars);

    stringRepresentation = EncryptString(stringRepresentation);
    stringRepresentation += chars[BaseToInt(stringRepresentation, totalChars) % totalChars];

    return EncryptString(stringRepresentation);
}

void GameData::fromString(std::string) {

};

/* ------------- Loading ------------- */

void PreloadAssets() {
    texturesToLoad[Textures::cart] = "resources/img/cart.png";
    texturesToLoad[Textures::tiles] = "resources/img/tiles.png";
    texturesToLoad[Textures::items] = "resources/img/items.png";
    texturesToLoad[Textures::tractor] = "resources/img/tractor.png";
    texturesToLoad[Textures::wheelLarge] = "resources/img/wheelLarge.png"; 
    texturesToLoad[Textures::wheelSmall] = "resources/img/wheelSmall.png";
    texturesToLoad[Textures::halo] = "resources/fx/halo.png";
    texturesToLoad[Textures::smoke] = "resources/fx/smoke.png";
    texturesToLoad[Textures::title] = "resources/img/title.png";
    texturesToLoad[Textures::coinsheet] = "resources/img/coin.png";
    texturesToLoad[Textures::normalFont] = "resources/img/normalFont.png";
    texturesToLoad[Textures::titleScreenBg1] = "resources/img/titleScreenBg1.png";
    texturesToLoad[Textures::titleScreenBg2] = "resources/img/titleScreenBg2.png";
    texturesToLoad[Textures::explosion] = "resources/fx/explosion.png";
    texturesToLoad[Textures::gui] = "resources/img/gui.png";
    texturesToLoad[Textures::pauseButtons] = "resources/img/pauseButtons.png";
    texturesToLoad[Textures::map1] = "resources/map/map1.png";
    texturesToLoad[Textures::map2] = "resources/map/map2.png";
    texturesToLoad[Textures::map3] = "resources/map/map3.png";

    soundsToLoad[Sounds::LongWagonVoice] = "resources/sound/LongWagonVoice.mp3";
    soundsToLoad[Sounds::ExtraLongWagonVoice] = "resources/sound/ExtraLongWagonVoice.mp3";
    soundsToLoad[Sounds::BoomVoice] = "resources/sound/BoomVoice.mp3";
    soundsToLoad[Sounds::MagnetVoice] = "resources/sound/MagnetVoice.mp3";
    soundsToLoad[Sounds::SpeedVoice] = "resources/sound/SpeedVoice.mp3";
}

void UnloadAssets() {
    for (auto &[name, item] : loadedTextures) {
        UnloadTexture(item);
    }
    for (auto &[name, item] : loadedSounds) {
        UnloadSound(item);
    }
    for (auto &[name, item] : loadedFonts) {
        UnloadTexture(item.texture);
    }
    for (auto &[name, item] : shaders) {
        UnloadShader(item);
    }
}

void LoadOther() {
    loadedFonts[Fonts::normal] = JakeFont(GetTexture(Textures::normalFont), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()-=_+[]{}\\/;:,.<>?`~", 5 );
    LoadShop();
}

void LoadShaders() {
    shaders[FX_GRAYSCALE] = LoadShader(0, TextFormat("resources/shaders/glsl%i/grayscale.fs", GLSL_VERSION));
    shaders[FX_POSTERIZATION] = LoadShader(0, TextFormat("resources/shaders/glsl%i/posterization.fs", GLSL_VERSION));
    shaders[FX_DREAM_VISION] = LoadShader(0, TextFormat("resources/shaders/glsl%i/dream_vision.fs", GLSL_VERSION));
    shaders[FX_PIXELIZER] = LoadShader(0, TextFormat("resources/shaders/glsl%i/pixelizer.fs", GLSL_VERSION));
    shaders[FX_CROSS_HATCHING] = LoadShader(0, TextFormat("resources/shaders/glsl%i/cross_hatching.fs", GLSL_VERSION));
    shaders[FX_CROSS_STITCHING] = LoadShader(0, TextFormat("resources/shaders/glsl%i/cross_stitching.fs", GLSL_VERSION));
    shaders[FX_PREDATOR_VIEW] = LoadShader(0, TextFormat("resources/shaders/glsl%i/predator.fs", GLSL_VERSION));
    shaders[FX_SCANLINES] = LoadShader(0, TextFormat("resources/shaders/glsl%i/scanlines.fs", GLSL_VERSION));
    shaders[FX_FISHEYE] = LoadShader(0, TextFormat("resources/shaders/glsl%i/fisheye.fs", GLSL_VERSION));
    shaders[FX_SOBEL] = LoadShader(0, TextFormat("resources/shaders/glsl%i/sobel.fs", GLSL_VERSION));
    shaders[FX_BLOOM] = LoadShader(0, TextFormat("resources/shaders/glsl%i/bloom.fs", GLSL_VERSION));
    shaders[FX_BLUR] = LoadShader(0, TextFormat("resources/shaders/glsl%i/blur.fs", GLSL_VERSION));
}

bool IsDoneLoadingAssets() {
    BeginDrawing();
        ClearBackground(BLACK);
        const char* text = "Loading Textures";
        int textWidth = MeasureText(text, 32);
        DrawText(text, (float) GetScreenWidth() / 2 - textWidth / 2, (float) GetScreenHeight() / 2 - 50, 32, WHITE);
    EndDrawing();

    WaitTime(0.1);

    while (true) {
        if (!texturesToLoad.empty()) {
            Textures name = texturesToLoad.begin()->first;
            loadedTextures[name] = LoadTexture(texturesToLoad[name]);
            texturesToLoad.erase(name);
        } else if (!soundsToLoad.empty()) {
            Sounds name = soundsToLoad.begin()->first;
            loadedSounds[name] = LoadSound(soundsToLoad[name]);
            soundsToLoad.erase(name);
        } else {
            LoadOther();
            LoadShaders();
            return true;
        }
    }
    
    return false;
}

bool isTransitionFinished(const char* name) {
    for (auto transition : transitions) {
        if (strcmp(transition->name, name) == 0) {
            if (transition->isFinished) return true;
        }
    }
    return false;
}

Texture2D &GetTexture(Textures texture) {
    return loadedTextures[texture];
}

Sound &GetSound(Sounds sound) {
    return loadedSounds[sound];
}

JakeFont &GetFont(Fonts font) {
    return loadedFonts[font];
}