#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <bitset>
#include "parser.hpp"
#include "types.hpp"

using namespace riscv;

struct OpcodeInfo {
    uint32_t opcode;
    uint32_t funct3;
    uint32_t funct7;
};

class Assembler {
public:
    explicit Assembler(const Parser& parser);
    bool assemble();
    bool writeToFile(const std::string& filename);
    const std::vector<std::pair<uint32_t, uint32_t>>& getMachineCode() const { return machineCode; }
    bool hasErrors() const { return errorCount > 0; }
    size_t getErrorCount() const { return errorCount; }

private:
    const Parser& parser;
    std::vector<std::pair<uint32_t, uint32_t>> machineCode;
    mutable size_t errorCount = 0;

    uint32_t generateRType(const std::string& opcode, const std::vector<std::string>& operands);
    uint32_t generateIType(const std::string& opcode, const std::vector<std::string>& operands);
    uint32_t generateSType(const std::string& opcode, const std::vector<std::string>& operands);
    uint32_t generateSBType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    uint32_t generateUType(const std::string& opcode, const std::vector<std::string>& operands);
    uint32_t generateUJType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    uint32_t generateStandalone(const std::string& opcode);
    
    int32_t getRegisterNumber(const std::string& reg) const;
    void reportError(const std::string& message) const;
    uint32_t calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const;

    OpcodeInfo getOpcodeInfo(const std::string& opcode) const;

    void processTextSegment(const std::vector<ParsedInstruction>& instructions);
    void processDataSegment(const std::unordered_map<std::string, SymbolEntry>& symbolTable);
};

Assembler::Assembler(const Parser& p) : parser(p), errorCount(0) {}

OpcodeInfo Assembler::getOpcodeInfo(const std::string& opcode) const {
    OpcodeInfo info = {0, 0, 0};

    if (opcode == "add")  { info = {0b0110011, 0b000, 0b0000000}; }
    else if (opcode == "sub")  { info = {0b0110011, 0b000, 0b0100000}; }
    else if (opcode == "sll")  { info = {0b0110011, 0b001, 0b0000000}; }
    else if (opcode == "slt")  { info = {0b0110011, 0b010, 0b0000000}; }
    else if (opcode == "sltu") { info = {0b0110011, 0b011, 0b0000000}; }
    else if (opcode == "xor")  { info = {0b0110011, 0b100, 0b0000000}; }
    else if (opcode == "srl")  { info = {0b0110011, 0b101, 0b0000000}; }
    else if (opcode == "sra")  { info = {0b0110011, 0b101, 0b0100000}; }
    else if (opcode == "or")   { info = {0b0110011, 0b110, 0b0000000}; }
    else if (opcode == "and")  { info = {0b0110011, 0b111, 0b0000000}; }

    else if (opcode == "addi")  { info = {0b0010011, 0b000, 0}; }
    else if (opcode == "slti")  { info = {0b0010011, 0b010, 0}; }
    else if (opcode == "sltiu") { info = {0b0010011, 0b011, 0}; }
    else if (opcode == "xori")  { info = {0b0010011, 0b100, 0}; }
    else if (opcode == "ori")   { info = {0b0010011, 0b110, 0}; }
    else if (opcode == "andi")  { info = {0b0010011, 0b111, 0}; }
    else if (opcode == "lb")    { info = {0b0000011, 0b000, 0}; }
    else if (opcode == "lh")    { info = {0b0000011, 0b001, 0}; }
    else if (opcode == "lw")    { info = {0b0000011, 0b010, 0}; }
    else if (opcode == "lbu")   { info = {0b0000011, 0b100, 0}; }
    else if (opcode == "lhu")   { info = {0b0000011, 0b101, 0}; }

    else if (opcode == "sb") { info = {0b0100011, 0b000, 0}; }
    else if (opcode == "sh") { info = {0b0100011, 0b001, 0}; }
    else if (opcode == "sw") { info = {0b0100011, 0b010, 0}; }

    else if (opcode == "beq")  { info = {0b1100011, 0b000, 0}; }
    else if (opcode == "bne")  { info = {0b1100011, 0b001, 0}; }
    else if (opcode == "blt")  { info = {0b1100011, 0b100, 0}; }
    else if (opcode == "bge")  { info = {0b1100011, 0b101, 0}; }
    else if (opcode == "bltu") { info = {0b1100011, 0b110, 0}; }
    else if (opcode == "bgeu") { info = {0b1100011, 0b111, 0}; }

    else if (opcode == "lui")   { info = {0b0110111, 0, 0}; }
    else if (opcode == "auipc") { info = {0b0010111, 0, 0}; }

    else if (opcode == "jal") { info = {0b1101111, 0, 0}; }
    return info;
}

