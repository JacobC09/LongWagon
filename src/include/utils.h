#pragma once
#include "raylib.h"
#include <cmath>

inline float min(float number, float minimum) {
    if (number < minimum) return minimum;
    return number;
}

inline float max(float number, float maximum) {
    if (number > maximum) return maximum;
    return number;
}

inline int cap(int integer, int minimum, int maximum) {
    if (integer > maximum) return maximum;
    if (integer < minimum) return minimum;
    return integer; 
}

inline Vector2 toScreenPos(Vector2 pos, Camera2D camera) {
    return Vector2 {
        (float) (pos.x - camera.offset.x) * camera.zoom,
        (float) (pos.y - camera.offset.y) * camera.zoom,
    };
}

inline Rectangle toScreenPos(Rectangle rect, Camera2D camera) {
    return Rectangle {
        (float) (rect.x - camera.offset.x) * camera.zoom,
        (float) (rect.y - camera.offset.y) * camera.zoom,
        (float) rect.width * camera.zoom,
        (float) rect.height * camera.zoom
    };
}

inline Vector2 toRelativePos(Vector2 pos, Camera2D camera) {
    return Vector2 {
        (float) (pos.x / camera.zoom) + camera.offset.x,
        (float) (pos.y / camera.zoom) + camera.offset.y,
    };
}

inline bool isNumber(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

inline float Diminish(float value, float diminisher, float target = 0) {
    if (std::abs(value - target) < diminisher) {
        return target;
    }
        
    if (value > target) {
        return value - diminisher;
    } else {
        return value + diminisher;
    }
}

inline float Distance(Vector2 pointA, Vector2 pointB) {
    return std::sqrt(pow(std::abs(pointA.x - pointB.x), 2) + pow(std::abs(pointA.y - pointB.y), 2));
}

inline Color Morph(float percentage, Color from, Color to) {
    return Color {
        (unsigned char) ((int) from.r + ((int) to.r - (int) from.r) * percentage), 
        (unsigned char) ((int) from.g + ((int) to.g - (int) from.g) * percentage), 
        (unsigned char) ((int) from.b + ((int) to.b - (int) from.b) * percentage),
        (unsigned char) ((int) from.a + ((int) to.a - (int) from.a) * percentage)
    };
}

template <typename T>
inline void clearHeapVector(std::vector<T*> &array) {
    for (auto &pointer : array) {
        free(pointer);
    }
    array.clear();
}