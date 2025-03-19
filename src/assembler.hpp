#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <bitset>
#include <sstream>
#include <map>
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
    explicit Assembler(const Parser &parser);
    bool assemble();
    bool writeToFile(const std::string &filename);
    const std::vector<std::pair<uint32_t, uint32_t>> &getMachineCode() const { return machineCode; }
    bool hasErrors() const { return errorCount > 0; }
    size_t getErrorCount() const { return errorCount; }

private:
    const Parser &parser;
    std::vector<std::pair<uint32_t, uint32_t>> machineCode;
    mutable size_t errorCount = 0;

    uint32_t generateRType(const std::string &opcode, const std::vector<std::string> &operands);
    bool generateIType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress);
    uint32_t generateSType(const std::string &opcode, const std::vector<std::string> &operands);
    uint32_t generateSBType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress);
    bool generateUType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress);
    uint32_t generateUJType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress);

    int32_t getRegisterNumber(const std::string &reg) const;
    int32_t parseImmediate(const std::string &imm) const;
    void reportError(const std::string &message) const;
    uint32_t calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const;
    std::string decodeInstruction(uint32_t instruction) const;

    OpcodeInfo getOpcodeInfo(const std::string &opcode) const;
    void processTextSegment(const std::vector<ParsedInstruction> &instructions);
    void processDataSegment(const std::unordered_map<std::string, SymbolEntry> &symbolTable);
    void formatInstruction(std::ofstream &outFile, uint32_t addr, uint32_t instruction) const;

    std::string formatSignedImmediate(int32_t value, int bitWidth) const {
        if (bitWidth <= 0 || bitWidth > 32) return "InvalidBitwidth";
        std::string binStr = std::bitset<32>(value & ((1LL << bitWidth) - 1)).to_string().substr(32 - bitWidth);
        std::stringstream hexStream;
        hexStream << std::hex << (value & ((1LL << bitWidth) - 1));
        return "0b" + binStr + " (signed: " + std::to_string(value) + ", hex: 0x" + hexStream.str() + ")";
    }
};

Assembler::Assembler(const Parser &p) : parser(p), errorCount(0) {}

OpcodeInfo Assembler::getOpcodeInfo(const std::string &opcode) const {
    if (opcode == "add")
        return {0b0110011, 0b000, 0b0000000};
    if (opcode == "sub")
        return {0b0110011, 0b000, 0b0100000};
    if (opcode == "sll")
        return {0b0110011, 0b001, 0b0000000};
    if (opcode == "slt")
        return {0b0110011, 0b010, 0b0000000};
    if (opcode == "sltu")
        return {0b0110011, 0b011, 0b0000000};
    if (opcode == "xor")
        return {0b0110011, 0b100, 0b0000000};
    if (opcode == "srl")
        return {0b0110011, 0b101, 0b0000000};
    if (opcode == "sra")
        return {0b0110011, 0b101, 0b0100000};
    if (opcode == "or")
        return {0b0110011, 0b110, 0b0000000};
    if (opcode == "and")
        return {0b0110011, 0b111, 0b0000000};
    if (opcode == "mul")
        return {0b0110011, 0b000, 0b0000001};
    if (opcode == "div")
        return {0b0110011, 0b100, 0b0000001};
    if (opcode == "rem")
        return {0b0110011, 0b110, 0b0000001};
    if (opcode == "addi")
        return {0b0010011, 0b000, 0};
    if (opcode == "slti")
        return {0b0010011, 0b010, 0};
    if (opcode == "sltiu")
        return {0b0010011, 0b011, 0};
    if (opcode == "xori")
        return {0b0010011, 0b100, 0};
    if (opcode == "ori")
        return {0b0010011, 0b110, 0};
    if (opcode == "andi")
        return {0b0010011, 0b111, 0};
    if (opcode == "slli")
        return {0b0010011, 0b001, 0b0000000};
    if (opcode == "srli")
        return {0b0010011, 0b101, 0b0000000};
    if (opcode == "srai")
        return {0b0010011, 0b101, 0b0100000};
    if (opcode == "lb")
        return {0b0000011, 0b000, 0};
    if (opcode == "lh")
        return {0b0000011, 0b001, 0};
    if (opcode == "lw")
        return {0b0000011, 0b010, 0};
    if (opcode == "lbu")
        return {0b0000011, 0b100, 0};
    if (opcode == "lhu")
        return {0b0000011, 0b101, 0};
    if (opcode == "ld")
        return {0b0000011, 0b011, 0};
    if (opcode == "sb")
        return {0b0100011, 0b000, 0};
    if (opcode == "sh")
        return {0b0100011, 0b001, 0};
    if (opcode == "sw")
        return {0b0100011, 0b010, 0};
    if (opcode == "sd")
        return {0b0100011, 0b011, 0};
    if (opcode == "beq")
        return {0b1100011, 0b000, 0};
    if (opcode == "bne")
        return {0b1100011, 0b001, 0};
    if (opcode == "blt")
        return {0b1100011, 0b100, 0};
    if (opcode == "bge")
        return {0b1100011, 0b101, 0};
    if (opcode == "bltu")
        return {0b1100011, 0b110, 0};
    if (opcode == "bgeu")
        return {0b1100011, 0b111, 0};
    if (opcode == "lui")
        return {0b0110111, 0, 0};
    if (opcode == "auipc")
        return {0b0010111, 0, 0};
    if (opcode == "jal")
        return {0b1101111, 0, 0};
    if (opcode == "jalr")
        return {0b1100111, 0b000, 0};
    return {0, 0, 0};
}