bool Assembler::assemble() {
    if (parser.hasErrors()) {
        reportError("Cannot assemble due to parser errors");
        return false;
    }

    machineCode.clear();
    const auto& instructions = parser.getParsedInstructions();
    const auto& symbolTable = parser.getSymbolTable();

    try {
        processTextSegment(instructions);
        machineCode.push_back({TEXT_SEGMENT_START + instructions.size() * 4, 0xDEADBEEF});
        processDataSegment(symbolTable);
    }
    catch (const std::exception& e) {
        reportError(std::string("Assembly failed: ") + e.what());
        return false;
    }

    return !hasErrors();
}

void Assembler::processTextSegment(const std::vector<ParsedInstruction>& instructions) {
    uint32_t currentAddress = TEXT_SEGMENT_START;

    for (const auto& inst : instructions) {
        uint32_t machineInstruction = 0;

        try {
            if (riscv::RTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateRType(inst.opcode, inst.operands);
            }
            else if (riscv::ITypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateIType(inst.opcode, inst.operands);
            }
            else if (riscv::STypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateSType(inst.opcode, inst.operands);
            }
            else if (riscv::SBTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateSBType(inst.opcode, inst.operands, currentAddress);
            }
            else if (riscv::UTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateUType(inst.opcode, inst.operands);
            }
            else if (riscv::UJTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateUJType(inst.opcode, inst.operands, currentAddress);
            }
            else {
                reportError("Unknown instruction type for opcode: " + inst.opcode);
                continue;
            }

            machineCode.push_back({currentAddress, machineInstruction});
            currentAddress += 4;
        }
        catch (const std::exception& e) {
            reportError("Error assembling instruction: " + inst.opcode + " - " + e.what());
        }
    }
}

void Assembler::processDataSegment(const std::unordered_map<std::string, SymbolEntry>& symbolTable) {
    for (const auto& pair : symbolTable) {
        const std::string& label = pair.first;
        const SymbolEntry& entry = pair.second;
        
        if (entry.address >= DATA_SEGMENT_START) {
            uint32_t addr = entry.address;
            
            if (entry.isString) {
                for (char c : entry.stringValue) {
                    machineCode.push_back(std::make_pair(addr, static_cast<uint32_t>(c)));
                    addr++;
                }
                machineCode.push_back(std::make_pair(addr, 0));
                addr++;
                while (addr % 4 != 0) {
                    machineCode.push_back(std::make_pair(addr, 0));
                    addr++;
                }
            } else {
                for (uint64_t value : entry.numericValues) {
                    if (addr % 4 != 0) {
                        addr = (addr + 3) & ~3;
                    }
                    machineCode.push_back(std::make_pair(addr, static_cast<uint32_t>(value)));
                    addr += 4;
                }
            }
        }
    }
    
    std::sort(machineCode.begin(), machineCode.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
}

uint32_t Assembler::generateRType(const std::string& opcode, const std::vector<std::string>& operands) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t rs1 = getRegisterNumber(operands[1]);
    int32_t rs2 = getRegisterNumber(operands[2]);

    if (rd < 0 || rs1 < 0 || rs2 < 0) {
        throw std::runtime_error("Invalid register in R-type instruction");
    }

    return (opcodeInfo.funct7 << 25) | 
           (rs2 << 20) | 
           (rs1 << 15) | 
           (opcodeInfo.funct3 << 12) | 
           (rd << 7) | 
           opcodeInfo.opcode;
}

