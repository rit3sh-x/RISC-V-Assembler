#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>
#include "lexer.hpp"
#include "types.hpp"

using namespace riscv;

struct SymbolEntry {
    uint32_t address;
    bool isString;
    std::vector<uint64_t> numericValues;
    std::string stringValue;
    std::string directive;
};

struct ParsedInstruction {
    std::string opcode;
    std::vector<std::string> operands;
    uint32_t address;

    ParsedInstruction(std::string opc, std::vector<std::string> ops, uint32_t addr) : opcode(std::move(opc)), operands(std::move(ops)), address(addr) {}
};

class Parser {
public:
    explicit Parser(const std::vector<std::vector<Token>> &tokenizedLines) : tokens(tokenizedLines), errorCount(0), lastLabel("") {}
    bool parse();
    const std::unordered_map<std::string, SymbolEntry> &getSymbolTable() const { return symbolTable; }
    const std::vector<ParsedInstruction> &getParsedInstructions() const { return parsedInstructions; }
    bool hasErrors() const { return errorCount > 0; }
    size_t getErrorCount() const { return errorCount; }

private:
    const std::vector<std::vector<Token>> &tokens;
    std::unordered_map<std::string, SymbolEntry> symbolTable;
    std::vector<ParsedInstruction> parsedInstructions;
    uint32_t currentAddress = 0;
    bool inTextSection = false;
    bool inDataSection = false;
    mutable size_t errorCount = 0;
    std::string lastLabel;

    bool processFirstPass();
    bool processSecondPass();
    void handleDirective(const std::vector<Token> &line);
    bool handleInstruction(const std::vector<Token>& line);
    void addLabel(const std::string &label);
    uint32_t resolveLabel(const std::string &label) const;
    void reportError(const std::string &message, int lineNumber = 0) const;
    int32_t getRegisterNumber(const std::string& reg) const;
    int64_t parseImmediate(const std::string& imm) const;

    void handleSectionDirective(const std::string& directive) {
        if (directive == ".data") {
            inDataSection = true;
            inTextSection = false;
            currentAddress = DATA_SEGMENT_START;
        } else if (directive == ".text") {
            inTextSection = true;
            inDataSection = false;
            currentAddress = TEXT_SEGMENT_START;
        }
    }

    uint32_t getDirectiveSize(const std::string& directive) const {
        auto it = directives.find(directive);
        if (it != directives.end()) {
            return it->second;
        }
        return 0;
    }

    friend class Assembler;
};

bool Parser::parse() {
    if (tokens.empty()) {
        reportError("No tokens provided for parsing");
        return false;
    }
    return processFirstPass() && processSecondPass() && !hasErrors();
}

bool Parser::processFirstPass() {
    currentAddress = TEXT_SEGMENT_START;
    inTextSection = true;
    inDataSection = false;
    lastLabel.clear();
    symbolTable.clear();

    for (const auto &line : tokens) {
        if (line.empty()) continue;

        size_t tokenIndex = 0;

        if (line[0].type == TokenType::DIRECTIVE) {
            handleSectionDirective(line[0].value);
            continue;
        }

        while (tokenIndex < line.size()) {
            const Token &currentToken = line[tokenIndex];

            if (currentToken.type == TokenType::LABEL) {
                if (inDataSection) {
                    std::vector<Token> dataPathTokens;
                    dataPathTokens.push_back(currentToken);
                    tokenIndex++;

                    while (tokenIndex < line.size() && (line[tokenIndex].type == TokenType::DIRECTIVE || 
                                                       line[tokenIndex].type == TokenType::IMMEDIATE || 
                                                       line[tokenIndex].type == TokenType::STRING)) {
                        dataPathTokens.push_back(line[tokenIndex]);
                        tokenIndex++;
                    }
                    handleDirective(dataPathTokens);
                }
                else if (inTextSection) {
                    addLabel(currentToken.value);
                    tokenIndex++;
                    if (tokenIndex < line.size() && line[tokenIndex].type == TokenType::OPCODE) {
                        currentAddress += INSTRUCTION_SIZE;
                    }
                }
            }
            else if (currentToken.type == TokenType::OPCODE) {
                currentAddress += INSTRUCTION_SIZE;
                tokenIndex++;
            }
            else {
                tokenIndex++;
            }
        }
    }
    return !hasErrors();
}

