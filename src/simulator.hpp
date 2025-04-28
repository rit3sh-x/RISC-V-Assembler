#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <iomanip>
#include "types.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include "execution.hpp"

using namespace riscv;

class Simulator {
private:
    uint32_t PC;
    uint32_t registers[NUM_REGISTERS];

    std::unordered_map<uint32_t, uint8_t> dataMap;
    std::map<uint32_t, std::pair<uint32_t, std::string>> textMap;

    std::map<Stage, InstructionNode*> pipeline;
    InstructionRegisters instructionRegisters;
    ForwardingStatus forwardingStatus;
    InstructionRegisters followedInstructionRegisters;

    bool running;
    bool isPipeline;
    bool isDataForwarding;
    bool isBranchPrediction;
    bool isFollowing;
    uint32_t followedInstruction;

    SimulationStats stats;
    std::unordered_map<uint32_t, RegisterDependency> registerDependencies;
    BranchPredictor branchPredictor;

    uint32_t instructionCount;
    uint32_t nextInstructionId;

    void advancePipeline();
    void flushPipeline(const std::string& reason = "");
    void applyDataForwarding(InstructionNode& node, const std::unordered_map<uint32_t, RegisterDependency>& depsSnapshot);
    bool checkDependencies(const InstructionNode& node, const std::unordered_map<uint32_t, RegisterDependency>& depsSnapshot) const;
    void updateDependencies(InstructionNode& node, Stage stage);
    bool checkLoadUseHazard(const InstructionNode& node, const std::unordered_map<uint32_t, RegisterDependency>& depsSnapshot, bool isStore = false);
    bool isPipelineEmpty() const;
    void reset();
    
    public:
    Simulator();
    bool loadProgram(const std::string &input);
    bool step();
    void run();
    void setEnvironment(bool pipeline, bool dataForwarding, bool branchPrediction, uint32_t instruction);
    const uint32_t *getRegisters() const;
    uint32_t getFollowedPC() const;
    std::map<uint32_t, std::pair<uint32_t, std::string>> getTextMap() const;
    uint32_t getCycles() const;
    SimulationStats getStats();
    InstructionRegisters getInstructionRegisters() const;
    InstructionRegisters getFollowedInstructionRegisters() const;
};

Simulator::Simulator() : PC(TEXT_SEGMENT_START),
                         instructionRegisters(InstructionRegisters()),
                         forwardingStatus(ForwardingStatus()),
                         running(false),
                         isPipeline(true),
                         isDataForwarding(true),
                         stats(SimulationStats()),
                         branchPredictor(BranchPredictor()),
                         instructionCount(0),
                         nextInstructionId(0)
{
    initialiseRegisters(registers);
    pipeline[Stage::FETCH] = nullptr;
    pipeline[Stage::DECODE] = nullptr;
    pipeline[Stage::EXECUTE] = nullptr;
    pipeline[Stage::MEMORY] = nullptr;
    pipeline[Stage::WRITEBACK] = nullptr;
    instructionRegisters = InstructionRegisters();
    followedInstructionRegisters = InstructionRegisters();
}

bool Simulator::loadProgram(const std::string &input) {
    try {
        bool wasPipeline = isPipeline;
        bool wasDataForwarding = isDataForwarding;
        bool wasBranchPrediction = isBranchPrediction;
        bool wasFollowing = isFollowing;
        
        reset();

        isPipeline = wasPipeline;
        isDataForwarding = wasDataForwarding;
        isBranchPrediction = wasBranchPrediction;
        isFollowing = wasFollowing;
        running = true;

        std::vector<std::vector<Token>> tokenizedLines = Lexer::tokenize(input);
        if (tokenizedLines.empty()) {
            std::cerr << RED << "Error: No tokens generated from input" << RESET << std::endl;
            return false;
        }

        Parser parser(tokenizedLines);
        if (!parser.parse()) {
            throw std::runtime_error("Parsing failed with " + std::to_string(parser.getErrorCount()) + " errors");
            return false;
        }

        std::unordered_map<std::string, SymbolEntry> symbolTable = parser.getSymbolTable();
        std::vector<ParsedInstruction> parsedInstructions = parser.getParsedInstructions();

        Assembler assembler(symbolTable, parsedInstructions);
        if (!assembler.assemble()) {
            throw std::runtime_error("Assembly failed with " + std::to_string(assembler.getErrorCount()) + " errors");
            return false;
        }

        for (const auto &[address, value] : assembler.getMachineCode()) {
            if (address >= DATA_SEGMENT_START) {
                dataMap[address] = static_cast<uint8_t>(value);
            } else {
                textMap[address] = std::make_pair(value, parseInstructions(value));
            }
        }
        
        PC = TEXT_SEGMENT_START;
        instructionCount = 0;
        nextInstructionId = 0;
        std::cout << GREEN << "Program loaded successfully" << RESET << std::endl;
        InstructionNode* firstNode = new InstructionNode(PC);
        pipeline[Stage::FETCH] = firstNode;
        firstNode->uniqueId = nextInstructionId++;
        return true;
    }
    catch (const std::exception &e) {
        std::cerr << RED << "Error: " << e.what() << RESET << std::endl;
        return false;
    }
}

