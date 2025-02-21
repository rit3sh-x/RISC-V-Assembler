#ifndef CLASSIFIER_HPP
#define CLASSIFIER_HPP

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <cctype>
#include <vector>
#include <algorithm>

enum InstructionType { R_TYPE, I_TYPE, S_TYPE, SB_TYPE, U_TYPE, UJ_TYPE, UNKNOWN };

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

bool isNumber(const std::string& str) {
    if (str.empty()) return false;
    if (str.size() > 2 && str[0] == '0' && str[1] == 'x') return true;
    return std::all_of(str.begin(), str.end(), ::isdigit);
}

std::vector<std::string> tokenize(const std::string& instruction) {
    std::istringstream stream(instruction);
    std::vector<std::string> tokens;
    std::string token;
    
    while (stream >> token) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

InstructionType getInstructionType(const std::string& instruction) {
    std::vector<std::string> tokens = tokenize(instruction);
    if (tokens.empty()) return UNKNOWN;

    std::string opcode = tokens[0];

    if (tokens.size() == 4 && tokens[1][0] == 'x' && tokens[2][0] == 'x' && tokens[3][0] == 'x') {
        return R_TYPE;
    }

    if (tokens.size() == 4 && tokens[1][0] == 'x' && tokens[2][0] == 'x' && isNumber(tokens[3])) {
        return I_TYPE;
    }

    if (tokens.size() == 3 && tokens[1][0] == 'x' && tokens[2].find('(') != std::string::npos && tokens[2].back() == ')') {
        return S_TYPE;
    }

    if (tokens.size() == 4 && tokens[1][0] == 'x' && tokens[2][0] == 'x' && (isNumber(tokens[3]) || isalpha(tokens[3][0]))) {
        return SB_TYPE;
    }

    if (tokens.size() == 3 && tokens[1][0] == 'x' && isNumber(tokens[2])) {
        return U_TYPE;
    }

    if (tokens.size() == 3 && tokens[1][0] == 'x' && (isNumber(tokens[2]) || isalpha(tokens[2][0]))) {
        return UJ_TYPE;
    }

    return UNKNOWN;
}

// int main() {
//     std::string instruction;
//     std::cout << "Enter an instruction: ";
//     std::getline(std::cin, instruction);
//     InstructionType type = getInstructionType(instruction);
//     switch (type) {
//         case R_TYPE: std::cout << "R-Type Instruction\n"; break;
//         case I_TYPE: std::cout << "I-Type Instruction\n"; break;
//         case S_TYPE: std::cout << "S-Type Instruction\n"; break;
//         case SB_TYPE: std::cout << "SB-Type Instruction\n"; break;
//         case U_TYPE: std::cout << "U-Type Instruction\n"; break;
//         case UJ_TYPE: std::cout << "UJ-Type Instruction\n"; break;
//         default: std::cout << "Unknown Instruction Format\n"; break;
//     }
//     return 0;
// }

#endif