#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <bitset>
#include "parser.hpp"

class Assembler {
public:
    struct MachineCode {
        uint32_t binary;         // 32-bit machine code
        std::string assembly;    // Original assembly string
        int address;             // Memory address
        int lineNumber;          // Source line number
    };

    static std::vector<MachineCode> assemble(const std::vector<Parser::Instruction>& instructions);
    static std::string binaryBreakdown(uint32_t binary, const std::string& format);
    static const std::unordered_map<std::string, std::string> instructionFormats;

private:
    static uint32_t encodeInstruction(const Parser::Instruction& inst, int instructionCount);
    static uint32_t encodeRType(const Parser::Instruction& inst);
    static uint32_t encodeIType(const Parser::Instruction& inst);
    static uint32_t encodeSType(const Parser::Instruction& inst);
    static uint32_t encodeBType(const Parser::Instruction& inst, int instructionCount);
    static uint32_t encodeUType(const Parser::Instruction& inst);
    static uint32_t encodeJType(const Parser::Instruction& inst, int instructionCount);
    static uint32_t encodeStandalone(const Parser::Instruction& inst);
    
    static int getRegisterNumber(const std::string& reg);
    static int getImmediate(const std::string& imm);
    static std::string instructionToString(const Parser::Instruction& inst);
    // static std::string binaryBreakdown(uint32_t binary, const std::string& format);

    
    static const std::unordered_map<std::string, uint32_t> opcodeMap;
    static const std::unordered_map<std::string, uint32_t> funct3Map;
    static const std::unordered_map<std::string, uint32_t> funct7Map;
    static std::unordered_map<std::string, int> labelPositions;
};

const std::unordered_map<std::string, std::string> Assembler::instructionFormats = {
    {"add", "R"}, {"sub", "R"}, {"and", "R"}, {"or", "R"}, {"xor", "R"},
    {"addi", "I"}, {"andi", "I"}, {"ori", "I"}, {"lb", "I"}, {"lh", "I"}, 
    {"lw", "I"}, {"jalr", "I"},
    {"sb", "S"}, {"sh", "S"}, {"sw", "S"},
    {"beq", "B"}, {"bne", "B"}, {"blt", "B"}, {"bge", "B"},
    {"lui", "U"}, {"auipc", "U"},
    {"jal", "J"},
    {"ecall", "Standalone"}
};

const std::unordered_map<std::string, uint32_t> Assembler::opcodeMap = {
    {"add", 0x33}, {"sub", 0x33}, {"and", 0x33}, {"or", 0x33}, {"xor", 0x33},
    {"addi", 0x13}, {"andi", 0x13}, {"ori", 0x13}, {"lb", 0x03}, {"lh", 0x03}, 
    {"lw", 0x03}, {"jalr", 0x67},
    {"sb", 0x23}, {"sh", 0x23}, {"sw", 0x23},
    {"beq", 0x63}, {"bne", 0x63}, {"blt", 0x63}, {"bge", 0x63},
    {"lui", 0x37}, {"auipc", 0x17},
    {"jal", 0x6F},
    {"ecall", 0x73}
};

const std::unordered_map<std::string, uint32_t> Assembler::funct3Map = {
    {"add", 0x0}, {"sub", 0x0}, {"and", 0x7}, {"or", 0x6}, {"xor", 0x4},
    {"addi", 0x0}, {"andi", 0x7}, {"ori", 0x6}, {"lb", 0x0}, {"lh", 0x1}, 
    {"lw", 0x2}, {"jalr", 0x0},
    {"sb", 0x0}, {"sh", 0x1}, {"sw", 0x2},
    {"beq", 0x0}, {"bne", 0x1}, {"blt", 0x4}, {"bge", 0x5}
};

const std::unordered_map<std::string, uint32_t> Assembler::funct7Map = {
    {"add", 0x00}, {"sub", 0x20}, {"and", 0x00}, {"or", 0x00}, {"xor", 0x00}
};

std::unordered_map<std::string, int> Assembler::labelPositions;

std::vector<Assembler::MachineCode> Assembler::assemble(const std::vector<Parser::Instruction>& instructions) {
    std::vector<MachineCode> machineCodes;
    labelPositions = Parser::labelPositions; // Accessible via friend declaration in Parser

    int instructionCount = 0;
    for (const auto& inst : instructions) {
        if (inst.isDirective) continue;

        MachineCode mc;
        mc.binary = encodeInstruction(inst, instructionCount);
        mc.assembly = instructionToString(inst);
        mc.address = instructionCount * 4;
        mc.lineNumber = inst.lineNumber;
        machineCodes.push_back(mc);
        instructionCount++;
    }

    return machineCodes;
}

