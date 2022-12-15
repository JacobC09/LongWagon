#include "ui.h"
#include "shop.h"
#include "game.h"
#include "utils.h"
#include "easing.h"
#include "tractor.h"

int localCoins = -1;
int coinSubMultipier;
float scale = 3;
bool shopOpen = false;
Texture2D shopBg;
Texture2D panelBtnLight;
Texture2D panelBtnMidDark;
Texture2D panelBtnDark;
Texture2D previewPanel;
Tractor previewTractor;
Panel currentPanel = Panel::Upgrades;
InterpolationFunction shopFadeIn = {InterpolationFunction::EaseInOut, 20};

std::map<Panel, int> panelTransitions;
std::map<int, int> upgrTransitions;
std::map<int, int> upgrBtnTransitions;
std::map<int, int> colorTransitions;
std::map<int, int> optionTransitions;
std::map<int, int> shaderTransitions;
std::map<Panel, int> lockedTransitions;

std::vector<Textures> backgroundOptions = {Textures::map1, Textures::map2, Textures::map3};

BorderBox shopBorder = {
    {0, 0, 16, 16},
    {32, 0, 16, 16},
    {32, 32, 16, 16},
    {0, 32, 16, 16},
    {16, 0, 16, 16},
    {0, 16, 16, 16},
    {16, 32, 16, 16},
    {32, 16, 16, 16},
    Color {32, 43, 62, 255}
};

BorderBox buttonBorderLight = {
    {48, 32, 3, 3},
    {52, 32, 3, 3},
    {52, 36, 3, 3},
    {48, 36, 3, 3},
    {51, 32, 1, 3}, 
    {48, 35, 3, 1},
    {51, 36, 1, 3},
    {52, 35, 3, 1},
    Color {76, 101, 126, 255}
};

BorderBox buttonBorderMidDark = {
    {48, 32 + 7, 3, 3},
    {52, 32 + 7, 3, 3},
    {52, 36 + 7, 3, 3},
    {48, 36 + 7, 3, 3},
    {51, 32 + 7, 1, 3},
    {48, 35 + 7, 3, 1},
    {51, 36 + 7,  1, 3},
    {52, 35 + 7, 3, 1},
    CgreyMidDark
};

BorderBox buttonBorderDark = {
    {48 + 7, 32, 3, 3},
    {52 + 7, 32, 3, 3},
    {52 + 7, 36, 3, 3},
    {48 + 7, 36, 3, 3},
    {51 + 7, 32, 1, 3},
    {48 + 7, 35, 3, 1},
    {51 + 7, 36,  1, 3},
    {52 + 7, 35, 3, 1},
    CgreyDark
};

void DrawUpgrades(Vector2 shopStart, GameData &game);
void DrawColors(Vector2 shopStart, GameData &game);
void DrawBackground(Vector2 shopStart, GameData &game);
void DrawEffects(Vector2 shopStart, GameData &game);
void DrawRectCutCorners(Rectangle rect, float scale, Color color);
bool DrawShopLocked(Vector2 shopStart, Panel panelName, int price, GameData &game);
void SpendCoins(int amount, GameData &game);
Color toDisplayColor(Color color);
std::string getPanelName(Panel panel);

void LoadShop() {
    Image sourceImage = LoadImageFromTexture(GetTexture(Textures::gui));
    shopBg = LoadTextureFromImage(shopBorder.GetImage(60 * 16, 42 * 16, scale, sourceImage));
    panelBtnLight = LoadTextureFromImage(buttonBorderLight.GetImage(60 * scale, 20 * scale, scale, sourceImage));
    panelBtnMidDark = LoadTextureFromImage(buttonBorderMidDark.GetImage(60 * scale, 20 * scale, scale, sourceImage));
    panelBtnDark = LoadTextureFromImage(buttonBorderDark.GetImage(60 * scale, 20 * scale, scale, sourceImage));
    previewPanel = LoadTextureFromImage(buttonBorderDark.GetImage(144 * scale, 84 * scale, scale, sourceImage));
}

void InitShop() {
    previewTractor.Init(0, Camera2D {{0, 0}, {0, 0}, 0, scale}, 116);
    shopFadeIn.timer = 0;
    localCoins = -1;
}