bool Assembler::assemble() {
    if (parser.hasErrors()) {
        reportError("Cannot assemble due to parser errors");
        return false;
    }
    machineCode.clear();
    processTextSegment(parser.getParsedInstructions());
    processDataSegment(parser.getSymbolTable());
    return !hasErrors();
}

void Assembler::processTextSegment(const std::vector<ParsedInstruction> &instructions) {
    uint32_t currentAddress = TEXT_SEGMENT_START;
    for (const auto &inst : instructions) {
        if (inst.opcode == "jalr") {
            generateIType(inst.opcode, inst.operands, currentAddress);
        }
        else if (inst.opcode == "mul" || inst.opcode == "div" || inst.opcode == "rem") {
            machineCode.push_back({currentAddress, generateRType(inst.opcode, inst.operands)});
        }
        else if (riscv::RTypeInstructions::getEncoding().opcodeMap.count(inst.opcode))
            machineCode.push_back({currentAddress, generateRType(inst.opcode, inst.operands)});
        else if (riscv::ITypeInstructions::getEncoding().opcodeMap.count(inst.opcode))
            generateIType(inst.opcode, inst.operands, currentAddress);
        else if (riscv::STypeInstructions::getEncoding().opcodeMap.count(inst.opcode))
            machineCode.push_back({currentAddress, generateSType(inst.opcode, inst.operands)});
        else if (riscv::SBTypeInstructions::getEncoding().opcodeMap.count(inst.opcode))
            machineCode.push_back({currentAddress, generateSBType(inst.opcode, inst.operands, currentAddress)});
        else if (riscv::UTypeInstructions::getEncoding().opcodeMap.count(inst.opcode))
            generateUType(inst.opcode, inst.operands, currentAddress);
        else if (riscv::UJTypeInstructions::getEncoding().opcodeMap.count(inst.opcode))
            machineCode.push_back({currentAddress, generateUJType(inst.opcode, inst.operands, currentAddress)});
        else {
            reportError("Unknown instruction type for opcode: " + inst.opcode);
            continue;
        }
        currentAddress += 4;
    }
}

