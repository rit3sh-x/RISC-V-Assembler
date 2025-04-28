#ifndef TYPES_HPP
#define TYPES_HPP

#include <unordered_set>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define ORANGE  "\033[38;5;208m"

namespace riscv {
    inline constexpr uint32_t TEXT_SEGMENT_START = 0x00000000;
    inline constexpr uint32_t DATA_SEGMENT_START = 0x10000000;
    inline constexpr uint32_t STACK_SEGMENT_START = 0x7FFFFDC;
    inline constexpr uint32_t INSTRUCTION_SIZE = 4;
    inline constexpr uint32_t MEMORY_SIZE = 0x80000000;

    inline constexpr int NUM_REGISTERS = 32;
    inline constexpr int MAX_STEPS = 100000;

    enum class Stage { FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK };

    inline const std::vector<Stage> reverseStageOrder = { Stage::WRITEBACK, Stage::MEMORY, Stage::EXECUTE, Stage::DECODE, Stage::FETCH };

    inline const std::vector<Stage> forwardStageOrder = { Stage::FETCH, Stage::DECODE, Stage::EXECUTE, Stage::MEMORY, Stage::WRITEBACK };

    inline std::string stageToString(Stage stage) {
        switch (stage) {
            case Stage::FETCH: return "FETCH";
            case Stage::DECODE: return "DECODE";
            case Stage::EXECUTE: return "EXECUTE";
            case Stage::MEMORY: return "MEMORY";
            case Stage::WRITEBACK: return "WRITEBACK";
            default: return "UNKNOWN";
        }
    }

    struct ForwardingStatus {
        bool raForwarded;
        bool rbForwarded;
        bool rmForwarded;

        ForwardingStatus() : raForwarded(false), rbForwarded(false), rmForwarded(false) {}
    };
    
    enum class InstructionType { R, I, S, SB, U, UJ };

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

    enum class Instructions {
        ADD, SUB, MUL, DIV, REM, AND, OR, XOR, SLL, SLT, SRA, SRL,
        ADDI, ANDI, ORI, LB, LH, LW, JALR,
        SB, SH, SW,
        BEQ, BNE, BGE, BLT,
        AUIPC, LUI, JAL,
        INVALID
    };

    inline const std::unordered_map<std::string, Instructions> stringToInstruction = {
        {"add", Instructions::ADD},   {"sub", Instructions::SUB},   {"mul", Instructions::MUL},
        {"div", Instructions::DIV},   {"rem", Instructions::REM},   {"and", Instructions::AND},
        {"or", Instructions::OR},     {"xor", Instructions::XOR},   {"sll", Instructions::SLL},
        {"slt", Instructions::SLT},   {"sra", Instructions::SRA},   {"srl", Instructions::SRL},
        {"addi", Instructions::ADDI}, {"andi", Instructions::ANDI}, {"ori", Instructions::ORI},
        {"lb", Instructions::LB},     {"lh", Instructions::LH},     {"lw", Instructions::LW},
        {"jalr", Instructions::JALR},
        {"sb", Instructions::SB},     {"sh", Instructions::SH},     {"sw", Instructions::SW},
        {"beq", Instructions::BEQ},   {"bne", Instructions::BNE},   {"bge", Instructions::BGE},
        {"blt", Instructions::BLT},
        {"auipc", Instructions::AUIPC}, {"lui", Instructions::LUI}, {"jal", Instructions::JAL}
    };

    inline const std::unordered_set<std::string> opcodes = {
        "add", "sub", "mul", "div", "rem", "and", "or", "xor", "sll", "slt", "sra", "srl",
        "addi", "andi", "ori", "lb", "lh", "lw", "jalr",
        "sb", "sh", "sw",
        "beq", "bne", "bge", "blt",
        "auipc", "lui", "jal"
    };

    inline const std::unordered_map<std::string, int> directives = {
        {".text", 0}, {".data", 0}, {".word", 4}, {".byte", 1},
        {".half", 2}, {".dword", 8}, {".asciz", 1}, {".asciiz", 1}, {".ascii", 1}
    };