void UpdateShop(GameData &game, int bgOpacity) {
    shopFadeIn.increment();

    JakeFont &font = GetFont(Fonts::normal);
    unsigned char alpha = (unsigned char) 255 * shopFadeIn.value();
    Vector2 shopStart = {(float) (GetScreenWidth() - shopBg.width) / 2, 40 + (32 * (1 - shopFadeIn.value()))};

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color {0, 0, 0, (unsigned char) ((float) bgOpacity * shopFadeIn.value())});
    
    if (localCoins == -1)
        localCoins = game.totalCoins();
    else
        if (std::abs(game.totalCoins() - localCoins) < coinSubMultipier)
            localCoins = game.totalCoins();
        else if (localCoins < game.totalCoins())
            localCoins += coinSubMultipier;
        else if (localCoins > game.totalCoins())
            localCoins -= coinSubMultipier;
    
    Vector2 coinPos = {6, 6};
    if (localCoins != game.totalCoins()) {
        coinPos.x -= GetRandomValue(-1, 1) * 4;
        coinPos.y -= GetRandomValue(-1, 1) * 4;
    }
    DrawCoins(coinPos, localCoins);
    
    DrawTexture(shopBg, shopStart.x, shopStart.y, {255, 255, 255, alpha});

    if (currentPanel == Panel::Upgrades) {
        DrawUpgrades(shopStart, game);
    } else if (currentPanel == Panel::Colors) {
        DrawColors(shopStart, game);
    } else if (currentPanel == Panel::Background) {
        DrawBackground(shopStart, game);
    } else if (currentPanel == Panel::Effects) {
        DrawEffects(shopStart, game);
    }

    Rectangle dest = {shopStart.x + (float) shopBg.width - 32 * scale, shopStart.y + 16 * scale, 16 * scale, 16 * scale};
    if (CheckCollisionPointRec(GetMousePosition(), dest)) {
        DrawTexturePro(GetTexture(Textures::gui), {48, 16, 16, 16}, dest, {0, 0}, 0, {255, 255, 255, alpha});
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            setShopStatus(false);
        }
    } else {
        DrawTexturePro(GetTexture(Textures::gui), {48, 0, 16, 16}, dest, {0, 0}, 0, {255, 255, 255, alpha});
    }
    
    // Draw Buttons
    for (int index = 0; index < totalPanels; index++) {
        Rectangle dest = {shopStart.x + shopBorder.left.width * scale + (panelBtnDark.width + 24) * index, shopStart.y + shopBg.height, (float) panelBtnDark.width, (float) panelBtnDark.height};
        
        if (index == (int) currentPanel) {
            DrawTexture(panelBtnLight, dest.x, dest.y, {255, 255, 255, alpha});
        } else {
            DrawTexture(panelBtnDark, dest.x, dest.y, {255, 255, 255, alpha});
            if (CheckCollisionPointRec(GetMousePosition(), dest)) {
                if (panelTransitions[(Panel) index] < 10) panelTransitions[(Panel) index]++;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) setShopPanel((Panel) index);
            } else if (panelTransitions[(Panel) index] > 0) {
                panelTransitions[(Panel) index]--;
            }

            unsigned char overlayButtonAlpha = (unsigned char) 255 * ((float) panelTransitions[(Panel) index] / 10);
            if (overlayButtonAlpha) {
                DrawTexture(panelBtnMidDark, dest.x, dest.y, {255, 255, 255, alpha < overlayButtonAlpha ? alpha : overlayButtonAlpha});
            }
        }

        DrawTextCentered({dest.x, dest.y, dest.width, dest.height}, font, getPanelName((Panel) index), 3, ColorAlpha(CgreyLight, 1 * shopFadeIn.value()));
    }    
}

