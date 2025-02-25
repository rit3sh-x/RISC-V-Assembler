#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include "lexer.hpp"

class Parser {
public:
    struct Instruction {
        std::string opcode;
        std::vector<std::string> operands;
        int lineNumber;
        bool isDirective = false;
    };

    static std::vector<Instruction> parse(const std::vector<std::vector<Token>>& tokens);
    
private:
    static void validateInstruction(const std::vector<Token>& tokens, std::vector<Instruction>& instructions, size_t& instructionCount);
    static bool isValidRType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst);
    static bool isValidIType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst);
    static bool isValidSType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst);
    static bool isValidBType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst);
    static bool isValidUType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst);
    static bool isValidJType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst);
    static std::string tokensToString(const std::vector<Token>& tokens);
    friend class Assembler;
    static const std::unordered_map<std::string, std::string> instructionFormats;
    static std::unordered_map<std::string, int> labelPositions;
};

const std::unordered_map<std::string, std::string> Parser::instructionFormats = {
    {"add", "R"}, {"sub", "R"}, {"and", "R"}, {"or", "R"}, {"xor", "R"},
    {"addi", "I"}, {"andi", "I"}, {"ori", "I"}, {"lb", "I"}, {"lh", "I"}, 
    {"lw", "I"}, {"jalr", "I"},
    {"sb", "S"}, {"sh", "S"}, {"sw", "S"},
    {"beq", "B"}, {"bne", "B"}, {"blt", "B"}, {"bge", "B"},
    {"lui", "U"}, {"auipc", "U"},
    {"jal", "J"},
    {"ecall", "Standalone"}
};

std::unordered_map<std::string, int> Parser::labelPositions;

std::string Parser::tokensToString(const std::vector<Token>& tokens) {
    std::stringstream ss;
    for (const auto& token : tokens) {
        ss << token.value << " ";
    }
    return ss.str();
}

std::vector<Parser::Instruction> Parser::parse(const std::vector<std::vector<Token>>& tokens) {
    std::vector<Instruction> instructions;
    labelPositions.clear();
    
    size_t instructionCount = 0;
    for (const auto& line : tokens) {
        if (!line.empty() && line[0].type == LABEL) {
            std::string labelName = line[0].value.substr(0, line[0].value.length() - 1);
            labelPositions[labelName] = instructionCount;
        }
        if (!line.empty() && (line[0].type == OPCODE || line[0].type == STANDALONE || 
            (line.size() > 1 && (line[1].type == OPCODE || line[1].type == STANDALONE)))) {
            instructionCount++;
        }
    }
    
    instructionCount = 0; // Reset for second pass
    for (const auto& line : tokens) {
        validateInstruction(line, instructions, instructionCount);
    }
    
    return instructions;
}

void Parser::validateInstruction(const std::vector<Token>& tokens, std::vector<Instruction>& instructions, size_t& instructionCount) {
    if (tokens.empty()) return;

    size_t pos = 0;
    Instruction inst;
    
    if (pos < tokens.size() && tokens[pos].type == LABEL) {
        pos++;
    }
    
    if (pos < tokens.size() && tokens[pos].type == DIRECTIVE) {
        inst.opcode = tokens[pos].value;
        inst.lineNumber = tokens[pos].lineNumber;
        inst.isDirective = true;
        instructions.push_back(inst);
        return;
    }
    
    if (pos >= tokens.size()) return;
    
    if (tokens[pos].type != OPCODE && tokens[pos].type != STANDALONE) {
        throw std::runtime_error("Line " + std::to_string(tokens[pos].lineNumber) + 
                               ": Expected opcode or standalone instruction, got '" + 
                               tokens[pos].value + "' in: " + tokensToString(tokens));
    }

    inst.opcode = tokens[pos].value;
    inst.lineNumber = tokens[pos].lineNumber;
    pos++;

    auto formatIt = instructionFormats.find(inst.opcode);
    if (formatIt == instructionFormats.end()) {
        throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + 
                               ": Unknown instruction '" + inst.opcode + "' in: " + 
                               tokensToString(tokens));
    }

    if (formatIt->second == "Standalone") {
        if (pos < tokens.size()) {
            throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + 
                                   ": Standalone instruction '" + inst.opcode + 
                                   "' takes no operands in: " + tokensToString(tokens));
        }
        instructions.push_back(inst);
        instructionCount++;
        return;
    }

    if (formatIt->second == "R" && isValidRType(tokens, pos, inst)) {}
    else if (formatIt->second == "I" && isValidIType(tokens, pos, inst)) {}
    else if (formatIt->second == "S" && isValidSType(tokens, pos, inst)) {}
    else if (formatIt->second == "B" && isValidBType(tokens, pos, inst)) {}
    else if (formatIt->second == "U" && isValidUType(tokens, pos, inst)) {}
    else if (formatIt->second == "J" && isValidJType(tokens, pos, inst)) {}
    else {
        throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + 
                               ": Invalid format for instruction '" + inst.opcode + 
                               "' in: " + tokensToString(tokens));
    }

    if (pos < tokens.size()) {
        throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + 
                               ": Extra tokens after instruction in: " + 
                               tokensToString(tokens));
    }

    instructions.push_back(inst);
    instructionCount++;
}