    inline const std::unordered_map<std::string, int> validRegisters = {
        {"zero", 0}, {"x0", 0}, {"ra", 1}, {"x1", 1}, {"sp", 2}, {"x2", 2},
        {"gp", 3}, {"x3", 3}, {"tp", 4}, {"x4", 4}, {"t0", 5}, {"x5", 5},
        {"t1", 6}, {"x6", 6}, {"t2", 7}, {"x7", 7}, {"s0", 8}, {"fp", 8}, {"x8", 8},
        {"s1", 9}, {"x9", 9}, {"a0", 10}, {"x10", 10}, {"a1", 11}, {"x11", 11},
        {"a2", 12}, {"x12", 12}, {"a3", 13}, {"x13", 13}, {"a4", 14}, {"x14", 14},
        {"a5", 15}, {"x15", 15}, {"a6", 16}, {"x16", 16}, {"a7", 17}, {"x17", 17},
        {"s2", 18}, {"x18", 18}, {"s3", 19}, {"x19", 19}, {"s4", 20}, {"x20", 20},
        {"s5", 21}, {"x21", 21}, {"s6", 22}, {"x22", 22}, {"s7", 23}, {"x23", 23},
        {"s8", 24}, {"x24", 24}, {"s9", 25}, {"x25", 25}, {"s10", 26}, {"x26", 26},
        {"s11", 27}, {"x27", 27}, {"t3", 28}, {"x28", 28}, {"t4", 29}, {"x29", 29},
        {"t5", 30}, {"x30", 30}, {"t6", 31}, {"x31", 31}
    };

    struct BranchPredictor {
        struct BTBEntry {
            uint32_t targetAddress;
            bool valid;
            
            BTBEntry() : targetAddress(0), valid(false) {}
            BTBEntry(uint32_t target) : targetAddress(target), valid(true) {}
        };
        
        std::unordered_map<uint32_t, bool> PHT;
        std::unordered_map<uint32_t, BTBEntry> BTB;
        
        uint32_t totalPredictions;
        uint32_t mispredictions;
        
        BranchPredictor() : totalPredictions(0), mispredictions(0) {}
        
        bool predict(uint32_t branchPC) {
            return PHT.count(branchPC) > 0 ? PHT[branchPC] : false;
        }
        
        uint32_t getTarget(uint32_t branchPC) {
            return BTB.count(branchPC) > 0 && BTB[branchPC].valid ? BTB[branchPC].targetAddress : 0;
        }

        bool getPHT(uint32_t branchPC) const {
            return PHT.count(branchPC) > 0 ? PHT.at(branchPC) : false;
        }
        
        void update(uint32_t branchPC, bool taken, uint32_t targetAddress) {
            totalPredictions++;
            bool predicted = predict(branchPC);
            PHT[branchPC] = taken;
            if (taken) {
                BTB[branchPC] = BTBEntry(targetAddress);
            }
            if (predicted != taken) {
                mispredictions++;
            }
        }

        bool isInBTB(uint32_t branchPC) const {
            return BTB.count(branchPC) > 0 && BTB.at(branchPC).valid;
        }
        
        double getAccuracy() const {
            return totalPredictions > 0 ? static_cast<double>(totalPredictions - mispredictions) / totalPredictions * 100.0 : 0.0;
        }
        
        void reset() {
            PHT.clear();
            BTB.clear();
            totalPredictions = 0;
            mispredictions = 0;
        }
    };

    struct Token {
        TokenType type;
        std::string value;
        int lineNumber;
        Token(TokenType t, const std::string& v, int ln) : type(t), value(v), lineNumber(ln) {}
    };

    struct SymbolEntry {
        uint32_t address;
        bool isString;
        std::vector<uint64_t> numericValues;
        std::string stringValue;
        std::string directive;
    };
    
    struct ParsedInstruction {
        std::string opcode;
        std::vector<std::string> operands;
        uint32_t address;
    
        ParsedInstruction(std::string opc, std::vector<std::string> ops, uint32_t addr) 
            : opcode(std::move(opc)), operands(std::move(ops)), address(addr) {}
    };

    struct InstructionNode {
        uint32_t PC, opcode, rs1, rs2, rd, instruction, func3, func7;
        InstructionType instructionType;
        Stage stage;
        bool stalled, isBranch, isJump, isLoad, isStore;
        Instructions instructionName;
        uint32_t uniqueId;
    
