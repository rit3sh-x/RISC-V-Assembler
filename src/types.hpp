#ifndef TYPES_HPP
#define TYPES_HPP

#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <vector>

namespace riscv {
    inline constexpr uint32_t TEXT_SEGMENT_START = 0x00000000;
    inline constexpr uint32_t DATA_SEGMENT_START = 0x10000000;
    inline constexpr uint32_t HEAP_SEGMENT_START = 0x10008000;
    inline constexpr uint32_t STACK_SEGMENT_START = 0x7FFFFDC;
    inline constexpr uint32_t INSTRUCTION_SIZE = 4;
    inline constexpr uint32_t MEMORY_SIZE = 0x80000000;

    inline constexpr int NUM_REGISTERS = 32;
    inline constexpr int MAX_STEPS = 100000;

    enum class Stage { FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK };
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

    inline const std::unordered_set<std::string> opcodes = {
        "add", "sub", "mul", "div", "rem", "and", "or", "xor", "sll", "slt", "sra", "srl",
        "addi", "andi", "ori", "lb", "lh", "lw", "ld", "jalr",
        "sb", "sh", "sw", "sd",
        "beq", "bne", "bge", "blt", "bgeu", "bltu",
        "auipc", "lui", "jal",
        "slti", "sltiu", "xori", "srli", "srai", "slli"
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

    inline std::unordered_map<int, std::string> logs;

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
        bool stalled;
    
        InstructionNode(uint32_t pc = 0) 
            : PC(pc), opcode(0), rs1(0), rs2(0), rd(0), instruction(0), func3(0), func7(0), stage(Stage::FETCH), stalled(false) {}
    };

    struct InstructionRegisters {
        uint32_t RA, RB, RM, RY, RZ;
        InstructionRegisters() : RA(0), RB(0), RM(0), RY(0), RZ(0) {}
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
        uint32_t branchMispredictions;
        uint32_t dataHazardStalls;
        uint32_t controlHazardStalls;

        bool enablePipeline;
        bool enableDataForwarding;
        bool printRegisterFile;
        bool printPipelineInfo;
        uint32_t followSpecificInstruction;
        bool printBranchPrediction;

        SimulationStats()
            : cyclesPerInstruction(0.0), totalCycles(0), instructionsExecuted(0),
              dataTransferInstructions(0), aluInstructions(0), controlInstructions(0),
              stallBubbles(0), dataHazards(0), controlHazards(0), branchMispredictions(0),
              dataHazardStalls(0), controlHazardStalls(0),
              enablePipeline(false), enableDataForwarding(false),
              printRegisterFile(false), printPipelineInfo(false),
              followSpecificInstruction(UINT32_MAX), printBranchPrediction(false) {}
    };

    struct BranchPredictionUnit {
        static const uint32_t PHT_SIZE = 1024;
        static const uint32_t PHT_MASK = PHT_SIZE - 1;
        static const uint32_t BTB_SIZE = 256;
        static const uint32_t BTB_MASK = BTB_SIZE - 1;
        static const uint32_t GHR_SIZE = 8; 
        static const uint32_t MAX_HISTORY_SIZE = 50;

        uint8_t patternHistoryTable[PHT_SIZE];
        struct BTBEntry {
            uint32_t tag;
            uint32_t target;
            bool valid;
        };
        BTBEntry branchTargetBuffer[BTB_SIZE];

        uint32_t globalHistoryRegister;
        uint32_t totalPredictions;
        uint32_t correctPredictions;
        uint32_t incorrectPredictions;

        struct BranchRecord {
            uint32_t pc;
            bool predicted;
            bool actual;
            uint32_t targetPC;
            uint8_t phtValue;
            uint32_t ghr;
            uint32_t phtIndex;
        };
        std::vector<BranchRecord> history;

        uint32_t getPHTIndex(uint32_t pc) const {
            uint32_t ghrHash = (globalHistoryRegister & ((1 << GHR_SIZE) - 1));
            uint32_t pcHash = ((pc >> 2) & PHT_MASK);
            return (pcHash ^ ghrHash) & PHT_MASK;
        }

        uint32_t getBTBTag(uint32_t pc) const {
            return pc >> 10;
        }

