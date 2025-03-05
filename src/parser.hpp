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

struct SymbolEntry
{
    uint32_t address;
    bool isString;
    std::vector<uint64_t> numericValues;
    std::string stringValue;
};

struct ParsedInstruction
{
    std::string opcode;
    std::vector<std::string> operands;
    uint32_t address;

    ParsedInstruction(std::string opc, std::vector<std::string> ops, uint32_t addr) : opcode(std::move(opc)), operands(std::move(ops)), address(addr) {}
};

class Parser
{
public:
    explicit Parser(const std::vector<std::vector<Token>> &tokenizedLines);
    bool parse();
    void printSymbolTable() const;
    void printParsedInstructions() const;

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
    bool handleInstruction(const std::vector<Token> &line);
    void addLabel(const std::string &label);
    uint32_t resolveLabel(const std::string &label) const;
    void reportError(const std::string &message, int lineNumber = 0) const;
    std::vector<Token>::const_iterator findNextDirectiveOrOpcode(const std::vector<Token> &line, std::vector<Token>::const_iterator start) const;
};

Parser::Parser(const std::vector<std::vector<Token>> &tokenizedLines) : tokens(tokenizedLines), errorCount(0), lastLabel("") {}

bool Parser::parse()
{
    if (tokens.empty())
    {
        reportError("No tokens provided for parsing");
        return false;
    }
    bool firstPassSuccess = processFirstPass();
    bool secondPassSuccess = processSecondPass();
    return firstPassSuccess && secondPassSuccess && !hasErrors();
}

std::vector<Token>::const_iterator Parser::findNextDirectiveOrOpcode(const std::vector<Token> &line, std::vector<Token>::const_iterator start) const
{
    for (auto it = start; it != line.end(); ++it)
    {
        if (it->type == TokenType::DIRECTIVE || it->type == TokenType::OPCODE)
        {
            return it;
        }
    }
    return line.end();
}

bool Parser::processFirstPass()
{
    currentAddress = TEXT_SEGMENT_START;
    inTextSection = true;
    inDataSection = false;
    lastLabel.clear();
    symbolTable.clear();

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const auto &line = tokens[i];
        if (line.empty())
            continue;

        size_t tokenIndex = 0;
        int lineNumber = line[0].lineNumber;

        if (line[0].type == TokenType::DIRECTIVE)
        {
            if (line[0].value == ".data")
            {
                inDataSection = true;
                inTextSection = false;
                currentAddress = DATA_SEGMENT_START;
            }
            else if (line[0].value == ".text")
            {
                inTextSection = true;
                inDataSection = false;
                currentAddress = TEXT_SEGMENT_START;
            }
            continue;
        }

        while (tokenIndex < line.size())
        {
            const Token &currentToken = line[tokenIndex];

            if (currentToken.type == TokenType::LABEL)
            {
                if (inDataSection)
                {
                    std::vector<Token> dataPathTokens;
                    dataPathTokens.push_back(currentToken);
                    tokenIndex++;

                    while (tokenIndex < line.size() &&
                           (line[tokenIndex].type == TokenType::DIRECTIVE || line[tokenIndex].type == TokenType::IMMEDIATE || line[tokenIndex].type == TokenType::STRING))
                    {
                        dataPathTokens.push_back(line[tokenIndex]);
                        tokenIndex++;
                    }

                    handleDirective(dataPathTokens);
                }
                else if (inTextSection)
                {
                    addLabel(currentToken.value);
                    tokenIndex++;
                    if (tokenIndex < line.size() &&
                        (line[tokenIndex].type == TokenType::OPCODE))
                    {
                        currentAddress += INSTRUCTION_SIZE;
                    }
                }
            }
            else if (currentToken.type == TokenType::OPCODE)
            {
                currentAddress += INSTRUCTION_SIZE;
            }
            tokenIndex++;
        }
    }
    return !hasErrors();
}

