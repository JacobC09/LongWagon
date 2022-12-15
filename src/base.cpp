#include <cmath>
#include "pch.h"
#include "base.h"

#define print(x) std::cout << x << std::endl;

std::string IntToBase(int integer, int base) {
    if (base > totalChars) {
        print("Base is too high")
        return "";
    }

    if (integer < base) return std::string(1, chars[integer]);

    std::string output;
    int placeValue = 0;
    int outputValue = 0;

    while (integer - outputValue != 0) {
        int placeValueExp = std::pow(base, placeValue);
        int value = ((integer - outputValue) % (int) std::pow(base, placeValue + 1)) / placeValueExp;
        output = chars[value] + output;
        outputValue += value * placeValueExp;
        placeValue++;
    }

    return output;
}

int BaseToInt(std::string number, int base) {
    int value = 0;
    int numberLength = (signed) number.size();
    
    for (int index = 0; index < numberLength; index++) {
        for (int charIndex = 0; charIndex < totalChars; charIndex++) {
            if (number[index] == chars[charIndex]) {
                value += charIndex * std::pow(base, numberLength - index - 1);
                break;
            }
        }
    }

    return value;
}

std::string EncryptString(std::string input) {
    std::string output = input;
    int inputSize = input.size();

    // Swap with the values three spaces ahead if its index is divisable by 3
    for (int index = 0; index < inputSize; index++) {
        if (index % 3 != 0) {
            char temp = output[(index * 2 + 3) % inputSize];
            output[(index * 2 + 3) % inputSize] = output[index];
            output[index] = temp;
        }
    }

    // Reverse everything
    std::string reversed;
    for (int index = inputSize - 1; index > -1; index--) {
        reversed += output[index];
    }

    return reversed;
}

std::string DecryptString(std::string input) {
    std::string output;
    int inputSize = input.size();

    // Reverse everything
    for (int index = inputSize - 1; index > -1; index--) {
        output += input[index];
    }

    // Undo: Swap with the values three spaces ahead if its index is divisable by 3
    for (int index = inputSize - 1; index > -1; index--) {
        if (index % 3 != 0) {
            char temp = output[(index * 2 + 3) % inputSize];
            output[(index * 2 + 3) % inputSize] = output[index];
            output[index] = temp;
        }
    }

    return output;
}