void DrawUpgrades(Vector2 shopStart, GameData &game) {
    JakeFont &font = GetFont(Fonts::normal);
    Color white = ColorAlpha(Cwhite, 1 * shopFadeIn.value());
    int totalUpgrades = 3;
    int padding = 10;
    Vector2 containerSize = Vector2 {80, 136};
    Vector2 startPos = {shopStart.x + shopBg.width / 2 - (containerSize.w * scale * totalUpgrades) / 2 - (padding * scale * (totalUpgrades - 1)) / 2, shopStart.y + 24 + (shopBg.height - containerSize.h * scale) / 2};

    std::string panelName = getPanelName(currentPanel);
    font.Render(panelName, {shopStart.x + shopBg.width / 2 - font.Measure(panelName) * 6 / 2, shopStart.y + 16 * scale}, 6, white);

    for (int upgradeIndex = 0; upgradeIndex < totalUpgrades; upgradeIndex++) {
        Rectangle dest = {startPos.x + (containerSize.w + padding) * upgradeIndex * scale, startPos.y, containerSize.w * scale, containerSize.h * scale};
        DrawRectangleRec(dest, ColorAlpha(CgreyDark, shopFadeIn.value()));
        
        GameData::Upgrade *upgrade;
        Color upgradeColor;
        std::string upgradeName;

        if (upgradeIndex == 0) {
            upgradeName = "Speed";
            upgrade = &game.speedUpgrade;
            upgradeColor = Color {99, 155, 255, 255};
        } else if (upgradeIndex == 1) {
            upgradeName = "Health";
            upgrade = &game.healthUpgrade;
            upgradeColor = Color {233, 67, 54, 255};
        } else if (upgradeIndex == 2) {
            upgradeName = "Luck";
            upgrade = &game.luckUpgrade;
            upgradeColor = Color {98, 173, 61, 255};
        } else {
            continue;
        }

        if (CheckCollisionPointRec(GetMousePosition(), dest)) {
            if (upgrTransitions[upgradeIndex] < 10) upgrTransitions[upgradeIndex]++; 
            DrawRectangleRec(dest, Color {255, 255, 255, 5});
        } else if (upgrTransitions[upgradeIndex]) {
            upgrTransitions[upgradeIndex]--;
        }

        // Border
        if (upgrTransitions[upgradeIndex])
            DrawRectangleLinesEx(dest, scale, ColorAlpha(Cgrey, 1 * (float) upgrTransitions[upgradeIndex] / 10));

        // Upgrade Title
        font.Render(upgradeName, {dest.x + dest.width / 2 - font.Measure(upgradeName) * 5 / 2, dest.y}, 5, white);
        Rectangle source = {(float) 80 * upgradeIndex, 48, 80, 64};
        
        // Icon
        DrawTexturePro(GetTexture(Textures::gui), source, {dest.x, dest.y + 64, source.width * scale, source.height * scale}, {0, 0}, 0, white);

        // Progress Bar
        int width = 64;
        int gap = upgrade->total / 2;
        int cellSize = (width - gap * (upgrade->total - 1)) / upgrade->total;
        for (int index = 0; index < upgrade->total; index++) {
            int x = (dest.x + dest.width / 2) - (width * scale / 2) + (gap + cellSize) * index * scale;
            DrawRectangle(x, dest.y + dest.height - 128, cellSize * scale, 36, index < upgrade->unlocked ? upgradeColor : Cgrey);
        }

        // Button
        std::string priceButtonText = "Max";
        if (upgrade->unlocked < upgrade->total)
            priceButtonText = std::string("Buy: ") + std::to_string(upgrade->prices[upgrade->unlocked]); 
        
        float textWidth = min(font.Measure(priceButtonText) * 4 + 32, 150);
        Rectangle buttonDest = {dest.x + (dest.width - textWidth) / 2, dest.y + dest.height - 68, textWidth, 50};
        
        if (upgrade->unlocked == upgrade->total) {
            DrawRectCutCorners(buttonDest, scale, CgreyMidDark);
            DrawTextCentered(buttonDest, font, priceButtonText, 4, Cgrey);
        } else {
            DrawRectCutCorners(buttonDest, scale, Cgrey);

            if (CheckCollisionPointRec(GetMousePosition(), buttonDest)) {
                if (upgrBtnTransitions[upgradeIndex] < 10) upgrBtnTransitions[upgradeIndex]++;
                DrawRectCutCorners(buttonDest, scale, ColorAlpha(CgreyLight, (float) upgrBtnTransitions[upgradeIndex] / 10));
            } else if (upgrBtnTransitions[upgradeIndex]) {
                upgrBtnTransitions[upgradeIndex]--;
            }
            
            if (game.totalCoins() >= upgrade->prices[upgrade->unlocked]) {
                DrawTextCentered(buttonDest, font, priceButtonText, 4, positiveColor);

                if (CheckCollisionPointRec(GetMousePosition(), buttonDest) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    SpendCoins(upgrade->prices[upgrade->unlocked], game);
                    upgrade->unlocked += 1;
                }
            } else
                DrawTextCentered(buttonDest, font, priceButtonText, 4, negativeColor);
        }
    }
}

