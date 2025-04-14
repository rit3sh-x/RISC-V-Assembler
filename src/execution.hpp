#ifndef EXECUTION_HPP
#define EXECUTION_HPP

#include <string>
#include <map>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <iomanip>
#include "types.hpp"

using namespace riscv;

inline void initialiseRegisters(uint32_t* registers) {
    std::memset(registers, 0, 32 * sizeof(uint32_t));
    registers[2] = 0x7FFFFFDC;
    registers[3] = 0x10000000;
    registers[10] = 0x00000001;
    registers[11] = 0x7FFFFFDC;
}

inline bool isValidAddress(uint32_t addr, uint32_t size) {
    if (addr + size > MEMORY_SIZE || addr + size < 0x0) {
        std::stringstream ss;
        ss << "Memory access error: Address 0x" << std::hex << addr << " with size " << std::dec << size << " is outside of valid memory range (0x0-0x" << std::hex << MEMORY_SIZE << ")";
        throw std::runtime_error(std::string(RED) + ss.str() + RESET);
    }
    return true;
}

inline InstructionType classifyInstructions(uint32_t instHex) {
    uint32_t opcode = instHex & 0x7F;
    uint32_t func3 = (instHex >> 12) & 0x7;
    uint32_t func7 = (instHex >> 25) & 0x7F;
    
    auto rTypeEncoding = RTypeInstructions::getEncoding();
    for (const auto &[name, op] : rTypeEncoding.opcodeMap) {
        if (op == opcode && rTypeEncoding.func3Map.at(name) == func3 && rTypeEncoding.func7Map.at(name) == func7) {
            return InstructionType::R;
        }
    }

    auto iTypeEncoding = ITypeInstructions::getEncoding();
    for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
        if (op == opcode && iTypeEncoding.func3Map.at(name) == func3) {
            return InstructionType::I;
        }
    }

    auto sTypeEncoding = STypeInstructions::getEncoding();
    for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
        if (op == opcode && sTypeEncoding.func3Map.at(name) == func3) {
            return InstructionType::S;
        }
    }

    auto uTypeEncoding = UTypeInstructions::getEncoding();
    for (const auto &[name, op] : uTypeEncoding.opcodeMap) {
        if (op == opcode) {
            return InstructionType::U;
        }
    }

    auto sbTypeEncoding = SBTypeInstructions::getEncoding();
    for (const auto &[name, op] : sbTypeEncoding.opcodeMap) {
        if (op == opcode && sbTypeEncoding.func3Map.at(name) == func3) {
            return InstructionType::SB;
        }
    }

    auto ujTypeEncoding = UJTypeInstructions::getEncoding();
    for (const auto &[name, op] : ujTypeEncoding.opcodeMap) {
        if (op == opcode) {
            return InstructionType::UJ;
        }
    }

    std::stringstream ss;
    ss << "Instruction 0x" << std::hex << instHex << " could not be classified: Invalid opcode (0x" << opcode << ")";
    throw std::runtime_error(std::string(RED) + ss.str() + RESET);
}

inline void fetchInstruction(InstructionNode* node, uint32_t& PC, bool& running, std::map<uint32_t, std::pair<uint32_t, std::string>>& textMap) {
    if (!isValidAddress(PC, 4)) {
        std::ostringstream oss;
        oss << "Fetch error: Invalid PC address 0x" << std::hex << PC;
        throw std::runtime_error(std::string(RED) + oss.str() + RESET);
    }
    auto it = textMap.find(PC);
    if (it != textMap.end()) {
        node->instruction = it->second.first;
        node->instructionType = classifyInstructions(node->instruction);
        node->PC = PC;
        PC += INSTRUCTION_SIZE;
    } else {
        node->instruction = 0;
        running = false;
    }
}

