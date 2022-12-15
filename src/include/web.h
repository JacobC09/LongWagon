#pragma once
#include <iostream>
#include <string>

#ifdef PLATFORM_WEB

#include "emscripten.h"

inline void submitScore(int score) {
    std::cout << "Submitting a new score" << std::endl;
    std::string code = std::string(std::string("submitNewScore(") + std::to_string(score) + std::string(")"));
    emscripten_run_script(code.c_str());
}

#else
    
inline void submitScore(int score) {
    std::cout << std::string("https://waf1le.pythonanywhere.com/add/?name=Jake&score=") + std::to_string(score) + std::string(")") << std::endl;
}

#endif