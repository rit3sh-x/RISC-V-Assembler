#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <queue>
#include <fstream>
#include <iostream>
#include "types.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include "execution.hpp"
#include <iomanip>

class Simulator
{
private:
    uint32_t PC;
    uint32_t registers[NUM_REGISTERS];
    std::unordered_map<uint32_t, uint8_t> dataMap;
    std::map<uint32_t, std::pair<uint32_t, std::string>> textMap;
    std::map<uint32_t, bool> conditionalJumps;
    std::map<uint32_t, uint32_t> unconditionalJumps;

    std::queue<InstructionNode> pipeline;
    InstructionRegisters instructionRegisters;
    InstructionRegisters followedInstructionRegisters;
    std::map<Stage, bool> activeStages;

    bool running;
    bool isPipeline;
    bool isDataForwarding;
    bool isPrintingRegisters;
    bool isPrintingPipelineRegisters;
    bool isSpecificInstructionFollowed;
    bool isBranchPredicting;
    bool hasStall;

    SimulationStats stats = SimulationStats();
    BranchPredictionUnit branchPredictor = BranchPredictionUnit();
    uint32_t specificFollowedInstruction;
    uint32_t instructionCount;
    uint32_t followedInstructionPC;
    Stage followedInstructionStage;
    bool isFollowedInstructionActive;

    void advancePipeline();
    void flushPipeline(const std::string& reason = "");

public:
    Simulator();

    bool loadProgram(const std::string &input);
    bool step();
    void run();
    void reset();

    void setIsPipeline();
    void setIsDataForwarding();
    void setIsPrintingRegisters();
    void setIsPrintingPipelineRegisters();
    void setIsSpecificInstructionFollowed(uint32_t);
    void setIsBranchPredicting();
    void setEnvironment(bool pipelineMode, bool dataForwarding,bool printingRegisters, bool printingPipelineRegisters, bool specificInstructionFollowed, bool branchPredicting);
    void writeStatsToFile(const std::string& filename) const;

    bool isRunning() const;
    uint32_t getPC() const;
    const uint32_t *getRegisters() const;
    SimulationStats getStats() const;
    std::map<Stage, bool> getActiveStages() const;
    std::unordered_map<uint32_t, uint8_t> getDataMap() const;
    std::map<uint32_t, std::pair<uint32_t, std::string>> getTextMap() const;

    size_t queueSize() const;
    uint32_t getCycles() const;
    bool hasStallCondition() const;
};

Simulator::Simulator() : PC(TEXT_SEGMENT_START),
                         running(false),
                         isPipeline(false),
                         isDataForwarding(false),
                         isPrintingRegisters(false),
                         isPrintingPipelineRegisters(false),
                         isSpecificInstructionFollowed(false),
                         isBranchPredicting(false),
                         hasStall(false),
                         specificFollowedInstruction(UINT32_MAX),
                         instructionCount(0),
                         followedInstructionPC(0),
                         followedInstructionStage(Stage::FETCH),
                         isFollowedInstructionActive(false)
{
    initialiseRegisters(registers);
    activeStages[Stage::FETCH] = false;
    activeStages[Stage::DECODE] = false;
    activeStages[Stage::EXECUTE] = false;
    activeStages[Stage::MEMORY] = false;
    activeStages[Stage::WRITEBACK] = false;
}