        uint32_t getBTBIndex(uint32_t pc) const {
            return (pc >> 2) & BTB_MASK;
        }
        
        bool predict(uint32_t pc) const {
            uint32_t index = getPHTIndex(pc);
            return patternHistoryTable[index] >= 2;
        }
        
        uint32_t getTargetPC(uint32_t pc) const {
            uint32_t index = getBTBIndex(pc);
            uint32_t tag = getBTBTag(pc);
            const BTBEntry& entry = branchTargetBuffer[index];
            
            if (entry.valid && entry.tag == tag) return entry.target;
            return 0;
        }

        void update(uint32_t pc, bool taken, uint32_t target = 0) {
            totalPredictions++;
            
            uint32_t phtIndex = getPHTIndex(pc);
            bool predicted = patternHistoryTable[phtIndex] >= 2;
            
            if (predicted == taken) correctPredictions++;
            else incorrectPredictions++;

            uint8_t& state = patternHistoryTable[phtIndex];
            if (taken) state = std::min<uint8_t>(3, state + 1);
            else state = state > 0 ? state - 1 : 0;

            if (target) {
                uint32_t btbIndex = getBTBIndex(pc);
                uint32_t tag = getBTBTag(pc);
                branchTargetBuffer[btbIndex].tag = tag;
                branchTargetBuffer[btbIndex].target = target;
                branchTargetBuffer[btbIndex].valid = true;
            }

            if (history.size() < MAX_HISTORY_SIZE) {
                history.push_back({
                    pc, 
                    predicted, 
                    taken, 
                    target, 
                    state, 
                    globalHistoryRegister,
                    phtIndex
                });
            } else if (history.size() == MAX_HISTORY_SIZE) {
                history.erase(history.begin());
                history.push_back({
                    pc, 
                    predicted, 
                    taken, 
                    target, 
                    state, 
                    globalHistoryRegister,
                    phtIndex
                });
            }
            globalHistoryRegister = ((globalHistoryRegister << 1) | (taken ? 1 : 0)) & ((1 << GHR_SIZE) - 1);
        }
        
        void printDetails(std::ostream& out) const {
            out << "===== Branch Prediction Unit Details =====" << std::endl;
            out << "Total predictions: " << totalPredictions << std::endl;
            out << "Correct predictions: " << correctPredictions << std::endl;
            out << "Incorrect predictions: " << incorrectPredictions << std::endl;
            
            double accuracy = 0.0;
            if (totalPredictions > 0) {
                accuracy = static_cast<double>(correctPredictions) / totalPredictions * 100.0;
            }
            out << "Prediction accuracy: " << accuracy << "%" << std::endl;
            
            out << "\nConfiguration:" << std::endl;
            out << "PHT size: " << PHT_SIZE << " entries" << std::endl;
            out << "BTB size: " << BTB_SIZE << " entries" << std::endl;
            out << "GHR size: " << GHR_SIZE << " bits" << std::endl;
            
            out << "\nCurrent Global History Register: 0b";
            for (int i = GHR_SIZE - 1; i >= 0; i--) {
                out << ((globalHistoryRegister & (1 << i)) ? 1 : 0);
            }
            out << " (" << globalHistoryRegister << ")" << std::endl;
            
            out << "\nActive Pattern History Table Entries:" << std::endl;
            int entriesShown = 0;
            for (uint32_t i = 0; i < PHT_SIZE && entriesShown < 10; i++) {
                if (patternHistoryTable[i] > 0) {
                    out << "Index: " << i 
                        << ", State: " << static_cast<int>(patternHistoryTable[i]) 
                        << " (" << (patternHistoryTable[i] >= 2 ? "Predict taken" : "Predict not taken") << ")" 
                        << std::endl;
                    entriesShown++;
                }
            }
            if (entriesShown == 0) {
                out << "No active PHT entries" << std::endl;
            } else if (entriesShown == 10) {
                out << "(showing 10 entries only)" << std::endl;
            }
            
            out << "\nActive Branch Target Buffer Entries:" << std::endl;
            entriesShown = 0;
            for (uint32_t i = 0; i < BTB_SIZE && entriesShown < 10; i++) {
                if (branchTargetBuffer[i].valid) {
                    out << "Index: " << i 
                        << ", Tag: 0x" << std::hex << branchTargetBuffer[i].tag << std::dec
                        << ", Target: 0x" << std::hex << branchTargetBuffer[i].target << std::dec
                        << std::endl;
                    entriesShown++;
                }
            }
            if (entriesShown == 0) {
                out << "No active BTB entries" << std::endl;
            } else if (entriesShown == 10) {
                out << "(showing 10 entries only)" << std::endl;
            }
            
            out << "\nRecent Branch History:" << std::endl;
            size_t historyStart = history.size() > 10 ? history.size() - 10 : 0;
            for (size_t i = historyStart; i < history.size(); i++) {
                const auto& record = history[i];
                out << "PC: 0x" << std::hex << record.pc 
                    << ", Predicted: " << (record.predicted ? "taken" : "not taken")
                    << ", Actual: " << (record.actual ? "taken" : "not taken")
                    << ", Target: 0x" << record.targetPC << std::dec
                    << ", PHT Index: " << record.phtIndex
                    << ", PHT State: " << static_cast<int>(record.phtValue)
                    << ", GHR: " << record.ghr 
                    << (record.predicted == record.actual ? " ✓" : " ✗")
                    << std::endl;
            }
            out << std::endl;
        }