inline void decodeInstruction(InstructionNode* node, InstructionRegisters& instructionRegisters, uint32_t* registers) {
    node->opcode = node->instruction & 0x7F;

    switch (node->instructionType) {
        case InstructionType::R:
            node->rd = (node->instruction >> 7) & 0x1F;
            node->func3 = (node->instruction >> 12) & 0x7;
            node->rs1 = (node->instruction >> 15) & 0x1F;
            node->rs2 = (node->instruction >> 20) & 0x1F;
            node->func7 = (node->instruction >> 25) & 0x7F;
            break;
            
        case InstructionType::I:
            node->rd = (node->instruction >> 7) & 0x1F;
            node->func3 = (node->instruction >> 12) & 0x7;
            node->rs1 = (node->instruction >> 15) & 0x1F;
            break;
            
        case InstructionType::S:
            node->func3 = (node->instruction >> 12) & 0x7;
            node->rs1 = (node->instruction >> 15) & 0x1F;
            node->rs2 = (node->instruction >> 20) & 0x1F;
            break;
            
        case InstructionType::SB:
            node->func3 = (node->instruction >> 12) & 0x7;
            node->rs1 = (node->instruction >> 15) & 0x1F;
            node->rs2 = (node->instruction >> 20) & 0x1F;
            break;
            
        case InstructionType::U:
            node->rd = (node->instruction >> 7) & 0x1F;
            break;
            
        case InstructionType::UJ:
            node->rd = (node->instruction >> 7) & 0x1F;
            break;
            
        default:
            throw std::runtime_error(std::string(RED) + "Invalid instruction type in decodeInstruction" + RESET);
    }

    instructionRegisters.RA = (node->rs1 != 0) ? registers[node->rs1] : 0;

    switch (node->instructionType) {
        case InstructionType::R: {
            auto rTypeEncoding = RTypeInstructions::getEncoding();
            for (const auto &[name, op] : rTypeEncoding.opcodeMap) {
                if (op == node->opcode && rTypeEncoding.func3Map.at(name) == node->func3 && rTypeEncoding.func7Map.at(name) == node->func7) {
                    node->instructionName = name;
                    instructionRegisters.RB = registers[node->rs2];
                    break;
                }
            }
            break;
        }
        case InstructionType::I: {
            auto iTypeEncoding = ITypeInstructions::getEncoding();
            for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
                if (op == node->opcode && iTypeEncoding.func3Map.at(name) == node->func3) {
                    node->instructionName = name;
                    int32_t imm = (node->instruction >> 20) & 0xFFF;
                    if (imm & 0x800) imm |= 0xFFFFF000;
                    instructionRegisters.RB = imm;
                    break;
                }
            }
            break;
        }
        case InstructionType::S: {
            auto sTypeEncoding = STypeInstructions::getEncoding();
            for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
                if (op == node->opcode && sTypeEncoding.func3Map.at(name) == node->func3) {
                    node->instructionName = name;
                    int32_t imm = ((node->instruction >> 25) & 0x7F) << 5 | ((node->instruction >> 7) & 0x1F);
                    if (imm & 0x800) imm |= 0xFFFFF000;
                    instructionRegisters.RB = imm;
                    instructionRegisters.RM = registers[node->rs2];
                    break;
                }
            }
            break;
        }
        case InstructionType::SB: {
            auto sbTypeEncoding = SBTypeInstructions::getEncoding();
            for (const auto &[name, op] : sbTypeEncoding.opcodeMap) {
                if (op == node->opcode && sbTypeEncoding.func3Map.at(name) == node->func3) {
                    node->instructionName = name;
                    instructionRegisters.RB = ((node->instruction >> 31) & 0x1) << 12 | 
                                             ((node->instruction >> 7) & 0x1) << 11 | 
                                             ((node->instruction >> 25) & 0x3F) << 5 | 
                                             ((node->instruction >> 8) & 0xF) << 1;
                    if (instructionRegisters.RB & 0x1000) instructionRegisters.RB |= 0xFFFFE000;
                    instructionRegisters.RM = registers[node->rs2];
                    break;
                }
            }
            break;
        }
        case InstructionType::U: {
            auto uTypeEncoding = UTypeInstructions::getEncoding();
            for (const auto &[name, op] : uTypeEncoding.opcodeMap) {
                if (op == node->opcode) {
                    node->instructionName = name;
                    instructionRegisters.RB = node->instruction & 0xFFFFF000;
                    break;
                }
            }
            break;
        }
        case InstructionType::UJ: {
            auto ujTypeEncoding = UJTypeInstructions::getEncoding();
            for (const auto &[name, op] : ujTypeEncoding.opcodeMap) {
                if (op == node->opcode) {
                    node->instructionName = name;
                    int32_t imm = ((node->instruction >> 31) & 0x1) << 20 | 
                                  ((node->instruction >> 12) & 0xFF) << 12 | 
                                  ((node->instruction >> 20) & 0x1) << 11 | 
                                  ((node->instruction >> 21) & 0x3FF) << 1;
                    if (imm & 0x100000) imm |= 0xFFE00000;
                    instructionRegisters.RB = imm;
                    break;
                }
            }
            break;
        }
        default:
            throw std::runtime_error(std::string(RED) + "Invalid instruction type in decodeInstruction register setup" + RESET);
    }
}