void Parser::handleDirective(const std::vector<Token> &line)
{
    if (line.empty())
    {
        reportError("Empty directive encountered", 0);
        return;
    }

    size_t tokenIndex = 0;
    std::string label = "";

    if (line[0].type == TokenType::LABEL)
    {
        label = line[0].value;
        tokenIndex++;
    }

    if (tokenIndex >= line.size() || line[tokenIndex].type != TokenType::DIRECTIVE)
    {
        reportError("Expected directive after label", line[0].lineNumber);
        return;
    }

    const std::string &directive = line[tokenIndex].value;
    tokenIndex++;

    auto it = directives.find(directive);
    if (it == directives.end())
    {
        reportError("Unsupported data directive '" + directive + "'", line[0].lineNumber);
        return;
    }

    uint32_t size = it->second;
    SymbolEntry entry;
    entry.address = currentAddress;

    if (directive == ".asciz" || directive == ".ascii" || directive == ".asciiz")
    {
        if (tokenIndex >= line.size() || line[tokenIndex].type != TokenType::STRING)
        {
            reportError("Invalid or missing string literal for " + directive + " directive", line[0].lineNumber);
            return;
        }

        entry.stringValue = line[tokenIndex].value;
        entry.isString = true;
        uint32_t stringSize = entry.stringValue.length() + ((directive == ".ascii") ? 0 : 1);
        uint32_t originalAddress = currentAddress;
        currentAddress += stringSize;

        currentAddress = (currentAddress + 3) & ~3;
        entry.address = originalAddress;
    }
    else
    {
        if (tokenIndex >= line.size())
        {
            reportError("Missing value(s) for " + directive + " directive", line[0].lineNumber);
            return;
        }

        entry.isString = false;

        while (tokenIndex < line.size())
        {
            if (line[tokenIndex].type == TokenType::IMMEDIATE)
            {
                entry.numericValues.push_back(std::stoull(line[tokenIndex].value));
            }
            else if (line[tokenIndex].type == TokenType::STRING)
            {

                std::string strValue = line[tokenIndex].value;

                if (directive == ".byte")
                {
                    if (strValue.length() > 1)
                    {
                        reportError("Too many characters in .byte directive; expected 1 per entry", line[0].lineNumber);
                        return;
                    }
                    entry.numericValues.push_back(static_cast<uint8_t>(strValue[0]));
                }
                else if (directive == ".half")
                {
                    if (strValue.length() > 2)
                    {
                        reportError("Too many characters in .half directive; expected 2 per entry", line[0].lineNumber);
                        return;
                    }
                    uint16_t packedValue = (strValue[0] << 8) | (strValue.length() > 1 ? strValue[1] : 0);
                    entry.numericValues.push_back(packedValue);
                }
                else if (directive == ".word")
                {
                    if (strValue.length() > 4)
                    {
                        reportError("Too many characters in .word directive; expected 4 per entry", line[0].lineNumber);
                        return;
                    }
                    uint32_t packedValue = 0;
                    for (size_t i = 0; i < strValue.length(); ++i)
                    {
                        packedValue |= (strValue[i] << (8 * i));
                    }
                    entry.numericValues.push_back(packedValue);
                }
                else if (directive == ".dword")
                {
                    if (strValue.length() > 8)
                    {
                        reportError("Too many characters in .dword directive; expected 8 per entry", line[0].lineNumber);
                        return;
                    }
                    uint64_t packedValue = 0;
                    for (size_t i = 0; i < strValue.length(); ++i)
                    {
                        packedValue |= (static_cast<uint64_t>(strValue[i]) << (8 * i));
                    }
                    entry.numericValues.push_back(packedValue);
                }
            }
            else
            {
                reportError("Invalid value in " + directive + " directive", line[0].lineNumber);
                return;
            }
            tokenIndex++;
        }

        if (size > 1)
        {
            currentAddress = (currentAddress + (size - 1)) & ~(size - 1);
        }

        currentAddress += size * entry.numericValues.size();
    }

    if (!label.empty())
    {
        symbolTable[label] = entry;
    }
}

