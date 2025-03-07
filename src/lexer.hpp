#ifndef LEXER_HPP
#define LEXER_HPP
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <cctype>
#include <algorithm>
#include "types.hpp"

using namespace riscv;

struct Token {
    TokenType type;
    std::string value;
    int lineNumber;
    Token(TokenType t, const std::string& v, int ln) : type(t), value(v), lineNumber(ln) {}
};

class Lexer {
public:
    static std::vector<std::vector<Token>> Tokenizer(const std::string& filename);
    static std::string getTokenTypeName(TokenType type);
    static bool isImmediate(const std::string& token);
    static bool isDirective(const std::string& token);
    static bool isLabel(const std::string& token);
    static bool isMemory(const std::string& token, std::string& offset, std::string& reg);
    static std::string trim(const std::string& str);

private:
    static std::vector<Token> tokenizeLine(const std::string& line, int lineNumber);
    static bool isRegister(const std::string& token);
    static std::string tokenTypeToString(TokenType type);
    static Token classifyToken(const std::string& token, int lineNumber);

    friend class Parser;
    friend class Assembler;
};

std::string Lexer::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string Lexer::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::OPCODE: return "OPCODE";
        case TokenType::REGISTER: return "REGISTER";
        case TokenType::IMMEDIATE: return "IMMEDIATE";
        case TokenType::LABEL: return "LABEL";
        case TokenType::DIRECTIVE: return "DIRECTIVE";
        case TokenType::STRING: return "STRING";
        case TokenType::ERROR: return "ERROR";
        case TokenType::UNKNOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}

std::string Lexer::getTokenTypeName(TokenType type) {
    return tokenTypeToString(type);
}

bool Lexer::isRegister(const std::string& token) {
    std::string lowerToken = token;
    std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), ::tolower);
    
    // Simply check if the register name exists in the validRegisters map
    return validRegisters.find(lowerToken) != validRegisters.end();
}

bool Lexer::isImmediate(const std::string& token) {
    if (token.empty()) return false;
    
    size_t pos = 0;
    bool isHex = false;
    bool isBinary = false;
    
    // Handle negative numbers
    if (token[0] == '-' || token[0] == '+') pos = 1;
    
    // Empty after sign
    if (pos >= token.length()) return false;
    
    // Handle hex numbers (0x or 0X prefix)
    if (token.length() > pos + 2 && token[pos] == '0') {
        char format = ::tolower(token[pos + 1]);
        if (format == 'x') {
            isHex = true;
            pos += 2;
        } else if (format == 'b') {
            isBinary = true;
            pos += 2;
        }
    }
    
    // Empty after prefix
    if (pos >= token.length()) return false;
    
    // For hex numbers, verify all remaining characters are valid hex digits
    if (isHex) {
        return std::all_of(token.begin() + pos, token.end(), ::isxdigit);
    }
    
    // For binary numbers, verify all remaining characters are 0 or 1
    if (isBinary) {
        return std::all_of(token.begin() + pos, token.end(), [](char c) { return c == '0' || c == '1'; });
    }
    
    // For decimal numbers, verify all remaining characters are digits
    return std::all_of(token.begin() + pos, token.end(), ::isdigit);
}

bool Lexer::isDirective(const std::string& token) {
    std::string lowerToken = token;
    std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), ::tolower);
    return directives.count(lowerToken) > 0;
}

bool Lexer::isLabel(const std::string& token) {
    return !token.empty() && token.back() == ':' && std::all_of(token.begin(), token.end() - 1, [](char c) { return std::isalnum(c) || c == '_' || c == '.'; });
}

bool Lexer::isMemory(const std::string& token, std::string& offset, std::string& reg) {
    size_t open = token.find('(');
    size_t close = token.find(')');
    
    // Check if we have both parentheses and they're in the right order
    if (open == std::string::npos || close == std::string::npos || close <= open) {
        return false;
    }
    
    // Extract the offset and register parts
    offset = trim(token.substr(0, open));
    reg = trim(token.substr(open + 1, close - open - 1));
    
    // Handle empty offset case (treat as 0)
    if (offset.empty()) {
        offset = "0";
    }
    
    // Verify that the register part is valid
    if (!isRegister(reg)) {
        return false;
    }
    
    // Verify that the offset is a valid immediate value
    if (!offset.empty() && !isImmediate(offset)) {
        return false;
    }
    
    // Verify nothing after the closing parenthesis
    if (close + 1 < token.length()) {
        std::string remainder = trim(token.substr(close + 1));
        if (!remainder.empty()) {
            return false;
        }
    }
    
    return true;
}

