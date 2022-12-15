#pragma once
#include "pch.h"
#include "game.h"

enum Panel {
    Upgrades,
    Colors,
    Background,
    Effects
};

const int totalPanels = 4;

/* Colors */
const Color Cgrey = {80, 105, 130, 255};
const Color CgreyLight = {114, 146, 172, 255};
const Color CgreyMidDark = {63, 84, 105, 255};
const Color CgreyDark = {50, 67, 84, 255};
const Color CgreyDarkDark = {32, 43, 62, 255};
const Color Cwhite = {241, 247, 255, 255};

bool isShopOpen();

void LoadShop();
void InitShop();
void UpdateShop(GameData &game, int bgOpacity=200);
void setShopPanel(Panel newPanel);
void setShopStatus(bool status);
