#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <stdexcept>
#include <iomanip>
#include "types.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include "execution.hpp"

class Simulator {
private:
    uint32_t PC;
    uint32_t registers[NUM_REGISTERS];

    std::unordered_map<uint32_t, uint8_t> dataMap;
    std::map<uint32_t, std::pair<uint32_t, std::string>> textMap;

    std::map<Stage, InstructionNode*> pipeline;
    InstructionRegisters instructionRegisters;
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
    void applyDataForwarding(InstructionNode& node);
    bool checkDependencies(const InstructionNode& node) const;
    void updateDependencies(InstructionNode& node, Stage stage);
    void setStageInstruction(Stage stage, InstructionNode* node);
    bool checkLoadUseHazard(const InstructionNode& node) const;
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
                         running(false),
                         isPipeline(true),
                         isDataForwarding(true),
                         isBranchPrediction(false),
                         isFollowing(false),
                         followedInstruction(UINT32_MAX),
                         stats(SimulationStats()),
                         instructionCount(0),
                         branchPredictor(BranchPredictor())
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

void Simulator::setStageInstruction(Stage stage, InstructionNode* node) {
    if (pipeline[stage] != nullptr) {
        delete pipeline[stage];
    }
    pipeline[stage] = node;
}

bool Simulator::loadProgram(const std::string &input) {
    try {
        bool wasPipeline = isPipeline;
        bool wasDataForwarding = isDataForwarding;
        
        reset();

        isPipeline = wasPipeline;
        isDataForwarding = wasDataForwarding;
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
        setStageInstruction(Stage::FETCH, firstNode);
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
    branchPredictor.reset();
    instructionCount = 0;
}

void Simulator::applyDataForwarding(InstructionNode& node) {
    if (!isPipeline || !isDataForwarding) return;

    if (node.stage == Stage::DECODE) {
        for (const auto& dep : registerDependencies) {
            if (dep.stage == Stage::MEMORY) {
                if (node.rs1 != 0 && node.rs1 == dep.reg) {
                    instructionRegisters.RA = instructionRegisters.RZ;
                    std::cout << YELLOW << "Data Forwarding: MEM->DE for rs1 (reg " + std::to_string(node.rs1) + ") of instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ")" << RESET << std::endl;
                }
                if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg) {
                    instructionRegisters.RB = instructionRegisters.RZ;
                    std::cout << YELLOW << "Data Forwarding: MEM->DE for rs2 (reg " + std::to_string(node.rs2) + ") of instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ")" << RESET << std::endl;
                }
            }
        }
        return;
    }

    for (const auto& dep : registerDependencies) {
        if (dep.stage == Stage::MEMORY) {
            if (node.rs1 != 0 && node.rs1 == dep.reg) {
                instructionRegisters.RA = instructionRegisters.RZ;
                std::cout << YELLOW << "Data Forwarding: MEM->EX for rs1 (reg " + std::to_string(node.rs1) + ") of instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ")" << RESET << std::endl;
            }
            if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg) {
                instructionRegisters.RB = instructionRegisters.RZ;
                std::cout << YELLOW << "Data Forwarding: MEM->EX for rs2 (reg " + std::to_string(node.rs2) + ") of instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ")" << RESET << std::endl;
            }
        }
    }

    for (const auto& dep : registerDependencies) {
        if (dep.stage == Stage::EXECUTE) {
            const uint32_t opcode = dep.opcode & 0x7F;
            const bool isLoad = (opcode == 0x03);
            if (!isLoad) {
                if (node.rs1 != 0 && node.rs1 == dep.reg) {
                    instructionRegisters.RA = instructionRegisters.RY;
                    std::cout << YELLOW << "Data Forwarding: EX->EX for rs1 (reg " + std::to_string(node.rs1) + ") of instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ")" << RESET << std::endl;
                }
                if ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == dep.reg) {
                    instructionRegisters.RB = instructionRegisters.RY;
                    std::cout << YELLOW << "Data Forwarding: EX->EX for rs2 (reg " + std::to_string(node.rs2) + ") of instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ")" << RESET << std::endl;
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

bool Simulator::checkLoadUseHazard(const InstructionNode& node) const {
    if (!isPipeline || !isDataForwarding) {
        return false;
    }

    if (pipeline.find(Stage::MEMORY) != pipeline.end() && pipeline.at(Stage::MEMORY) != nullptr) {
        InstructionNode* memNode = pipeline.at(Stage::MEMORY);
        uint32_t memOpcode = memNode->opcode & 0x7F;
        bool isLoad = (memOpcode == 0x03);
        if (isLoad && memNode->rd != 0 &&
            ((node.rs1 != 0 && node.rs1 == memNode->rd) || ((node.instructionType == InstructionType::R || node.instructionType == InstructionType::S || node.instructionType == InstructionType::SB) && node.rs2 != 0 && node.rs2 == memNode->rd))) {
            std::cout << GREEN << "Load-Use Hazard: Instruction at PC=" + std::to_string(node.PC) + " (" + parseInstructions(node.instruction) + ") depends on load at PC=" + std::to_string(memNode->PC) + " (rd=" + std::to_string(memNode->rd) + ")" << RESET << std::endl;
            return true;
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
            registerDependencies.push_back(dep);
        }
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
    bool instructionProcessed = false;
    bool stalled = false;
    
    for (auto& pair : pipeline) {
        newPipeline[pair.first] = nullptr;
    }

    const std::vector<Stage> stageOrder = {
        Stage::WRITEBACK, Stage::MEMORY, Stage::EXECUTE, Stage::DECODE, Stage::FETCH
    };

    for (const auto& stage : stageOrder) {
        InstructionNode* node = pipeline[stage];
        if (node == nullptr) continue;

        if (node->stalled) {
            node->stalled = false;

            switch (node->stage) {
                case Stage::DECODE:
                    node->stage = Stage::EXECUTE;
                    newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                    break;
                case Stage::EXECUTE:
                    node->stage = Stage::MEMORY;
                    newPipeline[Stage::MEMORY] = new InstructionNode(*node);
                    break;
                default:
                    newPipeline[node->stage] = new InstructionNode(*node);
                    break;
            }

            stats.stallBubbles++;
            instructionProcessed = true;
            if (stage == Stage::DECODE) stalled = true;
            continue;
        }

        switch (node->stage) {
            case Stage::FETCH:
                {
                    bool executeLoadUseStall = (isPipeline && pipeline[Stage::EXECUTE] != nullptr && checkLoadUseHazard(*pipeline[Stage::EXECUTE]));
                    if (executeLoadUseStall || (stalled && !isDataForwarding)) {
                        newPipeline[Stage::FETCH] = new InstructionNode(*node);
                        stats.stallBubbles++;
                        instructionProcessed = true;
                        continue;
                    }
                    instructionCount++;
                    fetchInstruction(node, PC, running, textMap);
                    if (running && node->instruction != 0) {
                        if (isPipeline && node->instructionType == InstructionType::SB && isBranchPrediction) {
                            if (branchPredictor.isInBTB(node->PC)) {
                                node->branchPredicted = branchPredictor.predict(node->PC);
                                std::cout << GREEN << "Branch Prediction for PC=" + std::to_string(node->PC) + ": Predicted " + (node->branchPredicted ? "Taken" : "Not Taken") + ", PHT=" + std::to_string(branchPredictor.getPHT(node->PC)) + ", BTB Target=" + std::to_string(branchPredictor.getTarget(node->PC)) << RESET << std::endl;
                                if (node->branchPredicted) {
                                    uint32_t target = branchPredictor.getTarget(node->PC);
                                    if (target != 0) {
                                        PC = target;
                                    }
                                }
                            } else {
                                node->branchPredicted = false;
                                std::cout << GREEN << "No Branch Prediction for PC=" + std::to_string(node->PC) + ": First encounter, assuming not taken" << RESET << std::endl;
                            }
                        }
                        node->stage = Stage::DECODE;
                        newPipeline[Stage::DECODE] = new InstructionNode(*node);
                        instructionProcessed = true;
                    }
                }
                break;
                
            case Stage::DECODE:
                {
                    bool executeLoadUseStall = (isPipeline && pipeline[Stage::EXECUTE] != nullptr && checkLoadUseHazard(*pipeline[Stage::EXECUTE]));
                    if (executeLoadUseStall) {
                        newPipeline[Stage::DECODE] = new InstructionNode(*node);
                        stats.stallBubbles++;
                        instructionProcessed = true;
                        continue;
                    }
                    decodeInstruction(node, instructionRegisters, registers);
                    if (isPipeline) {
                        applyDataForwarding(*node);
                    }
                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters.RA = instructionRegisters.RA;
                        followedInstructionRegisters.RB = instructionRegisters.RB;
                    }

                    updateDependencies(*node, Stage::DECODE);

                    if (isPipeline && checkDependencies(*node)) {
                        node->stalled = true;
                        stats.dataHazards++;
                        newPipeline[Stage::DECODE] = new InstructionNode(*node);
                        instructionProcessed = true;
                        continue;
                    }

                    node->stage = Stage::EXECUTE;
                    newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                    instructionProcessed = true;
                }
                break;
                
            case Stage::EXECUTE:
                {
                    if (isPipeline) {
                        if (checkLoadUseHazard(*node)) {
                            node->stalled = true;
                            stats.dataHazards++;
                            newPipeline[Stage::EXECUTE] = new InstructionNode(*node);
                            instructionProcessed = true;
                            stalled = true;
                            continue;
                        }
                        applyDataForwarding(*node);
                    }
                    
                    uint32_t oldPC = PC;
                    executeInstruction(node, instructionRegisters, registers, PC);

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters.RY = instructionRegisters.RY;
                    }

                    updateDependencies(*node, Stage::EXECUTE);

                    uint32_t opcode = node->opcode & 0x7F;
                    bool isJump = (opcode == 0x6F) || (opcode == 0x67);
                    bool isBranch = (opcode == 0x63);

                    if (isPipeline) {
                        if (isBranch) {
                            bool taken = (oldPC != PC);
                            uint32_t target = PC;
                            branchPredictor.update(node->PC, taken, target);
                            std::cout << GREEN << "Branch Update for PC=" + std::to_string(node->PC) + ": Taken=" + (taken ? "Yes" : "No") + ", New PHT=" + std::to_string(branchPredictor.getPHT(node->PC)) + ", BTB Updated to " + std::to_string(target) << RESET << std::endl;
                            
                            if (node->branchPredicted != taken) {
                                stats.branchMispredictions++;
                                flushPipeline("Branch misprediction");
                                stats.controlHazards++;
                                stats.controlHazardStalls += 2;
                                newPipeline[Stage::FETCH] = nullptr;
                                newPipeline[Stage::DECODE] = nullptr;
                            }
                        } 
                        else if (isJump) {
                            flushPipeline("Control hazard - jump taken");
                            stats.controlHazards++;
                            stats.controlHazardStalls += 2;
                            newPipeline[Stage::FETCH] = nullptr;
                            newPipeline[Stage::DECODE] = nullptr;
                        }
                    }
                    
                    node->stage = Stage::MEMORY;
                    newPipeline[Stage::MEMORY] = new InstructionNode(*node);
                    instructionProcessed = true;
                }
                break;
                
            case Stage::MEMORY:
                {
                    memoryAccess(node, instructionRegisters, registers, dataMap);

                    if (isFollowing && node->PC == followedInstruction) {
                        followedInstructionRegisters.RZ = instructionRegisters.RZ;
                    }

                    updateDependencies(*node, Stage::MEMORY);
                    
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
                    uint32_t opcode = node->opcode & 0x7F;
                    stats.instructionsExecuted++;
                    if (opcode == 0x03) stats.dataTransferInstructions++;
                    else if (opcode == 0x23) stats.dataTransferInstructions++;
                    else if (opcode == 0x63) stats.controlInstructions++;
                    else if (opcode == 0x67 || opcode == 0x6F) stats.controlInstructions++;
                    else stats.aluInstructions++;

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

    bool executeLoadUseStall = (isPipeline && pipeline[Stage::EXECUTE] != nullptr && checkLoadUseHazard(*pipeline[Stage::EXECUTE]));
    if (isPipeline && newPipeline[Stage::FETCH] == nullptr && textMap.find(PC) != textMap.end() && running) {
        if (!executeLoadUseStall && (!stalled || isDataForwarding)) {
            newPipeline[Stage::FETCH] = new InstructionNode(PC);
        } else {
            stats.stallBubbles++;
        }
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
    bool isEmpty = isPipelineEmpty();
    
    if (!running && isEmpty) {
        std::cerr << RED << "Cannot step - simulator is not running" << RESET << std::endl;
        return false;
    }
    
    try {
        advancePipeline();
        stats.instructionsExecuted = instructionCount;
        isEmpty = isPipelineEmpty();
        if (!running && isEmpty) {
            std::cout << GREEN << "Program execution completed" << RESET;
            if(isPipeline) {
                std::cout << GREEN << "Program execution completed. Stats: CPI=" + std::to_string(stats.cyclesPerInstruction) +
                        ", Instructions=" + std::to_string(stats.instructionsExecuted) +
                        ", Cycles=" + std::to_string(stats.totalCycles) +
                        ", Stalls=" + std::to_string(stats.stallBubbles) +
                        ", DataHazards=" + std::to_string(stats.dataHazards) +
                        ", ControlHazards=" + std::to_string(stats.controlHazards) +
                        ", Mispredictions=" + std::to_string(stats.branchMispredictions) << RESET;
            }
            std::cout << RESET << std::endl;
            return false;
        }

        if (textMap.find(PC) != textMap.end()) {
            running = true;
            if (pipeline[Stage::FETCH] == nullptr && running && 
                (isPipeline || isPipelineEmpty())) {
                setStageInstruction(Stage::FETCH, new InstructionNode(PC));
            }
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
    bool isEmpty = isPipelineEmpty();
    
    if (!running && isEmpty) {
        std::cerr << RED << "Cannot run - simulator is not running" << RESET << std::endl;
        return;
    }
    
    int stepCount = 0;
    while (running || !isEmpty) {
        if (!step())
            break;
            
        stepCount++;
        if (stepCount > MAX_STEPS) {
            std::cerr << RED << "Program execution terminated - exceeded maximum step count (" + std::to_string(MAX_STEPS) + ")" << RESET << std::endl;
            break;
        }
        isEmpty = isPipelineEmpty();
    }
    std::cout << GREEN << "Program execution completed" << RESET;
    if(isPipeline) {
        std::cout << GREEN << "Mispredictions: " + std::to_string(stats.branchMispredictions) + " Stalls: " + std::to_string(stats.stallBubbles) + " Data Hazards: " + std::to_string(stats.dataHazards) + " Control Hazards: " + std::to_string(stats.controlHazards) + " Pipeline Flushes: " + std::to_string(stats.pipelineFlushes) << RESET;
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

#endif