Token Lexer::classifyToken(const std::string& token, int lineNumber) {
    std::string trimmed = trim(token);
    if (trimmed.empty()) return {TokenType::UNKNOWN, "", lineNumber};

    // First check if it's a register to avoid misclassifying register names as immediates
    if (isRegister(trimmed)) {
        return {TokenType::REGISTER, trimmed, lineNumber};
    }

    if (opcodes.count(trimmed)) {
        return {TokenType::OPCODE, trimmed, lineNumber};
    }
    if (isDirective(trimmed)) {
        return {TokenType::DIRECTIVE, trimmed, lineNumber};
    }
    if (isImmediate(trimmed)) {
        return {TokenType::IMMEDIATE, trimmed, lineNumber};
    }
    if (isLabel(trimmed)) {
        std::string labelName = trimmed.substr(0, trimmed.length() - 1);
        return {TokenType::LABEL, labelName, lineNumber};
    }
    return {TokenType::UNKNOWN, trimmed, lineNumber};
}

std::vector<Token> Lexer::tokenizeLine(const std::string& line, int lineNumber) {
    std::vector<Token> tokens;
    std::string token;
    bool inString = false;
    bool expectOperand = false;
    bool expectValueAfterComma = false;
    bool lastWasOpcode = false;

    std::string trimmedLine = trim(line);
    if (trimmedLine.empty()) return tokens;

    size_t i = 0;
    while (i < trimmedLine.length()) {
        char c = trimmedLine[i];

        if (!inString && (c == '#' || (c == '/' && i + 1 < trimmedLine.length() && trimmedLine[i + 1] == '/'))) {
            if (!token.empty()) {
                std::string offset, reg;
                if (isMemory(token, offset, reg)) {
                    // For memory operations, ensure proper handling of offsets
                    if (offset.empty()) offset = "0";
                    tokens.push_back({TokenType::IMMEDIATE, offset, lineNumber});
                    tokens.push_back({TokenType::REGISTER, reg, lineNumber});
                } else {
                    Token t = classifyToken(token, lineNumber);
                    tokens.push_back(t);
                    lastWasOpcode = (t.type == TokenType::OPCODE);
                }
                token.clear();
            }
            break;
        }

        if (c == '"') {
            if (!inString) {
                inString = true;
                if (!token.empty()) {
                    std::string offset, reg;
                    if (isMemory(token, offset, reg)) {
                        if (offset.empty()) offset = "0";
                        tokens.push_back({TokenType::IMMEDIATE, offset, lineNumber});
                        tokens.push_back({TokenType::REGISTER, reg, lineNumber});
                    } else {
                        Token t = classifyToken(token, lineNumber);
                        tokens.push_back(t);
                        lastWasOpcode = (t.type == TokenType::OPCODE);
                    }
                    token.clear();
                }
            } else {
                inString = false;
                tokens.push_back({TokenType::STRING, token, lineNumber});
                lastWasOpcode = false;
                token.clear();
            }
        }
        else if (inString) {
            token += c;
        }
        else if (std::isspace(c)) {
            if (!token.empty()) {
                std::string offset, reg;
                if (isMemory(token, offset, reg)) {
                    if (offset.empty()) offset = "0";
                    tokens.push_back({TokenType::IMMEDIATE, offset, lineNumber});
                    tokens.push_back({TokenType::REGISTER, reg, lineNumber});
                } else {
                    Token t = classifyToken(token, lineNumber);
                    tokens.push_back(t);
                    lastWasOpcode = (t.type == TokenType::OPCODE);
                }
                token.clear();
                expectValueAfterComma = false;
            }
        }
        else if (c == ',') {
            if (!token.empty()) {
                std::string offset, reg;
                if (isMemory(token, offset, reg)) {
                    if (offset.empty()) offset = "0";
                    tokens.push_back({TokenType::IMMEDIATE, offset, lineNumber});
                    tokens.push_back({TokenType::REGISTER, reg, lineNumber});
                } else {
                    Token t = classifyToken(token, lineNumber);
                    tokens.push_back(t);
                    lastWasOpcode = (t.type == TokenType::OPCODE);
                }
                token.clear();
            }
            expectValueAfterComma = true;
        }
        else {
            token += c;
        }
        ++i;
    }

    if (!token.empty()) {
        std::string offset, reg;
        if (isMemory(token, offset, reg)) {
            if (offset.empty()) offset = "0";
            tokens.push_back({TokenType::IMMEDIATE, offset, lineNumber});
            tokens.push_back({TokenType::REGISTER, reg, lineNumber});
        } else {
            Token t = classifyToken(token, lineNumber);
            tokens.push_back(t);
            lastWasOpcode = (t.type == TokenType::OPCODE);
        }
    }

    if (inString) {
        std::cerr << "Lexer Error on Line " << lineNumber << ": Unterminated string\n";
        exit(1);
    }

    return tokens;
}

std::vector<std::vector<Token>> Lexer::Tokenizer(const std::string& filename) {
    std::vector<std::vector<Token>> tokenizedLines;
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        exit(1);
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(inputFile, line)) {
        ++lineNumber;
        std::vector<Token> tokens = tokenizeLine(line, lineNumber);
        if (!tokens.empty()) {
            tokenizedLines.push_back(tokens);
        }
    }
    inputFile.close();
    return tokenizedLines;
}

#endif