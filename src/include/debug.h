#pragma once
#include <iostream>
#include <vector>
#include <raylib.h>

#define print(x) std::cout << "\033[0;32m" << x << "\033[0m" << "\n";
#define error(x) std::cout << "\033[0;31mError: " << x << "\033[0m" << std::endl; exit(1);
#define ___ << ", " <<

std::ostream &operator << (std::ostream &os, std::vector<std::string> &vector);
std::ostream &operator << (std::ostream &os, std::vector<int> &vector);

std::ostream &operator << (std::ostream &os, Rectangle &rect);
std::ostream &operator << (std::ostream &os, Vector2 &vec);