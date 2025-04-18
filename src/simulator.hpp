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
    std::vector<RegisterDependency> registerDependencies;
    BranchPredictor branchPredictor;

    uint32_t instructionCount;

    void advancePipeline();
    void flushPipeline(const std::string& reason = "");
    void applyDataForwarding(InstructionNode& node, const std::vector<RegisterDependency>& depsSnapshot);
    bool checkDependencies(const InstructionNode& node) const;
    void updateDependencies(InstructionNode& node, Stage stage);
    bool checkLoadUseHazard(const InstructionNode& node, const std::vector<RegisterDependency>& dependencies);
    bool isPipelineEmpty() const;
    void reset();

public:
    Simulator();
    bool loadProgram(const std::string &input);
    bool step();
    void run();
    void setEnvironment(bool pipeline, bool dataForwarding, bool branchPrediction, uint32_t instruction);
    const uint32_t *getRegisters() const;
    std::map<uint32_t, std::pair<uint32_t, std::string>> getTextMap() const;
    uint32_t getCycles() const;
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
                         instructionCount(0)
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
        std::cout << GREEN << "Program loaded successfully" << RESET << std::endl;
        InstructionNode* firstNode = new InstructionNode(PC);
        pipeline[Stage::FETCH] = firstNode;
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

void Simulator::applyDataForwarding(InstructionNode& node, const std::vector<RegisterDependency>& depsSnapshot) {
    if (!isPipeline || !isDataForwarding) return;

    forwardingStatus = ForwardingStatus();

    for (const auto& dep : depsSnapshot) {
        if (dep.stage == Stage::EXECUTE && dep.reg != 0) {
            uint32_t opcode = dep.opcode & 0x7F;
            bool isLoad = (opcode == 0x03);
            if (!isLoad) {
                if (node.rs1 != 0 && node.rs1 == dep.reg) {
                    instructionRegisters.RA = dep.value;
                    forwardingStatus.raForwarded = true;

                    std::cout << YELLOW << "Data Forwarding: EX->EX for rs1 (reg " << node.rs1
                              << ") of instruction at PC=" << node.PC << " ("
                              << parseInstructions(node.instruction) << ")" << RESET << std::endl;
                }
                if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S ||
                     node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg) {
                    if (node.instructionType == InstructionType::S) {
                        instructionRegisters.RM = dep.value;
                        forwardingStatus.rmForwarded = true;

                        std::cout << YELLOW << "Data Forwarding: EX->EX for rs2 (reg " << node.rs2
                                  << ") to RM of instruction at PC=" << node.PC << " ("
                                  << parseInstructions(node.instruction) << ")" << RESET << std::endl;
                    } else {
                        instructionRegisters.RB = dep.value;
                        forwardingStatus.rbForwarded = true;

                        std::cout << YELLOW << "Data Forwarding: EX->EX for rs2 (reg " << node.rs2
                                  << ") of instruction at PC=" << node.PC << " ("
                                  << parseInstructions(node.instruction) << ")" << RESET << std::endl;
                    }
                }
            }
        }
    }

    for (const auto& dep : depsSnapshot) {
        if (dep.stage == Stage::MEMORY && dep.reg != 0) {
            uint32_t opcode = dep.opcode & 0x7F;
            bool isLoad = (opcode == 0x03);

            if (node.rs1 != 0 && node.rs1 == dep.reg && !forwardingStatus.raForwarded) {
                instructionRegisters.RA = dep.value;
                forwardingStatus.raForwarded = true;

                std::cout << YELLOW << "Data Forwarding: MEM->EX for rs1 (reg " << node.rs1
                          << ") of instruction at PC=" << node.PC << " ("
                          << parseInstructions(node.instruction) << ")"
                          << (isLoad ? " [Load]" : "") << RESET << std::endl;
            }

            if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S ||
                 node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg &&
                !forwardingStatus.rbForwarded && !forwardingStatus.rmForwarded) {
                if (node.instructionType == InstructionType::S) {
                    instructionRegisters.RM = dep.value;
                    forwardingStatus.rmForwarded = true;

                    std::cout << YELLOW << "Data Forwarding: MEM->EX for rs2 (reg " << node.rs2
                              << ") to RM of instruction at PC=" << node.PC << " ("
                              << parseInstructions(node.instruction) << ")"
                              << (isLoad ? " [Load]" : "") << RESET << std::endl;
                } else {
                    instructionRegisters.RB = dep.value;
                    forwardingStatus.rbForwarded = true;

                    std::cout << YELLOW << "Data Forwarding: MEM->EX for rs2 (reg " << node.rs2
                              << ") of instruction at PC=" << node.PC << " ("
                              << parseInstructions(node.instruction) << ")"
                              << (isLoad ? " [Load]" : "") << RESET << std::endl;
                }
            }
        }
    }
}