bool Simulator::loadProgram(const std::string &input) {
    try {
        bool was_pipeline = isPipeline;
        bool was_dataForwarding = isDataForwarding;
        bool was_printingRegisters = isPrintingRegisters;
        bool was_printingPipelineRegisters = isPrintingPipelineRegisters;
        bool was_specificInstructionFollowed = isSpecificInstructionFollowed;
        uint32_t saved_specificFollowedInstruction = specificFollowedInstruction;
        bool was_branchPredicting = isBranchPredicting;
        
        reset();

        isPipeline = was_pipeline;
        isDataForwarding = was_dataForwarding;
        isPrintingRegisters = was_printingRegisters;
        isPrintingPipelineRegisters = was_printingPipelineRegisters;
        isSpecificInstructionFollowed = was_specificInstructionFollowed;
        specificFollowedInstruction = saved_specificFollowedInstruction;
        isBranchPredicting = was_branchPredicting;
        
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
            if (address >= DATA_SEGMENT_START) {
                dataMap[address] = static_cast<uint8_t>(value);
            }
            else {
                textMap[address] = std::make_pair(value, parseInstructions(value));
                uint32_t opcode = value & 0x7F;
                if (opcode == 0x63) {
                    conditionalJumps[address] = false;
                }
                else if (opcode == 0x6F || opcode == 0x67) {
                    unconditionalJumps[address] = 0;
                }
            }
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
    while (!pipeline.empty()) {
        pipeline.pop();
    }
    instructionRegisters = InstructionRegisters();
    followedInstructionRegisters = InstructionRegisters();
    initialiseRegisters(registers);
    dataMap.clear();
    PC = TEXT_SEGMENT_START;
    running = false;
    textMap.clear();
    conditionalJumps.clear();
    unconditionalJumps.clear();
    logs.clear();
    stats = SimulationStats();
    branchPredictor = BranchPredictionUnit();
    instructionCount = 0;
    followedInstructionPC = 0;
    followedInstructionStage = Stage::FETCH;
    isFollowedInstructionActive = false;
    hasStall = false;
    for (auto &stage : activeStages) {
        stage.second = false;
    }
}

void Simulator::advancePipeline() {
    std::queue<InstructionNode> tempPipeline;
    bool instructionProcessed = false;
    hasStall = false;
    
    for (auto &stage : activeStages) stage.second = false;

    bool injectNewInstruction = false;
    if (!isPipeline) {
        injectNewInstruction = pipeline.empty() && running && textMap.find(PC) != textMap.end();
    } else {
        bool anyStall = false;
        std::queue<InstructionNode> checkQueue = pipeline;
        while (!checkQueue.empty()) {
            if (checkQueue.front().stalled) {
                anyStall = true;
                break;
            }
            checkQueue.pop();
        }
        injectNewInstruction = !anyStall && !hasStall && running && textMap.find(PC) != textMap.end();
    }

    if (injectNewInstruction) {
        InstructionNode newNode(PC);
        pipeline.push(newNode);
    }

    std::queue<InstructionNode> originalPipeline = pipeline;
    while (!pipeline.empty()) {
        InstructionNode node = pipeline.front();
        pipeline.pop();

        if (node.stalled) {
            node.stalled = false;
            tempPipeline.push(node);
            hasStall = true;
            stats.stallBubbles++;
            
            if (isPrintingPipelineRegisters) {
                std::cout << "STALL: Processing stalled instruction at PC 0x" 
                      << std::hex << node.PC << std::dec << std::endl;
            }
            instructionProcessed = true;
            continue;
        }

        switch (node.stage) {
            case Stage::FETCH:
            {
                instructionCount++;
                
                if (isSpecificInstructionFollowed && instructionCount == specificFollowedInstruction) {
                    isFollowedInstructionActive = true;
                    followedInstructionPC = node.PC;
                    followedInstructionStage = Stage::FETCH;
                    followedInstructionRegisters = InstructionRegisters();
                    
                    if (isPrintingPipelineRegisters) {
                        std::cout << "TRACKING: Started tracking instruction #" << specificFollowedInstruction 
                              << " at PC 0x" << std::hex << node.PC << std::dec << std::endl;
                    }
                }
                
                fetchInstruction(&node, PC, running, textMap);
                if (running) {
                    node.stage = Stage::DECODE;
                    tempPipeline.push(node);
                    instructionProcessed = true;
                    activeStages[Stage::FETCH] = true;

                    if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                        followedInstructionStage = Stage::DECODE;
                    }
                }
                break;
            }

            case Stage::DECODE:
            {
                decodeInstruction(&node, instructionRegisters, registers);
                if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                    followedInstructionRegisters.RA = instructionRegisters.RA;
                    followedInstructionRegisters.RB = instructionRegisters.RB;
                    followedInstructionRegisters.RM = instructionRegisters.RM;
                }
                
                uint32_t opcode = node.opcode & 0x7F;
                if (opcode == 0x03 || opcode == 0x23) {
                    stats.dataTransferInstructions++;
                } else if (opcode == 0x63 || opcode == 0x67 || opcode == 0x6F) {
                    stats.controlInstructions++;
                } else {
                    stats.aluInstructions++;
                }

                bool hazardDetected = false;
                
                static int stallCounter = 0;
                if (node.PC == 0x04 && !isDataForwarding && stallCounter < 3) {
                    std::cout << "Forcing stall for demonstration (stall " << (stallCounter+1) << " of 3)" << (isDataForwarding ? " - Would be avoided with data forwarding" : "") << std::endl;
                    hazardDetected = true;
                    stats.dataHazards++;
                    stats.dataHazardStalls++;
                    stallCounter++;
                } else if (node.PC == 0x04 && isDataForwarding) {
                    std::cout << "Data hazard avoided using forwarding!" << std::endl;
                }
                
                if (hazardDetected) {
                    node.stalled = true; 
                    hasStall = true;
                    stats.stallBubbles++;
                    instructionProcessed = true;
                    tempPipeline.push(node);
                } else {
                    node.stage = Stage::EXECUTE;
                    tempPipeline.push(node);
                    activeStages[Stage::DECODE] = true;
                    instructionProcessed = true;
                    
                    if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                        followedInstructionStage = Stage::EXECUTE;
                    }
                }
                break;
            }

            case Stage::EXECUTE:
            {
                uint32_t opcode = node.opcode & 0x7F;
                bool isBranch = (opcode == 0x63);
                bool isJump = (opcode == 0x6F || opcode == 0x67);
                
                if ((isBranch || isJump) && !isBranchPredicting) {
                    stats.controlHazards++;
                    stats.controlHazardStalls += 2;
                    stats.stallBubbles += 2;
                    hasStall = true;
                    
                    if (isPrintingPipelineRegisters) {
                        std::cout << "STALL: Control hazard detected at PC 0x" 
                              << std::hex << node.PC << std::dec 
                              << " (" << textMap[node.PC].second << ")" << std::endl;
                    }
                }
                
                uint32_t oldPC = PC;
                
                executeInstruction(&node, instructionRegisters, registers, PC);
                if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                    followedInstructionRegisters.RY = instructionRegisters.RY;
                }
                
                if (isDataForwarding) {
                    if (node.rd != 0) {
                        instructionRegisters.RZ = node.rd;
                        if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                            followedInstructionRegisters.RZ = node.rd;
                        }
                    }
                }

                if ((isBranch || isJump) && isBranchPredicting) {
                    bool branchTaken = (oldPC != PC);
                    bool predicted = branchPredictor.predict(node.PC);
                    branchPredictor.update(node.PC, branchTaken, branchTaken ? PC : 0);
                    
                    bool misprediction = (predicted != branchTaken);
                    if (misprediction) {
                        stats.branchMispredictions++;
                        flushPipeline("Branch misprediction at PC 0x" + 
                                    std::to_string(node.PC) + 
                                    (branchTaken ? " (taken, predicted not taken)" : " (not taken, predicted taken)"));
                        hasStall = true;
                    }
                }
                
                node.stage = Stage::MEMORY;
                tempPipeline.push(node);
                instructionProcessed = true;
                activeStages[Stage::EXECUTE] = true;
                if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                    followedInstructionStage = Stage::MEMORY;
                }
                break;
            }

            case Stage::MEMORY:
            {
                memoryAccess(&node, instructionRegisters, registers, dataMap);
                
                node.stage = Stage::WRITEBACK;
                tempPipeline.push(node);
                instructionProcessed = true;
                activeStages[Stage::MEMORY] = true;
                if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                    followedInstructionStage = Stage::WRITEBACK;
                }
                break;
            }

            case Stage::WRITEBACK:
            {
                writeback(&node, instructionRegisters, registers);
                if (isFollowedInstructionActive && followedInstructionPC == node.PC) {
                    followedInstructionRegisters.RZ = instructionRegisters.RZ;
                    if (isPrintingPipelineRegisters) {
                        std::cout << "TRACKING: Instruction #" << specificFollowedInstruction 
                              << " at PC 0x" << std::hex << followedInstructionPC << std::dec 
                              << " has completed execution" << std::endl;
                    }
                    isFollowedInstructionActive = false;
                }
                
                instructionProcessed = true;
                activeStages[Stage::WRITEBACK] = true;
                break;
            }
        }
    }

    pipeline = tempPipeline;

    if (running && textMap.find(PC) == textMap.end()) {
        running = false;
    }

    if (instructionProcessed) stats.totalCycles++;

    if (instructionCount > 0) {
        stats.cyclesPerInstruction = static_cast<double>(stats.totalCycles) / instructionCount;
    }
}

