#ifndef LEXER_HPP
#define LEXER_HPP

#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include "types.hpp"

using namespace riscv;

class Lexer {
public:
    static std::vector<std::vector<Token>> tokenize(const std::string& input);

private:
    static std::vector<Token> tokenizeLine(const std::string& line, int lineNumber);

    static Token classifyToken(const std::string& token, int lineNumber);

    static bool isDirective(const std::string& token);
    static bool isLabel(const std::string& token);

    static void reportError(const std::string& message, int lineNumber);
};

inline bool Lexer::isDirective(const std::string& token) {
    return directives.count(token) > 0;
}

inline bool Lexer::isLabel(const std::string& token) {
    return !token.empty() && token.back() == ':' && std::all_of(token.begin(), token.end() - 1, [](char c) { return std::isalnum(c) || c == '_' || c == '.'; });
}

inline Token Lexer::classifyToken(const std::string& token, int lineNumber) {
    std::string trimmed = trim(token);
    if (trimmed.empty()) {
        throw std::runtime_error(std::string(RED) + "Empty token found on line " + std::to_string(lineNumber) + RESET);
        return {TokenType::UNKNOWN, "", lineNumber};
    }
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
    if (isLabel(trimmed) && trimmed.length() > 1) {
        return {TokenType::LABEL, trimmed.substr(0, trimmed.length() - 1), lineNumber};
    }
    return {TokenType::UNKNOWN, trimmed, lineNumber};
}

inline void Lexer::reportError(const std::string& message, int lineNumber) {
    throw std::runtime_error(std::string(RED) + "Lexer Error on Line " + std::to_string(lineNumber) + ": " + message + RESET);
}

inline std::vector<Token> Lexer::tokenizeLine(const std::string& line, int lineNumber) {
    std::vector<Token> tokens;
    std::string currentToken;
    bool inString = false;
    bool inMemory = false;
    int parenthesesCount = 0;

    const std::string& trimmedLine = trim(line);
    if (trimmedLine.empty()) return tokens;

    for (size_t i = 0; i < trimmedLine.length(); ++i) {
        char c = trimmedLine[i];
        if (!inString && !inMemory && (c == '#' || (c == '/' && i + 1 < trimmedLine.length() && trimmedLine[i + 1] == '/'))) {
            break;
        }
        if (c == '"' && !inMemory) {
            if (inString) {
                tokens.push_back({TokenType::STRING, currentToken, lineNumber});
                currentToken.clear();
                inString = false;
            } else {
                if (!currentToken.empty()) {
                    tokens.push_back(classifyToken(currentToken, lineNumber));
                    currentToken.clear();
                }
                inString = true;
            }
            continue;
        }
        if (inString) {
            currentToken += c;
            continue;
        }
        if (c == '(' && !inMemory) {
            inMemory = true;
            parenthesesCount = 1;
            currentToken += c;
            continue;
        }
        if (inMemory) {
            currentToken += c;
            if (c == '(') ++parenthesesCount;
            if (c == ')') --parenthesesCount;
            if (parenthesesCount == 0) {
                inMemory = false;
                std::string offset, reg;
                if (isMemory(currentToken, offset, reg)) {
                    tokens.push_back({TokenType::IMMEDIATE, offset, lineNumber});
                    tokens.push_back({TokenType::REGISTER, reg, lineNumber});
                } else {
                    throw std::runtime_error(std::string(RED) + "Invalid memory reference: " + currentToken + RESET);
                }
                currentToken.clear();
            }
            continue;
        }
        if (std::isspace(c) || c == ',') {
            if (!currentToken.empty()) {
                tokens.push_back(classifyToken(currentToken, lineNumber));
                currentToken.clear();
            }
            continue;
        }
        currentToken += c;
    }
    if (!currentToken.empty()) {
        tokens.push_back(classifyToken(currentToken, lineNumber));
    }
    if (inString) {
        reportError("Unterminated string", lineNumber);
    }
    if (inMemory) {
        reportError("Unterminated memory reference", lineNumber);
    }
    return tokens;
}

inline std::vector<std::vector<Token>> Lexer::tokenize(const std::string& input) {
    std::vector<std::vector<Token>> tokenizedLines;

    if (input.empty()) {
        reportError("Empty input provided", 0);
        return tokenizedLines;
    }

    std::string inputStr(input);
    std::istringstream inputStream(inputStr);
    std::string line;
    int lineNumber = 0;

    while (std::getline(inputStream, line)) {
        ++lineNumber;
        std::vector<Token> tokens = tokenizeLine(line, lineNumber);
        if (!tokens.empty()) {
            tokenizedLines.push_back(std::move(tokens));
        }
    }
    return tokenizedLines;
}

#endif