void Parser::handleDirective(const std::vector<Token> &line) {
    if (line.empty()) {
        reportError("Empty directive encountered", 0);
        return;
    }

    size_t tokenIndex = 0;
    std::string label = "";

    if (line[0].type == TokenType::LABEL) {
        label = line[0].value;
        tokenIndex++;
    }

    if (tokenIndex >= line.size() || line[tokenIndex].type != TokenType::DIRECTIVE) {
        reportError("Expected directive after label", line[0].lineNumber);
        return;
    }

    const std::string &directive = line[tokenIndex].value;
    tokenIndex++;

    auto it = directives.find(directive);
    if (it == directives.end()) {
        reportError("Unsupported data directive '" + directive + "'", line[0].lineNumber);
        return;
    }

    uint32_t size = it->second;
    SymbolEntry entry;
    entry.address = currentAddress;
    entry.directive = directive;

    if (directive == ".asciz" || directive == ".ascii" || directive == ".asciiz") {
        if (tokenIndex >= line.size() || line[tokenIndex].type != TokenType::STRING) {
            reportError("Invalid or missing string literal for " + directive + " directive", line[0].lineNumber);
            return;
        }
        entry.stringValue = line[tokenIndex].value;
        entry.isString = true;
        uint32_t stringSize = entry.stringValue.length();
        bool addNullTerminator = (directive == ".asciz" || directive == ".asciiz");
        uint32_t wordsNeeded = (stringSize + (addNullTerminator ? 1 : 0) + 3) / 4;
        currentAddress += wordsNeeded * 4;
    }
    else {
        if (tokenIndex >= line.size()) {
            reportError("Missing value(s) for " + directive + " directive", line[0].lineNumber);
            return;
        }

        entry.isString = false;

        while (tokenIndex < line.size()) {
            if (line[tokenIndex].type == TokenType::IMMEDIATE) {
                try {
                    int64_t signedValue = parseImmediate(line[tokenIndex].value);
                    uint64_t value = static_cast<uint64_t>(signedValue);
                    if (directive == ".byte") {
                        if (signedValue < -128 || signedValue > 127) {
                            reportError("Value out of range for .byte directive: " + line[tokenIndex].value, line[0].lineNumber);
                            return;
                        }
                    } else if (directive == ".half") {
                        if (signedValue < -32768 || signedValue > 32767) {
                            reportError("Value out of range for .half directive: " + line[tokenIndex].value, line[0].lineNumber);
                            return;
                        }
                    } else if (directive == ".word") {
                        if (signedValue < -2147483648LL || signedValue > 2147483647LL) {
                            reportError("Value out of range for .word directive: " + line[tokenIndex].value, line[0].lineNumber);
                            return;
                        }
                    }
                    entry.numericValues.push_back(value);
                } catch (const std::exception& e) {
                    reportError("Invalid numeric value in " + directive + " directive: " + e.what(), line[0].lineNumber);
                    return;
                }
            }
            else if (line[tokenIndex].type == TokenType::STRING) {
                std::string strValue = line[tokenIndex].value;
                uint64_t packedValue = 0;
                size_t maxChars = 0;

                if (directive == ".byte") maxChars = 1;
                else if (directive == ".half") maxChars = 2;
                else if (directive == ".word") maxChars = 4;
                else if (directive == ".dword") maxChars = 8;

                if (strValue.length() > maxChars) {
                    reportError("Too many characters in " + directive + " directive; expected " + 
                               std::to_string(maxChars) + " per entry", line[0].lineNumber);
                    return;
                }

                for (size_t i = 0; i < strValue.length(); ++i) {
                    packedValue |= (static_cast<uint64_t>(strValue[i]) << (8 * i));
                }
                entry.numericValues.push_back(packedValue);
            }
            else {
                reportError("Invalid value in " + directive + " directive", line[0].lineNumber);
                return;
            }
            tokenIndex++;
        }
        currentAddress += size * entry.numericValues.size();
    }

    if (!label.empty()) {
        symbolTable[label] = entry;
    }
}

void Parser::addLabel(const std::string &label) {
    auto [it, inserted] = symbolTable.emplace(label, SymbolEntry{currentAddress, false, {}, "", ""});
    if (!inserted) reportError("Duplicate label '" + label + "'");
}