void Simulator::reset() {
    for (auto& [stage, node] : pipeline) {
        if (node != nullptr) {
            delete node;
            node = nullptr;
        }
    }
    
    instructionRegisters = InstructionRegisters();
    initialiseRegisters(registers);
    registerDependencies.clear();
    dataMap.clear();
    textMap.clear();
    
    PC = TEXT_SEGMENT_START;
    running = false;
    stats = SimulationStats();
    forwardingStatus = ForwardingStatus();
    branchPredictor.reset();
    instructionCount = 0;
}

void Simulator::applyDataForwarding(InstructionNode& node, const std::unordered_map<uint32_t, RegisterDependency>& depsSnapshot) {
    if (!isPipeline || !isDataForwarding) return;

    forwardingStatus = ForwardingStatus();

    if (node.stage == Stage::MEMORY) {
        for (const auto& [uniqueId, dep] : depsSnapshot) {
            if (dep.stage == Stage::MEMORY && dep.reg != 0 && dep.isLoad) {
                if (node.rs1 != 0 && node.rs1 == dep.reg && !forwardingStatus.raForwarded) {
                    instructionRegisters.RA = dep.value;
                    forwardingStatus.raForwarded = true;
                    std::cout << YELLOW << "\nData Forwarding: MEM->MEM for rs1 (reg " << node.rs1
                              << ") of instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ")"
                              << (dep.isLoad ? " [Load]" : "") << " from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                }
                if (node.rs2 != 0 && node.rs2 == dep.reg && !forwardingStatus.rbForwarded && !forwardingStatus.rmForwarded) {
                    if (node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) {
                        instructionRegisters.RM = dep.value;
                        forwardingStatus.rmForwarded = true;
                        std::cout << YELLOW << "\nData Forwarding: MEM->MEM for rs2 (reg " << node.rs2
                                  << ") to RM of instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ")"
                                  << (dep.isLoad ? " [Load]" : "") << " from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                    } else {
                        instructionRegisters.RB = dep.value;
                        forwardingStatus.rbForwarded = true;
                        std::cout << YELLOW << "\nData Forwarding: MEM->MEM for rs2 (reg " << node.rs2
                                  << ") of instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ")"
                                  << (dep.isLoad ? " [Load]" : "") << " from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                    }
                }
            }
        }
        return;
    }

    for (const auto& [uniqueId, dep] : depsSnapshot) {
        if (dep.stage == Stage::EXECUTE && dep.reg != 0) {
            if (!dep.isLoad) {
                if (node.rs1 != 0 && node.rs1 == dep.reg) {
                    instructionRegisters.RA = dep.value;
                    forwardingStatus.raForwarded = true;

                    std::cout << YELLOW << "\nData Forwarding: EX->EX for rs1 (reg " << node.rs1 << ") of instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ") from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                }
                if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg) {
                    if (node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) {
                        instructionRegisters.RM = dep.value;
                        forwardingStatus.rmForwarded = true;

                        std::cout << YELLOW << "Data Forwarding: EX->EX for rs2 (reg " << node.rs2
                        << ") to RM of instruction at PC=" << node.PC << " (" << textMap[node.PC].second
                        << ") from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                    } else {
                        instructionRegisters.RB = dep.value;
                        forwardingStatus.rbForwarded = true;

                        std::cout << YELLOW << "\nData Forwarding: EX->EX for rs2 (reg " << node.rs2
                        << ") of instruction at PC=" << node.PC << " (" << textMap[node.PC].second
                        << ") from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                    }
                }
            }
        }
    }

    for (const auto& [pc, dep] : depsSnapshot) {
        if (dep.stage == Stage::MEMORY && dep.reg != 0) {
            if (node.rs1 != 0 && node.rs1 == dep.reg && !forwardingStatus.raForwarded) {
                instructionRegisters.RA = dep.value;
                forwardingStatus.raForwarded = true;

                std::cout << YELLOW << "\nData Forwarding: MEM->EX for rs1 (reg " << node.rs1
                << ") of instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ")"
                << (dep.isLoad ? " [Load]" : "") << " from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
            }

            if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S ||
                 node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg &&
                !forwardingStatus.rbForwarded && !forwardingStatus.rmForwarded) {
                if (node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) {
                    instructionRegisters.RM = dep.value;
                    forwardingStatus.rmForwarded = true;

                    std::cout << YELLOW << "\nData Forwarding: MEM->EX for rs2 (reg " << node.rs2
                    << ") to RM of instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ")"
                    << (dep.isLoad ? " [Load]" : "") << " from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                } else {
                    instructionRegisters.RB = dep.value;
                    forwardingStatus.rbForwarded = true;

                    std::cout << YELLOW << "\nData Forwarding: MEM->EX for rs2 (reg " << node.rs2
                    << ") of instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ")"
                    << (dep.isLoad ? " [Load]" : "") << " from instruction (" << textMap[dep.pc].second << ")" << RESET << std::endl;
                }
            }
        }
    }
}