void Assembler::processDataSegment(const std::unordered_map<std::string, SymbolEntry> &symbolTable) {
    for (const auto &pair : symbolTable) {
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
                    const auto directiveSize = parser.getDirectiveSize(entry.directive);
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

uint32_t Assembler::generateRType(const std::string &opcode, const std::vector<std::string> &operands) {
    OpcodeInfo info = getOpcodeInfo(opcode);
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t rs1 = getRegisterNumber(operands[1]);
    int32_t rs2 = getRegisterNumber(operands[2]);
    if (rd < 0 || rs1 < 0 || rs2 < 0 || rd > 31 || rs1 > 31 || rs2 > 31) throw std::runtime_error("Invalid register in R-type instruction");
    return (info.funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (info.funct3 << 12) | (rd << 7) | info.opcode;
}

bool Assembler::generateIType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    if (operands.size() != 3) {
        reportError("I-type instruction requires 3 operands");
        return false;
    }
    int32_t rd = parser.getRegisterNumber(operands[0]);
    int32_t rs1;
    int32_t imm;
    if (opcode == "lb" || opcode == "lh" || opcode == "lw" || opcode == "lbu" || opcode == "lhu" || opcode == "ld") {
        std::string offset, baseReg;
        if (Lexer::isMemory(operands[1], offset, baseReg)) {
            rs1 = parser.getRegisterNumber(baseReg);
            imm = parser.parseImmediate(offset);
        }
        else {
            imm = parser.parseImmediate(operands[1]);
            rs1 = parser.getRegisterNumber(operands[2]);
        }
    }
    else {
        rs1 = parser.getRegisterNumber(operands[1]);
        imm = parser.parseImmediate(operands[2]);
    }
    if (rd < 0 || rs1 < 0) {
        reportError("Invalid register in I-type instruction");
        return false;
    }
    if ((opcode == "slli" || opcode == "srli" || opcode == "srai") && (imm < 0 || imm > 31)) {
        reportError("Shift amount must be between 0 and 31");
        return false;
    }
    if (imm < -2048 || imm > 2047) {
        reportError("Immediate value out of range for I-type instruction (-2048 to 2047)");
        return false;
    }
    uint32_t funct7 = (opcode == "srai") ? 0b0100000 : 0;
    uint32_t instruction = (funct7 << 25) | ((imm & 0xFFF) << 20) | (rs1 << 15) | (getOpcodeInfo(opcode).funct3 << 12) | (rd << 7) | getOpcodeInfo(opcode).opcode;
    machineCode.push_back({currentAddress, instruction});
    return true;
}

uint32_t Assembler::generateSType(const std::string &opcode, const std::vector<std::string> &operands) {
    OpcodeInfo info = getOpcodeInfo(opcode);
    int32_t rs2 = getRegisterNumber(operands[0]);
    int32_t rs1, imm;
    if (operands.size() == 2) {
        std::string offset, reg;
        if (!Lexer::isMemory(operands[1], offset, reg)) throw std::runtime_error("Invalid memory operand format");
        imm = parseImmediate(offset);
        rs1 = getRegisterNumber(reg);
    }
    else if (operands.size() == 3) {
        imm = parseImmediate(operands[1]);
        rs1 = getRegisterNumber(operands[2]);
    }
    else throw std::runtime_error("Invalid number of operands for S-type instruction");
    if (rs1 < 0 || rs2 < 0 || rs1 > 31 || rs2 > 31 || imm < -2048 || imm > 2047) throw std::runtime_error("Invalid parameter in S-type instruction");
    
    uint32_t imm_11_5 = ((imm >> 5) & 0x7F) << 25;
    uint32_t imm_4_0 = (imm & 0x1F) << 7;
    
    return imm_11_5 | (rs2 << 20) | (rs1 << 15) | (info.funct3 << 12) | imm_4_0 | info.opcode;
}

uint32_t Assembler::generateSBType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    OpcodeInfo info = getOpcodeInfo(opcode);
    int32_t rs1 = getRegisterNumber(operands[0]);
    int32_t rs2 = getRegisterNumber(operands[1]);
    int32_t offset = (operands[2].find("0x") == 0 || operands[2].find("0b") == 0 ||
                      std::all_of(operands[2].begin(), operands[2].end(), ::isdigit) ||
                      operands[2][0] == '-')
                         ? parseImmediate(operands[2])
                         : calculateRelativeOffset(currentAddress, std::stoul(operands[2], nullptr, 0));
    if (rs1 < 0 || rs2 < 0 || rs1 > 31 || rs2 > 31 || offset < -4096 || offset > 4095 || offset & 1)
        throw std::runtime_error("Invalid parameter in SB-type instruction");
    return ((offset < 0 ? 1 : (offset >> 12) & 0x1) << 31) | ((offset >> 11 & 0x1) << 7) | ((offset >> 5 & 0x3F) << 25) |
           (rs2 << 20) | (rs1 << 15) | (info.funct3 << 12) | ((offset >> 1 & 0xF) << 8) | info.opcode;
}

bool Assembler::generateUType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    if (operands.size() != 2) {
        reportError("U-type instruction requires 2 operands");
        return false;
    }
    int32_t rd = parser.getRegisterNumber(operands[0]);
    int32_t imm = parser.parseImmediate(operands[1]);
    if (rd < 0 || imm < 0 || imm > 0xFFFFF) {
        reportError("Invalid parameter in U-type instruction");
        return false;
    }
    uint32_t instruction = ((imm & 0xFFFFF) << 12) | (rd << 7) | getOpcodeInfo(opcode).opcode;
    machineCode.push_back({currentAddress, instruction});
    return true;
}