uint32_t Assembler::encodeInstruction(const Parser::Instruction& inst, int instructionCount) {
    auto formatIt = instructionFormats.find(inst.opcode);
    if (formatIt == instructionFormats.end()) {
        throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + ": Unknown instruction '" + inst.opcode + "'");
    }

    std::string format = formatIt->second;
    if (format == "R") return encodeRType(inst);
    if (format == "I") return encodeIType(inst);
    if (format == "S") return encodeSType(inst);
    if (format == "B") return encodeBType(inst, instructionCount);
    if (format == "U") return encodeUType(inst);
    if (format == "J") return encodeJType(inst, instructionCount);
    if (format == "Standalone") return encodeStandalone(inst);
    
    throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + ": Unsupported format for '" + inst.opcode + "'");
}

int Assembler::getRegisterNumber(const std::string& reg) {
    if (reg[0] == 'x') return std::stoi(reg.substr(1));
    static const std::unordered_map<std::string, int> aliases = {
        {"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
        {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"s1", 9},
        {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14},
        {"a5", 15}, {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19},
        {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24},
        {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29},
        {"t5", 30}, {"t6", 31}
    };
    auto it = aliases.find(reg);
    if (it == aliases.end()) throw std::runtime_error("Invalid register: " + reg);
    return it->second;
}

int Assembler::getImmediate(const std::string& imm) {
    if (imm.substr(0, 2) == "0x") return std::stoi(imm, nullptr, 16);
    return std::stoi(imm);
}

uint32_t Assembler::encodeRType(const Parser::Instruction& inst) {
    int rd = getRegisterNumber(inst.operands[0]);
    int rs1 = getRegisterNumber(inst.operands[1]);
    int rs2 = getRegisterNumber(inst.operands[2]);
    
    uint32_t instBinary = 0;
    instBinary |= opcodeMap.at(inst.opcode);
    instBinary |= (rd & 0x1F) << 7;
    instBinary |= (funct3Map.at(inst.opcode) & 0x7) << 12;
    instBinary |= (rs1 & 0x1F) << 15;
    instBinary |= (rs2 & 0x1F) << 20;
    instBinary |= (funct7Map.at(inst.opcode) & 0x7F) << 25;
    return instBinary;
}

uint32_t Assembler::encodeIType(const Parser::Instruction& inst) {
    int rd = getRegisterNumber(inst.operands[0]);
    int rs1 = getRegisterNumber(inst.operands[1]);
    int imm = getImmediate(inst.operands[2]);
    
    uint32_t instBinary = 0;
    instBinary |= opcodeMap.at(inst.opcode);
    instBinary |= (rd & 0x1F) << 7;
    instBinary |= (funct3Map.at(inst.opcode) & 0x7) << 12;
    instBinary |= (rs1 & 0x1F) << 15;
    instBinary |= (imm & 0xFFF) << 20;
    return instBinary;
}

uint32_t Assembler::encodeSType(const Parser::Instruction& inst) {
    int rs2 = getRegisterNumber(inst.operands[0]);
    int rs1 = getRegisterNumber(inst.operands[1]);
    int imm = getImmediate(inst.operands[2]);
    
    uint32_t instBinary = 0;
    instBinary |= opcodeMap.at(inst.opcode);
    instBinary |= (imm & 0x1F) << 7;
    instBinary |= (funct3Map.at(inst.opcode) & 0x7) << 12;
    instBinary |= (rs1 & 0x1F) << 15;
    instBinary |= (rs2 & 0x1F) << 20;
    instBinary |= ((imm >> 5) & 0x7F) << 25;
    return instBinary;
}

uint32_t Assembler::encodeBType(const Parser::Instruction& inst, int instructionCount) {
    int rs1 = getRegisterNumber(inst.operands[0]);
    int rs2 = getRegisterNumber(inst.operands[1]);
    int target = labelPositions.at(inst.operands[2]);
    int imm = (target - instructionCount) * 4;
    
    uint32_t instBinary = 0;
    instBinary |= opcodeMap.at(inst.opcode);
    instBinary |= ((imm >> 11) & 0x1) << 7;
    instBinary |= ((imm >> 1) & 0xF) << 8;
    instBinary |= (funct3Map.at(inst.opcode) & 0x7) << 12;
    instBinary |= (rs1 & 0x1F) << 15;
    instBinary |= (rs2 & 0x1F) << 20;
    instBinary |= ((imm >> 5) & 0x3F) << 25;
    instBinary |= ((imm >> 12) & 0x1) << 31;
    return instBinary;
}

uint32_t Assembler::encodeUType(const Parser::Instruction& inst) {
    int rd = getRegisterNumber(inst.operands[0]);
    int imm = getImmediate(inst.operands[1]);
    
    uint32_t instBinary = 0;
    instBinary |= opcodeMap.at(inst.opcode);
    instBinary |= (rd & 0x1F) << 7;
    instBinary |= (imm & 0xFFFFF) << 12;
    return instBinary;
}

uint32_t Assembler::encodeJType(const Parser::Instruction& inst, int instructionCount) {
    int rd = getRegisterNumber(inst.operands[0]);
    int target = labelPositions.at(inst.operands[1]);
    int imm = (target - instructionCount) * 4;
    
    uint32_t instBinary = 0;
    instBinary |= opcodeMap.at(inst.opcode);
    instBinary |= (rd & 0x1F) << 7;
    instBinary |= ((imm >> 12) & 0xFF) << 12;
    instBinary |= ((imm >> 11) & 0x1) << 20;
    instBinary |= ((imm >> 1) & 0x3FF) << 21;
    instBinary |= ((imm >> 20) & 0x1) << 31;
    return instBinary;
}

uint32_t Assembler::encodeStandalone(const Parser::Instruction& inst) {
    if (inst.opcode == "ecall") return 0x00000073;
    throw std::runtime_error("Line " + std::to_string(inst.lineNumber) + ": Unknown standalone instruction '" + inst.opcode + "'");
}

std::string Assembler::instructionToString(const Parser::Instruction& inst) {
    std::stringstream ss;
    ss << inst.opcode;
    for (const auto& op : inst.operands) {
        ss << "," << op; // Match your requested output format with commas
    }
    return ss.str();
}

std::string Assembler::binaryBreakdown(uint32_t binary, const std::string& format) {
    std::stringstream ss;
    ss << std::bitset<7>(binary & 0x7F) << "-"           // opcode
       << std::bitset<3>((binary >> 12) & 0x7) << "-";    // funct3
    
    if (format == "R") {
        ss << std::bitset<7>((binary >> 25) & 0x7F) << "-" // funct7
           << std::bitset<5>((binary >> 7) & 0x1F) << "-"  // rd
           << std::bitset<5>((binary >> 15) & 0x1F) << "-" // rs1
           << std::bitset<5>((binary >> 20) & 0x1F) << "-NULL"; // rs2
    } else if (format == "I") {
        ss << "NULL-" << std::bitset<5>((binary >> 7) & 0x1F) << "-" // rd
           << std::bitset<5>((binary >> 15) & 0x1F) << "-"           // rs1
           << std::bitset<12>((binary >> 20) & 0xFFF);               // imm
    } else if (format == "S") {
        ss << std::bitset<7>((binary >> 25) & 0x7F) << "-" // imm[11:5]
           << std::bitset<5>((binary >> 7) & 0x1F) << "-"  // imm[4:0]
           << std::bitset<5>((binary >> 15) & 0x1F) << "-" // rs1
           << std::bitset<5>((binary >> 20) & 0x1F);       // rs2
    } else if (format == "B") {
        ss << std::bitset<7>((binary >> 25) & 0x7F) << "-" // imm[12|10:5]
           << std::bitset<5>((binary >> 7) & 0x1F) << "-"  // imm[4:1|11]
           << std::bitset<5>((binary >> 15) & 0x1F) << "-" // rs1
           << std::bitset<5>((binary >> 20) & 0x1F);       // rs2
    } else if (format == "U") {
        ss << std::bitset<20>((binary >> 12) & 0xFFFFF) << "-" // imm
           << std::bitset<5>((binary >> 7) & 0x1F) << "-NULL-NULL"; // rd
    } else if (format == "J") {
        ss << std::bitset<20>((binary >> 12) & 0xFFFFF) << "-" // imm
           << std::bitset<5>((binary >> 7) & 0x1F) << "-NULL-NULL"; // rd
    } else if (format == "Standalone") {
        ss << "NULL-NULL-NULL-NULL-NULL-NULL";
    } else {
        ss << "INVALID";
    }
    return ss.str();
}

#endif