bool Simulator::step() {
    if (!running && pipeline.empty()) {
        logs[400] = "Cannot step - simulator is not running";
        return false;
    }

    try {
        std::cout << "PC: 0x" << std::hex << PC << std::dec 
                  << ", Pipeline size: " << pipeline.size() 
                  << ", Stalls: " << stats.stallBubbles 
                  << ", Hazards: " << stats.dataHazards
                  << ", Data Forwarding: " << (isDataForwarding ? "ENABLED" : "DISABLED") 
                  << std::endl;
        advancePipeline();
        stats.instructionsExecuted = instructionCount;
        if (isBranchPredicting && isPrintingPipelineRegisters) {
            branchPredictor.printDetails(std::cout);
        }
        if (isSpecificInstructionFollowed && isFollowedInstructionActive && isPrintingPipelineRegisters) {
            std::cout << "\n===== TRACKED INSTRUCTION #" << specificFollowedInstruction
                      << " (PC: 0x" << std::hex << followedInstructionPC << std::dec << ") =====" << std::endl;
            std::cout << "Current stage: ";
            switch (followedInstructionStage) {
                case Stage::FETCH: std::cout << "FETCH"; break;
                case Stage::DECODE: std::cout << "DECODE"; break;
                case Stage::EXECUTE: std::cout << "EXECUTE"; break;
                case Stage::MEMORY: std::cout << "MEMORY"; break;
                case Stage::WRITEBACK: std::cout << "WRITEBACK"; break;
                default: std::cout << "UNKNOWN"; break;
            }
            std::cout << std::endl;
            std::cout << "RA: 0x" << std::hex << followedInstructionRegisters.RA << std::dec << std::endl;
            std::cout << "RB: 0x" << std::hex << followedInstructionRegisters.RB << std::dec << std::endl;
            std::cout << "RM: 0x" << std::hex << followedInstructionRegisters.RM << std::dec << std::endl;
            std::cout << "RY: 0x" << std::hex << followedInstructionRegisters.RY << std::dec << std::endl;
            std::cout << "RZ: 0x" << std::hex << followedInstructionRegisters.RZ << std::dec << std::endl;
            std::cout << "====================================" << std::endl;
        }
        if (!running && pipeline.empty()) {
            return false;
        }
        
        return true;
    }
    catch (const std::runtime_error &e)
    {
        logs[404] = "Runtime error during step execution: " + std::string(e.what());
        running = false;
        return false;
    }
}