bool Simulator::checkDependencies(const InstructionNode& node) const {
    if (!isPipeline || isDataForwarding) {
        return false;
    }
    
    for (const auto& dep : registerDependencies) {
        if (dep.stage == Stage::EXECUTE || dep.stage == Stage::MEMORY) {
            if ((node.rs1 != 0 && node.rs1 == dep.reg) || 
                ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg)) {
                std::cout << YELLOW << "Data Hazard: Instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ") depends on reg " + std::to_string(dep.reg) + " in " + stageToString(dep.stage) << RESET << std::endl;
                return true;
            }
        }
    }
    return false;
}

bool Simulator::checkLoadUseHazard(const InstructionNode& node, const std::vector<RegisterDependency>& dependencies) {
    if (!isPipeline) {
        return false;
    }

    uint32_t instruction = node.instruction;
    uint32_t opcode = instruction & 0x7F;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;

    bool hasRS2 = false;
    switch (opcode) {
        case 0x33:
        case 0x23:
        case 0x63:
            hasRS2 = true;
            break;
        case 0x03:
        case 0x13:
        case 0x67:
            hasRS2 = false;
            break;
        case 0x17:
        case 0x37:
        case 0x6F:
            rs1 = 0;
            hasRS2 = false;
            break;
        default:
            return false;
    }

    for (const auto& dep : dependencies) {
        if (dep.stage == Stage::EXECUTE && (dep.opcode & 0x7F) == 0x03) {
            if ((rs1 != 0 && rs1 == dep.reg) || (hasRS2 && rs2 != 0 && rs2 == dep.reg)) {
                std::cout << GREEN << "Load-Use Hazard: Instruction at PC=" << node.PC << " ("
                          << parseInstructions(instruction) << ") depends on load at PC=" << dep.pc
                          << " (rd=" << dep.reg << ")" << RESET << std::endl;

                stats.stallBubbles++;
                stats.dataHazardStalls++;
                return true;
            }
        }
    }
    return false;
}

