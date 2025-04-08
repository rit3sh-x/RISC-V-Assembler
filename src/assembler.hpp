#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "types.hpp"

using namespace riscv;

class Assembler {
public:
    explicit Assembler(std::unordered_map<std::string, SymbolEntry> symbolTable, 
        std::vector<ParsedInstruction> parsedInstructions) 
        : errorCount(0), 
        symTable(std::move(symbolTable)), 
        parseInstructions(std::move(parsedInstructions)) {}

    inline bool assemble();

    inline const std::vector<std::pair<uint32_t, uint32_t>>& getMachineCode() const { return machineCode; }
    
    inline size_t getErrorCount() const { return errorCount; }

private:
    mutable size_t errorCount;

    std::unordered_map<std::string, SymbolEntry> symTable;

    std::vector<std::pair<uint32_t, uint32_t>> machineCode;
    std::vector<ParsedInstruction> parseInstructions;

    inline bool generateIType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    inline bool generateUType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);

    inline uint32_t generateRType(const std::string& opcode, const std::vector<std::string>& operands);
    inline uint32_t generateSType(const std::string& opcode, const std::vector<std::string>& operands);
    inline uint32_t generateSBType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    inline uint32_t generateUJType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    inline uint32_t calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const;
    
    inline void reportError(const std::string& message) const;
    inline void processTextSegment();
    inline void processDataSegment();
};

inline bool Assembler::assemble() {
    machineCode.clear();
    processTextSegment();
    processDataSegment();
    return errorCount == 0;
}

inline void Assembler::processTextSegment() {
    uint32_t currentAddress = TEXT_SEGMENT_START;

    for (const auto &inst : parseInstructions) {
        if (RTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
            machineCode.push_back({currentAddress, generateRType(inst.opcode, inst.operands)});
        }
        else if (ITypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
            generateIType(inst.opcode, inst.operands, currentAddress);
        }
        else if (STypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
            machineCode.push_back({currentAddress, generateSType(inst.opcode, inst.operands)});
        }
        else if (SBTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
            machineCode.push_back({currentAddress, generateSBType(inst.opcode, inst.operands, currentAddress)});
        }
        else if (UTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
            generateUType(inst.opcode, inst.operands, currentAddress);
        }
        else if (UJTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
            machineCode.push_back({currentAddress, generateUJType(inst.opcode, inst.operands, currentAddress)});
        }
        else {
            reportError("Unknown instruction type for opcode: " + inst.opcode);
            continue;
        }
        currentAddress += 4;
    }
}