uint32_t Assembler::generateUJType(const std::string &opcode, const std::vector<std::string> &operands, uint32_t currentAddress) {
    OpcodeInfo info = getOpcodeInfo(opcode);
    int32_t rd = getRegisterNumber(operands[0]);
    int32_t offset = (operands[1].find("0x") == 0 || operands[1].find("0b") == 0 ||
                      std::all_of(operands[1].begin(), operands[1].end(), ::isdigit) ||
                      operands[1][0] == '-')
                         ? parseImmediate(operands[1])
                         : calculateRelativeOffset(currentAddress, std::stoul(operands[1], nullptr, 0));
    if (rd < 0 || rd > 31 || offset < -1048576 || offset > 1048575 || offset & 1)
        throw std::runtime_error("Invalid parameter in UJ-type instruction");
    return (((offset >> 20) & 0x1) << 31) | (((offset >> 1) & 0x3FF) << 21) | (((offset >> 11) & 0x1) << 20) |
           (((offset >> 12) & 0xFF) << 12) | (rd << 7) | info.opcode;
}

int32_t Assembler::getRegisterNumber(const std::string &reg) const {
    if (reg.empty()) return -1;
    std::string cleanReg = reg;
    cleanReg.erase(std::remove_if(cleanReg.begin(), cleanReg.end(), ::isspace), cleanReg.end());
    std::transform(cleanReg.begin(), cleanReg.end(), cleanReg.begin(), ::tolower);
    auto it = riscv::validRegisters.find(cleanReg);
    if (it != riscv::validRegisters.end()) return it->second;
    if (cleanReg == "zero" || cleanReg == "x0") return 0;
    reportError("Invalid register name: " + reg);
    return -1;
}

