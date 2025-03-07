#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <bitset>
#include <sstream>
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
    bool generateIType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    uint32_t generateSType(const std::string& opcode, const std::vector<std::string>& operands);
    uint32_t generateSBType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    bool generateUType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    uint32_t generateUJType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress);
    uint32_t generateStandalone(const std::string& opcode);
    
    int32_t getRegisterNumber(const std::string& reg) const;
    int32_t parseImmediate(const std::string& imm) const;
    void reportError(const std::string& message) const;
    uint32_t calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const;
    std::string decodeInstruction(uint32_t instruction) const;

    OpcodeInfo getOpcodeInfo(const std::string& opcode) const;
    void processTextSegment(const std::vector<ParsedInstruction>& instructions);
    void processDataSegment(const std::unordered_map<std::string, SymbolEntry>& symbolTable);
    void formatInstruction(std::ofstream& outFile, uint32_t addr, uint32_t instruction) const;
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
    else if (opcode == "slli")  { info = {0b0010011, 0b001, 0b0000000}; }
    else if (opcode == "srli")  { info = {0b0010011, 0b101, 0b0000000}; }
    else if (opcode == "srai")  { info = {0b0010011, 0b101, 0b0100000}; }
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
                machineCode.push_back({currentAddress, machineInstruction});
            }
            else if (riscv::ITypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                if (!generateIType(inst.opcode, inst.operands, currentAddress)) {
                    continue;
                }
            }
            else if (riscv::STypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateSType(inst.opcode, inst.operands);
                machineCode.push_back({currentAddress, machineInstruction});
            }
            else if (riscv::SBTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateSBType(inst.opcode, inst.operands, currentAddress);
                machineCode.push_back({currentAddress, machineInstruction});
            }
            else if (riscv::UTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                if (!generateUType(inst.opcode, inst.operands, currentAddress)) {
                    continue;
                }
            }
            else if (riscv::UJTypeInstructions::getEncoding().opcodeMap.count(inst.opcode)) {
                machineInstruction = generateUJType(inst.opcode, inst.operands, currentAddress);
                machineCode.push_back({currentAddress, machineInstruction});
            }
            else {
                reportError("Unknown instruction type for opcode: " + inst.opcode);
                continue;
            }
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
    if (rd > 31 || rs1 > 31 || rs2 > 31) {
        throw std::runtime_error("Register number out of range (0-31)");
    }

    return (opcodeInfo.funct7 << 25) | 
           (rs2 << 20) | 
           (rs1 << 15) | 
           (opcodeInfo.funct3 << 12) | 
           (rd << 7) | 
           opcodeInfo.opcode;
}

bool Assembler::generateIType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress) {
    if (operands.size() != 3) {
        reportError("I-type instruction requires 3 operands");
        return false;
    }

    int32_t rd = parser.getRegisterNumber(operands[0]);
    int32_t rs1;
    int32_t imm;
    if (opcode == "lb" || opcode == "lh" || opcode == "lw" || opcode == "lbu" || opcode == "lhu") {
        std::string offset, baseReg;
        if (Lexer::isMemory(operands[1], offset, baseReg)) {
            rs1 = parser.getRegisterNumber(baseReg);
            imm = parser.parseImmediate(offset);
        } else {
            imm = parser.parseImmediate(operands[1]);
            rs1 = parser.getRegisterNumber(operands[2]);
        }
    } else {
        rs1 = parser.getRegisterNumber(operands[1]);
        imm = parser.parseImmediate(operands[2]);
    }

    if (rd < 0 || rs1 < 0) {
        reportError("Invalid register in I-type instruction");
        return false;
    }

    if (opcode == "slli" || opcode == "srli" || opcode == "srai") {
        if (imm < 0 || imm > 31) {
            reportError("Shift amount must be between 0 and 31");
            return false;
        }
        imm &= 0x1F;
    } else {
        if (imm < -2048 || imm > 2047) {
            reportError("Immediate value out of range for I-type instruction (-2048 to 2047)");
            return false;
        }
    }

    uint32_t encoding = riscv::ITypeInstructions::getEncoding().opcodeMap.at(opcode);
    uint32_t funct3 = riscv::ITypeInstructions::getEncoding().func3Map.at(opcode);
    uint32_t funct7 = 0;

    if (opcode == "srai") {
        funct7 = riscv::ITypeInstructions::getEncoding().func7Map.at(opcode);
    }

    uint32_t instruction = (funct7 << 25) |
                          ((imm & 0xFFF) << 20) |
                          (rs1 << 15) |
                          (funct3 << 12) |
                          (rd << 7) |
                          encoding;

    machineCode.push_back(std::make_pair(currentAddress, instruction));
    return true;
}

