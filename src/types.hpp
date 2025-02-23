#ifndef TYPES_HPP
#define TYPES_HPP

#include <unordered_set>
#include <unordered_map>
#include <string>

enum TokenType {
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

std::unordered_set<std::string> const opcodes = {
    "add", "sub", "mul", "div", "rem", "and", "or", "xor", "sll", "slt", "sra", "srl",
    "addi", "andi", "ori", "lb", "lh", "lw", "ld", "jalr", 
    "sb", "sh", "sw", "sd", 
    "beq", "bne", "bge", "blt", 
    "auipc", "lui", "jal"
};

// It will be a shared resource
static std::unordered_set<std::string> definedLabels;

const std::unordered_set<std::string> standaloneOpcodes = {"ecall", "ebreak"};

const std::unordered_set<std::string> directives = {
    ".text", ".data", ".bss", ".globl", ".word", ".byte", ".half", ".dword",
    ".zero", ".string", ".asciiz", ".ascii", ".align", ".space", ".section",
    ".include", ".equ", ".set", ".option", ".comm", ".lcomm"
};

const std::unordered_map<std::string, std::string> validRegisters = {
    {"zero", "x0"},  {"ra", "x1"},  {"sp", "x2"},  {"gp", "x3"},  {"tp", "x4"},
    {"t0", "x5"},  {"t1", "x6"},  {"t2", "x7"},  {"s0", "x8"},  {"s1", "x9"},
    {"a0", "x10"}, {"a1", "x11"}, {"a2", "x12"}, {"a3", "x13"}, {"a4", "x14"},
    {"a5", "x15"}, {"a6", "x16"}, {"a7", "x17"}, {"s2", "x18"}, {"s3", "x19"},
    {"s4", "x20"}, {"s5", "x21"}, {"s6", "x22"}, {"s7", "x23"}, {"s8", "x24"},
    {"s9", "x25"}, {"s10", "x26"}, {"s11", "x27"}, {"t3", "x28"}, {"t4", "x29"},
    {"t5", "x30"}, {"t6", "x31"}
};


// Structure to hold instruction encodings
struct InstructionEncoding {
    std::unordered_map<std::string, std::string> func7Map;
    std::unordered_map<std::string, std::string> func3Map;
    std::unordered_map<std::string, std::string> opcodeMap;
};

// Define instruction encoding groups
struct RTypeInstructions {
    static const InstructionEncoding encoding;
};

struct ITypeInstructions {
    static const InstructionEncoding encoding;
};

struct STypeInstructions {
    static const InstructionEncoding encoding;
};

struct SBTypeInstructions {
    static const std::unordered_map<std::string, std::string> func3Map;
};

struct UTypeInstructions {
    static const std::unordered_map<std::string, std::string> opcodeMap;
};

struct UJTypeInstructions {
    static const std::unordered_map<std::string, std::string> opcodeMap;
};

#endif