inline void executeInstruction(InstructionNode* node, InstructionRegisters& instructionRegisters, uint32_t* registers, uint32_t& PC) {
    uint32_t result = 0;
    const std::string& name = node->instructionName;

    if (node->instructionType == InstructionType::R) {
        if (name == "add") {
            result = instructionRegisters.RA + instructionRegisters.RB;
        } else if (name == "sub") {
            result = instructionRegisters.RA - instructionRegisters.RB;
        } else if (name == "mul") {
            result = instructionRegisters.RA * instructionRegisters.RB;
        } else if (name == "div") {
            if (instructionRegisters.RB == 0) {
                std::cerr << RED << "Division by zero at PC 0x" << std::hex << node->PC << RESET << std::endl;
                exit(1);
            } else {
                result = static_cast<uint32_t>(static_cast<int32_t>(instructionRegisters.RA) / static_cast<int32_t>(instructionRegisters.RB));
            }
        } else if (name == "rem") {
            if (instructionRegisters.RB == 0) {
                std::cerr << RED << "Remainder by zero at PC 0x" << std::hex << node->PC << RESET << std::endl;
                exit(1);
            } else {
                result = static_cast<uint32_t>(static_cast<int32_t>(instructionRegisters.RA) % static_cast<int32_t>(instructionRegisters.RB));
            }
        } else if (name == "and") {
            result = instructionRegisters.RA & instructionRegisters.RB;
        } else if (name == "or") {
            result = instructionRegisters.RA | instructionRegisters.RB;
        } else if (name == "xor") {
            result = instructionRegisters.RA ^ instructionRegisters.RB;
        } else if (name == "sll") {
            result = instructionRegisters.RA << (instructionRegisters.RB & 0x1F);
        } else if (name == "srl") {
            result = instructionRegisters.RA >> (instructionRegisters.RB & 0x1F);
        } else if (name == "sra") {
            result = static_cast<uint32_t>(static_cast<int32_t>(instructionRegisters.RA) >> (instructionRegisters.RB & 0x1F));
        } else if (name == "slt") {
            result = (static_cast<int32_t>(instructionRegisters.RA) < static_cast<int32_t>(instructionRegisters.RB)) ? 1 : 0;
        }
        instructionRegisters.RY = result;
        return;
    }

    if (node->instructionType == InstructionType::I) {
        if (name == "addi") {
            result = instructionRegisters.RA + instructionRegisters.RB;
        } else if (name == "andi") {
            result = instructionRegisters.RA & instructionRegisters.RB;
        } else if (name == "ori") {
            result = instructionRegisters.RA | instructionRegisters.RB;
        } else if (name == "lb" || name == "lh" || name == "lw") {
            result = instructionRegisters.RA + instructionRegisters.RB;
            instructionRegisters.RY = result;
            return;
        } else if (name == "jalr") {
            result = node->PC + INSTRUCTION_SIZE;
            PC = (instructionRegisters.RA + instructionRegisters.RB) & ~1;
        }
        instructionRegisters.RY = result;
        return;
    }

    if (node->instructionType == InstructionType::S) {
        result = instructionRegisters.RA + instructionRegisters.RB;
        instructionRegisters.RY = result;
        return;
    }

    if (node->instructionType == InstructionType::SB) {
        bool branchTaken = false;
        if (name == "beq") {
            branchTaken = (instructionRegisters.RA == instructionRegisters.RM);
        } else if (name == "bne") {
            branchTaken = (instructionRegisters.RA != instructionRegisters.RM);
        } else if (name == "blt") {
            branchTaken = (static_cast<int32_t>(instructionRegisters.RA) < static_cast<int32_t>(instructionRegisters.RM));
        } else if (name == "bge") {
            branchTaken = (static_cast<int32_t>(instructionRegisters.RA) >= static_cast<int32_t>(instructionRegisters.RM));
        }
        PC = branchTaken ? (node->PC + instructionRegisters.RB) : PC;
        instructionRegisters.RY = branchTaken;
        return;
    }

    if (node->instructionType == InstructionType::U) {
        if (name == "lui") {
            result = instructionRegisters.RB;
        } else if (name == "auipc") {
            result = node->PC + instructionRegisters.RB;
        }
        instructionRegisters.RY = result;
        return;
    }

    if (node->instructionType == InstructionType::UJ) {
        if (name == "jal") {
            result = node->PC + INSTRUCTION_SIZE;
            PC = node->PC + instructionRegisters.RB;
        }
        instructionRegisters.RY = result;
        return;
    }
}