void Simulator::updateDependencies(InstructionNode& node, Stage stage) {
    auto it = std::find_if(
        registerDependencies.begin(), 
        registerDependencies.end(),
        [&node](const RegisterDependency& dep) { return dep.pc == node.PC; }
    );
    
    if (stage == Stage::DECODE && node.rd != 0) {
        if (it != registerDependencies.end()) {
            it->reg = node.rd;
            it->stage = stage;
            it->opcode = node.opcode;
        } else {
            RegisterDependency dep;
            dep.reg = node.rd;
            dep.pc = node.PC;
            dep.stage = stage;
            dep.opcode = node.opcode;
            dep.value = 0;
            registerDependencies.push_back(dep);
        }
    } else if (stage == Stage::EXECUTE && it != registerDependencies.end()) {
        it->stage = stage;
        it->value = instructionRegisters.RY;
    } else if (stage == Stage::MEMORY && it != registerDependencies.end()) {
        it->stage = stage;
        it->value = instructionRegisters.RZ;
    } else if (it != registerDependencies.end()) {
        it->stage = stage;
    }
    
    if (stage == Stage::WRITEBACK) {
        registerDependencies.erase(
            std::remove_if(
                registerDependencies.begin(), 
                registerDependencies.end(), 
                [&node](const RegisterDependency& dep) { return dep.pc == node.PC; }
            ),
            registerDependencies.end()
        );
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

    std::vector<RegisterDependency> depsSnapshot = registerDependencies;

    forwardingStatus = ForwardingStatus();

    for (const auto& stage : reverseStageOrder) {
        InstructionNode* node = pipeline[stage];
        if (node == nullptr) continue;

        if (node->stalled) {
            node->stalled = false;
            
            bool shouldStall = false;
            if (node->stage == Stage::FETCH && (stalled || loadUseHazard)) {
                shouldStall = true;
            } else if (node->stage == Stage::DECODE && (stalled || loadUseHazard || (!isDataForwarding && checkDependencies(*node)))) {
                shouldStall = true;
                if (!isDataForwarding && checkDependencies(*node)) {
                    stats.dataHazards++;
                    stats.stallBubbles++;
                    stats.dataHazardStalls++;
                    std::cout << YELLOW << "Stalling DECODE (resume) at PC=" + std::to_string(node->PC) + " due to RAW hazard" << RESET << std::endl;
                }
            } else if (node->stage == Stage::EXECUTE && (loadUseHazard || (!isDataForwarding && checkDependencies(*node)))) {
                shouldStall = true;
                if (!isDataForwarding && checkDependencies(*node)) {
                    stats.dataHazards++;
                    stats.stallBubbles++;
                    stats.dataHazardStalls++;
                    std::cout << YELLOW << "Stalling EXECUTE (resume) at PC=" + std::to_string(node->PC) + " due to RAW hazard" << RESET << std::endl;
                }
            }

            if (shouldStall) {
                node->stalled = true;
                newPipeline[node->stage] = new InstructionNode(*node);
                instructionProcessed = true;
                if (node->stage == Stage::DECODE || node->stage == Stage::EXECUTE) {
                    stalled = true;
                }
                continue;
            }

            if (node->stage == Stage::FETCH) {
                instructionCount++;
                fetchInstruction(node, PC, running, textMap);
                if (running && node->instruction != 0) {
                    if ((node->isBranch || node->isJump) && isPipeline && isBranchPrediction) {
                        bool predictedTaken = branchPredictor.predict(node->PC);
                        std::cout << YELLOW << (node->isBranch ? "Branch" : "Jump") + std::string(" predicted ") + (predictedTaken ? "taken" : "not taken") + " at PC=" + std::to_string(node->PC) + " (" + parseInstructions(node->instruction) + ")" << RESET << std::endl;
                        if (predictedTaken && branchPredictor.isInBTB(node->PC)) {
                            PC = branchPredictor.getTarget(node->PC);
                        }
                    }

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters = instructionRegisters;
                    }

                    node->stage = Stage::DECODE;
                    newPipeline[Stage::DECODE] = new InstructionNode(*node);
                    instructionProcessed = true;
                    continue;
                }
            } else if (node->stage == Stage::DECODE) {
                decodeInstruction(node, instructionRegisters, registers);

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
                    followedInstructionRegisters = instructionRegisters;
                }

                updateDependencies(*node, Stage::DECODE);
                node->stage = Stage::EXECUTE;
                newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                instructionProcessed = true;
            } else if (node->stage == Stage::EXECUTE) {
                applyDataForwarding(*node, depsSnapshot);

                bool taken = false;
                executeInstruction(node, instructionRegisters, registers, PC, taken);
                updateDependencies(*node, Stage::EXECUTE);
            
                if (isPipeline && isBranchPrediction && (node->isBranch || node->isJump)) {
                    bool predictedTaken = branchPredictor.getPHT(node->PC);

                    if(node->instructionName == Instructions::JALR) {
                        branchPredictor.update(node->PC, taken, (instructionRegisters.RA + instructionRegisters.RB) & ~1);
                    } else {
                        branchPredictor.update(node->PC, taken, (node->PC + instructionRegisters.RB));
                    }

                    if (predictedTaken != taken) {
                        flushPipeline(node->isBranch ? "Branch misprediction" : "Jump misprediction");
                        newPipeline[Stage::FETCH] = nullptr;
                        newPipeline[Stage::DECODE] = nullptr;
                        stats.controlHazards++;
                        stats.controlHazardStalls++;
                        std::cout << YELLOW << (node->isBranch ? "Branch" : "Jump") + std::string(" misprediction at PC=") + std::to_string(node->PC) + " (" + parseInstructions(node->instruction) + "), actual: " + (taken || node->isJump ? "taken to " + std::to_string(PC) : "not taken") << RESET << std::endl;
                    }
                }

                if (isFollowing && node->PC == followedInstruction) {
                    followedInstructionRegisters = instructionRegisters;
                }

                node->stage = Stage::MEMORY;
                newPipeline[Stage::MEMORY] = new InstructionNode(*node);
                instructionProcessed = true;
            } else {
                newPipeline[node->stage] = new InstructionNode(*node);
                instructionProcessed = true;
            }
            continue;
        }

        switch (node->stage) {
            case Stage::FETCH:
                {
                    if (stalled || loadUseHazard) {
                        node->stalled = true;
                        newPipeline[Stage::FETCH] = new InstructionNode(*node);
                        instructionProcessed = true;
                        continue;
                    }
                    instructionCount++;
                    fetchInstruction(node, PC, running, textMap);
                    if (running && node->instruction != 0) {
                        if ((node->isBranch || node->isJump) && isPipeline && isBranchPrediction) {
                            bool predictedTaken = branchPredictor.predict(node->PC);
                            std::cout << YELLOW << (node->isBranch ? "Branch" : "Jump") + std::string(" predicted ") + (predictedTaken ? "taken" : "not taken") + " at PC=" + std::to_string(node->PC) + " (" + parseInstructions(node->instruction) + ")" << RESET << std::endl;
                            if (predictedTaken && branchPredictor.isInBTB(node->PC)) {
                                PC = branchPredictor.getTarget(node->PC);
                            }
                        }

                        if (isFollowing && node->PC == followedInstruction) {
                            followedInstructionRegisters = instructionRegisters;
                        }

                        node->stage = Stage::DECODE;
                        newPipeline[Stage::DECODE] = new InstructionNode(*node);
                        instructionProcessed = true;
                    }
                }
                break;
                
            case Stage::DECODE:
                {
                    if (stalled || loadUseHazard) {
                        node->stalled = true;
                        newPipeline[Stage::DECODE] = new InstructionNode(*node);
                        instructionProcessed = true;
                        stalled = true;
                        continue;
                    }

                    decodeInstruction(node, instructionRegisters, registers);

                    if (!isDataForwarding && checkDependencies(*node)) {
                        node->stalled = true;
                        newPipeline[Stage::DECODE] = new InstructionNode(*node);
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
                        followedInstructionRegisters = instructionRegisters;
                    }
                    
                    updateDependencies(*node, Stage::DECODE);
                    node->stage = Stage::EXECUTE;
                    newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                    instructionProcessed = true;
                }
                break;
                
            case Stage::EXECUTE:
                {
                    loadUseHazard = checkLoadUseHazard(*node, depsSnapshot);
                    if (loadUseHazard) {
                        node->stalled = true;
                        newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                        instructionProcessed = true;
                        continue;
                    }
                    if (!isDataForwarding && checkDependencies(*node)) {
                        node->stalled = true;
                        newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                        instructionProcessed = true;
                        stalled = true;
                        stats.dataHazards++;
                        stats.stallBubbles++;
                        stats.dataHazardStalls++;
                        std::cout << YELLOW << "Stalling EXECUTE at PC=" + std::to_string(node->PC) + " due to RAW hazard" << RESET << std::endl;
                        continue;
                    }
                    applyDataForwarding(*node, depsSnapshot);

                    bool taken = false;
                    executeInstruction(node, instructionRegisters, registers, PC, taken);
                    updateDependencies(*node, Stage::EXECUTE);
                    
                    if (isPipeline && isBranchPrediction && (node->isBranch || node->isJump)) {
                        bool predictedTaken = branchPredictor.getPHT(node->PC);
                        if(node->instructionName == Instructions::JALR) {
                            branchPredictor.update(node->PC, taken, (instructionRegisters.RA + instructionRegisters.RB) & ~1);
                        } else {
                            branchPredictor.update(node->PC, taken, (node->PC + instructionRegisters.RB));
                        }

                        if (predictedTaken != taken) {
                            flushPipeline(node->isBranch ? "Branch misprediction" : "Jump misprediction");
                            newPipeline[Stage::FETCH] = nullptr;
                            newPipeline[Stage::DECODE] = nullptr;
                            stats.controlHazards++;
                            stats.controlHazardStalls++;
                            std::cout << YELLOW << (node->isBranch ? "Branch" : "Jump") + std::string(" misprediction at PC=") + std::to_string(node->PC) + " (" + parseInstructions(node->instruction) + "), actual: " + (taken || node->isJump ? "taken to " + std::to_string(PC) : "not taken") << RESET << std::endl;
                        }
                    }

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters = instructionRegisters;
                    }
                    
                    node->stage = Stage::MEMORY;
                    newPipeline[Stage::MEMORY] = new InstructionNode(*node);
                    instructionProcessed = true;
                }
                break;
                
            case Stage::MEMORY:
                {
                    memoryAccess(node, instructionRegisters, registers, dataMap);
                    updateDependencies(*node, Stage::MEMORY);

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters = instructionRegisters;
                    }

                    node->stage = Stage::WRITEBACK;
                    newPipeline[Stage::WRITEBACK] = new InstructionNode(*node);
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
                        }
                    }
                }
                break;
        }
    }

    if (isPipeline && !stalled && newPipeline[Stage::FETCH] == nullptr && running && textMap.find(PC) != textMap.end()) {
        newPipeline[Stage::FETCH] = new InstructionNode(PC);
    }

    for (auto& pair : pipeline) {
        if (pair.second != nullptr) {
            delete pair.second;
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
            std::cout << GREEN << "Program execution completed" << RESET;
            if(isPipeline) {
                std::cout << GREEN << ". Stats: CPI=" << std::fixed << std::setprecision(2) << stats.cyclesPerInstruction
                          << ", Instructions=" << stats.instructionsExecuted
                          << ", Cycles=" << stats.totalCycles
                          << ", Stalls=" << stats.stallBubbles
                          << ", DataHazards=" << stats.dataHazards
                          << ", ControlHazards=" << stats.controlHazards
                          << ", DataHazardStalls=" << stats.dataHazardStalls
                          << ", ControlHazardStalls=" << stats.controlHazardStalls
                          << ", PipelineFlushes=" << stats.pipelineFlushes
                          << ", DataTransferInstructions=" << stats.dataTransferInstructions
                          << ", ALUInstructions=" << stats.aluInstructions
                          << ", ControlInstructions=" << stats.controlInstructions
                          << ", BranchPredictionAccuracy=" << std::fixed << std::setprecision(2) << branchPredictor.getAccuracy() << "%" << RESET;
            }
            std::cout << RESET << std::endl;
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
    std::cout << GREEN << "Program execution completed" << RESET;
    if(isPipeline) {
        std::cout << GREEN << "\nStats:"
                  << "\nCPI: " << std::fixed << std::setprecision(2) << stats.cyclesPerInstruction
                  << "\nInstructions: " << stats.instructionsExecuted
                  << "\nCycles: " << stats.totalCycles
                  << "\nStalls: " << stats.stallBubbles
                  << "\nData Hazards: " << stats.dataHazards
                  << "\nControl Hazards: " << stats.controlHazards
                  << "\nData Hazard Stalls: " << stats.dataHazardStalls
                  << "\nControl Hazard Stalls: " << stats.controlHazardStalls
                  << "\nPipeline Flushes: " << stats.pipelineFlushes
                  << "\nData Transfer Instructions: " << stats.dataTransferInstructions
                  << "\nALU Instructions: " << stats.aluInstructions
                  << "\nControl Instructions: " << stats.controlInstructions
                  << "\nBranch Prediction Accuracy: " << std::fixed << std::setprecision(2) << branchPredictor.getAccuracy() << "%"
                  << RESET;
    }
    std::cout << RESET << std::endl;
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
    
    if (pipeline[Stage::FETCH] != nullptr) {
        delete pipeline[Stage::FETCH];
        pipeline[Stage::FETCH] = nullptr;
    }
    
    if (pipeline[Stage::DECODE] != nullptr) {
        delete pipeline[Stage::DECODE];
        pipeline[Stage::DECODE] = nullptr;
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

#endif