bool Simulator::checkDependencies(const InstructionNode& node, const std::unordered_map<uint32_t, RegisterDependency>& depsSnapshot) const {
    if (!isPipeline || isDataForwarding) {
        return false;
    }
    
    for (const auto& [uniqueId, dep] : depsSnapshot) {
        if (dep.stage == Stage::MEMORY) continue;
        if (uniqueId != node.uniqueId) {
            if (node.rs1 != 0 && node.rs1 == dep.reg) {
                std::cout << YELLOW << "Data Hazard: Instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ") depends on reg " + std::to_string(dep.reg) + " in " + stageToString(dep.stage) << RESET << std::endl;
                return true;
            } else if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg) {
                std::cout << YELLOW << "Data Hazard: Instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ") depends on reg " + std::to_string(dep.reg) + " in " + stageToString(dep.stage) << RESET << std::endl;
                return true;
            }
        }
    }
    return false;
}

bool Simulator::checkLoadUseHazard(const InstructionNode& node, const std::unordered_map<uint32_t, RegisterDependency>& depsSnapshot, bool isStore) {
    if (!isPipeline) {
        return false;
    }

    uint32_t rs1 = node.rs1;
    uint32_t rs2 = node.rs2;
    bool hasRS2 = (node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB);

    for (const auto& [uniqueId, dep] : depsSnapshot) {
        if (uniqueId != node.uniqueId && dep.stage == Stage::EXECUTE && dep.isLoad && !isStore) {
            if ((rs1 != 0 && rs1 == dep.reg) || (hasRS2 && rs2 != 0 && rs2 == dep.reg)) {
                std::cout << GREEN << "Load-Use Hazard: Instruction at PC=" << node.PC << " (" << textMap[node.PC].second << ") depends on load at PC=" << dep.pc << " (rd=" << dep.reg << ")" << RESET << std::endl;
                stats.stallBubbles++;
                stats.dataHazardStalls++;
                return true;
            }
        }
    }
    return false;
}

void Simulator::updateDependencies(InstructionNode& node, Stage stage) {
    if (stage == Stage::DECODE && node.rd != 0) {
        RegisterDependency dep;
        dep.stage = stage;
        dep.opcode = node.opcode;
        dep.value = 0;
        dep.reg = node.rd;
        dep.isLoad = node.isLoad;
        dep.uniqueId = node.uniqueId;
        dep.pc = node.PC;
        registerDependencies[node.uniqueId] = dep;
    }
    else if (stage == Stage::EXECUTE && node.rd != 0) {
        if (registerDependencies.find(node.uniqueId) != registerDependencies.end()) {
            registerDependencies[node.uniqueId].stage = stage;
            registerDependencies[node.uniqueId].value = instructionRegisters.RY;
        }
    }
    else if (stage == Stage::MEMORY && node.rd != 0) {
        if (registerDependencies.find(node.uniqueId) != registerDependencies.end()) {
            registerDependencies[node.uniqueId].stage = stage;
            registerDependencies[node.uniqueId].value = instructionRegisters.RZ;
        }
    } 
    else if (stage == Stage::WRITEBACK && node.rd != 0) {
        if (registerDependencies.find(node.uniqueId) != registerDependencies.end()) {
            registerDependencies.erase(node.uniqueId);
        }
    }
}