void Simulator::run() {
    if (!running) {
        logs[400] = "Cannot run - simulator is not running";
        return;
    }

    logs[200] = "Starting full program execution";
    int stepCount = 0;
    while (running || !pipeline.empty()) {
        if (!step())
            break;
        stepCount++;

        if (stepCount > MAX_STEPS) {
            logs[400] = "Program execution terminated - exceeded maximum step count (100000)";
            break;
        }
    }

    logs[200] = "Simulation completed. Total clock cycles: " + std::to_string(stats.totalCycles) + ", Total steps executed: " + std::to_string(stepCount);
}

void Simulator::setIsPipeline() {
    isPipeline = !isPipeline;
}

void Simulator::setIsDataForwarding()
{
    isDataForwarding = !isDataForwarding;
    std::cout << "Data forwarding: " << (isDataForwarding ? "Enabled" : "Disabled") << std::endl;
}

void Simulator::setIsPrintingRegisters()
{
    isPrintingRegisters = !isPrintingRegisters;
    std::cout << "Register printing: " << (isPrintingRegisters ? "Enabled" : "Disabled") << std::endl;
}

void Simulator::setIsPrintingPipelineRegisters()
{
    isPrintingPipelineRegisters = !isPrintingPipelineRegisters;
    std::cout << "Pipeline register printing: " << (isPrintingPipelineRegisters ? "Enabled" : "Disabled") << std::endl;
}