uint32_t Assembler::generateSType(const std::string& opcode, const std::vector<std::string>& operands) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rs2 = getRegisterNumber(operands[0]);
    int32_t rs1;
    int32_t imm;
    
    if (operands.size() == 2) {
        std::string offset, reg;
        if (!Lexer::isMemory(operands[1], offset, reg)) {
            throw std::runtime_error("Invalid memory operand format");
        }
        imm = parseImmediate(offset);
        rs1 = getRegisterNumber(reg);
    } else if (operands.size() == 3) {
        imm = parseImmediate(operands[1]);
        rs1 = getRegisterNumber(operands[2]);
    } else {
        throw std::runtime_error("Invalid number of operands for S-type instruction");
    }

    if (rs1 < 0 || rs2 < 0) {
        throw std::runtime_error("Invalid register in S-type instruction");
    }
    if (rs1 > 31 || rs2 > 31) {
        throw std::runtime_error("Register number out of range (0-31)");
    }
    if (imm < -2048 || imm > 2047) {
        throw std::runtime_error("Immediate value out of range for S-type instruction (-2048 to 2047)");
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
    int32_t offset;
    
    try {
        if (operands[2].find("0x") == 0 || operands[2].find("0b") == 0 || 
            std::all_of(operands[2].begin(), operands[2].end(), ::isdigit) ||
            operands[2][0] == '-') {
            offset = parseImmediate(operands[2]);
        } else {
            uint32_t targetAddress = std::stoul(operands[2], nullptr, 0);
            offset = calculateRelativeOffset(currentAddress, targetAddress);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid branch target: " + operands[2]);
    }

    if (rs1 < 0 || rs2 < 0) {
        throw std::runtime_error("Invalid register in SB-type instruction");
    }
    if (rs1 > 31 || rs2 > 31) {
        throw std::runtime_error("Register number out of range (0-31)");
    }
    if (offset < -4096 || offset > 4095) {
        throw std::runtime_error("Branch offset out of range (-4096 to 4095)");
    }
    if (offset & 1) {
        throw std::runtime_error("Branch offset must be even");
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

bool Assembler::generateUType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress) {
    if (operands.size() != 2) {
        reportError("U-type instruction requires 2 operands");
        return false;
    }

    int32_t rd = parser.getRegisterNumber(operands[0]);
    if (rd < 0) {
        reportError("Invalid destination register in U-type instruction");
        return false;
    }

    int32_t imm = parser.parseImmediate(operands[1]);
    if (imm < 0 || imm > 0xFFFFF) {
        reportError("Immediate value out of range for U-type instruction (0 to 0xFFFFF)");
        return false;
    }

    uint32_t encoding = riscv::UTypeInstructions::getEncoding().opcodeMap.at(opcode);
    uint32_t instruction = ((imm & 0xFFFFF) << 12) |
                          (rd << 7) |
                          encoding;

    machineCode.push_back(std::make_pair(currentAddress, instruction));
    return true;
}

uint32_t Assembler::generateUJType(const std::string& opcode, const std::vector<std::string>& operands, uint32_t currentAddress) {
    auto opcodeInfo = getOpcodeInfo(opcode);
    
    int32_t rd = getRegisterNumber(operands[0]);
    if (rd < 0) {
        throw std::runtime_error("Invalid destination register in UJ-type instruction");
    }
    if (rd > 31) {
        throw std::runtime_error("Register number out of range (0-31)");
    }

    int32_t offset;
    try {
        if (operands[1].find("0x") == 0 || operands[1].find("0b") == 0 || 
            std::all_of(operands[1].begin(), operands[1].end(), ::isdigit) ||
            operands[1][0] == '-') {
            offset = parseImmediate(operands[1]);
        } else {
            uint32_t targetAddress = std::stoul(operands[1], nullptr, 0);
            offset = calculateRelativeOffset(currentAddress, targetAddress);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid jump target: " + operands[1]);
    }
    if (offset < -1048576 || offset > 1048575) {
        throw std::runtime_error("Jump offset out of range (-1M to +1M)");
    }
    if (offset & 1) {
        throw std::runtime_error("Jump offset must be even");
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
    if (reg.empty()) {
        return -1;
    }

    std::string cleanReg = reg;
    cleanReg.erase(std::remove_if(cleanReg.begin(), cleanReg.end(), ::isspace), cleanReg.end());
    std::transform(cleanReg.begin(), cleanReg.end(), cleanReg.begin(), ::tolower);
    auto it = riscv::validRegisters.find(cleanReg);
    if (it != riscv::validRegisters.end()) {
        return it->second;
    }
    if (cleanReg == "zero" || cleanReg == "x0") {
        return 0;
    }
    reportError("Invalid register name: " + reg);
    return -1;
}

bool Assembler::writeToFile(const std::string& filename) {
    try {
        std::ofstream outFile(filename);
        if (!outFile) {
            throw std::runtime_error("Could not open output file: " + filename);
        }

        std::vector<std::pair<uint32_t, uint32_t>> sortedCode = machineCode;
        std::sort(sortedCode.begin(), sortedCode.end(), 
                 [](const auto& a, const auto& b) { return a.first < b.first; });

        outFile << std::hex << std::uppercase;
        outFile.fill('0');

        for (const auto& pair : sortedCode) {
            if (pair.first < TEXT_SEGMENT_START) {
                std::stringstream ss;
                ss << std::hex << "Invalid address (0x" << pair.first 
                   << ") below text segment start (0x" << TEXT_SEGMENT_START << ")";
                throw std::runtime_error(ss.str());
            }
            
            if (pair.first < DATA_SEGMENT_START) {
                if (pair.first >= DATA_SEGMENT_START) {
                    std::stringstream ss;
                    ss << std::hex << "Invalid text segment address (0x" << pair.first 
                       << ") exceeds text segment boundary (0x" << DATA_SEGMENT_START << ")";
                    throw std::runtime_error(ss.str());
                }
            } else if (pair.first >= DATA_SEGMENT_START && pair.first < HEAP_SEGMENT_START) {
                if (pair.first >= HEAP_SEGMENT_START) {
                    std::stringstream ss;
                    ss << std::hex << "Invalid data segment address (0x" << pair.first 
                       << ") exceeds data segment boundary (0x" << HEAP_SEGMENT_START << ")";
                    throw std::runtime_error(ss.str());
                }
            }
        }

        outFile << "################    DATA    #################\n";
        bool hasDataSegment = false;
        for (const auto& pair : sortedCode) {
            if (pair.first >= DATA_SEGMENT_START && pair.first < HEAP_SEGMENT_START) {
                hasDataSegment = true;
                outFile << "0x" << std::setw(8) << pair.first << " 0x" << std::setw(8) << pair.second;
                outFile << "\n";
            }
        }
        if (hasDataSegment) outFile << "\n";
        outFile << "################    TEXT    #################\n";
        for (const auto& pair : sortedCode) {
            if (pair.first < DATA_SEGMENT_START) {
                formatInstruction(outFile, pair.first, pair.second);
            }
        }
        outFile << "\n";
        return true;
    } catch (const std::exception& e) {
        reportError("File writing error: " + std::string(e.what()));
        return false;
    }
}

std::string Assembler::decodeInstruction(uint32_t instruction) const {
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    
    std::stringstream ss;

    if (opcode == 0b0110011) {
        if (funct3 == 0b000 && funct7 == 0b0000000) ss << "add";
        else if (funct3 == 0b000 && funct7 == 0b0100000) ss << "sub";
        else if (funct3 == 0b001 && funct7 == 0b0000000) ss << "sll";
        else if (funct3 == 0b010 && funct7 == 0b0000000) ss << "slt";
        else if (funct3 == 0b011 && funct7 == 0b0000000) ss << "sltu";
        else if (funct3 == 0b100 && funct7 == 0b0000000) ss << "xor";
        else if (funct3 == 0b101 && funct7 == 0b0000000) ss << "srl";
        else if (funct3 == 0b101 && funct7 == 0b0100000) ss << "sra";
        else if (funct3 == 0b110 && funct7 == 0b0000000) ss << "or";
        else if (funct3 == 0b111 && funct7 == 0b0000000) ss << "and";
        ss << " x" << rd << ",x" << rs1 << ",x" << rs2;
    }
    else if (opcode == 0b0010011 || opcode == 0b0000011) {
        int32_t imm = (instruction >> 20);
        if (opcode == 0b0010011) {
            if (funct3 == 0b000) ss << "addi";
            else if (funct3 == 0b001) ss << "slli";
            else if (funct3 == 0b010) ss << "slti";
            else if (funct3 == 0b011) ss << "sltiu";
            else if (funct3 == 0b100) ss << "xori";
            else if (funct3 == 0b101) {
                if ((imm >> 5) & 0x7F) ss << "srai";
                else ss << "srli";
            }
            else if (funct3 == 0b110) ss << "ori";
            else if (funct3 == 0b111) ss << "andi";
            if (funct3 == 0b001 || funct3 == 0b101) {
                ss << " x" << rd << ",x" << rs1 << "," << (imm & 0x1F);
            } else {
                ss << " x" << rd << ",x" << rs1 << "," << imm;
            }
        } else {
            if (funct3 == 0b000) ss << "lb";
            else if (funct3 == 0b001) ss << "lh";
            else if (funct3 == 0b010) ss << "lw";
            else if (funct3 == 0b100) ss << "lbu";
            else if (funct3 == 0b101) ss << "lhu";
            ss << " x" << rd << "," << imm << "(x" << rs1 << ")";
        }
    }
    else if (opcode == 0b0100011) {
        uint32_t imm11_5 = (instruction >> 25) & 0x7F;
        uint32_t imm4_0 = (instruction >> 7) & 0x1F;
        int32_t imm = (imm11_5 << 5) | imm4_0;
        if (funct3 == 0b000) ss << "sb";
        else if (funct3 == 0b001) ss << "sh";
        else if (funct3 == 0b010) ss << "sw";
        ss << " x" << rs2 << "," << imm << "(x" << rs1 << ")";
    }
    else if (opcode == 0b1100011) {
        uint32_t imm12 = (instruction >> 31) & 0x1;
        uint32_t imm11 = (instruction >> 7) & 0x1;
        uint32_t imm10_5 = (instruction >> 25) & 0x3F;
        uint32_t imm4_1 = (instruction >> 8) & 0xF;
        int32_t imm = (imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1);
        if (funct3 == 0b000) ss << "beq";
        else if (funct3 == 0b001) ss << "bne";
        else if (funct3 == 0b100) ss << "blt";
        else if (funct3 == 0b101) ss << "bge";
        else if (funct3 == 0b110) ss << "bltu";
        else if (funct3 == 0b111) ss << "bgeu";
        ss << " x" << rs1 << ",x" << rs2 << "," << imm;
    }
    else if (opcode == 0b0110111 || opcode == 0b0010111) {
        int32_t imm = (instruction & 0xFFFFF000);
        if (opcode == 0b0110111) ss << "lui";
        else ss << "auipc";
        ss << " x" << rd << "," << (imm >> 12);
    }
    else if (opcode == 0b1101111) {
        uint32_t imm20 = (instruction >> 31) & 0x1;
        uint32_t imm10_1 = (instruction >> 21) & 0x3FF;
        uint32_t imm11 = (instruction >> 20) & 0x1;
        uint32_t imm19_12 = (instruction >> 12) & 0xFF;
        int32_t imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
        ss << "jal x" << rd << "," << imm;
    }
    else {
        ss << "unknown";
    }

    return ss.str();
}

void Assembler::formatInstruction(std::ofstream& outFile, uint32_t addr, uint32_t instruction) const {
    try {
        if (instruction == 0xDEADBEEF) {
            outFile << "0x" << std::setw(8) << addr << " 0x" << std::setw(8) << instruction 
                   << " , [TEXT_SEGMENT_END]\n";
            return;
        }

        uint32_t opcode = instruction & 0x7F;
        uint32_t rd = (instruction >> 7) & 0x1F;
        uint32_t funct3 = (instruction >> 12) & 0x7;
        uint32_t rs1 = (instruction >> 15) & 0x1F;
        uint32_t rs2 = (instruction >> 20) & 0x1F;
        uint32_t funct7 = (instruction >> 25) & 0x7F;

        std::string instStr = decodeInstruction(instruction);
        std::string opcodeStr = std::bitset<7>(opcode).to_string();
        std::string func3Str = std::bitset<3>(funct3).to_string();
        std::string func7Str = std::bitset<7>(funct7).to_string();
        std::string rdStr = std::bitset<5>(rd).to_string();
        std::string rs1Str = std::bitset<5>(rs1).to_string();
        std::string rs2Str = std::bitset<5>(rs2).to_string();
        std::string immStr = "NULL";

        outFile << "0x" << std::setw(8) << addr << " 0x" << std::setw(8) << instruction 
               << " , " << instStr << " # " 
               << opcodeStr << "-" 
               << func3Str << "-"
               << func7Str << "-"
               << rdStr << "-"
               << rs1Str << "-"
               << rs2Str << "-"
               << immStr << "\n";
    } catch (const std::exception& e) {
        reportError("Error formatting instruction at address 0x" + 
                   std::to_string(addr) + ": " + e.what());
    }
}

void Assembler::reportError(const std::string& message) const {
    std::cerr << "\033[1;31mAssembler Error: " << message << "\033[0m\n";
    ++errorCount;
}

uint32_t Assembler::calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const {
    int32_t offset = static_cast<int32_t>(targetAddress - currentAddress);
    return offset;
}

int32_t Assembler::parseImmediate(const std::string& imm) const {
    try {
        std::string cleanImm = Lexer::trim(imm);
        if (cleanImm.empty()) {
            throw std::runtime_error("Empty immediate value");
        }

        bool isNegative = cleanImm[0] == '-';
        if (isNegative) {
            cleanImm = cleanImm.substr(1);
        }

        uint32_t value;
        if (cleanImm.length() > 2 && cleanImm[0] == '0') {
            char format = ::tolower(cleanImm[1]);
            if (format == 'x') {
                value = std::stoul(cleanImm, nullptr, 16);
            } else if (format == 'b') {
                value = std::stoul(cleanImm.substr(2), nullptr, 2);
            } else {
                value = std::stoul(cleanImm, nullptr, 10);
            }
        } else {
            value = std::stoul(cleanImm, nullptr, 10);
        }
        return isNegative ? -static_cast<int32_t>(value) : static_cast<int32_t>(value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid immediate value '" + imm + "': " + e.what());
    }
}

#endif