uint32_t Assembler::generateIType(const std::string& opcode, const std::vector<std::string>& operands) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t rs1 = getRegisterNumber(operands[1]);
    int32_t imm = std::stoi(operands[2]);

    if (rd < 0 || rs1 < 0) {
        throw std::runtime_error("Invalid register in I-type instruction");
    }

    if (imm < -2048 || imm > 2047) {
        throw std::runtime_error("Immediate value out of range for I-type instruction");
    }

    return ((imm & 0xFFF) << 20) |
           (rs1 << 15) |
           (opcodeInfo.funct3 << 12) |
           (rd << 7) |
           opcodeInfo.opcode;
}

uint32_t Assembler::generateSType(const std::string& opcode, const std::vector<std::string>& operands) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rs2 = getRegisterNumber(operands[0]);
    int32_t rs1 = getRegisterNumber(operands[2]);
    int32_t imm = std::stoi(operands[1]);

    if (rs1 < 0 || rs2 < 0) {
        throw std::runtime_error("Invalid register in S-type instruction");
    }

    if (imm < -2048 || imm > 2047) {
        throw std::runtime_error("Immediate value out of range for S-type instruction");
    }

    uint32_t imm11_5 = (imm >> 5) & 0x7F;
    uint32_t imm4_0 = imm & 0x1F;

    return (imm11_5 << 25) |
           (rs2 << 20) |
           (rs1 << 15) |
           (opcodeInfo.funct3 << 12) |
           (imm4_0 << 7) |
           opcodeInfo.opcode;
}

uint32_t Assembler::generateSBType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rs1 = getRegisterNumber(operands[0]);
    int32_t rs2 = getRegisterNumber(operands[1]);
    uint32_t targetAddress = std::stoul(operands[2], nullptr, 0);
    int32_t offset = calculateRelativeOffset(currentAddress, targetAddress);

    if (rs1 < 0 || rs2 < 0) {
        throw std::runtime_error("Invalid register in SB-type instruction");
    }

    if (offset < -4096 || offset > 4095 || (offset & 1)) {
        throw std::runtime_error("Invalid branch offset");
    }

    uint32_t imm12 = (offset >> 12) & 0x1;
    uint32_t imm11 = (offset >> 11) & 0x1;
    uint32_t imm10_5 = (offset >> 5) & 0x3F;
    uint32_t imm4_1 = (offset >> 1) & 0xF;

    return (imm12 << 31) |
           (imm11 << 7) |
           (imm10_5 << 25) |
           (rs2 << 20) |
           (rs1 << 15) |
           (opcodeInfo.funct3 << 12) |
           (imm4_1 << 8) |
           opcodeInfo.opcode;
}

uint32_t Assembler::generateUType(const std::string& opcode, const std::vector<std::string>& operands) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t imm = std::stoi(operands[1]);

    if (rd < 0) {
        throw std::runtime_error("Invalid register in U-type instruction");
    }

    return (imm & 0xFFFFF000) |
           (rd << 7) |
           opcodeInfo.opcode;
}

uint32_t Assembler::generateUJType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    uint32_t targetAddress = std::stoul(operands[1], nullptr, 0);
    int32_t offset = calculateRelativeOffset(currentAddress, targetAddress);

    if (rd < 0) {
        throw std::runtime_error("Invalid register in UJ-type instruction");
    }

    if (offset < -1048576 || offset > 1048575 || (offset & 1)) {
        throw std::runtime_error("Invalid jump offset (must be even and within ±1MiB)");
    }

    uint32_t imm20 = (offset >> 20) & 0x1;
    uint32_t imm10_1 = (offset >> 1) & 0x3FF;
    uint32_t imm11 = (offset >> 11) & 0x1;
    uint32_t imm19_12 = (offset >> 12) & 0xFF;

    return (imm20 << 31) |
           (imm10_1 << 21) |
           (imm11 << 20) |
           (imm19_12 << 12) |
           (rd << 7) |
           opcodeInfo.opcode;
}