        InstructionNode(uint32_t pc = 0) 
            : PC(pc), opcode(0), rs1(0), rs2(0), rd(0), instruction(0), func3(0), func7(0), stage(Stage::FETCH), stalled(false), isBranch(false), isJump(false), isLoad(false), isStore(false), instructionName(Instructions::INVALID), uniqueId(0){}

        InstructionNode(const InstructionNode& other)
            : PC(other.PC), opcode(other.opcode), rs1(other.rs1), rs2(other.rs2), rd(other.rd), 
              instruction(other.instruction), func3(other.func3), func7(other.func7),
              instructionType(other.instructionType), stage(other.stage), 
              stalled(other.stalled), isBranch(other.isBranch), isJump(other.isJump), isLoad(other.isLoad), isStore(other.isStore), 
              instructionName(other.instructionName), uniqueId(other.uniqueId) {}
    };

    struct InstructionRegisters {
        uint32_t RA, RB, RM, RY, RZ;
        InstructionRegisters() : RA(0), RB(0), RM(0), RY(0), RZ(0) {}
    };

    struct RegisterDependency {
        uint32_t reg;
        uint32_t opcode;
        uint32_t pc;
        Stage stage;
        uint32_t value;
        bool isLoad;
        uint32_t uniqueId;
    };

    struct SimulationStats {
        double cyclesPerInstruction;
        uint32_t totalCycles;
        uint32_t instructionsExecuted;
        uint32_t dataTransferInstructions;
        uint32_t aluInstructions;
        uint32_t controlInstructions;
        uint32_t stallBubbles;
        uint32_t dataHazards;
        uint32_t controlHazards;
        uint32_t dataHazardStalls;
        uint32_t controlHazardStalls;
        uint32_t pipelineFlushes;
        uint32_t branchMispredictions;

        SimulationStats()
            : cyclesPerInstruction(0.0), totalCycles(0), instructionsExecuted(0),
              dataTransferInstructions(0), aluInstructions(0), controlInstructions(0),
              stallBubbles(0), dataHazards(0), controlHazards(0), dataHazardStalls(0),
              controlHazardStalls(0), pipelineFlushes(0), branchMispredictions(0) {}
    };

    struct InstructionEncoding {
        std::unordered_map<std::string, uint32_t> func7Map;
        std::unordered_map<std::string, uint32_t> func3Map;
        std::unordered_map<std::string, uint32_t> opcodeMap;
    };

