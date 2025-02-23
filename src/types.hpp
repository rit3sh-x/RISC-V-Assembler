#include <unordered_set>
#include <unordered_map>
#include <string>

#ifndef TYPES_HPP
#define TYPES_HPP

enum TokenType {
    OPCODE,
    REGISTER,
    IMMEDIATE,
    MEMORY,
    LABEL,
    DIRECTIVE,
    COMMA,
    UNKNOWN,
    ERROR
};

std::unordered_set<std::string> opcodes = {
    "add", "sub", "mul", "div", "rem", "and", "or", "xor", "sll", "slt", "sra", "srl",
    "addi", "andi", "ori", "lb", "lh", "lw", "ld", "jalr", 
    "sb", "sh", "sw", "sd", 
    "beq", "bne", "bge", "blt", 
    "auipc", "lui", "jal"
};

std::unordered_set<std::string> directives = {
    ".text", ".data", ".bss", ".globl", ".word", ".byte", ".half", ".dword",
    ".zero", ".string", ".asciz", ".ascii", ".align", ".space", ".section",
    ".include", ".equ", ".set", ".option", ".comm", ".lcomm"
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

#endif // INSTRUCTION_TYPES_HPP