int32_t Assembler::getRegisterNumber(const std::string& reg) const {
    if (reg[0] == 'x') {
        try {
            int num = std::stoi(reg.substr(1));
            if (num >= 0 && num <= 31) {
                return num;
            }
        } catch (...) {}
    }
    
    for (int i = 0; i <= 31; i++) {
        if (riscv::validRegisters.find(reg) != riscv::validRegisters.end()) {
            return i;
        }
    }
    
    return -1;
}

bool Assembler::writeToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile) {
        reportError("Could not open output file: " + filename);
        return false;
    }

    std::vector<std::pair<uint32_t, uint32_t>> sortedCode = machineCode;
    std::sort(sortedCode.begin(), sortedCode.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    outFile << std::hex << std::uppercase;
    outFile.fill('0');

    const auto& symbolTable = parser.getSymbolTable();

    outFile << "################    DATA    #################\n";
    bool hasDataSegment = false;
    for (const auto& pair : sortedCode) {
        if (pair.first >= DATA_SEGMENT_START) {
            hasDataSegment = true;
            outFile << "0x" << std::setw(8) << pair.first << " 0x" << std::setw(8) << pair.second;
            
            bool isStringData = false;
            for (const auto& symbol : symbolTable) {
                const auto& entry = symbol.second;
                if (pair.first >= entry.address && 
                    pair.first < entry.address + (entry.isString ? entry.stringValue.length() + 1 : entry.numericValues.size() * 4)) {
                    isStringData = entry.isString;
                    break;
                }
            }

            if (isStringData) {
                if (pair.second == 0) {
                    outFile << "    # Null terminator";
                } else {
                    outFile << "    # ASCII: '" << static_cast<char>(pair.second) << "'";
                }
            } else {
                outFile << "    # Decimal value: " << std::dec << pair.second << std::hex;
            }
            outFile << "\n";
        }
    }
    if (hasDataSegment) outFile << "\n";

    outFile << "################    TEXT    #################\n";
    for (const auto& pair : sortedCode) {
        if (pair.first < DATA_SEGMENT_START) {
            uint32_t addr = pair.first;
            uint32_t instruction = pair.second;

            outFile << "0x" << std::setw(8) << addr << " 0x" << std::setw(8) << instruction;

            if (instruction == 0xDEADBEEF) {
                outFile << "    # [TEXT_SEGMENT_END]   | Binary: ";
            } else {
                uint32_t opcode = instruction & 0x7F;
                uint32_t rd = (instruction >> 7) & 0x1F;
                uint32_t funct3 = (instruction >> 12) & 0x7;
                uint32_t rs1 = (instruction >> 15) & 0x1F;
                uint32_t rs2 = (instruction >> 20) & 0x1F;
                uint32_t funct7 = (instruction >> 25) & 0x7F;

                std::string instType;
                if (opcode == 0b0110011) {
                    instType = "R-type: ";
                } else if (opcode == 0b0010011 || opcode == 0b0000011) {
                    instType = "I-type: ";
                } else if (opcode == 0b0100011) {
                    instType = "S-type: ";
                } else if (opcode == 0b1100011) {
                    instType = "SB-type: ";
                } else if (opcode == 0b0110111 || opcode == 0b0010111) {
                    instType = "U-type: ";
                } else if (opcode == 0b1101111) {
                    instType = "UJ-type: ";
                } else if (instruction == 0x00000073) {
                    instType = "System: ";
                }

                outFile << "    # " << instType;
            }

            std::bitset<32> bits(instruction);
            std::string binStr = bits.to_string();
            outFile << binStr.substr(0, 7) << "-"
                   << binStr.substr(7, 5) << "-"
                   << binStr.substr(12, 3) << "-"
                   << binStr.substr(15, 5) << "-"
                   << binStr.substr(20, 5) << "-"
                   << binStr.substr(25, 7);
            outFile << "\n";
        }
    }
    outFile << "\n";

    return true;
}

void Assembler::reportError(const std::string& message) const {
    std::cerr << "Assembler Error: " << message << "\n";
    ++errorCount;
}

uint32_t Assembler::calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const {
    // Calculate PC-relative offset in bytes
    // For branches and jumps, PC is the address of the instruction
    int32_t offset = static_cast<int32_t>(targetAddress - currentAddress);
    return offset;
}

#endif