inline void Assembler::processDataSegment() {
    for (const auto &pair : symTable) {
        const auto &entry = pair.second;
        if (entry.address >= DATA_SEGMENT_START) {
            uint32_t addr = entry.address;

            if (entry.isString) {
                for (size_t i = 0; i < entry.stringValue.length(); i++) {
                    machineCode.push_back({addr + i, static_cast<uint8_t>(entry.stringValue[i])});
                }
                if (entry.stringValue.empty() || entry.stringValue.back() != '\0') {
                    machineCode.push_back({addr + entry.stringValue.length(), 0});
                }
            }
            else {
                for (const auto &value : entry.numericValues) {
                    const auto directiveSize = getDirectiveSize(entry.directive);
                    if (directiveSize == 1) {
                        machineCode.push_back({addr, value & 0xFF});
                        addr += 1;
                    } else if (directiveSize == 2) {
                        machineCode.push_back({addr, value & 0xFF});
                        machineCode.push_back({addr + 1, (value >> 8) & 0xFF});
                        addr += 2;
                    } else if (directiveSize == 4) {
                        machineCode.push_back({addr, value & 0xFF});
                        machineCode.push_back({addr + 1, (value >> 8) & 0xFF});
                        machineCode.push_back({addr + 2, (value >> 16) & 0xFF});
                        machineCode.push_back({addr + 3, (value >> 24) & 0xFF});
                        addr += 4;
                    } else if (directiveSize == 8) {
                        machineCode.push_back({addr, value & 0xFF});
                        machineCode.push_back({addr + 1, (value >> 8) & 0xFF});
                        machineCode.push_back({addr + 2, (value >> 16) & 0xFF});
                        machineCode.push_back({addr + 3, (value >> 24) & 0xFF});
                        machineCode.push_back({addr + 4, (value >> 32) & 0xFF});
                        machineCode.push_back({addr + 5, (value >> 40) & 0xFF});
                        machineCode.push_back({addr + 6, (value >> 48) & 0xFF});
                        machineCode.push_back({addr + 7, (value >> 56) & 0xFF});
                        addr += 8;
                    }
                }
            }
        }
    }
    std::sort(machineCode.begin(), machineCode.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
}

inline uint32_t Assembler::generateRType(const std::string &opcode, const std::vector<std::string> &operands) {
    const auto& encoding = RTypeInstructions::getEncoding();
    uint32_t opcodeVal = encoding.opcodeMap.at(opcode);
    uint32_t funct3 = encoding.func3Map.at(opcode);
    uint32_t funct7 = encoding.func7Map.at(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t rs1 = getRegisterNumber(operands[1]);
    int32_t rs2 = getRegisterNumber(operands[2]);
    
    if (rd < 0 || rs1 < 0 || rs2 < 0 || rd > 31 || rs1 > 31 || rs2 > 31) {
        throw std::runtime_error(std::string(RED) + "Invalid register in R-type instruction" + RESET);
    }
    
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcodeVal;
}

inline bool Assembler::generateIType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    if (operands.size() != 3) {
        throw std::runtime_error(std::string(RED) + "I-type instruction requires 3 operands" + RESET);
    }
    
    const auto& encoding = ITypeInstructions::getEncoding();
    uint32_t opcodeVal = encoding.opcodeMap.at(opcode);
    uint32_t funct3 = encoding.func3Map.at(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t rs1;
    int32_t imm;
    
    if (opcode == "lb" || opcode == "lh" || opcode == "lw" || opcode == "lbu" || opcode == "lhu" || opcode == "ld") {
        std::string offset, baseReg;
        if (isMemory(operands[1], offset, baseReg)) {
            rs1 = getRegisterNumber(baseReg);
            imm = parseImmediate(offset);
        }
        else {
            imm = parseImmediate(operands[1]);
            rs1 = getRegisterNumber(operands[2]);
        }
    }
    else {
        rs1 = getRegisterNumber(operands[1]);
        imm = parseImmediate(operands[2]);
    }
    
    if (rd < 0 || rs1 < 0) {
        throw std::runtime_error(std::string(RED) + "Invalid register in I-type instruction" + RESET);
    }
    
    if ((opcode == "slli" || opcode == "srli" || opcode == "srai") && (imm < 0 || imm > 31)) {
        throw std::runtime_error(std::string(RED) + "Shift amount must be between 0 and 31" + RESET);
    }
    
    if (imm < -2048 || imm > 2047) {
        throw std::runtime_error(std::string(RED) + "Immediate value out of range for I-type instruction (-2048 to 2047)" + RESET);
    }
    
    uint32_t funct7 = (opcode == "srai") ? 0b0100000 : 0;
    if (encoding.func7Map.count(opcode)) {
        funct7 = encoding.func7Map.at(opcode);
    }
    
    uint32_t instruction = (funct7 << 25) | ((imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcodeVal;
    machineCode.push_back({currentAddress, instruction});
    return true;
}

inline uint32_t Assembler::generateSType(const std::string &opcode, const std::vector<std::string> &operands) {
    const auto& encoding = STypeInstructions::getEncoding();
    uint32_t opcodeVal = encoding.opcodeMap.at(opcode);
    uint32_t funct3 = encoding.func3Map.at(opcode);
    
    int32_t rs2 = getRegisterNumber(operands[0]);
    int32_t rs1, imm;
    
    if (operands.size() == 2) {
        std::string offset, reg;
        if (!isMemory(operands[1], offset, reg)) {
            throw std::runtime_error(std::string(RED) + "Invalid memory operand format" + RESET);
        }
        imm = parseImmediate(offset);
        rs1 = getRegisterNumber(reg);
    }
    else if (operands.size() == 3) {
        imm = parseImmediate(operands[1]);
        rs1 = getRegisterNumber(operands[2]);
    }
    else {
        throw std::runtime_error(std::string(RED) + "Invalid number of operands for S-type instruction" + RESET);
    }
    
    if (rs1 < 0 || rs2 < 0 || rs1 > 31 || rs2 > 31 || imm < -2048 || imm > 2047) {
        throw std::runtime_error(std::string(RED) + "Invalid parameter in S-type instruction" + RESET);
    }
    
    uint32_t imm_11_5 = ((imm >> 5) & 0x7F) << 25;
    uint32_t imm_4_0 = (imm & 0x1F) << 7;
    
    return imm_11_5 | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | imm_4_0 | opcodeVal;
}

inline uint32_t Assembler::generateSBType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    const auto& encoding = SBTypeInstructions::getEncoding();
    uint32_t opcodeVal = encoding.opcodeMap.at(opcode);
    uint32_t funct3 = encoding.func3Map.at(opcode);
    
    int32_t rs1 = getRegisterNumber(operands[0]);
    int32_t rs2 = getRegisterNumber(operands[1]);
    int32_t offset = (operands[2].find("0x") == 0 || operands[2].find("0b") == 0 ||
                    std::all_of(operands[2].begin(), operands[2].end(), ::isdigit) ||
                    operands[2][0] == '-')
                        ? parseImmediate(operands[2])
                        : calculateRelativeOffset(currentAddress, std::stoul(operands[2], nullptr, 0));
    
    if (rs1 < 0 || rs2 < 0 || rs1 > 31 || rs2 > 31 || offset < -4096 || offset > 4095 || offset & 1) {
        throw std::runtime_error(std::string(RED) + "Invalid parameter in SB-type instruction" + RESET);
    }
    
    return ((offset < 0 ? 1 : (offset >> 12) & 0x1) << 31) | ((offset >> 11 & 0x1) << 7) | 
           ((offset >> 5 & 0x3F) << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | 
           ((offset >> 1 & 0xF) << 8) | opcodeVal;
}

inline bool Assembler::generateUType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    if (operands.size() != 2) {
        throw std::runtime_error(std::string(RED) + "U-type instruction requires 2 operands" + RESET);
    }
    
    const auto& encoding = UTypeInstructions::getEncoding();
    uint32_t opcodeVal = encoding.opcodeMap.at(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t imm = parseImmediate(operands[1]);
    
    if (rd < 0 || imm < 0 || imm > 0xFFFFF) {
        throw std::runtime_error(std::string(RED) + "Invalid parameter in U-type instruction" + RESET);
    }
    
    uint32_t instruction = ((imm & 0xFFFFF) << 12) | (rd << 7) | opcodeVal;
    machineCode.push_back({currentAddress, instruction});
    return true;
}

inline uint32_t Assembler::generateUJType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    const auto& encoding = UJTypeInstructions::getEncoding();
    uint32_t opcodeVal = encoding.opcodeMap.at(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t offset = (operands[1].find("0x") == 0 || operands[1].find("0b") == 0 ||
                    std::all_of(operands[1].begin(), operands[1].end(), ::isdigit) ||
                    operands[1][0] == '-')
                        ? parseImmediate(operands[1])
                        : calculateRelativeOffset(currentAddress, std::stoul(operands[1], nullptr, 0));
    
    if (rd < 0 || rd > 31 || offset < -1048576 || offset > 1048575 || offset & 1) {
        throw std::runtime_error(std::string(RED) + "Invalid parameter in UJ-type instruction" + RESET);
    }
    
    return (((offset >> 20) & 0x1) << 31) | (((offset >> 1) & 0x3FF) << 21) | (((offset >> 11) & 0x1) << 20) |
           (((offset >> 12) & 0xFF) << 12) | (rd << 7) | opcodeVal;
}

inline void Assembler::reportError(const std::string &message) const {
    throw std::runtime_error(std::string(RED) + "Assembler Error: " + message + RESET);
    ++errorCount;
}

inline uint32_t Assembler::calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const {
    int32_t offset = static_cast<int32_t>(targetAddress - currentAddress);
    if (currentAddress == 0 || targetAddress == 0 || offset < -4096 || offset > 4095) {
        throw std::runtime_error(std::string(RED) + "Invalid offset calculation" + RESET);
    }
    return offset;
}

#endif