        BranchPredictionUnit() {
            for (uint32_t i = 0; i < PHT_SIZE; i++) {
                patternHistoryTable[i] = 1;
            }
            for (uint32_t i = 0; i < BTB_SIZE; i++) {
                branchTargetBuffer[i].valid = false;
                branchTargetBuffer[i].tag = 0;
                branchTargetBuffer[i].target = 0;
            }
            
            globalHistoryRegister = 0;
            totalPredictions = 0;
            correctPredictions = 0;
            incorrectPredictions = 0;
            history.clear();
        }
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
                logs[404] = "Empty immediate value";
                return 0;
            }
            
            bool isNegative = cleanImm[0] == '-';
            if (isNegative) {
                cleanImm = cleanImm.substr(1);
                if (cleanImm.empty()) {
                    logs[404] = "Invalid immediate value: just a negative sign";
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
            std::string errorMsg = "Invalid immediate value '" + imm + "': " + e.what();
            logs[404] = errorMsg;
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
                {{"slli", 0b0000000}, {"srli", 0b0000000}, {"srai", 0b0100000}},
                {{"addi", 0b000}, {"andi", 0b111}, {"ori", 0b110}, {"slti", 0b010}, 
                 {"sltiu", 0b011}, {"xori", 0b100}, {"lb", 0b000}, {"lh", 0b001}, 
                 {"lw", 0b010}, {"ld", 0b011}, {"jalr", 0b000}, {"slli", 0b001}, 
                 {"srli", 0b101}, {"srai", 0b101}},
                {{"addi", 0b0010011}, {"andi", 0b0010011}, {"ori", 0b0010011}, 
                 {"slti", 0b0010011}, {"sltiu", 0b0010011}, {"xori", 0b0010011}, 
                 {"lb", 0b0000011}, {"lh", 0b0000011}, {"lw", 0b0000011}, {"ld", 0b0000011}, 
                 {"jalr", 0b1100111}, {"slli", 0b0010011}, {"srli", 0b0010011}, 
                 {"srai", 0b0010011}}
            };
            return encoding;
        }
    };

    struct STypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {{"sb", 0b000}, {"sh", 0b001}, {"sw", 0b010}, {"sd", 0b011}},
                {{"sb", 0b0100011}, {"sh", 0b0100011}, {"sw", 0b0100011}, {"sd", 0b0100011}}
            };
            return encoding;
        }
    };

    struct SBTypeInstructions {
        static inline const InstructionEncoding& getEncoding() {
            static const InstructionEncoding encoding = {
                {},
                {{"beq", 0b000}, {"bne", 0b001}, {"bge", 0b101}, {"blt", 0b100}, 
                 {"bgeu", 0b111}, {"bltu", 0b110}},
                {{"beq", 0b1100011}, {"bne", 0b1100011}, {"bge", 0b1100011}, 
                 {"blt", 0b1100011}, {"bgeu", 0b1100011}, {"bltu", 0b1100011}}
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