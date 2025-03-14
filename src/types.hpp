#ifndef TYPES_HPP
#define TYPES_HPP

#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <string>

namespace riscv {
    static const uint32_t TEXT_SEGMENT_START = 0x00000000;
    static const uint32_t DATA_SEGMENT_START = 0x10000000;
    static const uint32_t HEAP_SEGMENT_START = 0x10008000;
    static const uint32_t STACK_SEGMENT_START = 0x7FFFFDC;
    static const uint32_t INSTRUCTION_SIZE = 4;

    enum TokenType {
        OPCODE,
        REGISTER,
        IMMEDIATE,
        MEMORY,
        LABEL,
        DIRECTIVE,
        UNKNOWN,
        ERROR,
        STRING
    };

    static const std::unordered_set<std::string> opcodes = {
        "add", "sub", "mul", "div", "rem", "and", "or", "xor", "sll", "slt", "sra", "srl",
        "addi", "andi", "ori", "lb", "lh", "lw", "ld", "jalr",
        "sb", "sh", "sw", "sd",
        "beq", "bne", "bge", "blt",
        "auipc", "lui", "jal",
        "slti", "sltiu", "xori", "srli", "srai", "bgeu", "bltu", "slli"};

    static const std::unordered_map<std::string, int> directives = {
        {".text", 0},
        {".data", 0},
        {".word", 4},
        {".byte", 1},
        {".half", 2},
        {".dword", 8},
        {".asciz", 1},
        {".asciiz", 1},
        {".ascii", 1}
    };

    static const std::unordered_map<std::string, int> validRegisters = {
        {"zero", 0}, {"x0", 0},
        {"ra", 1}, {"x1", 1},
        {"sp", 2}, {"x2", 2},
        {"gp", 3}, {"x3", 3},
        {"tp", 4}, {"x4", 4},
        {"t0", 5}, {"x5", 5},
        {"t1", 6}, {"x6", 6},
        {"t2", 7}, {"x7", 7},
        {"s0", 8}, {"fp", 8}, {"x8", 8},
        {"s1", 9}, {"x9", 9},
        {"a0", 10}, {"x10", 10},
        {"a1", 11}, {"x11", 11},
        {"a2", 12}, {"x12", 12},
        {"a3", 13}, {"x13", 13},
        {"a4", 14}, {"x14", 14},
        {"a5", 15}, {"x15", 15},
        {"a6", 16}, {"x16", 16},
        {"a7", 17}, {"x17", 17},
        {"s2", 18}, {"x18", 18},
        {"s3", 19}, {"x19", 19},
        {"s4", 20}, {"x20", 20},
        {"s5", 21}, {"x21", 21},
        {"s6", 22}, {"x22", 22},
        {"s7", 23}, {"x23", 23},
        {"s8", 24}, {"x24", 24},
        {"s9", 25}, {"x25", 25},
        {"s10", 26}, {"x26", 26},
        {"s11", 27}, {"x27", 27},
        {"t3", 28}, {"x28", 28},
        {"t4", 29}, {"x29", 29},
        {"t5", 30}, {"x30", 30},
        {"t6", 31}, {"x31", 31}
    };

    struct InstructionEncoding {
        std::unordered_map<std::string, uint32_t> func7Map;
        std::unordered_map<std::string, uint32_t> func3Map;
        std::unordered_map<std::string, uint32_t> opcodeMap;
    };

    struct RTypeInstructions {
        static const InstructionEncoding &getEncoding() {
            static const InstructionEncoding encoding = {
                {{"add", 0b0000000}, {"sub", 0b0100000}, {"mul", 0b0000001}, {"div", 0b0000001}, {"rem", 0b0000001}, {"and", 0b0000000}, {"or", 0b0000000}, {"xor", 0b0000000}, {"sll", 0b0000000}, {"slt", 0b0000000}, {"sra", 0b0100000}, {"srl", 0b0000000}},
                {{"add", 0b000}, {"sub", 0b000}, {"mul", 0b000}, {"div", 0b100}, {"rem", 0b110}, {"and", 0b111}, {"or", 0b110}, {"xor", 0b100}, {"sll", 0b001}, {"slt", 0b010}, {"sra", 0b101}, {"srl", 0b101}},
                {{"add", 0b0110011}, {"sub", 0b0110011}, {"mul", 0b0110011}, {"div", 0b0110011}, {"rem", 0b0110011}, {"and", 0b0110011}, {"or", 0b0110011}, {"xor", 0b0110011}, {"sll", 0b0110011}, {"slt", 0b0110011}, {"sra", 0b0110011}, {"srl", 0b0110011}}};
            return encoding;
        }
    };

    struct ITypeInstructions {
        static const InstructionEncoding &getEncoding() {
            static const InstructionEncoding encoding = {
                {{"slli", 0b0000000}, {"srli", 0b0000000}, {"srai", 0b0100000}},
                {{"addi", 0b000}, {"andi", 0b111}, {"ori", 0b110}, {"slti", 0b010}, {"sltiu", 0b011}, 
                 {"xori", 0b100}, {"lb", 0b000}, {"lh", 0b001}, {"lw", 0b010}, {"ld", 0b011}, 
                 {"jalr", 0b000}, {"slli", 0b001}, {"srli", 0b101}, {"srai", 0b101}},
                {{"addi", 0b0010011}, {"andi", 0b0010011}, {"ori", 0b0010011}, {"slti", 0b0010011}, 
                 {"sltiu", 0b0010011}, {"xori", 0b0010011}, {"lb", 0b0000011}, {"lh", 0b0000011}, 
                 {"lw", 0b0000011}, {"ld", 0b0000011}, {"jalr", 0b1100111}, {"slli", 0b0010011}, 
                 {"srli", 0b0010011}, {"srai", 0b0010011}}};
            return encoding;
        }
    };

    struct STypeInstructions {
        static const InstructionEncoding &getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {{"sb", 0b000}, {"sh", 0b001}, {"sw", 0b010}, {"sd", 0b011}},
                {{"sb", 0b0100011}, {"sh", 0b0100011}, {"sw", 0b0100011}, {"sd", 0b0100011}}};
            return encoding;
        }
    };

    struct SBTypeInstructions {
        static const InstructionEncoding &getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {{"beq", 0b000}, {"bne", 0b001}, {"bge", 0b101}, {"blt", 0b100}, {"bgeu", 0b111}, {"bltu", 0b110}},
                {{"beq", 0b1100011}, {"bne", 0b1100011}, {"bge", 0b1100011}, {"blt", 0b1100011}, {"bgeu", 0b1100011}, {"bltu", 0b1100011}}};
            return encoding;
        }
    };

    struct UTypeInstructions {
        static const InstructionEncoding &getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {},
                {{"lui", 0b0110111}, {"auipc", 0b0010111}}};
            return encoding;
        }
    };

    struct UJTypeInstructions {
        static const InstructionEncoding &getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {},
                {{"jal", 0b1101111}}};
            return encoding;
        }
    };

    std::string getTokenTypeName(TokenType type) {
        switch (type) {
            case TokenType::OPCODE: return "OPCODE";
            case TokenType::REGISTER: return "REGISTER";
            case TokenType::IMMEDIATE: return "IMMEDIATE";
            case TokenType::MEMORY: return "MEMORY";
            case TokenType::LABEL: return "LABEL";
            case TokenType::DIRECTIVE: return "DIRECTIVE";
            case TokenType::UNKNOWN: return "UNKNOWN";
            case TokenType::ERROR: return "ERROR";
            case TokenType::STRING: return "STRING";
            default: return "UNDEFINED";
        }
    }
}
#endif