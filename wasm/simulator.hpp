#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <stack>
#include <iostream>
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

    std::stack<InstructionNode> pipeline;
    InstructionRegisters instructionRegisters;

    bool running;
    bool isPipeline;
    bool isDataForwarding;

    SimulationStats stats;
    // BranchPredictionUnit branchPredictor;
    std::vector<RegisterDependency> registerDependencies;

    uint32_t instructionCount;
    
    void advancePipeline();
    void flushPipeline(const std::string& reason = "");
    void applyDataForwarding(InstructionNode& node);

public:
    Simulator();
    bool loadProgram(const std::string &input);
    bool step();
    void run();
    void reset();
    void setEnvironment(bool pipeline, bool dataForwarding);
    bool isRunning() const;
    uint32_t getPC() const;
    const uint32_t *getRegisters() const;
    uint32_t getStalls() const;
    std::map<Stage, std::pair<bool, uint32_t>> getActiveStages() const;
    std::unordered_map<uint32_t, uint8_t> getDataMap() const;
    std::map<uint32_t, std::pair<uint32_t, std::string>> getTextMap() const;
    uint32_t getCycles() const;
    bool hasStallCondition() const;
    InstructionRegisters getInstructionRegisters() const;
    std::unordered_map<int, std::string> getLogs();
};

Simulator::Simulator() : PC(TEXT_SEGMENT_START),
                         running(false),
                         isPipeline(true),
                         isDataForwarding(true),
                         stats(SimulationStats()),
                        //  branchPredictor(BranchPredictionUnit()),
                         instructionCount(0)
{
    initialiseRegisters(registers);
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
            logs[300] = "Empty Code";
            return false;
        }

        Parser parser(tokenizedLines);
        if (!parser.parse()) {
            logs[404] = "Parsing failed with " + std::to_string(parser.getErrorCount()) + " errors";
            return false;
        }

        std::unordered_map<std::string, SymbolEntry> symbolTable = parser.getSymbolTable();
        std::vector<ParsedInstruction> parsedInstructions = parser.getParsedInstructions();

        Assembler assembler(symbolTable, parsedInstructions);
        if (!assembler.assemble()) {
            logs[404] = "Assembly failed with " + std::to_string(assembler.getErrorCount()) + " errors";
            return false;
        }

        for (const auto &[address, value] : assembler.getMachineCode()) {
            if (address >= DATA_SEGMENT_START) dataMap[address] = static_cast<uint8_t>(value);
            else textMap[address] = std::make_pair(value, parseInstructions(value));
        }
        
        PC = TEXT_SEGMENT_START;
        instructionCount = 0;
        logs[200] = "Program loaded successfully";
        return true;
    }
    catch (const std::exception &e) {
        logs[404] = "Error: " + std::string(e.what());
        return false;
    }
}

void Simulator::reset() {
    while (!pipeline.empty()) pipeline.pop();
    
    instructionRegisters = InstructionRegisters();
    initialiseRegisters(registers);
    registerDependencies.clear();
    dataMap.clear();
    textMap.clear();
    logs.clear();
    
    PC = TEXT_SEGMENT_START;
    running = false;
    stats = SimulationStats();
    // branchPredictor = BranchPredictionUnit();
    instructionCount = 0;
}

void Simulator::applyDataForwarding(InstructionNode& node) {
    if (!isPipeline || !isDataForwarding) return;

    for (const auto& dep : registerDependencies) {
        if (!dep.isWrite) continue;

        if (dep.stage == Stage::EXECUTE) {
            if (node.rs1 != 0 && node.rs1 == dep.reg) {
                uint32_t depOpcode = dep.opcode & 0x7F;
                if (depOpcode == 0x03) {
                    node.stalled = true;
                } else {
                    instructionRegisters.RA = instructionRegisters.RY;
                }
            }
            if (node.rs2 != 0 && node.rs2 == dep.reg) {
                uint32_t depOpcode = dep.opcode & 0x7F;
                if (depOpcode == 0x03) {
                    node.stalled = true;
                } else {
                    instructionRegisters.RB = instructionRegisters.RY;
                }
            }
        } else if (dep.stage == Stage::MEMORY) {
            if (node.rs1 != 0 && node.rs1 == dep.reg) {
                instructionRegisters.RA = instructionRegisters.RY;
            }
            if (node.rs2 != 0 && node.rs2 == dep.reg) {
                instructionRegisters.RB = instructionRegisters.RY;
            }
        }
    }
}