bool Parser::processSecondPass() {
    currentAddress = TEXT_SEGMENT_START;
    inTextSection = true;
    inDataSection = false;
    parsedInstructions.clear();
    lastLabel.clear();

    for (const auto &line : tokens) {
        if (line.empty()) continue;

        if (line[0].type == TokenType::DIRECTIVE) {
            if (line[0].value == ".data") {
                inDataSection = true;
                inTextSection = false;
                currentAddress = DATA_SEGMENT_START;
            }
            else if (line[0].value == ".text") {
                inTextSection = true;
                inDataSection = false;
                currentAddress = TEXT_SEGMENT_START;
            }
            continue;
        }

        size_t tokenIndex = 0;
        while (tokenIndex < line.size()) {
            const Token &currentToken = line[tokenIndex];

            if (currentToken.type == TokenType::LABEL && inTextSection) {
                lastLabel = currentToken.value;
                tokenIndex++;

                if (tokenIndex < line.size() && line[tokenIndex].type == TokenType::OPCODE) {
                    std::vector<Token> instructionTokens;
                    while (tokenIndex < line.size() && line[tokenIndex].type != TokenType::DIRECTIVE && 
                          line[tokenIndex].type != TokenType::LABEL) {
                        instructionTokens.push_back(line[tokenIndex]);
                        tokenIndex++;
                    }

                    if (!handleInstruction(instructionTokens)) {
                        reportError("Invalid instruction following label '" + currentToken.value + "'", line[0].lineNumber);
                    }
                    else {
                        currentAddress += INSTRUCTION_SIZE;
                    }
                }
            }
            else if (currentToken.type == TokenType::OPCODE) {
                std::vector<Token> instructionTokens;
                while (tokenIndex < line.size() && line[tokenIndex].type != TokenType::DIRECTIVE && 
                      line[tokenIndex].type != TokenType::LABEL) {
                    instructionTokens.push_back(line[tokenIndex]);
                    tokenIndex++;
                }

                if (!handleInstruction(instructionTokens)) {
                    reportError("Invalid instruction", line[0].lineNumber);
                }
                else {
                    currentAddress += INSTRUCTION_SIZE;
                }
            }
            else {
                tokenIndex++;
            }
        }
    }
    return !hasErrors();
}