bool Simulator::isPipelineEmpty() const {
    for (const auto& pair : pipeline) {
        if (pair.second != nullptr) {
            return false;
        }
    }
    return true;
}

void Simulator::advancePipeline() {
    std::map<Stage, InstructionNode*> newPipeline;
    bool stalled = false;
    bool instructionProcessed = false;
    bool loadUseHazard = false;

    for (auto& pair : pipeline) {
        newPipeline[pair.first] = nullptr;
    }

    const std::unordered_map<uint32_t, RegisterDependency> depsSnapshot = registerDependencies;

    forwardingStatus = ForwardingStatus();

    for (const auto& stage : reverseStageOrder) {
        InstructionNode* node = pipeline[stage];
        if (node == nullptr) continue;

        if (node->stalled) {
            node->stalled = false;
            
            bool shouldStall = false;
            if (node->stage == Stage::FETCH && (stalled || loadUseHazard)) {
                shouldStall = true;
            } else if (node->stage == Stage::DECODE && (stalled || loadUseHazard || (!isDataForwarding && checkDependencies(*node, depsSnapshot)))) {
                shouldStall = true;
                if (!isDataForwarding && checkDependencies(*node, depsSnapshot)) {
                    stats.dataHazards++;
                    stats.stallBubbles++;
                    stats.dataHazardStalls++;
                    std::cout << YELLOW << "Stalling DECODE (resume) at PC=" + std::to_string(node->PC) + " due to RAW hazard" << RESET << std::endl;
                }
            } else if (node->stage == Stage::EXECUTE && loadUseHazard) {
                shouldStall = true;
            }

            if (shouldStall) {
                node->stalled = true;
                newPipeline[node->stage] = node;
                pipeline[stage] = nullptr;
                instructionProcessed = true;
                if (node->stage == Stage::DECODE || node->stage == Stage::EXECUTE) {
                    stalled = true;
                }
                continue;
            }
        }

        if (isFollowing && node->PC == followedInstruction) {
            std::cout << GREEN << "Cycle " << stats.totalCycles << ": Followed instruction at PC=0x" << std::hex << node->PC << std::dec << " (" << textMap[node->PC].second << ") completes " << stageToString(node->stage) << RESET << std::endl;
        }

        switch (node->stage) {
            case Stage::FETCH:
                {
                    if (stalled || loadUseHazard) {
                        node->stalled = true;
                        newPipeline[Stage::FETCH] = node;
                        pipeline[stage] = nullptr;
                        instructionProcessed = true;
                        continue;
                    }
                    instructionCount++;
                    fetchInstruction(node, PC, running, textMap);
                    if (running && node->instruction != 0) {
                        if (isPipeline && isBranchPrediction) {
                            bool predictedTaken = branchPredictor.predict(node->PC);
                            std::cout << YELLOW << (node->isBranch ? "Branch" : "Jump") + std::string(" predicted ") + (predictedTaken ? "taken" : "not taken") + " at PC=" + std::to_string(node->PC) + " (" + parseInstructions(node->instruction) + ")" << RESET << std::endl;
                            if (predictedTaken && branchPredictor.isInBTB(node->PC)) {
                                PC = branchPredictor.getTarget(node->PC);
                            }
                        }

                        node->stage = Stage::DECODE;
                        newPipeline[Stage::DECODE] = node;
                        pipeline[stage] = nullptr;
                        instructionProcessed = true;
                    }
                }
                break;
                
            case Stage::DECODE:
                {
                    if (stalled || loadUseHazard) {
                        node->stalled = true;
                        newPipeline[Stage::DECODE] = node;
                        pipeline[stage] = nullptr;
                        instructionProcessed = true;
                        stalled = true;
                        continue;
                    }

                    decodeInstruction(node, instructionRegisters, registers);

                    if (!isDataForwarding && checkDependencies(*node, depsSnapshot)) {
                        node->stalled = true;
                        newPipeline[Stage::DECODE] = node;
                        pipeline[stage] = nullptr;
                        instructionProcessed = true;
                        stalled = true;
                        stats.dataHazards++;
                        stats.stallBubbles++;
                        stats.dataHazardStalls++;
                        std::cout << YELLOW << "Stalling DECODE at PC=" + std::to_string(node->PC) + " due to RAW hazard" << RESET << std::endl;
                        continue;
                    }

                    uint32_t opcode = node->opcode & 0x7F;
                    if (node->instructionType == InstructionType::I && opcode == 0x03) {
                        stats.dataTransferInstructions++;
                    } else if (node->instructionType == InstructionType::S) {
                        stats.dataTransferInstructions++;
                    } else if (node->instructionType == InstructionType::R || (node->instructionType == InstructionType::I && opcode == 0x13) || node->instructionType == InstructionType::U) {
                        stats.aluInstructions++;
                    } else if (node->instructionType == InstructionType::SB || node->instructionType == InstructionType::UJ || (node->instructionType == InstructionType::I && opcode == 0x67)) {
                        stats.controlInstructions++;
                    }

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters.RA = instructionRegisters.RA;
                        followedInstructionRegisters.RB = instructionRegisters.RB;
                    }
                    
                    updateDependencies(*node, Stage::DECODE);
                    node->stage = Stage::EXECUTE;
                    newPipeline[Stage::EXECUTE] = node;
                    pipeline[stage] = nullptr;
                    instructionProcessed = true;
                }
                break;
                
            case Stage::EXECUTE:
                {
                    loadUseHazard = checkLoadUseHazard(*node, depsSnapshot, node->isStore);
                    if (loadUseHazard) {
                        node->stalled = true;
                        newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                        instructionProcessed = true;
                        continue;
                    }

                    applyDataForwarding(*node, depsSnapshot);

                    bool taken = false;
                    uint32_t oldPC = PC;
                    executeInstruction(node, instructionRegisters, registers, PC, taken, forwardingStatus);
                    updateDependencies(*node, Stage::EXECUTE);
                    
                    if (isPipeline && (node->isBranch || node->isJump)) {
                        bool predictedTaken = branchPredictor.getPHT(node->PC);
                        bool targetMismatch = false;
                    
                        if (predictedTaken && taken && branchPredictor.isInBTB(node->PC)) {
                            uint32_t predictedTarget = branchPredictor.getTarget(node->PC);
                            targetMismatch = (PC != predictedTarget);
                        }
                    
                        branchPredictor.update(node->PC, taken, PC);
                    
                        if (predictedTaken != taken || targetMismatch) {
                            flushPipeline(node->isBranch ? "Branch misprediction" : "Jump misprediction");
                            newPipeline[Stage::FETCH] = nullptr;
                            newPipeline[Stage::DECODE] = nullptr;
                            stats.controlHazards++;
                            stats.controlHazardStalls++;
                            
                            std::string misType = predictedTaken != taken ? "direction" : "target address";
                            std::string predictionDetails = predictedTaken ? "taken to " + std::to_string(branchPredictor.getTarget(node->PC)) : "not taken";

                            std::cout << YELLOW << (node->isBranch ? "Branch" : "Jump")
                            << " misprediction (" << misType << ") at PC=" << node->PC
                            << " (" << parseInstructions(node->instruction) << "), predicted: "
                            << predictionDetails << ", actual: "
                            << (taken ? "taken to " + std::to_string(PC) : "not taken")
                            << RESET << std::endl;

                        } else {
                            PC = oldPC;
                            std::cout << YELLOW << (node->isBranch ? "Branch" : "Jump")
                            << " correctly predicted at PC=" << node->PC
                            << ", restored PC=" << PC
                            << RESET << std::endl;
                        }
                    }
                    
                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters.RY = instructionRegisters.RY;
                        followedInstructionRegisters.RM = instructionRegisters.RM;
                    }
                    
                    node->stage = Stage::MEMORY;
                    newPipeline[Stage::MEMORY] = node;
                    pipeline[stage] = nullptr;
                    instructionProcessed = true;
                }
                break;
                
            case Stage::MEMORY:
                {
                    applyDataForwarding(*node, depsSnapshot);
                    memoryAccess(node, instructionRegisters, registers, dataMap);
                    updateDependencies(*node, Stage::MEMORY);

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters.RZ = instructionRegisters.RZ;
                    }

                    node->stage = Stage::WRITEBACK;
                    newPipeline[Stage::WRITEBACK] = node;
                    pipeline[stage] = nullptr;
                    instructionProcessed = true;
                }
                break;
                
            case Stage::WRITEBACK:
                {
                    writeback(node, instructionRegisters, registers);
                    updateDependencies(*node, Stage::WRITEBACK);
                    instructionProcessed = true;

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters = instructionRegisters;
                    }

                    delete node;
                    pipeline[Stage::WRITEBACK] = nullptr;
                    
                    if (!isPipeline && running && textMap.find(PC) != textMap.end()) {
                        bool pipelineEmpty = true;
                        for (const auto& [_, node] : newPipeline) {
                            if (node != nullptr) {
                                pipelineEmpty = false;
                                break;
                            }
                        }
                        if (pipelineEmpty) {
                            newPipeline[Stage::FETCH] = new InstructionNode(PC);
                            newPipeline[Stage::FETCH]->uniqueId = nextInstructionId++;
                        }
                    }
                }
                break;
        }
    }

    if (isPipeline && !stalled && newPipeline[Stage::FETCH] == nullptr && running && textMap.find(PC) != textMap.end()) {
        InstructionNode* newNode = new InstructionNode(PC);
        newNode->uniqueId = nextInstructionId++;
        newPipeline[Stage::FETCH] = newNode;
    }

    for (auto& pair : pipeline) {
        if (pair.second != nullptr) {
            delete pair.second;
            pair.second = nullptr;
        }
    }
    pipeline = newPipeline;

    bool isEmpty = isPipelineEmpty();
    if (isEmpty && !textMap.empty() && textMap.find(PC) == textMap.end()) {
        running = false;
    }

    if (instructionProcessed) {
        stats.totalCycles++;
        if (instructionCount > 0) {
            stats.cyclesPerInstruction = static_cast<double>(stats.totalCycles) / instructionCount;
        }
    }
}