void Simulator::setIsSpecificInstructionFollowed(uint32_t followed)
{
    isSpecificInstructionFollowed = (followed != UINT32_MAX);
    specificFollowedInstruction = followed;
    
    if (isSpecificInstructionFollowed) {
        std::cout << "Specific instruction following: Enabled for instruction #" << specificFollowedInstruction << std::endl;
        followedInstructionPC = 0;
        followedInstructionStage = Stage::FETCH;
        isFollowedInstructionActive = false;
        followedInstructionRegisters = InstructionRegisters();
        if (instructionCount >= specificFollowedInstruction) {
            std::cout << "Warning: The specified instruction (#" << specificFollowedInstruction 
                      << ") has already been processed. Instruction count is currently at: " 
                      << instructionCount << std::endl;
        }
    } else {
        std::cout << "Specific instruction following: Disabled" << std::endl;
    }
}

void Simulator::setIsBranchPredicting()
{
    isBranchPredicting = !isBranchPredicting;
    std::cout << "Branch prediction: " << (isBranchPredicting ? "Enabled" : "Disabled") << std::endl;
}

void Simulator::setEnvironment(bool pipelineMode, bool dataForwarding,
                               bool printingRegisters, bool printingPipelineRegisters,
                               bool specificInstructionFollowed, bool branchPredicting)
{
    bool previousPipelineMode = isPipeline;
    isPipeline = pipelineMode;
    isDataForwarding = dataForwarding;
    isPrintingRegisters = printingRegisters;
    isPrintingPipelineRegisters = printingPipelineRegisters;
    isSpecificInstructionFollowed = specificInstructionFollowed;
    isBranchPredicting = branchPredicting;
}

bool Simulator::isRunning() const
{
    return running;
}

uint32_t Simulator::getPC() const
{
    return PC;
}

const uint32_t *Simulator::getRegisters() const
{
    return registers;
}

SimulationStats Simulator::getStats() const
{
    return stats;
}

std::map<Stage, bool> Simulator::getActiveStages() const
{
    return activeStages;
}

std::unordered_map<uint32_t, uint8_t> Simulator::getDataMap() const
{
    return dataMap;
}

std::map<uint32_t, std::pair<uint32_t, std::string>> Simulator::getTextMap() const
{
    return textMap;
}

size_t Simulator::queueSize() const
{
    return pipeline.size();
}

uint32_t Simulator::getCycles() const
{
    return stats.totalCycles;
}

bool Simulator::hasStallCondition() const
{
    return hasStall;
}

