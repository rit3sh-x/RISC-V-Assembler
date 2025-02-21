#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

class Utility {
public:
    static std::string decimalToBinary(int num, int bits);
    static std::string decimalToHex(int num);
    static std::string binaryToHex(const std::string &binary);
    static int hexToDecimal(const std::string &hex);
    static void separateOffset(std::vector<std::string> &tokens);
    static bool isValidNumber(const std::string &str, int base);
    static bool isBranchSequential(const std::string &curPc, const std::string &nextPc);
};

std::string Utility::decimalToBinary(int num, int bits) {
    std::string binary(bits, '0');
    for (int i = bits - 1; i >= 0 && num; --i, num /= 2) {
        binary[i] = (num % 2) ? '1' : '0';
    }
    return binary;
}

std::string Utility::decimalToHex(int num) {
    if (num == 0) return "0";
    const char hexChars[] = "0123456789ABCDEF";
    std::string hex;
    hex.reserve(8);
    while (num) {
        hex.push_back(hexChars[num & 0xF]);
        num >>= 4;
    }
    std::reverse(hex.begin(), hex.end());
    return hex;
}

std::string Utility::binaryToHex(const std::string &binary) {
    if (binary.empty()) return "0";
    static const char binToHexMap[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    size_t len = binary.size();
    size_t padLen = (4 - len % 4) % 4;
    std::string hex;
    hex.reserve((len + padLen) / 4);
    int value = 0;
    for (size_t i = 0; i < padLen; ++i) value = (value << 1);
    for (size_t i = 0; i < len; ++i) {
        value = (value << 1) | (binary[i] - '0');
        if ((i + padLen + 1) % 4 == 0) {
            hex.push_back(binToHexMap[value]);
            value = 0;
        }
    }
    return hex;
}

int Utility::hexToDecimal(const std::string &hex) {
    int num = 0;
    for (char digit : hex) {
        num = (num << 4) | ((digit >= '0' && digit <= '9') ? (digit - '0') : ((digit & 0xDF) - 'A' + 10));
    }
    return num;
}

void Utility::separateOffset(std::vector<std::string> &tokens) {
    size_t openParenPos = tokens[2].find('(');
    size_t closeParenPos = tokens[2].find(')', openParenPos);
    if (openParenPos == std::string::npos || closeParenPos == std::string::npos) return;
    tokens.push_back(tokens[2].substr(0, openParenPos));
    tokens[2] = tokens[2].substr(openParenPos + 1, closeParenPos - openParenPos - 1);
}

bool Utility::isValidNumber(const std::string &str, int base) {
    if (str.empty()) return false;
    size_t start = (str[0] == '-') ? 1 : 0;
    for (size_t i = start; i < str.size(); ++i) {
        char c = str[i];

        if (base == 2) {
            if (c != '0' && c != '1') return false;
        } else if (base == 10) {
            if (c < '0' || c > '9') return false;
        } else if (base == 16) {
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
                return false;
        } else {
            return false;
        }
    }
    return true;
}

bool Utility::isBranchSequential(const std::string &curPc, const std::string &nextPc) {
    return hexToDecimal(nextPc) - hexToDecimal(curPc) == 4;
}

#endif