void DrawColors(Vector2 shopStart, GameData &game) {
    JakeFont &font = GetFont(Fonts::normal);
    Color white = ColorAlpha(Cwhite, 1 * shopFadeIn.value());
    bool unlocked = game.colorsUnlocked;

    std::string panelName = getPanelName(currentPanel);
    font.Render(panelName, {shopStart.x + shopBg.width / 2 - font.Measure(panelName) * 6 / 2, shopStart.y + 16 * scale}, 6, white);

    // Preview Panel
    Vector2 viewPanelPos = {shopStart.x + (shopBg.width - previewPanel.width) / 2, shopStart.y + 132};
    DrawTexture(previewPanel, viewPanelPos.x, viewPanelPos.y, WHITE);

    // Background
    int borderSize = 15;

    Rectangle clipArea;
    if (game.selectedMap == Textures::map1)
        clipArea = {256, 178, (previewPanel.width - borderSize * 2) / scale * 2, (previewPanel.height - borderSize * 2) / scale * 2};
    else
        clipArea = {112, 89, (previewPanel.width - borderSize * 2) / scale, (previewPanel.height - borderSize * 2) / scale};
    
    DrawTexturePro(GetTexture(game.selectedMap), clipArea, 
        {viewPanelPos.x + borderSize, viewPanelPos.y + borderSize, (float) previewPanel.width - borderSize * 2, (float) previewPanel.height - borderSize * 2}, {0, 0}, 0, WHITE);

    // Tractor
    Camera2D cam = Camera2D {{16, 0}, {0, 0}, 0, 3};
    BeginMode2D(cam);
        previewTractor.rect.y = (shopStart.y + 213) / cam.zoom;
        previewTractor.color = game.colors[game.selectedColor];
        previewTractor.Draw(cam, true);
    EndMode2D();
    
    // Colors 
    float y = viewPanelPos.y + previewPanel.height + 8;
    font.Render("Colors", {shopStart.x + 88, y}, 5, Cwhite);
    y += font.height * 5 + 16;

    int hoverIndex = -1;
    for (int index = 0; index < (signed) game.colors.size(); index++) {
        Rectangle dest = {shopStart.x + 104 + (64 + 36) * index, y, 56, 56};
        Color displayColor = toDisplayColor(game.colors[index]);
        DrawRectangle(dest.x, dest.y, dest.width, dest.height, displayColor);

        if (game.selectedColor == index) {
            DrawRectangleLinesEx({dest.x - 10, dest.y - 10, dest.width + 20, dest.height + 20}, 4, displayColor);
        } else {
            DrawRectangleLinesEx({dest.x - 10, dest.y - 10, dest.width + 20, dest.height + 20}, 4, Cgrey);

            if (CheckCollisionPointRec(GetMousePosition(), dest) && unlocked) {
                hoverIndex = index;
                if (colorTransitions[index] < 10)
                    colorTransitions[index]++;
            } else if (colorTransitions[index] > 0) {
                colorTransitions[index]--;
            }

            DrawRectangleLinesEx({dest.x - 10, dest.y - 10, dest.width + 20, dest.height + 20}, 4, ColorAlpha(CgreyLight, (float) colorTransitions[index] / 10));
        }
    }

    if (hoverIndex != -1) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            game.selectedColor = hoverIndex;
        }
    }

    if (!unlocked) {
        if (DrawShopLocked(shopStart, Panel::Colors, 100, game)) {
            game.colorsUnlocked = true;
        }
    }
}