void Parser::addLabel(const std::string &label)
{
    if (symbolTable.find(label) != symbolTable.end())
    {
        reportError("Duplicate label '" + label + "' found", 0);
    }
    else
    {
        symbolTable[label] = {currentAddress, false, {}, ""};
        lastLabel = label;
    }
}

bool Parser::processSecondPass()
{
    currentAddress = TEXT_SEGMENT_START;
    inTextSection = true;
    inDataSection = false;
    parsedInstructions.clear();
    lastLabel.clear();

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const auto &line = tokens[i];
        if (line.empty())
            continue;

        size_t tokenIndex = 0;
        int lineNumber = line[0].lineNumber;

        if (line[0].type == TokenType::DIRECTIVE)
        {
            if (line[0].value == ".data")
            {
                inDataSection = true;
                inTextSection = false;
                currentAddress = DATA_SEGMENT_START;
            }
            else if (line[0].value == ".text")
            {
                inTextSection = true;
                inDataSection = false;
                currentAddress = TEXT_SEGMENT_START;
            }
            continue;
        }

        while (tokenIndex < line.size())
        {
            const Token &currentToken = line[tokenIndex];

            if (currentToken.type == TokenType::LABEL && inTextSection)
            {
                lastLabel = currentToken.value;
                tokenIndex++;

                if (tokenIndex < line.size() &&
                    (line[tokenIndex].type == TokenType::OPCODE))
                {

                    std::vector<Token> instructionTokens;
                    while (tokenIndex < line.size() && line[tokenIndex].type != TokenType::DIRECTIVE && line[tokenIndex].type != TokenType::LABEL)
                    {
                        instructionTokens.push_back(line[tokenIndex]);
                        tokenIndex++;
                    }

                    if (!handleInstruction(instructionTokens))
                    {
                        reportError("Invalid instruction following label '" + currentToken.value + "'", lineNumber);
                    }
                    else
                    {
                        currentAddress += INSTRUCTION_SIZE;
                    }
                }
            }
            else if (currentToken.type == TokenType::OPCODE)
            {
                std::vector<Token> instructionTokens;
                while (tokenIndex < line.size() && line[tokenIndex].type != TokenType::DIRECTIVE && line[tokenIndex].type != TokenType::LABEL)
                {
                    instructionTokens.push_back(line[tokenIndex]);
                    tokenIndex++;
                }

                if (!handleInstruction(instructionTokens))
                {
                    reportError("Invalid instruction", lineNumber);
                }
                else
                {
                    currentAddress += INSTRUCTION_SIZE;
                }
            }
            else
            {
                tokenIndex++;
            }
        }
    }
    return !hasErrors();
}

