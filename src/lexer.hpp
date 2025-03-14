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

private:
    static std::string trim(const std::string& str);
    static bool isImmediate(const std::string& token);
    static bool isDirective(const std::string& token);
    static bool isLabel(const std::string& token);
    static std::vector<Token> tokenizeLine(const std::string& line, int lineNumber);
    static bool isRegister(const std::string& token);
    static bool isMemory(const std::string& token, std::string& offset, std::string& reg);
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

bool Lexer::isRegister(const std::string& token) {
    return validRegisters.count(token) > 0;
}

bool Lexer::isImmediate(const std::string& token) {
    if (token.empty()) return false;
    
    size_t pos = 0;
    if (token[0] == '-' || token[0] == '+') {
        if (token.length() == 1) return false;
        pos = 1;
    }
    if (token.length() > pos + 2 && token[pos] == '0') {
        char format = std::tolower(token[pos + 1]);
        if (format == 'x') {
            return token.length() > pos + 2 && std::all_of(token.begin() + pos + 2, token.end(), ::isxdigit);
        } else if (format == 'b') {
            return token.length() > pos + 2 && std::all_of(token.begin() + pos + 2, token.end(), [](char c) { return c == '0' || c == '1'; });
        }
    }
    return std::all_of(token.begin() + pos, token.end(), ::isdigit);
}

bool Lexer::isDirective(const std::string& token) {
    return directives.count(token) > 0;
}

bool Lexer::isLabel(const std::string& token) {
    return !token.empty() && token.back() == ':' && std::all_of(token.begin(), token.end() - 1, [](char c) { return std::isalnum(c) || c == '_' || c == '.'; });
}

bool Lexer::isMemory(const std::string& token, std::string& offset, std::string& reg) {
    size_t open = token.find('(');
    size_t close = token.find(')', open);
    if (open == std::string::npos || close == std::string::npos || close <= open) return false;
    offset = trim(token.substr(0, open));
    reg = trim(token.substr(open + 1, close - open - 1));
    if (offset.empty()) offset = "0";
    if (!isRegister(reg) || (!offset.empty() && !isImmediate(offset))) return false;
    if (close + 1 < token.length() && !trim(token.substr(close + 1)).empty()) return false;
    return true;
}

Token Lexer::classifyToken(const std::string& token, int lineNumber) {
    std::string trimmed = trim(token);
    if (trimmed.empty()) 
        return {TokenType::UNKNOWN, "", lineNumber};

    if (isRegister(trimmed)) 
        return {TokenType::REGISTER, trimmed, lineNumber};
    
    if (opcodes.count(trimmed)) 
        return {TokenType::OPCODE, trimmed, lineNumber};
    
    if (isDirective(trimmed)) 
        return {TokenType::DIRECTIVE, trimmed, lineNumber};
    
    if (isImmediate(trimmed)) 
        return {TokenType::IMMEDIATE, trimmed, lineNumber};
    
    if (isLabel(trimmed) && trimmed.length() > 1) {
        std::string labelName = trimmed.substr(0, trimmed.length() - 1);
        return {TokenType::LABEL, labelName, lineNumber};
    }
    
    return {TokenType::UNKNOWN, trimmed, lineNumber};
}

std::vector<Token> Lexer::tokenizeLine(const std::string& line, int lineNumber) {
    std::vector<Token> tokens;
    std::string token;
    bool inString = false;
    bool inMemory = false;
    int parenthesesCount = 0;
    
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty()) return tokens;

    for (size_t i = 0; i < trimmedLine.length(); ++i) {
        char c = trimmedLine[i];
        if (!inString && !inMemory && (c == '#' || (c == '/' && i + 1 < trimmedLine.length() && trimmedLine[i + 1] == '/'))) {
            break;
        }
        if (c == '"' && !inMemory) {
            if (inString) {
                tokens.push_back({TokenType::STRING, token, lineNumber});
                token.clear();
                inString = false;
            } else {
                if (!token.empty()) {
                    tokens.push_back(classifyToken(token, lineNumber));
                    token.clear();
                }
                inString = true;
            }
            continue;
        }
        if (inString) {
            token += c;
            continue;
        }
        if (c == '(' && !inMemory) {
            inMemory = true;
            parenthesesCount = 1;
            token += c;
            continue;
        }
        if (inMemory) {
            token += c;
            if (c == '(') parenthesesCount++;
            if (c == ')') parenthesesCount--;
            if (parenthesesCount == 0) {
                inMemory = false;
                std::string offset, reg;
                if (isMemory(token, offset, reg)) {
                    tokens.push_back({TokenType::IMMEDIATE, offset, lineNumber});
                    tokens.push_back({TokenType::REGISTER, reg, lineNumber});
                } else {
                    tokens.push_back(classifyToken(token, lineNumber));
                }
                token.clear();
            }
            continue;
        }
        if (std::isspace(c) || c == ',') {
            if (!token.empty()) {
                tokens.push_back(classifyToken(token, lineNumber));
                token.clear();
            }
            continue;
        }
        
        token += c;
    }
    if (!token.empty()) {
        tokens.push_back(classifyToken(token, lineNumber));
    }
    if (inString) {
        std::cerr << "Lexer Error on Line " << lineNumber << ": Unterminated string\n";
        exit(1);
    }
    if (inMemory) {
        std::cerr << "Lexer Error on Line " << lineNumber << ": Unterminated memory reference\n";
        exit(1);
    }
    return tokens;
}

std::vector<std::vector<Token>> Lexer::Tokenizer(const std::string& filename) {
    std::vector<std::vector<Token>> tokenizedLines;
    if (filename.empty()) {
        std::cerr << "Error: Empty filename provided\n";
        exit(1);
    }
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        exit(1);
    }  
    std::string line;
    int lineNumber = 0;
    try {
        while (std::getline(inputFile, line)) {
            ++lineNumber;
            std::vector<Token> tokens = tokenizeLine(line, lineNumber);
            if (!tokens.empty()) {
                tokenizedLines.push_back(std::move(tokens));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error while processing line " << lineNumber << ": " << e.what() << "\n";
        inputFile.close();
        exit(1);
    }
    inputFile.close();
    return tokenizedLines;
}

#endif