    inline std::string getTokenTypeName(TokenType type) {
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
            default: return "UNKNOWN";
        }
    }

    inline bool isRegister(const std::string& token) {
        return validRegisters.count(token) > 0;
    }

    inline bool isImmediate(const std::string& token) {
        if (token.empty()) return false;
        
        size_t pos = 0;
        if (token[0] == '-' || token[0] == '+') {
            if (token.length() == 1) return false;
            pos = 1;
        }
        if (token.length() > pos + 2 && token[pos] == '0') {
            char format = std::tolower(token[pos + 1]);
            if (format == 'x') {
                return token.length() > pos + 2 && std::all_of(token.begin() + pos + 2, token.end(), ::isxdigit);
            } else if (format == 'b') {
                return token.length() > pos + 2 && std::all_of(token.begin() + pos + 2, token.end(), [](char c) { return c == '0' || c == '1'; });
            }
        }
        return std::all_of(token.begin() + pos, token.end(), ::isdigit);
    }

    inline std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    inline bool isMemory(const std::string& token, std::string& offset, std::string& reg) {
        size_t open = token.find('(');
        size_t close = token.find(')', open);
        if (open == std::string::npos || close == std::string::npos || close <= open) return false;
        offset = trim(token.substr(0, open));
        reg = trim(token.substr(open + 1, close - open - 1));
        if (offset.empty()) offset = "0";
        if (!isRegister(reg) || (!offset.empty() && !isImmediate(offset))) return false;
        if (close + 1 < token.length() && !trim(token.substr(close + 1)).empty()) return false;
        return true;
    }    

    inline uint32_t getDirectiveSize(const std::string& directive) {
        auto it = directives.find(directive);
        return (it != directives.end()) ? it->second : 0;
    }

    inline int32_t getRegisterNumber(const std::string& reg) {
        auto it = validRegisters.find(reg);
        if (it != validRegisters.end()) {
            return it->second;
        }
        
        if (reg.length() > 1 && reg[0] == 'x') {
            try {
                int num = std::stoi(reg.substr(1));
                return (num >= 0 && num < 32) ? num : -1;
            } catch (...) {
                return -1;
            }
        }
        
        return -1;
    }

    inline int32_t parseImmediate(const std::string& imm) {
        try {
            std::string cleanImm = trim(imm);
            if (cleanImm.empty()) {
                throw std::invalid_argument("Empty immediate value");
                return 0;
            }
            
            bool isNegative = cleanImm[0] == '-';
            if (isNegative) {
                cleanImm = cleanImm.substr(1);
                if (cleanImm.empty()) {
                    throw std::invalid_argument("Invalid immediate value");
                    return 0;
                }
            }
            
            uint64_t value = 0;
            if (cleanImm.length() > 2 && cleanImm[0] == '0') {
                char format = ::tolower(cleanImm[1]);
                if (format == 'x') {
                    value = std::stoull(cleanImm, nullptr, 16);
                } else if (format == 'b') {
                    value = std::stoull(cleanImm.substr(2), nullptr, 2);
                } else {
                    value = std::stoull(cleanImm, nullptr, 10);
                }
            } else {
                value = std::stoull(cleanImm, nullptr, 10);
            }
            
            return isNegative ? -static_cast<int64_t>(value) : static_cast<int64_t>(value);
        } catch (const std::exception& e) {
            std::string errorMsg = "Error parsing immediate value: " + std::string(e.what());
            throw std::runtime_error(errorMsg);
        }
    }
    
    struct RTypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {{"add", 0b0000000}, {"sub", 0b0100000}, {"mul", 0b0000001}, {"div", 0b0000001}, 
                 {"rem", 0b0000001}, {"and", 0b0000000}, {"or", 0b0000000}, {"xor", 0b0000000}, 
                 {"sll", 0b0000000}, {"slt", 0b0000000}, {"sra", 0b0100000}, {"srl", 0b0000000}},
                {{"add", 0b000}, {"sub", 0b000}, {"mul", 0b000}, {"div", 0b100}, {"rem", 0b110}, 
                 {"and", 0b111}, {"or", 0b110}, {"xor", 0b100}, {"sll", 0b001}, {"slt", 0b010}, 
                 {"sra", 0b101}, {"srl", 0b101}},
                {{"add", 0b0110011}, {"sub", 0b0110011}, {"mul", 0b0110011}, {"div", 0b0110011}, 
                 {"rem", 0b0110011}, {"and", 0b0110011}, {"or", 0b0110011}, {"xor", 0b0110011}, 
                 {"sll", 0b0110011}, {"slt", 0b0110011}, {"sra", 0b0110011}, {"srl", 0b0110011}}
            };
            return encoding;
        }
    };

    struct ITypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {{"addi", 0b000}, {"andi", 0b111}, {"ori", 0b110}, 
                 {"lb", 0b000}, {"lh", 0b001}, {"lw", 0b010}, {"jalr", 0b000}},
                {{"addi", 0b0010011}, {"andi", 0b0010011}, {"ori", 0b0010011}, 
                 {"lb", 0b0000011}, {"lh", 0b0000011}, {"lw", 0b0000011}, {"jalr", 0b1100111}}
            };
            return encoding;
        }
    };

    struct STypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {{"sb", 0b000}, {"sh", 0b001}, {"sw", 0b010}},
                {{"sb", 0b0100011}, {"sh", 0b0100011}, {"sw", 0b0100011}}
            };
            return encoding;
        }
    };

    struct SBTypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {{"beq", 0b000}, {"bne", 0b001}, {"bge", 0b101}, {"blt", 0b100}},
                {{"beq", 0b1100011}, {"bne", 0b1100011}, {"bge", 0b1100011}, {"blt", 0b1100011}}
            };
            return encoding;
        }
    };

    struct UTypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {},
                {{"lui", 0b0110111}, {"auipc", 0b0010111}}
            };
            return encoding;
        }
    };

    struct UJTypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {},
                {{"jal", 0b1101111}}
            };
            return encoding;
        }
    };
}

#endif