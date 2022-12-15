#include "ui.h"
#include "debug.h"

JakeFont::JakeFont(Texture2D text, std::string chars, int _spaceSize, Color splitColor) {
    texture = text;
    spaceSize = _spaceSize;

    height = (float) text.height;

    int characterIndex = 0;
    int begin = 0;
    int current = 1;
    Image image = LoadImageFromTexture(text);
    while (current < text.width && characterIndex < (signed) chars.size()) {
        Color pixelColor = GetImageColor(image, current, 0);
        if (pixelColor.r == splitColor.r && pixelColor.g == splitColor.g && pixelColor.b == splitColor.b && pixelColor.a == splitColor.a) {
            if (current > begin) {
                letters[chars[characterIndex]] = Rectangle {(float) begin, 0, (float) current - begin, (float) text.height};
                current++;
                characterIndex++;
                begin = current;
            }
        }
        current++;
    }
}

int JakeFont::Measure(std::string text) {
    int width = 0;
    for (char character : text) {
        if (character == '\n') {
            width = 0;
        } else if (character == ' ') {
            width += spaceSize;
        } else if (character == '\t') {
            width += spaceSize * 4;
        } else if (letters.find(character) != letters.end()) {
            width += letters[character].width + letterDistance;
        }
    }
    return width;
}

void JakeFont::Render(std::string text, Vector2 pos, float size, Color color) {
    for (char character : text) {
        if (character == '\n') {
            pos.x = 0;
            pos.y += (texture.height + lineOffset) * size;
        } else if (character == ' ') {
            pos.x += spaceSize * size;
        } else if (character == '\t') {
            pos.x += spaceSize * 4 * size;
        } else if (letters.find(character) != letters.end()) {
            Rectangle source = letters[character];
            Rectangle dest = {pos.x, pos.y, source.width * size, source.height * size};
            DrawTexturePro(texture, source, dest, {0, 0}, 0, color);
            pos.x += (source.width + letterDistance) * size;
        }
    }
}

void JakeFont::SetValues(int _letterDistance, int _lineOffset) {
    letterDistance = _letterDistance;
    lineOffset = _lineOffset;
}

Image BorderBox::GetImage(int width, int height, int scale, Image &sourceImage) {
    Image img = GenImageColor(width / scale, height / scale, Color {0, 0, 0, 0});
    ImageDraw(&img, sourceImage, topleft, {0, 0, topleft.width, topleft.height}, WHITE);
    ImageDraw(&img, sourceImage, topright, {img.width - topright.width, 0, topright.width, topright.height}, WHITE);
    ImageDraw(&img, sourceImage, bottomright, {img.width - bottomright.width, img.height - bottomright.height, bottomright.width, bottomright.height}, WHITE);
    ImageDraw(&img, sourceImage, bottomleft, {0, img.height - bottomleft.height, bottomleft.width, bottomleft.height}, WHITE);

    ImageDraw(&img, sourceImage, top, {topleft.width, 0, img.width - topleft.width - topright.width, top.height}, WHITE);
    ImageDraw(&img, sourceImage, bottom, {bottomleft.width, img.height - bottom.height, img.width - bottomleft.width - bottomright.width, bottom.height}, WHITE);
    ImageDraw(&img, sourceImage, left, {0, topleft.height, left.width, img.height - topleft.height - bottomleft.height}, WHITE);
    ImageDraw(&img, sourceImage, right, {img.width - right.width, topright.height, right.width, img.height - topright.height - bottomright.height}, WHITE);
    
    ImageDrawRectangle(&img, topleft.width, topleft.height, img.width - topleft.width - bottomright.width, img.height - topleft.height - bottomright.height, center);

    ImageResizeNN(&img, width, height);
    return img;
}

void DrawTextCentered(Rectangle region, JakeFont &font, std::string text, int size, Color color) {
    font.Render(text, {region.x + region.width / 2 - font.Measure(text) * size / 2, region.y + region.height / 2 - font.height * size / 2 - (size - 1)}, size, color);
}