inline void memoryAccess(InstructionNode* node, InstructionRegisters& instructionRegisters, uint32_t* registers, std::unordered_map<uint32_t, uint8_t>& dataMap) {
    uint32_t address = instructionRegisters.RY;
    instructionRegisters.RZ = instructionRegisters.RY;
    
    auto iTypeEncoding = ITypeInstructions::getEncoding();
    for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
        if (op == node->opcode && iTypeEncoding.func3Map.at(name) == node->func3) {
            if (name == "lb") {
                isValidAddress(address, 1);
                instructionRegisters.RZ = dataMap.count(address) ? static_cast<int8_t>(dataMap[address]) : 0;
            } else if (name == "lh") {
                isValidAddress(address, 2);
                instructionRegisters.RZ = static_cast<int16_t>(
                    (dataMap.count(address + 1) ? dataMap[address + 1] : 0) << 8 |
                    (dataMap.count(address) ? dataMap[address] : 0)
                );
            } else if (name == "lw") {
                isValidAddress(address, 4);
                instructionRegisters.RZ = 
                    ((dataMap.count(address + 3) ? dataMap[address + 3] : 0) << 24) |
                    ((dataMap.count(address + 2) ? dataMap[address + 2] : 0) << 16) |
                    ((dataMap.count(address + 1) ? dataMap[address + 1] : 0) << 8)  |
                    (dataMap.count(address) ? dataMap[address] : 0);
            }
            return;
        }
    }

    auto sTypeEncoding = STypeInstructions::getEncoding();
    for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
        if (op == node->opcode && sTypeEncoding.func3Map.at(name) == node->func3) {
            uint32_t valueToStore = instructionRegisters.RM;
            if (name == "sb") {
                isValidAddress(address, 1);
                dataMap[address] = valueToStore & 0xFF;
            } else if (name == "sh") {
                isValidAddress(address, 2);
                dataMap[address] = valueToStore & 0xFF;
                dataMap[address + 1] = (valueToStore >> 8) & 0xFF;
            } else if (name == "sw") {
                isValidAddress(address, 4);
                dataMap[address] = valueToStore & 0xFF;
                dataMap[address + 1] = (valueToStore >> 8) & 0xFF;
                dataMap[address + 2] = (valueToStore >> 16) & 0xFF;
                dataMap[address + 3] = (valueToStore >> 24) & 0xFF;
            }
            return;
        }
    }
}

inline void writeback(InstructionNode* node, InstructionRegisters& instructionRegisters, uint32_t* registers) {
    if (node->rd != 0) {
        switch (node->instructionType) {
            case InstructionType::R:
            case InstructionType::U:
            case InstructionType::UJ:
            case InstructionType::I:
                registers[node->rd] = instructionRegisters.RZ;
                break;
            case InstructionType::SB:
            case InstructionType::S:
                break;
            default:
                throw std::runtime_error(std::string(RED) + "Invalid instruction type in writeback" + RESET);
        }
    }
    registers[0] = 0;
}