bool Parser::handleInstruction(const std::vector<Token>& line) {
    if (line.empty()) {
        reportError("Empty instruction encountered", 0);
        return false;
    }

    if (!inTextSection) {
        reportError("Instruction outside of .text section", line[0].lineNumber);
        return false;
    }

    std::string opcode = line[0].value;
    std::vector<std::string> operands;

    if (riscv::opcodes.count(opcode) == 0) {
        reportError("Unknown opcode '" + opcode + "'", line[0].lineNumber);
        return false;
    }

    size_t expectedOperands = 0;
    bool isMemoryOp = false;
    bool isStore = false;
    bool isImm = false;
    bool isBranch = false;
    bool isUType = false;
    bool isUJType = false;
    
    const auto& iTypeEncoding = riscv::ITypeInstructions::getEncoding();
    if (iTypeEncoding.opcodeMap.count(opcode) && 
        iTypeEncoding.func3Map.count(opcode) &&
        (iTypeEncoding.func3Map.at(opcode) == 0b001 || 
         (iTypeEncoding.func3Map.at(opcode) == 0b101 && iTypeEncoding.func7Map.count(opcode)))) {
        expectedOperands = 3;
        isImm = true;
    }
    else if (riscv::RTypeInstructions::getEncoding().opcodeMap.count(opcode)) {
        expectedOperands = 3;
    }
    else if (riscv::ITypeInstructions::getEncoding().opcodeMap.count(opcode)) {
        expectedOperands = 3;
        isImm = true;
        if (opcode == "lb" || opcode == "lh" || opcode == "lw" || opcode == "lbu" || opcode == "lhu" || opcode == "ld") {
            isMemoryOp = true;
        }
    }
    else if (riscv::STypeInstructions::getEncoding().opcodeMap.count(opcode)) {
        expectedOperands = 3;
        isMemoryOp = true;
        isStore = true;
    }
    else if (riscv::SBTypeInstructions::getEncoding().opcodeMap.count(opcode)) {
        expectedOperands = 3;
        isBranch = true;
    }
    else if (riscv::UTypeInstructions::getEncoding().opcodeMap.count(opcode)) {
        expectedOperands = 2;
        isImm = true;
        isUType = true;
    }
    else if (riscv::UJTypeInstructions::getEncoding().opcodeMap.count(opcode)) {
        expectedOperands = 2;
        isImm = true;
        isUJType = true;
    }

    size_t i = 1;
    bool foundMemoryFormat = false;
    
    if (line.size() <= 1) {
        reportError("Missing operands for instruction '" + opcode + "'", line[0].lineNumber);
        return false;
    }
    
    while (i < line.size()) {
        const Token& token = line[i];
        
        if (token.value.empty()) {
            reportError("Empty token value in instruction", line[0].lineNumber);
            i++;
            continue;
        }
        
        if (isStore && i == 1) {
            if (!Lexer::isRegister(token.value)) {
                reportError("First operand of store instruction must be a register", line[0].lineNumber);
                return false;
            }
            operands.push_back(token.value);
            i++;
            continue;
        }

        if (isMemoryOp && ((isStore && i == 2) || (!isStore && i == 2))) {
            std::string offset, reg;
            if (Lexer::isMemory(token.value, offset, reg)) {
                foundMemoryFormat = true;
                try {
                    int32_t regNum = getRegisterNumber(reg);
                    if (regNum < 0) {
                        reportError("Invalid register in memory operand: " + reg, line[0].lineNumber);
                        return false;
                    }
                    int32_t imm = parseImmediate(offset);
                    if (imm < -2048 || imm > 2047) {
                        reportError("Memory offset out of range (-2048 to 2047): " + offset, line[0].lineNumber);
                        return false;
                    }
                    
                    operands.push_back(offset);
                    operands.push_back(reg);
                    i++;
                    continue;
                } catch (const std::exception& e) {
                    reportError("Invalid memory offset: " + offset + " - " + e.what(), line[0].lineNumber);
                    return false;
                }
            }
        }

        switch (token.type) {
            case TokenType::REGISTER: {
                int32_t regNum = getRegisterNumber(token.value);
                if (regNum < 0) {
                    reportError("Invalid register: " + token.value, line[0].lineNumber);
                    return false;
                }
                operands.push_back(token.value);
                break;
            }
            case TokenType::IMMEDIATE: {
                try {
                    int32_t imm = parseImmediate(token.value);
                    
                    if (isMemoryOp && !foundMemoryFormat) {
                        if (imm < -2048 || imm > 2047) {
                            reportError("Memory offset out of range (-2048 to 2047): " + token.value, line[0].lineNumber);
                            return false;
                        }
                    }
                    else if (isBranch) {
                        if (imm < -4096 || imm > 4095 || (imm & 1)) {
                            reportError("Branch offset must be even and in range (-4096 to 4095): " + token.value, line[0].lineNumber);
                            return false;
                        }
                    }
                    else if (isUType) {
                        if (imm < 0 || imm > 0xFFFFF) {
                            reportError("Immediate value out of range for U-type instruction (0 to 0xFFFFF): " + token.value, line[0].lineNumber);
                            return false;
                        }
                    }
                    else if (isUJType) {
                        if (imm < -524288 || imm > 524287 || (imm & 1)) {
                            reportError("Jump immediate must be even and in range (-524288 to 524287): " + token.value, line[0].lineNumber);
                            return false;
                        }
                    }
                    else if (isImm) {
                        if (imm < -2048 || imm > 2047) {
                            reportError("Immediate value out of range (-2048 to 2047): " + token.value, line[0].lineNumber);
                            return false;
                        }
                    }
                    operands.push_back(token.value);
                } catch (const std::exception& e) {
                    if (isMemoryOp && !foundMemoryFormat && Lexer::isRegister(token.value)) {
                        operands.push_back(token.value);
                    } else {
                        reportError("Invalid immediate value: " + token.value + " - " + e.what(), line[0].lineNumber);
                        return false;
                    }
                }
                break;
            }
            case TokenType::LABEL: {
                uint32_t labelAddress = resolveLabel(token.value);
                if (labelAddress == static_cast<uint32_t>(-1)) return false;
                
                if (isBranch || isUJType || opcode == "j") {
                    int32_t offset = static_cast<int32_t>(labelAddress - currentAddress);
                    if (isBranch && (offset < -4096 || offset > 4095 || (offset & 1))) {
                        reportError("Branch target out of range or misaligned: " + token.value, line[0].lineNumber);
                        return false;
                    } else if ((isUJType || opcode == "j") && (offset < -1048576 || offset > 1048575 || (offset & 1))) {
                        reportError("Jump target out of range or misaligned: " + token.value, line[0].lineNumber);
                        return false;
                    }
                    operands.push_back(std::to_string(offset));
                } else {
                    operands.push_back(std::to_string(labelAddress));
                }
                break;
            }
            case TokenType::UNKNOWN: {
                if (symbolTable.find(token.value) != symbolTable.end()) {
                    uint32_t labelAddress = resolveLabel(token.value);
                    if (labelAddress == static_cast<uint32_t>(-1)) return false;
                    
                    if (isBranch || isUJType || opcode == "j") {
                        int32_t offset = static_cast<int32_t>(labelAddress - currentAddress);
                        if (isBranch && (offset < -4096 || offset > 4095 || (offset & 1))) {
                            reportError("Branch target out of range or misaligned: " + token.value, line[0].lineNumber);
                            return false;
                        } else if ((isUJType || opcode == "j") && (offset < -1048576 || offset > 1048575 || (offset & 1))) {
                            reportError("Jump target out of range or misaligned: " + token.value, line[0].lineNumber);
                            return false;
                        }
                        operands.push_back(std::to_string(offset));
                    } else {
                        operands.push_back(std::to_string(labelAddress));
                    }
                } else if (Lexer::isRegister(token.value)) {
                    operands.push_back(token.value);
                } else {
                    reportError("Invalid operand or undefined label '" + token.value + "' in instruction", line[0].lineNumber);
                    return false;
                }
                break;
            }
            default:
                reportError("Invalid token type '" + getTokenTypeName(token.type) + "' with value '" + 
                           (token.value.empty() ? "empty" : token.value) + "' in instruction", line[0].lineNumber);
                return false;
        }
        i++;
    }

    if (isMemoryOp && !foundMemoryFormat && operands.size() == expectedOperands) {
        if (operands.empty() || operands.back().empty()) {
            reportError("Missing base register in memory operation", line[0].lineNumber);
            return false;
        }
        if (!Lexer::isRegister(operands.back())) {
            reportError("Invalid base register in memory operation: " + operands.back(), line[0].lineNumber);
            return false;
        }
    }
    
    if (operands.size() != expectedOperands) {
        reportError("Incorrect number of operands for '" + opcode + "' (expected " + std::to_string(expectedOperands) + 
                   ", got " + std::to_string(operands.size()) + ")", line[0].lineNumber);
        return false;
    }
    
    parsedInstructions.emplace_back(opcode, operands, currentAddress);
    return true;
}

