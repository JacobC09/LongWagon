#include "debug.h"

std::ostream &operator << (std::ostream &os, Rectangle &rect)
{
    os << "Rectangle {" << rect.x << ", " << rect.y << ", " << rect.width << ", " << rect.height << "}";
    return os;
}

std::ostream &operator << (std::ostream &os, Vector2 &vec)
{
    os << "Vector2 {" << vec.x << ", " << vec.y << "}";
    return os;
}

std::ostream &operator << (std::ostream &os, std::vector<std::string> vector) {
    os << "Vector {";
    for (unsigned int i = 0; i <  vector.size(); i++) {
        os << vector[i] << (i == vector.size() - 1 ? "}" : ", ");
    }
    return os;
}

std::ostream &operator << (std::ostream &os, std::vector<int> vector) {
    os << "Vector {";
    for (unsigned int i = 0; i < vector.size(); i++) {
        os << vector[i] << (i == vector.size() - 1 ? "}" : ", ");
    }
    return os;
}