bool Assembler::writeToFile(const std::string &filename) {
    std::ofstream outFile(filename);
    if (!outFile) {
        reportError("Could not open output file: " + filename);
        return false;
    }

    std::vector<std::pair<uint32_t, uint32_t>> sortedCode = machineCode;
    std::sort(sortedCode.begin(), sortedCode.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    outFile << "# ------------ TEXT SEGMENT ------------ #\n";
    for (const auto& [addr, inst] : machineCode) {
        if (addr < DATA_SEGMENT_START) {
            formatInstruction(outFile, addr, inst);
        }
    }

    outFile << "\n# ------------ DATA SEGMENT ------------ #\n";
    for (const auto& [addr, value] : machineCode) {
        if (addr >= DATA_SEGMENT_START && addr < HEAP_SEGMENT_START) {
            outFile << "0x" << std::setw(8) << std::setfill('0') << addr << " 0x"
                    << std::setw(8) << value << "\n";
        }
    }

    outFile.close();
    return true;
}

void Assembler::formatInstruction(std::ofstream &outFile, uint32_t addr, uint32_t instruction) const {
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;

    outFile << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr << " 0x" << std::setw(8) << instruction;


    std::string instStr;
    try {
        instStr = decodeInstruction(instruction);
    }
    catch (const std::exception &e) {
        throw std::runtime_error("Unknown instruction");
    }

    outFile << " , " << instStr << " # "
            << std::bitset<7>(opcode) << "-"
            << std::bitset<3>(funct3) << "-"
            << std::bitset<7>(funct7) << "-"
            << std::bitset<5>(rd) << "-"
            << std::bitset<5>(rs1) << "-"
            << std::bitset<5>(rs2);

    std::string immStr = "NULL";

    try {
        if (opcode == 0b1100011) {
            int32_t imm = ((instruction >> 31) & 0x1 ? -4096 : 0) |
                          ((instruction >> 7) & 0x1) << 11 |
                          ((instruction >> 25) & 0x3F) << 5 |
                          ((instruction >> 8) & 0xF) << 1;
            immStr = std::bitset<13>(imm & 0x1FFF).to_string();
        }
        else if (opcode == 0b0000011 || opcode == 0b0010011) {
            int32_t imm = (instruction >> 20 & 0xFFF);
            if (imm & 0x800) imm |= 0xFFFFF000;
            immStr = std::bitset<12>(imm & 0xFFF).to_string();
        }
        else if (opcode == 0b0100011) {
            int32_t imm = ((instruction >> 25) & 0x7F) << 5 | ((instruction >> 7) & 0x1F);
            if (imm & 0x800) imm |= 0xFFFFF000;
            immStr = std::bitset<12>(imm & 0xFFF).to_string();
        }
        else if (opcode == 0b1101111) {
            int32_t imm = ((instruction >> 31) & 0x1) << 20 |
                          ((instruction >> 12) & 0xFF) << 12 |
                          ((instruction >> 20) & 0x1) << 11 |
                          ((instruction >> 21) & 0x3FF) << 1;
            if (imm & 0x100000) imm |= 0xFFE00000;
            immStr = std::bitset<21>(imm & 0x1FFFFF).to_string();
        }
        else if (opcode == 0b0110111 || opcode == 0b0010111) {
            int32_t imm = instruction & 0xFFFFF000;
            immStr = std::bitset<20>((imm >> 12) & 0xFFFFF).to_string();
        }
    }
    catch (const std::exception &e) {
        immStr = "NULL";
    }

    if (immStr == "NULL" || (opcode == 0b0110011)) {
        immStr = "NULL";
    }
    outFile << "-" << immStr << "\n";
}

void Assembler::reportError(const std::string &message) const {
    std::cerr << "\033[1;31mAssembler Error: " << message << "\033[0m\n";
    ++errorCount;
}

uint32_t Assembler::calculateRelativeOffset(uint32_t currentAddress, uint32_t targetAddress) const {
    int32_t offset = static_cast<int32_t>(targetAddress - currentAddress);
    if (currentAddress == 0 || targetAddress == 0 || offset < -4096 || offset > 4095) throw std::runtime_error("Invalid offset calculation");
    return offset;
}

int32_t Assembler::parseImmediate(const std::string &imm) const {
    std::string cleanImm = Lexer::trim(imm);
    if (cleanImm.empty()) {
        throw std::runtime_error("Empty immediate value");
    }
    bool isNegative = cleanImm[0] == '-';
    if (isNegative) {
        cleanImm = cleanImm.substr(1);
    }
    uint32_t value = 0;
    if (cleanImm.length() > 2 && cleanImm[0] == '0') {
        char prefix = std::tolower(cleanImm[1]);
        if (prefix == 'x') {
            value = std::stoul(cleanImm, nullptr, 16);
        } else if (prefix == 'b') {
            value = std::stoul(cleanImm.substr(2), nullptr, 2);
        } else {
            value = std::stoul(cleanImm, nullptr, 10);
        }
    } else {
        value = std::stoul(cleanImm, nullptr, 10);
    }

    return isNegative ? -static_cast<int32_t>(value) : static_cast<int32_t>(value);
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
        if (funct3 == 0b000 && funct7 == 0b0000000)
            ss << "add";
        else if (funct3 == 0b000 && funct7 == 0b0100000)
            ss << "sub";
        else if (funct3 == 0b001 && funct7 == 0b0000000)
            ss << "sll";
        else if (funct3 == 0b010 && funct7 == 0b0000000)
            ss << "slt";
        else if (funct3 == 0b011 && funct7 == 0b0000000)
            ss << "sltu";
        else if (funct3 == 0b100 && funct7 == 0b0000000)
            ss << "xor";
        else if (funct3 == 0b101 && funct7 == 0b0000000)
            ss << "srl";
        else if (funct3 == 0b101 && funct7 == 0b0100000)
            ss << "sra";
        else if (funct3 == 0b110 && funct7 == 0b0000000)
            ss << "or";
        else if (funct3 == 0b111 && funct7 == 0b0000000)
            ss << "and";
        else if (funct3 == 0b000 && funct7 == 0b0000001)
            ss << "mul";
        else if (funct3 == 0b100 && funct7 == 0b0000001)
            ss << "div";
        else if (funct3 == 0b110 && funct7 == 0b0000001)
            ss << "rem";
        else
            return "unknown_r_type";
        ss << " x" << rd << ",x" << rs1 << ",x" << rs2;
    }
    else if (opcode == 0b0010011 || opcode == 0b0000011) {
        int32_t imm = instruction >> 20;
        if (opcode == 0b0010011) {
            if (funct3 == 0b000)
                ss << "addi";
            else if (funct3 == 0b001)
                ss << "slli";
            else if (funct3 == 0b010)
                ss << "slti";
            else if (funct3 == 0b011)
                ss << "sltiu";
            else if (funct3 == 0b100)
                ss << "xori";
            else if (funct3 == 0b101)
                ss << ((imm >> 5) & 0x7F ? "srai" : "srli");
            else if (funct3 == 0b110)
                ss << "ori";
            else if (funct3 == 0b111)
                ss << "andi";
            else
                return "UnknownIImmTypes";
            ss << " x" << rd << ",x" << rs1 << "," << (funct3 == 0b001 || funct3 == 0b101 ? imm & 0x1F : imm);
        }
        else {
            if (funct3 == 0b000)
                ss << "lb";
            else if (funct3 == 0b001)
                ss << "lh";
            else if (funct3 == 0b010)
                ss << "lw";
            else if (funct3 == 0b011)
                ss << "ld";
            else if (funct3 == 0b100)
                ss << "lbu";
            else if (funct3 == 0b101)
                ss << "lhu";
            else
                return "UnknownILoadTypes";
            ss << " x" << rd << "," << imm << "(x" << rs1 << ")";
        }
    }
    else if (opcode == 0b0100011) {
        int32_t imm = ((instruction >> 25) & 0x7F) << 5 | ((instruction >> 7) & 0x1F);
        if (funct3 == 0b000)
            ss << "sb";
        else if (funct3 == 0b001)
            ss << "sh";
        else if (funct3 == 0b010)
            ss << "sw";
        else if (funct3 == 0b011)
            ss << "sd";
        else
            return "UnknownSStoreTypes";
        ss << " x" << rs2 << "," << imm << "(x" << rs1 << ")";
    }
    else if (opcode == 0b1100011) {
        int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 |
                      ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
        if (funct3 == 0b000)
            ss << "beq";
        else if (funct3 == 0b001)
            ss << "bne";
        else if (funct3 == 0b100)
            ss << "blt";
        else if (funct3 == 0b101)
            ss << "bge";
        else if (funct3 == 0b110)
            ss << "bltu";
        else if (funct3 == 0b111)
            ss << "bgeu";
        else
            return "UnknownSBType";
        ss << " x" << rs1 << ",x" << rs2 << "," << imm;
    }
    else if (opcode == 0b0110111 || opcode == 0b0010111) {
        int32_t imm = instruction & 0xFFFFF000;
        ss << (opcode == 0b0110111 ? "lui" : "auipc") << " x" << rd << "," << (imm >> 12);
    }
    else if (opcode == 0b1101111) {
        int32_t imm = ((instruction >> 31) & 0x1) << 20 | ((instruction >> 12) & 0xFF) << 12 |
                      ((instruction >> 20) & 0x1) << 11 | ((instruction >> 21) & 0x3FF) << 1;
        ss << "jal x" << rd << "," << imm;
    }
    else if (opcode == 0b1100111) {
        int32_t imm = instruction >> 20;
        if (funct3 == 0b000)
            ss << "jalr x" << rd << ",x" << rs1 << "," << imm;
        else
            return "UnknownJalrType";
    }
    else ss << "UnknownOpcode_" << std::hex << opcode;
    return ss.str();
}

#endif