bool Simulator::step() {
    try {
        advancePipeline();
        stats.instructionsExecuted = instructionCount;
        bool pipelineEmpty = isPipelineEmpty();
        if (!running && pipelineEmpty) {
            std::cout << GREEN << "Program execution completed" << RESET << std::endl;
            return false;
        }
        return true;
    }
    catch (const std::runtime_error &e) {
        std::cerr << RED << "Runtime error during step execution: " + std::string(e.what()) << RESET << std::endl;
        running = false;
        return false;
    }
}

void Simulator::run() {
    int stepCount = 0;
    while (step()) {   
        stepCount++;
        if (stepCount > MAX_STEPS) {
            std::cout << RED << "Program execution terminated - exceeded maximum step count (" + std::to_string(MAX_STEPS) + ")" << RESET;
            break;
        }
    }
}

void Simulator::setEnvironment(bool pipeline, bool dataForwarding, bool branchPrediction, uint32_t instruction) {
    isPipeline = pipeline;
    isDataForwarding = dataForwarding;
    isBranchPrediction = branchPrediction;
    followedInstruction = instruction;
    if(followedInstruction != UINT32_MAX) {
        isFollowing = true;
    }
}

std::map<uint32_t, std::pair<uint32_t, std::string>> Simulator::getTextMap() const {
    return textMap;
}

