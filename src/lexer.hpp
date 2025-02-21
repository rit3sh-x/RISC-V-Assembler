#ifndef LEXER_HPP
#define LEXER_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_set>
#include <cctype>
#include <algorithm>
#include "types.hpp"

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
public:
    static std::vector<std::vector<Token>> Tokenizer(const std::string& filename);
    static std::string getTokenTypeName(TokenType type) {
        return tokenTypeToString(type);
    }
private:
    static std::vector<Token> tokenizeLine(const std::string& line, int lineNumber);
    static bool isRegister(const std::string& token);
    static bool isImmediate(const std::string& token);
    static bool isMemoryAccess(const std::string& token);
    static bool isLabel(const std::string& token);
    static void parseMemoryAccess(const std::string& token, Token& memToken, int lineNumber);
    static std::string tokenTypeToString(TokenType type);
};

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string Lexer::tokenTypeToString(TokenType type) {
    switch (type) {
        case OPCODE: return "OPCODE";
        case REGISTER: return "REGISTER";
        case IMMEDIATE: return "IMMEDIATE";
        case MEMORY: return "MEMORY";
        case LABEL: return "LABEL";
        case DIRECTIVE: return "DIRECTIVE";
        default: return "UNKNOWN";
    }
}

bool Lexer::isRegister(const std::string& token) {
    if (token.size() <= 1 || token[0] != 'x') return false;
    int regNum = std::stoi(token.substr(1));
    return regNum >= 0 && regNum <= 31;
}

bool Lexer::isImmediate(const std::string& token) {
    if (token.empty()) return false;
    size_t pos = 0;
    if (token[0] == '-') pos = 1;
    if (token.size() > pos + 2 && token[pos] == '0' && token[pos + 1] == 'x') {
        return std::all_of(token.begin() + pos + 2, token.end(), ::isxdigit);
    }
    return std::all_of(token.begin() + pos, token.end(), ::isdigit);
}

bool Lexer::isMemoryAccess(const std::string& token) {
    size_t openParen = token.find('(');
    size_t closeParen = token.find(')');
    return openParen != std::string::npos && closeParen != std::string::npos && openParen < closeParen;
}

bool Lexer::isLabel(const std::string& token) {
    return !token.empty() && token.back() == ':';
}

void Lexer::parseMemoryAccess(const std::string& token, Token& memToken, int lineNumber) {
    size_t openParen = token.find('(');
    size_t closeParen = token.find(')', openParen);
    if (openParen == std::string::npos || closeParen == std::string::npos || openParen >= closeParen) {
        std::cerr << "Syntax Error (Line " << lineNumber << "): Invalid memory access format '" << token << "'\n";
        return;
    }

    std::string offsetStr = token.substr(0, openParen);
    std::string regStr = token.substr(openParen + 1, closeParen - openParen - 1);

    if (!offsetStr.empty() && !isImmediate(offsetStr)) {
        std::cerr << "Syntax Error (Line " << lineNumber << "): Invalid offset in memory access '" << token << "'\n";
        return;
    }

    if (!isRegister(regStr)) {
        std::cerr << "Syntax Error (Line " << lineNumber << "): Invalid register in memory access '" << token << "'\n";
        return;
    }

    memToken.type = MEMORY;
    memToken.value = offsetStr + "(" + regStr + ")";
}

std::vector<Token> Lexer::tokenizeLine(const std::string& line, int lineNumber) {
    std::vector<Token> tokens;
    std::stringstream ss(line);
    std::string token;
    int tokenIndex = 0;

    while (ss >> token) {
        if (!token.empty() && token.back() == ',') {
            token.pop_back();
        }
        
        if (token == "#" || token == "//") break;

        Token classifiedToken = {UNKNOWN, token};
        if (opcodes.count(token)) {
            classifiedToken.type = OPCODE;
        } else if (directives.count(token)) {
            classifiedToken.type = DIRECTIVE;
        } else if (isRegister(token)) {
            classifiedToken.type = REGISTER;
        } else if (isImmediate(token)) {
            classifiedToken.type = IMMEDIATE;
        } else if (isMemoryAccess(token)) {
            parseMemoryAccess(token, classifiedToken, lineNumber);
        } else if (isLabel(token)) {
            classifiedToken.type = LABEL;
        } else if (classifiedToken.type == UNKNOWN && tokenIndex != 0) {
            classifiedToken.type = LABEL;
        }
        tokenIndex++;
        if(classifiedToken.value != "") tokens.push_back(classifiedToken);
    }

    return tokens;
}

std::vector<std::vector<Token>> Lexer::Tokenizer(const std::string& filename) {
    std::vector<std::vector<Token>> tokenizedLines;
    std::ifstream inputFile(filename);

    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return {};
    }

    std::string rawLine;
    int lineNumber = 0;
    while (std::getline(inputFile, rawLine)) {
        ++lineNumber;

        std::string trimmedLine = trim(rawLine);
        if (trimmedLine.empty()) continue;

        std::vector<Token> tokens = tokenizeLine(trimmedLine, lineNumber);

        if (!tokens.empty()) {
            tokenizedLines.push_back(tokens);
        }
    }

    inputFile.close();
    return tokenizedLines;
}

#endif