void DrawBackground(Vector2 shopStart, GameData &game) {
    JakeFont &font = GetFont(Fonts::normal);
    Color white = ColorAlpha(Cwhite, 1 * shopFadeIn.value());
    bool unlocked = game.backgroundUnlocked;

    std::string panelName = getPanelName(currentPanel);
    font.Render(panelName, {shopStart.x + shopBg.width / 2 - font.Measure(panelName) * 6 / 2, shopStart.y + 16 * scale}, 6, white);

    // Preview Panel
    Vector2 viewPanelPos = {shopStart.x + (shopBg.width - previewPanel.width) / 2, shopStart.y + 132};
    DrawTexture(previewPanel, viewPanelPos.x, viewPanelPos.y, WHITE);

    // Background
    int borderSize = 15;

    Rectangle clipArea;
    if (game.selectedMap == Textures::map1)
        clipArea = {256, 178, (previewPanel.width - borderSize * 2) / scale * 2, (previewPanel.height - borderSize * 2) / scale * 2};
    else
        clipArea = {112, 89, (previewPanel.width - borderSize * 2) / scale, (previewPanel.height - borderSize * 2) / scale};
    
    DrawTexturePro(GetTexture(game.selectedMap), clipArea, 
        {viewPanelPos.x + borderSize, viewPanelPos.y + borderSize, (float) previewPanel.width - borderSize * 2, (float) previewPanel.height - borderSize * 2}, {0, 0}, 0, WHITE);

    // Tractor
    Camera2D cam = Camera2D {{16, 0}, {0, 0}, 0, 3};
    BeginMode2D(cam);
        previewTractor.rect.y = (shopStart.y + 213) / cam.zoom;
        previewTractor.color = game.colors[game.selectedColor];
        previewTractor.Draw(cam, true);
    EndMode2D();
    
    // Options 
    float y = viewPanelPos.y + previewPanel.height + 8;
        font.Render("Options", {shopStart.x + 88, y}, 5, Cwhite);
    y += font.height * 5 + 4;

    std::string names[3] = {"Farm", "Tropical", "Forest"};
    int hoverIndex = -1;
    for (int index = 0; index < (signed) backgroundOptions.size(); index++) {
        Vector2 pos = {shopStart.x + 118, y + font.height * 4 * index};

        if (backgroundOptions[index] == game.selectedMap) {
            font.Render(names[index], pos, 4, Cwhite);
        } else {
            font.Render(names[index], pos, 4, Cgrey);

            if (CheckCollisionPointRec(GetMousePosition(), {pos.x, pos.y, font.Measure(names[index]) * 4.0f, font.height * 4}) && unlocked) {
                if (optionTransitions[index] < 10)
                    optionTransitions[index]++;
                font.Render(names[index], pos, 4, ColorAlpha(CgreyLight, (float) optionTransitions[index] / 10));
                hoverIndex = index;
            } else if (optionTransitions[index] > 0) {
                optionTransitions[index]--;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverIndex != -1) {
        game.selectedMap = backgroundOptions[hoverIndex];
    }

    if (!unlocked) {
        if (DrawShopLocked(shopStart, Panel::Colors, 250, game)) {
            game.backgroundUnlocked = true;
        }
    }
}

void DrawEffects(Vector2 shopStart, GameData &game) {
    JakeFont &font = GetFont(Fonts::normal);
    Color white = ColorAlpha(Cwhite, 1 * shopFadeIn.value());
    bool unlocked = game.effectsUnlocked;

    std::string panelName = getPanelName(currentPanel);
    font.Render(panelName, {shopStart.x + shopBg.width / 2 - font.Measure(panelName) * 6 / 2, shopStart.y + 16 * scale}, 6, white);
    
    // Options 
    float y = shopStart.y + 136;
    font.Render("Effects", {shopStart.x + 88, y}, 5, Cwhite);
    y += font.height * 5 + 4;

    std::string names[13] = {
        "None", "Greyscale", "Posterization", "Dream Vision", "Pixelated", "Cross Hatching", 
        "Cross Stiching", "Predator View", "Scanlines", "Fisheye", "Sombel", "Bloom", "Blur"
    };

    int hoverIndex = -1;
    for (int index = 0; index < 13; index++) {
        Vector2 pos = {(index % 2 == 0 ? (shopStart.x + 118) : (shopStart.x + shopBg.width / 2)), (float) (y + (font.height + 2) * 4 * std::floor(index / 2))};

        if (index == (int) game.selectedShader) {
            font.Render(names[index], pos, 4, Cwhite);
        } else {
            font.Render(names[index], pos, 4, Cgrey);

            if (CheckCollisionPointRec(GetMousePosition(), {pos.x, pos.y, font.Measure(names[index]) * 4.0f, font.height * 4}) && unlocked) {
                if (shaderTransitions[index] < 10)
                    shaderTransitions[index]++;
                font.Render(names[index], pos, 4, ColorAlpha(CgreyLight, (float) shaderTransitions[index] / 10));
                hoverIndex = index;
            } else if (shaderTransitions[index] > 0) {
                shaderTransitions[index]--;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hoverIndex != -1) {
        SetNextShader((Shaders) hoverIndex);
    }

    if (!unlocked) {
        if (DrawShopLocked(shopStart, Panel::Colors, 500, game)) {
            game.effectsUnlocked = true;
        }
    }
}

std::string getPanelName(Panel panel) {
    switch (panel)
    {
    case Panel::Upgrades:
        return "Upgrades";
        break;
    
    case Panel::Colors:
        return "Colors";
        break;
    case Panel::Background:
        return "Background";
        break;
    case Panel::Effects:
        return "Effects";
        break;
    default:
        return "Error";
        break;
    }
}

void DrawRectCutCorners(Rectangle rect, float scale, Color color) {
    DrawRectangleRec(Rectangle {rect.x + scale, rect.y, rect.width - scale * 2, rect.height}, color);
    DrawRectangleRec(Rectangle {rect.x, rect.y + scale, scale, rect.height - scale * 2}, color);
    DrawRectangleRec(Rectangle {rect.x + rect.width - scale, rect.y + scale, scale, rect.height - scale * 2}, color);
}

bool DrawShopLocked(Vector2 shopStart, Panel panelName, int price, GameData &game) {
    bool isPressed = false;
    
    JakeFont &font = GetFont(Fonts::normal);
    DrawRectangle(shopStart.x + 42, shopStart.y + 42, shopBg.width - 84, shopBg.height - 84, Color {0, 0, 0, 200});

    int headerFontSize = 8;
    font.Render("Locked", {shopStart.x + (shopBg.width - font.Measure("Locked") * headerFontSize) / 2, shopStart.y + 228}, headerFontSize, ColorAlpha(Cwhite, shopFadeIn.value()));

    int buttonSize = 4;
    std::string buttonText = std::string("Buy: ") + std::to_string(price);
    Rectangle dest = {shopStart.x + (shopBg.width - font.Measure(buttonText) * buttonSize - 48) / 2, shopStart.y + 332, (float) font.Measure(buttonText) * buttonSize + 48, font.height * buttonSize + 10};
    DrawRectCutCorners(dest, buttonSize, CgreyMidDark);

    if (CheckCollisionPointRec(GetMousePosition(), dest)) {
        if (lockedTransitions[panelName] < 10)
            lockedTransitions[panelName]++;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && game.totalCoins() >= price) {
            isPressed = true;
            SpendCoins(price, game);
        }
    } else if (lockedTransitions[panelName] > 0) {
        lockedTransitions[panelName]--;
    }

    if (lockedTransitions[panelName] > 0)
        DrawRectCutCorners(dest, buttonSize, ColorAlpha(Cgrey, ((float) lockedTransitions[panelName] / 10.0f)));

    if (game.totalCoins() >= price)
        DrawTextCentered(dest, font, buttonText, buttonSize, positiveColor);
    else
        DrawTextCentered(dest, font, buttonText, buttonSize, negativeColor);
    
    return isPressed;
}

void setShopPanel(Panel newPanel) {
    currentPanel = newPanel;
}

void setShopStatus(bool status) {
    if (status && !shopOpen) {
        InitShop();
    }
    shopOpen = status;
}

void SpendCoins(int amount, GameData &game) {
    if (game.coins + game.inGameCoins >= amount) {
        game.coins -= amount;
        if (game.coins < 0) {
            game.inGameCoins += game.coins;
            game.coins = 0;
        }

        coinSubMultipier = (std::abs(game.totalCoins() - localCoins) / 120) * 5 + 7;
    }
}

Color toDisplayColor(Color color) {
    Vector3 hsv = ColorToHSV(color);
    return ColorFromHSV(hsv.x, max(hsv.y + 0.20f, 1), hsv.z);
}

bool isShopOpen() {
    return shopOpen;
}