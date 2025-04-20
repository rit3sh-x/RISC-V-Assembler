#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <iomanip>
#include <signal.h>
#include "types.hpp"
#include "simulator.hpp"

using namespace riscv;

static Simulator* globalSimulatorPtr = nullptr;
static bool simulationInterrupted = false;

const uint32_t* registers;
InstructionRegisters instructionRegisters;
InstructionRegisters followedRegisters;

void printDetails(bool reg, bool normalIR , bool follow) {
    if(reg) {
        registers = globalSimulatorPtr->getRegisters();
        std::cout << ORANGE << "Registers:" << RESET << std::endl;
        for (int i = 0; i < NUM_REGISTERS; i++) {
            std::cout << ORANGE << "x" << i << ": " << std::hex << registers[i] << RESET << std::endl;
        }
    }

    if(normalIR) {
        instructionRegisters = globalSimulatorPtr->getInstructionRegisters();
        std::cout << ORANGE << "Instruction Registers:" << RESET << std::endl;
        std::cout << ORANGE << "RA : 0x" << std::setw(8) << std::setfill('0') << std::hex << instructionRegisters.RA << RESET << std::endl;
        std::cout << ORANGE << "RB : 0x" << std::setw(8) << std::setfill('0') << std::hex << instructionRegisters.RB << RESET << std::endl;
        std::cout << ORANGE << "RM : 0x" << std::setw(8) << std::setfill('0') << std::hex << instructionRegisters.RM << RESET << std::endl;
        std::cout << ORANGE << "RY : 0x" << std::setw(8) << std::setfill('0') << std::hex << instructionRegisters.RY << RESET << std::endl;
        std::cout << ORANGE << "RZ : 0x" << std::setw(8) << std::setfill('0') << std::hex << instructionRegisters.RZ << RESET << std::endl;
        std::cout << std::dec;
    }

    if(follow) {
        followedRegisters = globalSimulatorPtr->getFollowedInstructionRegisters();
        std::cout << GREEN << "Change occured in cycle: " << globalSimulatorPtr->getCycles() << RESET << std::endl;
        std::cout << ORANGE << "Followed Registers:" << RESET << std::endl;
        std::cout << ORANGE << "RA : 0x" << std::setw(8) << std::setfill('0') << std::hex << followedRegisters.RA << RESET << std::endl;
        std::cout << ORANGE << "RB : 0x" << std::setw(8) << std::setfill('0') << std::hex << followedRegisters.RB << RESET << std::endl;
        std::cout << ORANGE << "RM : 0x" << std::setw(8) << std::setfill('0') << std::hex << followedRegisters.RM << RESET << std::endl;
        std::cout << ORANGE << "RY : 0x" << std::setw(8) << std::setfill('0') << std::hex << followedRegisters.RY << RESET << std::endl;
        std::cout << ORANGE << "RZ : 0x" << std::setw(8) << std::setfill('0') << std::hex << followedRegisters.RZ << RESET << std::endl;
        std::cout << std::dec;
    }
}


void signalHandler(int signal) {
    std::cout << std::endl << ORANGE << "Terminating simulator..." << RESET << std::endl;
    simulationInterrupted = true;
    exit(0);
}

