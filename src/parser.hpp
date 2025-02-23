#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include "lexer.hpp"

struct ParsedInstruction {
    std::string opcode;
    std::vector<std::string> operands;
    int lineNumber;
};

class Parser {
private:
    std::unordered_map<std::string, int> symbolTable;  // Stores label definitions
    std::vector<std::pair<std::string, int>> forwardReferences;  // Tracks undefined labels
    std::vector<ParsedInstruction> parsedInstructions;  // Stores valid instructions

public:
    void processTokens(const std::vector<std::vector<Token>>& tokenizedLines);
    void resolveLabels();
    void printParsedInstructions();  // Debug function

private:
    bool validateInstruction(const std::vector<Token>& tokens, int lineNumber);
};

void Parser::processTokens(const std::vector<std::vector<Token>>& tokenizedLines) {
    int lineNumber = 0;

    for (const auto& lineTokens : tokenizedLines) {
        if (lineTokens.empty()) continue;
        lineNumber++;

        // Handle label definitions
        if (lineTokens[0].type == LABEL) {
            std::string labelName = lineTokens[0].value.substr(0, lineTokens[0].value.size() - 1); // Remove ':'
            symbolTable[labelName] = lineNumber;
        }

        // Validate instruction format
        if (!validateInstruction(lineTokens, lineNumber)) {
            std::cerr << "Syntax error on line " << lineNumber << "\n";
            continue;
        }

        // Store parsed instruction
        ParsedInstruction instruction;
        instruction.opcode = lineTokens[0].value;
        for (size_t i = 1; i < lineTokens.size(); i++) {
            instruction.operands.push_back(lineTokens[i].value);
        }
        instruction.lineNumber = lineNumber;

        parsedInstructions.push_back(instruction);
    }

    resolveLabels();
}

void Parser::resolveLabels() {
    for (const auto& ref : forwardReferences) {
        const std::string& label = ref.first;
        int lineNumber = ref.second;

        if (symbolTable.find(label) == symbolTable.end()) {
            std::cerr << "Error: Undefined label '" << label << "' referenced on line " << lineNumber << "\n";
        }
    }
}

bool Parser::validateInstruction(const std::vector<Token>& tokens, int lineNumber) {
    if (tokens.empty() || tokens[0].type != OPCODE) {
        std::cerr << "Error: Invalid instruction format on line " << lineNumber << "\n";
        return false;
    }

    // TODO: Add checks for each instruction type (R, I, S, B, U, J format)
    return true;  // Assume valid for now
}

void Parser::printParsedInstructions() {
    for (const auto& instr : parsedInstructions) {
        std::cout << instr.opcode << " ";
        for (const auto& operand : instr.operands) {
            std::cout << operand << " ";
        }
        std::cout << "(Line: " << instr.lineNumber << ")\n";
    }
}

#endif