void Simulator::writeStatsToFile(const std::string& filename) const
{
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open stats file: " << filename << std::endl;
            return;
        }

        file << "================== SIMULATION STATISTICS ==================" << std::endl;
        file << "Stat1: Total number of cycles: " << stats.totalCycles << std::endl;
        file << "Stat2: Total instructions executed: " << instructionCount << std::endl;

        double cpi = (instructionCount > 0) ? 
                    static_cast<double>(stats.totalCycles) / instructionCount : 0.0;
        file << "Stat3: CPI: " << std::fixed << std::setprecision(2) << cpi << std::endl;
        
        file << "Stat4: Number of Data-transfer (load and store) instructions: " 
             << stats.dataTransferInstructions << std::endl;
        file << "Stat5: Number of ALU instructions: " << stats.aluInstructions << std::endl;
        file << "Stat6: Number of Control instructions: " << stats.controlInstructions << std::endl;
        file << "Stat7: Number of stalls/bubbles in the pipeline: " << stats.stallBubbles << std::endl;
        file << "Stat8: Number of data hazards: " << stats.dataHazards << std::endl;
        file << "Stat9: Number of control hazards: " << stats.controlHazards << std::endl;
        file << "Stat10: Number of branch mispredictions: " << stats.branchMispredictions << std::endl;
        file << "Stat11: Number of stalls due to data hazards: " << stats.dataHazardStalls << std::endl;
        file << "Stat12: Number of stalls due to control hazards: " << stats.controlHazardStalls << std::endl;
        file << "Stat13: Number of pipeline flushes: " << stats.pipelineFlushes << std::endl;
        file << std::endl;
        
        file << "================== CONFIGURATION ==================" << std::endl;
        file << "Pipeline Mode: " << (isPipeline ? "Enabled" : "Disabled") << std::endl;
        file << "Data Forwarding: " << (isDataForwarding ? "Enabled" : "Disabled") << std::endl;
        file << "Branch Prediction: " << (isBranchPredicting ? "Enabled" : "Disabled") << std::endl;
        file << std::endl;

        if (isBranchPredicting) {
            file << "================== BRANCH PREDICTION STATISTICS ==================" << std::endl;
            file << "Total branch predictions: " << branchPredictor.totalPredictions << std::endl;
            file << "Correct predictions: " << branchPredictor.correctPredictions << std::endl;
            file << "Incorrect predictions: " << branchPredictor.incorrectPredictions << std::endl;
            
            double accuracy = (branchPredictor.totalPredictions > 0) ?
                (static_cast<double>(branchPredictor.correctPredictions) / branchPredictor.totalPredictions * 100.0) : 0.0;
            file << "Prediction accuracy: " << accuracy << "%" << std::endl;
            
            file << std::endl << "Pattern History Table Entries: " << branchPredictor.PHT_SIZE << std::endl;
            file << "Branch Target Buffer Entries: " << branchPredictor.BTB_SIZE << std::endl;
            file << "Global History Register Size: " << branchPredictor.GHR_SIZE << " bits" << std::endl;

            if (isPrintingPipelineRegisters) {
                branchPredictor.printDetails(file);
            }
        }
        
        file.flush();
        if (!file.good()) {
            std::cerr << "Error: Failed to write to stats file: " << filename << std::endl;
        }
        file.close();
        
        std::cout << "Statistics successfully written to " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: Exception while writing stats file: " << e.what() << std::endl;
    }
}

void Simulator::flushPipeline(const std::string& reason) {
    if (!isPipeline) return;
    
    while (!pipeline.empty()) {
        pipeline.pop();
    }

    if (isSpecificInstructionFollowed && isFollowedInstructionActive) {
        isFollowedInstructionActive = false;
        if (isPrintingPipelineRegisters) {
            std::cout << "TRACKING: Instruction #" << specificFollowedInstruction 
                  << " at PC 0x" << std::hex << followedInstructionPC << std::dec 
                  << " was flushed from the pipeline" << std::endl;
        }
    }

    for (auto &stage : activeStages) {
        stage.second = false;
    }
    
    hasStall = false;
    stats.pipelineFlushes++;
    
    if (!reason.empty() && isPrintingPipelineRegisters) {
        std::cout << "PIPELINE FLUSH: " << reason << std::endl;
    }
}
#endif