bool Parser::isValidRType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst) {
    if (pos + 2 >= tokens.size()) return false;
    if (tokens[pos].type != REGISTER || tokens[pos+1].type != REGISTER || 
        tokens[pos+2].type != REGISTER) return false;
    
    inst.operands = {tokens[pos].value, tokens[pos+1].value, tokens[pos+2].value};
    pos += 3;
    return true;
}

bool Parser::isValidIType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst) {
    if (pos + 1 >= tokens.size()) return false;
    
    if (tokens[pos].type == REGISTER && tokens[pos+1].type == MEMORY) {
        size_t paren = tokens[pos+1].value.find('(');
        if (paren == std::string::npos) return false;
        std::string offset = tokens[pos+1].value.substr(0, paren);
        std::string reg = tokens[pos+1].value.substr(paren + 1, tokens[pos+1].value.length() - paren - 2);
        inst.operands = {tokens[pos].value, reg, offset};
        pos += 2;
        return true;
    }
    if (pos + 2 >= tokens.size()) return false;
    if (tokens[pos].type != REGISTER || tokens[pos+1].type != REGISTER || 
        tokens[pos+2].type != IMMEDIATE) return false;
    
    inst.operands = {tokens[pos].value, tokens[pos+1].value, tokens[pos+2].value};
    pos += 3;
    return true;
}

bool Parser::isValidSType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst) {
    if (pos + 1 >= tokens.size()) return false;
    if (tokens[pos].type != REGISTER || tokens[pos+1].type != MEMORY) return false;
    
    size_t paren = tokens[pos+1].value.find('(');
    if (paren == std::string::npos) return false;
    std::string offset = tokens[pos+1].value.substr(0, paren);
    std::string reg = tokens[pos+1].value.substr(paren + 1, tokens[pos+1].value.length() - paren - 2);
    inst.operands = {tokens[pos].value, reg, offset};
    pos += 2;
    return true;
}

bool Parser::isValidBType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst) {
    if (pos + 2 >= tokens.size()) return false;
    if (tokens[pos].type != REGISTER || tokens[pos+1].type != REGISTER || 
        tokens[pos+2].type != LABEL) return false;
    
    std::string label = tokens[pos+2].value;
    if (labelPositions.find(label) == labelPositions.end()) {
        throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + 
                               ": Undefined label '" + label + "'");
    }
    
    inst.operands = {tokens[pos].value, tokens[pos+1].value, label};
    pos += 3;
    return true;
}

bool Parser::isValidUType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst) {
    if (pos + 1 >= tokens.size()) return false;
    if (tokens[pos].type != REGISTER || tokens[pos+1].type != IMMEDIATE) return false;
    
    inst.operands = {tokens[pos].value, tokens[pos+1].value};
    pos += 2;
    return true;
}

bool Parser::isValidJType(const std::vector<Token>& tokens, size_t& pos, Instruction& inst) {
    if (pos + 1 >= tokens.size()) return false;
    if (tokens[pos].type != REGISTER || tokens[pos+1].type != LABEL) return false;
    
    std::string label = tokens[pos+1].value;
    if (labelPositions.find(label) == labelPositions.end()) {
        throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + 
                               ": Undefined label '" + label + "'");
    }
    
    inst.operands = {tokens[pos].value, label};
    pos += 2;
    return true;
}

#endif