inline std::string parseInstructions(uint32_t instHex) {
    uint32_t opcode = instHex & 0x7F;
    uint32_t rd = (instHex >> 7) & 0x1F;
    uint32_t func3 = (instHex >> 12) & 0x7;
    uint32_t rs1 = (instHex >> 15) & 0x1F;
    uint32_t rs2 = (instHex >> 20) & 0x1F;
    uint32_t func7 = (instHex >> 25) & 0x7F;

    auto rTypeEncoding = RTypeInstructions::getEncoding();
    for (const auto &[name, op] : rTypeEncoding.opcodeMap) {
        if (op == opcode && rTypeEncoding.func3Map.at(name) == func3 && rTypeEncoding.func7Map.at(name) == func7) {
            std::stringstream ss;
            ss << name << " x" << rd << ", x" << rs1 << ", x" << rs2;
            return ss.str();
        }
    }

    auto iTypeEncoding = ITypeInstructions::getEncoding();
    for (const auto &[name, op] : iTypeEncoding.opcodeMap) {
        if (op == opcode && iTypeEncoding.func3Map.at(name) == func3) {
            int32_t imm = (instHex >> 20);
            if (imm & 0x800) imm |= 0xFFFFF000;
            std::stringstream ss;
            if (name == "lb" || name == "lh" || name == "lw") {
                ss << name << " x" << rd << ", " << imm << "(x" << rs1 << ")";
            } else {
                ss << name << " x" << rd << ", x" << rs1 << ", " << imm;
            }
            return ss.str();
        }
    }

    auto sTypeEncoding = STypeInstructions::getEncoding();
    for (const auto &[name, op] : sTypeEncoding.opcodeMap) {
        if (op == opcode && sTypeEncoding.func3Map.at(name) == func3) {
            int32_t imm = ((instHex >> 25) << 5) | ((instHex >> 7) & 0x1F);
            if (imm & 0x800) imm |= 0xFFFFF000;
            std::stringstream ss;
            ss << name << " x" << rs2 << ", " << imm << "(x" << rs1 << ")";
            return ss.str();
        }
    }

    auto sbTypeEncoding = SBTypeInstructions::getEncoding();
    for (const auto &[name, op] : sbTypeEncoding.opcodeMap) {
        if (op == opcode && sbTypeEncoding.func3Map.at(name) == func3) {
            int32_t imm = ((instHex >> 31) << 12) | (((instHex >> 7) & 1) << 11) | (((instHex >> 25) & 0x3F) << 5) | (((instHex >> 8) & 0xF) << 1);
            if (imm & 0x1000) imm |= 0xFFFFE000;
            std::stringstream ss;
            ss << name << " x" << rs1 << ", x" << rs2 << ", " << imm;
            return ss.str();
        }
    }

    auto uTypeEncoding = UTypeInstructions::getEncoding();
    for (const auto &[name, op] : uTypeEncoding.opcodeMap) {
        if (op == opcode) {
            uint32_t imm = instHex & 0xFFFFF000;
            std::stringstream ss;
            ss << name << " x" << rd << ", " << (imm >> 12);
            return ss.str();
        }
    }

    auto ujTypeEncoding = UJTypeInstructions::getEncoding();
    for (const auto &[name, op] : ujTypeEncoding.opcodeMap) {
        if (op == opcode) {
            int32_t imm = ((instHex >> 31) << 20) | (((instHex >> 12) & 0xFF) << 12) | (((instHex >> 20) & 1) << 11) | (((instHex >> 21) & 0x3FF) << 1);
            if (imm & 0x100000) imm |= 0xFFE00000;
            std::stringstream ss;
            ss << name << " x" << rd << ", " << imm;
            return ss.str();
        }
    }
    std::stringstream ss;
    ss << "Invalid instruction: 0x" << std::hex << instHex;
    throw std::runtime_error(std::string(RED) + ss.str() + RESET);
}

#endif