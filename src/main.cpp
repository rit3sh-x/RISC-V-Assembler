#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <iomanip>
#include <signal.h>
#include "simulator.hpp"

static Simulator* globalSimulatorPtr = nullptr;
static std::string globalOutputFile;
static bool simulationInterrupted = false;

void signalHandler(int signal) {
    std::cout << std::endl << "Received termination signal. Cleaning up..." << std::endl;
    simulationInterrupted = true;
    
    if (globalSimulatorPtr) {
        std::cout << "Writing final statistics..." << std::endl;
        globalSimulatorPtr->writeStatsToFile(globalOutputFile);
    }
    
    std::cout << "Simulation terminated." << std::endl;
    exit(0);
}

void printUsage() {
    std::cout << "RISC-V Simulator Usage:" << std::endl;
    std::cout << "  -d, --data-forwarding      Enable data forwarding" << std::endl;
    std::cout << "  -r, --registers            Print register values" << std::endl;
    std::cout << "  -p, --pipeline             Print pipeline registers" << std::endl;
    std::cout << "  -b, --branch-predict       Enable branch prediction" << std::endl;
    std::cout << "  -i, --input FILE           Specify input assembly file (default: input.asm)" << std::endl;
    std::cout << "  -o, --output FILE          Specify stats output file (default: stats.txt)" << std::endl;
    std::cout << "  -a, --auto                 Run simulation automatically (non-interactive)" << std::endl;
    std::cout << "  -f, --follow NUM           Track specific instruction by number" << std::endl;
    std::cout << "  -h, --help                 Display this help message" << std::endl;
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
    bool pipelineMode = true;
    bool dataForwarding = false;
    bool printRegisters = false;
    bool printPipeline = false;
    bool followInstr = false;
    uint32_t followInstrNum = UINT32_MAX;
    bool branchPredict = false;
    bool autoRun = false;
    std::string inputFile = "input.asm";
    std::string outputFile = "stats.txt";
    globalOutputFile = outputFile;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data-forwarding") == 0) {
            dataForwarding = true;
            std::cout << "Data forwarding: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--registers") == 0) {
            printRegisters = true;
            std::cout << "Register printing: ENABLED" << std::endl;
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pipeline") == 0) {
            printPipeline = true;
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
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[++i];
                globalOutputFile = outputFile;
                std::cout << "Output file: " << outputFile << std::endl;
            } else {
                std::cerr << "Error: Missing output file name" << std::endl;
                printUsage();
                return 1;
            }
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--follow") == 0) {
            if (i + 1 < argc) {
                try {
                    followInstrNum = std::stoul(argv[++i]);
                    followInstr = true;
                    std::cout << "Following instruction: " << followInstrNum << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid instruction number: " << argv[i] << std::endl;
                    printUsage();
                    return 1;
                }
            } else {
                std::cerr << "Error: Missing follow instruction number" << std::endl;
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
    sim.setEnvironment(pipelineMode, dataForwarding, printRegisters, printPipeline, followInstr, branchPredict);
    if (followInstr) {
        sim.setIsSpecificInstructionFollowed(followInstrNum);
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

    if (autoRun) {
        std::cout << "Running simulation in automatic mode...\n";
        std::cout << "Press Ctrl+C to terminate and save statistics\n";
        sim.run();
    } else {
        std::cout << "Press Enter to step through execution. Press 'q' then Enter to quit.\n";
        
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

        } while (choice != 'q' && !simulationInterrupted);
    }

    std::cout << "Total cycles: " << sim.getCycles() << std::endl;
    sim.writeStatsToFile(outputFile);
    return 0;
}