void printUsage() {
    std::cout << GREEN << "RISC-V Simulator Usage:" << RESET << std::endl;
    std::cout << YELLOW << "  -p, --pipeline             Print full pipeline state each cycle" << RESET << std::endl;
    std::cout << YELLOW << "  -d, --data-forwarding      Enable data forwarding" << RESET << std::endl;
    std::cout << YELLOW << "  -r, --registers            Print register values" << RESET << std::endl;
    std::cout << YELLOW << "  -l, --pipeline-regs        Print pipeline register values only" << RESET << std::endl;
    std::cout << YELLOW << "  -b, --branch-predict       Enable branch prediction" << RESET << std::endl;
    std::cout << YELLOW << "  -a, --auto                 Run simulation automatically (non-interactive)" << RESET << std::endl;
    std::cout << YELLOW << "  -f, --follow [n|p]=NUM     Track specific instruction by number (n=NUM) or PC (p=NUM), supports decimal or hex (0x prefix)" << RESET << std::endl;
    std::cout << YELLOW << "  -i, --input FILE           Specify input assembly file (default: input.asm)" << RESET << std::endl;
    std::cout << YELLOW << "  -h, --help                 Display this help message" << RESET << std::endl;
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    Simulator sim;
    globalSimulatorPtr = &sim;
    bool pipelineMode = false;
    bool dataForwarding = false;
    bool printRegisters = false;
    bool printPipelineRegs = false;
    uint32_t followInstrNum = UINT32_MAX;
    bool branchPredict = false;
    bool autoRun = false;
    std::string inputFile = "input.asm";
    std::string followArg;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pipeline") == 0) {
            pipelineMode = true;
            std::cout << "Pipeline mode: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data-forwarding") == 0) {
            dataForwarding = true;
            std::cout << "Data forwarding: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--registers") == 0) {
            printRegisters = true;
            std::cout << "Register printing: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--pipeline-regs") == 0) {
            printPipelineRegs = true;
            std::cout << "Pipeline register printing: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--branch-predict") == 0) {
            branchPredict = true;
            std::cout << "Branch prediction: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--auto") == 0) {
            autoRun = true;
            std::cout << "Auto run: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (i + 1 < argc) {
                inputFile = argv[++i];
                if (!fileExists(inputFile)) {
                    std::cerr << "Error: Input file not found: " << inputFile << std::endl;
                    return 1;
                }
                std::cout << "Input file: " << inputFile << std::endl;
            } else {
                std::cerr << "Error: Missing input file name" << std::endl;
                printUsage();
                return 1;
            }
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--follow") == 0) {
            if (i + 1 < argc) {
                followArg = argv[++i];
                bool isPC = false;
                size_t equalPos = followArg.find('=');
                std::string typeStr, numStr;
                
                if (equalPos != std::string::npos) {
                    typeStr = followArg.substr(0, equalPos);
                    numStr = followArg.substr(equalPos + 1);
                    if (typeStr == "p") {
                        isPC = true;
                    } else if (typeStr != "n") {
                        std::cerr << "Error: Invalid follow type '" << typeStr << "'. Use n=NUM for instruction number or p=NUM for PC" << std::endl;
                        printUsage();
                        return 1;
                    }
                } else {
                    std::cerr << "Error: Invalid follow format. Use n=NUM or p=NUM" << std::endl;
                    printUsage();
                    return 1;
                }

                if (numStr.empty()) {
                    std::cerr << "Error: Missing number after '=' in follow argument" << std::endl;
                    printUsage();
                    return 1;
                }

                try {
                    followInstrNum = std::stoul(numStr, nullptr, 0);
                    std::cout << "Following instruction: " << followArg << (isPC ? " (PC)" : " (instruction number)") << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid instruction number or PC: " << numStr << std::endl;
                    printUsage();
                    return 1;
                }
            } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            printUsage();
            return 1;
        }
    }

    try {
        std::string program = readFile(inputFile);
        if (!sim.loadProgram(program)) {
            std::cerr << "Failed to load program!\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading file: " << e.what() << "\n";
        return 1;
    }

    size_t length = sim.getTextMap().size();
    if(length == 0) {
        std::cerr << "Error: No text segment found in the program." << std::endl;
        return 1;
    } else {
        std::cout << "Text segment size: " << length << " bytes" << std::endl;
    }
    
    if(followInstrNum != UINT32_MAX) {
        bool isValid = true;
        std::string errorMsg;
        if (followArg.find("p=") != std::string::npos) {
            if (followInstrNum >= static_cast<uint32_t>(length) || (followInstrNum % 4) != 0) {
                isValid = false;
                errorMsg = "PC is outside text segment or not 4-byte aligned";
            }
        } else {
            if (followInstrNum == 0 || followInstrNum > static_cast<uint32_t>(length / 4)) {
                isValid = false;
                errorMsg = "Instruction number is out of range";
            } else {
                followInstrNum = (followInstrNum - 1) * 4;
            }
        }

        if (!isValid) {
            std::cout << ORANGE << "Warning: " << errorMsg << ". Skipping follow" << RESET << std::endl;
            followInstrNum = UINT32_MAX;
        }
    }

    sim.setEnvironment(pipelineMode, dataForwarding, branchPredict, followInstrNum);

    if (autoRun) {
        std::cout << YELLOW << "Running simulation in automatic mode...\n" << RESET << std::endl;
        sim.run();
        printDetails(printRegisters, printPipelineRegs, followInstrNum == UINT32_MAX);
    } else {
        std::cout << YELLOW << "Press Enter to step through execution. Press 'q' then Enter to quit.\n" << RESET << std::endl;
        
        char choice;
        do {
            if (!sim.step() || simulationInterrupted) {
                std::cout << "Simulation stopped.\n";
                break;
            }
            choice = std::cin.get();

            if (choice == '\n') {
                continue;
            }
            printDetails(printRegisters, printPipelineRegs, followInstrNum == UINT32_MAX);
        } while (choice != 'q' && !simulationInterrupted);
    }

    std::cout << "Total cycles: " << sim.getCycles() << std::endl;

    try {
        std::ofstream statsFile("stats.txt");
        if (!statsFile.is_open()) {
            std::cerr << "Error: Could not open stats.txt for writing" << std::endl;
            return 1;
        }

        SimulationStats stats = sim.getStats();
        statsFile << "Simulation Statistics:\n";
        statsFile << "Cycles Per Instruction: " << stats.cyclesPerInstruction << "\n";
        statsFile << "Total Cycles: " << stats.totalCycles << "\n";
        statsFile << "Instructions Executed: " << stats.instructionsExecuted << "\n";
        statsFile << "Data Transfer Instructions: " << stats.dataTransferInstructions << "\n";
        statsFile << "ALU Instructions: " << stats.aluInstructions << "\n";
        statsFile << "Control Instructions: " << stats.controlInstructions << "\n";
        statsFile << "Stall Bubbles: " << stats.stallBubbles << "\n";
        statsFile << "Data Hazards: " << stats.dataHazards << "\n";
        statsFile << "Control Hazards: " << stats.controlHazards << "\n";
        statsFile << "Data Hazard Stalls: " << stats.dataHazardStalls << "\n";
        statsFile << "Control Hazard Stalls: " << stats.controlHazardStalls << "\n";
        statsFile << "Pipeline Flushes: " << stats.pipelineFlushes << "\n";
        statsFile << "Branch Mispredictions: " << stats.branchMispredictions << "\n";

        statsFile.close();
        std::cout << "Simulation stats written to stats.txt" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error writing to stats.txt: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}