uint32_t Parser::resolveLabel(const std::string &label) const {
    if (label.empty()) {
        reportError("Empty label encountered");
        return static_cast<uint32_t>(-1);
    }
    
    auto it = symbolTable.find(label);
    if (it != symbolTable.end()) {
        return it->second.address;
    }
    reportError("Undefined label '" + label + "'");
    return static_cast<uint32_t>(-1);
}

void Parser::reportError(const std::string &message, int lineNumber) const {
    if (lineNumber > 0) {
        std::cerr << "Parser Error on Line " << lineNumber << ": " << message << "\n";
    } else {
        std::cerr << "Parser Error: " << message << "\n";
    }
    ++errorCount;
}

int32_t Parser::getRegisterNumber(const std::string& reg) const {
    if (reg.empty()) {
        reportError("Empty register name");
        return -1;
    }

    std::string cleanReg = reg;
    cleanReg.erase(std::remove_if(cleanReg.begin(), cleanReg.end(), ::isspace), cleanReg.end());
    std::string lowerReg = cleanReg;
    std::transform(lowerReg.begin(), lowerReg.end(), lowerReg.begin(), ::tolower);
    
    auto it = riscv::validRegisters.find(lowerReg);
    if (it != riscv::validRegisters.end()) {
        return it->second;
    }
    
    if (lowerReg[0] == 'x' && lowerReg.length() > 1) {
        try {
            int num = std::stoi(lowerReg.substr(1));
            if (num >= 0 && num <= 31) {
                return num;
            }
        } catch (...) {}
    }
    
    reportError("Invalid register name: " + reg);
    return -1;
}

int64_t Parser::parseImmediate(const std::string& imm) const {
    try {
        std::string cleanImm = Lexer::trim(imm);
        if (cleanImm.empty()) {
            reportError("Empty immediate value");
            return 0;
        }

        bool isNegative = cleanImm[0] == '-';
        if (isNegative) {
            cleanImm = cleanImm.substr(1);
            if (cleanImm.empty()) {
                reportError("Invalid immediate value: just a negative sign");
                return 0;
            }
        }

        uint64_t value = 0;
        if (cleanImm.length() > 2 && cleanImm[0] == '0') {
            char format = ::tolower(cleanImm[1]);
            if (format == 'x') {
                if (cleanImm.length() <= 2) {
                    reportError("Invalid hexadecimal format: missing digits after '0x'");
                    return 0;
                }
                value = std::stoull(cleanImm, nullptr, 16);
            } else if (format == 'b') {
                if (cleanImm.length() <= 2) {
                    reportError("Invalid binary format: missing digits after '0b'");
                    return 0;
                }
                value = std::stoull(cleanImm.substr(2), nullptr, 2);
            } else {
                value = std::stoull(cleanImm, nullptr, 10);
            }
        } else {
            value = std::stoull(cleanImm, nullptr, 10);
        }
        return isNegative ? -static_cast<int64_t>(value) : static_cast<int64_t>(value);
    } catch (const std::exception& e) {
        std::string errorMsg = "Invalid immediate value '" + imm + "': " + e.what();
        reportError(errorMsg);
        throw std::runtime_error(errorMsg);
    }
}

#endif