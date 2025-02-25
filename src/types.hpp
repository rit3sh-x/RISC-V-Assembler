#ifndef TYPES_HPP
#define TYPES_HPP

#include <unordered_set>
#include <unordered_map>
#include <string>

enum TokenType
{
    OPCODE,
    REGISTER,
    IMMEDIATE,
    MEMORY,
    LABEL,
    DIRECTIVE,
    UNKNOWN,
    ERROR,
    STRING,
    STANDALONE
};

namespace riscv
{
    static std::unordered_set<std::string> definedLabels;

    static const std::unordered_set<std::string> opcodes = {
        "add", "sub", "mul", "div", "rem", "and", "or", "xor", "sll", "slt", "sra", "srl",
        "addi", "andi", "ori", "lb", "lh", "lw", "ld", "jalr",
        "sb", "sh", "sw", "sd",
        "beq", "bne", "bge", "blt",
        "auipc", "lui", "jal",
        "slti", "sltiu", "xori", "srli", "srai", "bgeu", "bltu"};

    static const std::unordered_set<std::string> standaloneOpcodes = {"ecall", "ebreak"};

    static const std::unordered_map<std::string, int> directives = {
        {".text", 0},
        {".data", 0},
        {".word", 4},
        {".byte", 1},
        {".half", 2},
        {".dword", 8},
        {".asciz", 1},
        {".asciiz", 1},
        {".ascii", 0},
    };

    static const std::unordered_map<std::string, std::string> validRegisters = {
        {"zero", "x0"}, {"ra", "x1"}, {"sp", "x2"}, {"gp", "x3"}, {"tp", "x4"}, {"t0", "x5"}, {"t1", "x6"}, {"t2", "x7"}, {"s0", "x8"}, {"s1", "x9"}, {"a0", "x10"}, {"a1", "x11"}, {"a2", "x12"}, {"a3", "x13"}, {"a4", "x14"}, {"a5", "x15"}, {"a6", "x16"}, {"a7", "x17"}, {"s2", "x18"}, {"s3", "x19"}, {"s4", "x20"}, {"s5", "x21"}, {"s6", "x22"}, {"s7", "x23"}, {"s8", "x24"}, {"s9", "x25"}, {"s10", "x26"}, {"s11", "x27"}, {"t3", "x28"}, {"t4", "x29"}, {"t5", "x30"}, {"t6", "x31"}};
}

struct InstructionEncoding
{
    std::unordered_map<std::string, std::string> func7Map;
    std::unordered_map<std::string, std::string> func3Map;
    std::unordered_map<std::string, std::string> opcodeMap;
};

struct RTypeInstructions
{
    static const InstructionEncoding &getEncoding()
    {
        static const InstructionEncoding encoding = {
            {{"add", "0000000"}, {"sub", "0100000"}, {"mul", "0000001"}, {"div", "0000001"}, {"rem", "0000001"}, {"and", "0000000"}, {"or", "0000000"}, {"xor", "0000000"}, {"sll", "0000000"}, {"slt", "0000000"}, {"sra", "0100000"}, {"srl", "0000000"}},
            {{"add", "000"}, {"sub", "000"}, {"mul", "000"}, {"div", "100"}, {"rem", "110"}, {"and", "111"}, {"or", "110"}, {"xor", "100"}, {"sll", "001"}, {"slt", "010"}, {"sra", "101"}, {"srl", "101"}},
            {{"add", "0110011"}, {"sub", "0110011"}, {"mul", "0110011"}, {"div", "0110011"}, {"rem", "0110011"}, {"and", "0110011"}, {"or", "0110011"}, {"xor", "0110011"}, {"sll", "0110011"}, {"slt", "0110011"}, {"sra", "0110011"}, {"srl", "0110011"}}};
        return encoding;
    }
};

struct ITypeInstructions
{
    static const InstructionEncoding &getEncoding()
    {
        static const InstructionEncoding encoding = {
            {{"slli", "0000000"}, {"srli", "0000000"}, {"srai", "0100000"}},
            {{"addi", "000"}, {"andi", "111"}, {"ori", "110"}, {"slti", "010"}, {"sltiu", "011"}, {"xori", "100"}, {"lb", "000"}, {"lh", "001"}, {"lw", "010"}, {"ld", "011"}, {"jalr", "000"}, {"slli", "001"}, {"srli", "101"}, {"srai", "101"}},
            {{"addi", "0010011"}, {"andi", "0010011"}, {"ori", "0010011"}, {"slti", "0010011"}, {"sltiu", "0010011"}, {"xori", "0010011"}, {"lb", "0000011"}, {"lh", "0000011"}, {"lw", "0000011"}, {"ld", "0000011"}, {"jalr", "1100111"}, {"slli", "0010011"}, {"srli", "0010011"}, {"srai", "0010011"}}};
        return encoding;
    }
};

struct STypeInstructions
{
    static const InstructionEncoding &getEncoding()
    {
        static const InstructionEncoding encoding = {
            {},
            {{"sb", "000"}, {"sh", "001"}, {"sw", "010"}, {"sd", "011"}},
            {{"sb", "0100011"}, {"sh", "0100011"}, {"sw", "0100011"}, {"sd", "0100011"}}};
        return encoding;
    }
};

struct SBTypeInstructions
{
    static const InstructionEncoding &getEncoding()
    {
        static const InstructionEncoding encoding = {
            {},
            {{"beq", "000"}, {"bne", "001"}, {"bge", "101"}, {"blt", "100"}, {"bgeu", "111"}, {"bltu", "110"}},
            {{"beq", "1100011"}, {"bne", "1100011"}, {"bge", "1100011"}, {"blt", "1100011"}, {"bgeu", "1100011"}, {"bltu", "1100011"}}};
        return encoding;
    }
};

struct UTypeInstructions
{
    static const InstructionEncoding &getEncoding()
    {
        static const InstructionEncoding encoding = {
            {},
            {},
            {{"auipc", "0010111"}, {"lui", "0110111"}}};
        return encoding;
    }
};

struct UJTypeInstructions
{
    static const InstructionEncoding &getEncoding()
    {
        static const InstructionEncoding encoding = {
            {},
            {},
            {{"jal", "1101111"}}};
        return encoding;
    }
};

#endif