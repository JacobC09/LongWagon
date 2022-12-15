#pragma once
#include "pch.h"

struct BorderBox {
    Rectangle topleft;
    Rectangle topright;
    Rectangle bottomright;
    Rectangle bottomleft;
    Rectangle top;
    Rectangle left;
    Rectangle bottom;
    Rectangle right;
    
    Color center = WHITE;

    BorderBox() = default;
    Image GetImage(int width, int height, int scale, Image &sourceImage);
};

class JakeFont {
public:
    int spaceSize;
    int letterDistance = 1;
    int lineOffset = 0;
    float height;
    Texture2D texture;
    std::map<char, Rectangle> letters;

    JakeFont() = default;
    JakeFont(Texture2D text, std::string chars, int _spaceSize, Color splitColor = BLACK);

    int Measure(std::string text);
    void Render(std::string text, Vector2 pos, float size, Color color);
    void SetValues(int _letterDistance, int _lineOffset);
};

void DrawTextCentered(Rectangle region, JakeFont &font, std::string text, int size, Color color);