void Simulator::advancePipeline() {
    std::stack<InstructionNode> newPipeline;
    bool instructionProcessed = false;
    bool isFlushed = false;

    while (!pipeline.empty()) {
        InstructionNode node = pipeline.top();
        pipeline.pop();

        if (node.stalled) {
            node.stalled = false;
            newPipeline.push(node);
            stats.stallBubbles++;
            instructionProcessed = true;
            continue;
        }

        switch (node.stage) {
            case Stage::FETCH:
                {
                    instructionCount++;
                    fetchInstruction(&node, PC, running, textMap);
                    if (running) {
                        node.stage = Stage::DECODE;
                        newPipeline.push(node);
                        instructionProcessed = true;
                    }
                }
                break;
                
            case Stage::DECODE:
                {
                    decodeInstruction(&node, instructionRegisters, registers);

                    if (node.rd != 0) {
                        auto it = std::find_if(registerDependencies.begin(), registerDependencies.end(),[&node](const RegisterDependency& dep) { return dep.pc == node.PC; });
                        if (it != registerDependencies.end()) {
                            it->reg = node.rd;
                            it->stage = Stage::DECODE;
                            it->isWrite = true;
                            it->opcode = node.opcode;
                        } else {
                            RegisterDependency dep;
                            dep.reg = node.rd;
                            dep.pc = node.PC;
                            dep.isWrite = true;
                            dep.stage = Stage::DECODE;
                            dep.opcode = node.opcode;
                            registerDependencies.push_back(dep);
                        }
                    }

                    if (isPipeline && !isDataForwarding) {
                        for (const auto& dep : registerDependencies) {
                            if (dep.isWrite && (dep.stage == Stage::EXECUTE || dep.stage == Stage::MEMORY)) {
                                if ((node.rs1 != 0 && node.rs1 == dep.reg) || (node.rs2 != 0 && node.rs2 == dep.reg)) {
                                    node.stalled = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!node.stalled) {
                        node.stage = Stage::EXECUTE;
                    }
                    newPipeline.push(node);
                    instructionProcessed = true;
                }
                break;
                
            case Stage::EXECUTE:
                {
                    applyDataForwarding(node);
                    if (!node.stalled) {
                        uint32_t oldPC = PC;
                        executeInstruction(&node, instructionRegisters, registers, PC);
                        if (oldPC != PC) {
                            isFlushed = true;
                            flushPipeline("Control hazard - branch/jump taken");
                        }
                        for (auto& dep : registerDependencies) {
                            if (dep.pc == node.PC) {
                                dep.stage = Stage::EXECUTE;
                            }
                        }
                        node.stage = Stage::MEMORY;
                    }
                    newPipeline.push(node);
                    instructionProcessed = true;
                }
                break;
                
            case Stage::MEMORY:
                {
                    memoryAccess(&node, instructionRegisters, registers, dataMap);

                    for (auto& dep : registerDependencies) {
                        if (dep.pc == node.PC) {
                            dep.stage = Stage::MEMORY;
                        }
                    }
                    
                    node.stage = Stage::WRITEBACK;
                    newPipeline.push(node);
                    instructionProcessed = true;
                }
                break;
                
            case Stage::WRITEBACK:
                {
                    writeback(&node, instructionRegisters, registers);

                    registerDependencies.erase(
                        std::remove_if(registerDependencies.begin(), registerDependencies.end(), 
                                      [&node](const RegisterDependency& dep) { return dep.pc == node.PC; }),
                        registerDependencies.end()
                    );
                    
                    instructionProcessed = true;
                    uint32_t opcode = node.opcode & 0x7F;
                    if (opcode == 0x03 || opcode == 0x23) {
                        stats.dataTransferInstructions++;
                    }
                    else if (opcode == 0x63 || opcode == 0x67 || opcode == 0x6F) {
                        stats.controlInstructions++;
                    }
                    else {
                        stats.aluInstructions++;
                    }
                }
                break;
        }
    }

    bool fetchStageEmpty = getActiveStages()[Stage::FETCH].first;
    if (fetchStageEmpty && !isFlushed && running && textMap.find(PC) != textMap.end()) {
        InstructionNode newNode(PC);
        newPipeline.push(newNode);
    }
    
    while (!newPipeline.empty()) {
        pipeline.push(newPipeline.top());
        newPipeline.pop();
    }

    if (running && pipeline.empty() && textMap.find(PC) == textMap.end()) {
        running = false;
    }

    if (instructionProcessed) {
        stats.totalCycles++;
    }
    
    if (instructionCount > 0) {
        stats.cyclesPerInstruction = static_cast<double>(stats.totalCycles) / instructionCount;
    }
}

bool Simulator::step() {
    if (!running && pipeline.empty()) {
        logs[404] = "Cannot step - simulator is not running";
        return false;
    }
    
    try {
        advancePipeline();
        stats.instructionsExecuted = instructionCount;
        
        if (!running && pipeline.empty()) {
            return false;
        }
        return true;
    }
    catch (const std::runtime_error &e) {
        logs[404] = "Runtime error during step execution: " + std::string(e.what());
        running = false;
        return false;
    }
}

void Simulator::run() {
    if (!running) {
        logs[404] = "Cannot run - simulator is not running";
        return;
    }
    
    int stepCount = 0;
    while (running || !pipeline.empty()) {
        if (!step())
            break;
            
        stepCount++;
        if (stepCount > MAX_STEPS) {
            logs[400] = "Program execution terminated - exceeded maximum step count (" + std::to_string(MAX_STEPS) + ")";
            break;
        }
    }
    
    logs[200] = "Simulation completed. Total clock cycles: " + std::to_string(stats.totalCycles) + ", Total steps executed: " + std::to_string(stepCount);
}

void Simulator::setEnvironment(bool pipeline, bool dataForwarding) {
    isPipeline = pipeline;
    isDataForwarding = dataForwarding;
}

bool Simulator::isRunning() const {
    return running;
}

uint32_t Simulator::getPC() const {
    return PC;
}

const uint32_t *Simulator::getRegisters() const {
    return registers;
}

uint32_t Simulator::getStalls() const {
    return stats.stallBubbles;
}

std::map<Stage, std::pair<bool, uint32_t>> Simulator::getActiveStages() const {
    std::map<Stage, std::pair<bool, uint32_t>> activeStages;
    activeStages[Stage::FETCH] = std::make_pair(false, 0);
    activeStages[Stage::DECODE] = std::make_pair(false, 0);
    activeStages[Stage::EXECUTE] = std::make_pair(false, 0);
    activeStages[Stage::MEMORY] = std::make_pair(false, 0);
    activeStages[Stage::WRITEBACK] = std::make_pair(false, 0);

    std::stack<InstructionNode> tempPipeline = pipeline;
    while (!tempPipeline.empty()) {
        InstructionNode node = tempPipeline.top();
        tempPipeline.pop();
        activeStages[node.stage] = std::make_pair(true, node.PC);
    }
    return activeStages;
}

std::unordered_map<uint32_t, uint8_t> Simulator::getDataMap() const {
    return dataMap;
}

std::map<uint32_t, std::pair<uint32_t, std::string>> Simulator::getTextMap() const {
    return textMap;
}

uint32_t Simulator::getCycles() const {
    return stats.totalCycles;
}

void Simulator::flushPipeline(const std::string& reason) {
    if (!isPipeline) {
        return;
    }

    std::stack<InstructionNode> tempPipeline;
    while (!pipeline.empty()) {
        InstructionNode node = pipeline.top();
        pipeline.pop();
        if (node.stage == Stage::EXECUTE || node.stage == Stage::MEMORY || node.stage == Stage::WRITEBACK) {
            tempPipeline.push(node);
        }
    }
    while (!tempPipeline.empty()) {
        pipeline.push(tempPipeline.top());
        tempPipeline.pop();
    }
    
    stats.pipelineFlushes++;
    logs[300] = "Pipeline flushed: " + reason;
}

std::unordered_map<int, std::string> Simulator::getLogs() {
    std::unordered_map<int, std::string> result = logs;
    logs.clear();
    return result;
}

InstructionRegisters Simulator::getInstructionRegisters() const {
    return instructionRegisters;
}

#endif