uint32_t Simulator::getCycles() const {
    return stats.totalCycles;
}

void Simulator::flushPipeline(const std::string& reason) {
    if (!isPipeline) return;
    std::vector<uint32_t> idsToRemove;
    
    if (pipeline[Stage::FETCH] != nullptr) {
        idsToRemove.push_back(pipeline[Stage::FETCH]->uniqueId);
        delete pipeline[Stage::FETCH];
        pipeline[Stage::FETCH] = nullptr;
    }
    
    if (pipeline[Stage::DECODE] != nullptr) {
        idsToRemove.push_back(pipeline[Stage::DECODE]->uniqueId);
        delete pipeline[Stage::DECODE];
        pipeline[Stage::DECODE] = nullptr;
    }

    for (const auto& id : idsToRemove) {
        registerDependencies.erase(id);
    }
    
    stats.pipelineFlushes++;
    std::cout << YELLOW << "Pipeline flushed: " + reason;
}

InstructionRegisters Simulator::getInstructionRegisters() const {
    return instructionRegisters;
}

InstructionRegisters Simulator::getFollowedInstructionRegisters() const {
    return followedInstructionRegisters;
}

const uint32_t *Simulator::getRegisters() const {
    return registers;
}

SimulationStats Simulator::getStats() {
    stats.branchMispredictions = branchPredictor.mispredictions;
    return stats;
}

uint32_t Simulator::getFollowedPC() const {
    return followedInstruction;
}

#endif