bool Parser::handleInstruction(const std::vector<Token> &line)
{
    if (line.empty())
    {
        reportError("Empty instruction encountered", 0);
        return false;
    }

    if (!inTextSection)
    {
        reportError("Instruction outside of .text section", line[0].lineNumber);
        return false;
    }

    std::string opcode = line[0].value;
    std::vector<std::string> operands;

    if (riscv::opcodes.count(opcode) == 0)
    {
        reportError("Unknown opcode '" + opcode + "'", line[0].lineNumber);
        return false;
    }

    size_t expectedOperands = 0;
    if (riscv::RTypeInstructions::getEncoding().opcodeMap.count(opcode))
    {
        expectedOperands = 3;
    }
    else if (riscv::ITypeInstructions::getEncoding().opcodeMap.count(opcode))
    {
        expectedOperands = 3;
    }
    else if (riscv::STypeInstructions::getEncoding().opcodeMap.count(opcode))
    {
        expectedOperands = 3;
    }
    else if (riscv::SBTypeInstructions::getEncoding().opcodeMap.count(opcode))
    {
        expectedOperands = 3;
    }
    else if (riscv::UTypeInstructions::getEncoding().opcodeMap.count(opcode))
    {
        expectedOperands = 2;
    }
    else if (riscv::UJTypeInstructions::getEncoding().opcodeMap.count(opcode))
    {
        expectedOperands = 2;
    }

    size_t actualOperands = line.size() - 1;
    if (actualOperands != expectedOperands)
    {
        reportError("Incorrect number of operands for '" + opcode + "' (expected " + std::to_string(expectedOperands) + ", got " + std::to_string(actualOperands) + ")", line[0].lineNumber);
        return false;
    }

    for (size_t i = 1; i < line.size(); ++i)
    {
        const Token &token = line[i];
        switch (token.type)
        {
        case TokenType::REGISTER:
            if (riscv::validRegisters.find(token.value) == riscv::validRegisters.end() &&
                std::find_if(riscv::validRegisters.begin(), riscv::validRegisters.end(), [&](const auto &pair)
                             { return pair.second == token.value; }) == riscv::validRegisters.end())
            {
                reportError("Invalid register '" + token.value + "'", line[0].lineNumber);
                return false;
            }
            operands.push_back(token.value);
            break;
        case TokenType::IMMEDIATE:
            operands.push_back(token.value);
            break;
        case TokenType::LABEL:
        {
            uint32_t labelAddress = resolveLabel(token.value);
            if (labelAddress == static_cast<uint32_t>(-1)) return false;
            operands.push_back(std::to_string(labelAddress));
        }
        break;
        case TokenType::UNKNOWN:
        {
            std::string offset, reg;

            if (symbolTable.find(token.value) != symbolTable.end())
            {
                uint32_t labelAddress = resolveLabel(token.value);
                if (labelAddress == static_cast<uint32_t>(-1)) return false;
                operands.push_back(std::to_string(labelAddress));
            }
            else if (Lexer::isMemory(token.value, offset, reg))
            {
                operands.push_back(offset);
                operands.push_back(reg);
            }
            else
            {
                reportError("Invalid operand or undefined label '" + token.value + "' in instruction", line[0].lineNumber);
                return false;
            }
        }
        break;
        default:
            reportError("Invalid token type '" + Lexer::getTokenTypeName(token.type) + "' with value '" + token.value + "' in instruction", line[0].lineNumber);
            return false;
        }
    }

    parsedInstructions.emplace_back(opcode, operands, currentAddress);
    return true;
}

uint32_t Parser::resolveLabel(const std::string &label) const
{
    auto it = symbolTable.find(label);
    if (it != symbolTable.end())
    {
        return it->second.address;
    }
    reportError("Undefined label '" + label + "'");
    return static_cast<uint32_t>(-1);
}

void Parser::reportError(const std::string &message, int lineNumber) const
{
    if (lineNumber > 0)
    {
        std::cerr << "Parser Error on Line " << lineNumber << ": " << message << "\n";
    }
    else
    {
        std::cerr << "Parser Error: " << message << "\n";
    }
    ++errorCount;
}

void Parser::printSymbolTable() const
{
    std::cout << "Symbol Table:\n";
    if (symbolTable.empty())
    {
        std::cout << "  (empty)\n";
    }
    else
    {
        for (const auto &entry : symbolTable)
        {
            if (entry.second.isString)
            {
                std::cout << "  " << entry.first << " = " << entry.second.stringValue << " (0x" << std::hex << entry.second.address << std::dec << ")\n";
            }
            else
            {
                std::cout << "  " << entry.first << " = ";
                for (const auto &val : entry.second.numericValues)
                {
                    std::cout << val << " ";
                }
                std::cout << "(0x" << std::hex << entry.second.address << std::dec << ")\n";
            }
        }
    }
}

void Parser::printParsedInstructions() const
{
    std::cout << "Parsed Instructions:\n";
    if (parsedInstructions.empty())
    {
        std::cout << "  (none)\n";
    }
    else
    {
        for (const auto &inst : parsedInstructions)
        {
            std::cout << "  0x" << std::hex << inst.address << std::dec << ": " << inst.opcode;
            for (const auto &op : inst.operands)
            {
                std::cout << " " << op;
            }
            std::cout << "\n";
        }
    }
}

#endif