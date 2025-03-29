#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include "types.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"
#include "execution.hpp"

void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " <input_file.asm> [output_file.mc]" << std::endl;
    std::cout << "If output file is not specified, the output will be written to <input_file>.mc" << std::endl;
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

std::string decryptInstruction(uint32_t instruction) {
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    std::stringstream ss;
    if (opcode == 0b0110011) {
        if (funct3 == 0b000 && funct7 == 0b0000000)
            ss << "add";
        else if (funct3 == 0b000 && funct7 == 0b0100000)
            ss << "sub";
        else if (funct3 == 0b001 && funct7 == 0b0000000)
            ss << "sll";
        else if (funct3 == 0b010 && funct7 == 0b0000000)
            ss << "slt";
        else if (funct3 == 0b011 && funct7 == 0b0000000)
            ss << "sltu";
        else if (funct3 == 0b100 && funct7 == 0b0000000)
            ss << "xor";
        else if (funct3 == 0b101 && funct7 == 0b0000000)
            ss << "srl";
        else if (funct3 == 0b101 && funct7 == 0b0100000)
            ss << "sra";
        else if (funct3 == 0b110 && funct7 == 0b0000000)
            ss << "or";
        else if (funct3 == 0b111 && funct7 == 0b0000000)
            ss << "and";
        else if (funct3 == 0b000 && funct7 == 0b0000001)
            ss << "mul";
        else if (funct3 == 0b100 && funct7 == 0b0000001)
            ss << "div";
        else if (funct3 == 0b110 && funct7 == 0b0000001)
            ss << "rem";
        else
            return "UNKNOWN";
        ss << " x" << rd << ",x" << rs1 << ",x" << rs2;
    }
    else if (opcode == 0b0010011 || opcode == 0b0000011) {
        int32_t imm = instruction >> 20;
        if (opcode == 0b0010011) {
            if (funct3 == 0b000)
                ss << "addi";
            else if (funct3 == 0b001)
                ss << "slli";
            else if (funct3 == 0b010)
                ss << "slti";
            else if (funct3 == 0b011)
                ss << "sltiu";
            else if (funct3 == 0b100)
                ss << "xori";
            else if (funct3 == 0b101)
                ss << ((imm >> 5) & 0x7F ? "srai" : "srli");
            else if (funct3 == 0b110)
                ss << "ori";
            else if (funct3 == 0b111)
                ss << "andi";
            else
                return "UNKNOWN";
            ss << " x" << rd << ",x" << rs1 << "," << (funct3 == 0b001 || funct3 == 0b101 ? imm & 0x1F : imm);
        }
        else {
            if (funct3 == 0b000)
                ss << "lb";
            else if (funct3 == 0b001)
                ss << "lh";
            else if (funct3 == 0b010)
                ss << "lw";
            else if (funct3 == 0b011)
                ss << "ld";
            else if (funct3 == 0b100)
                ss << "lbu";
            else if (funct3 == 0b101)
                ss << "lhu";
            else
                return "UNKNOWN";
            ss << " x" << rd << "," << imm << "(x" << rs1 << ")";
        }
    }
    else if (opcode == 0b0100011) {
        int32_t imm = ((instruction >> 25) & 0x7F) << 5 | ((instruction >> 7) & 0x1F);
        if (funct3 == 0b000)
            ss << "sb";
        else if (funct3 == 0b001)
            ss << "sh";
        else if (funct3 == 0b010)
            ss << "sw";
        else if (funct3 == 0b011)
            ss << "sd";
        else
            return "UNKNOWN";
        ss << " x" << rs2 << "," << imm << "(x" << rs1 << ")";
    }
    else if (opcode == 0b1100011) {
        int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 |
                      ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
        if (funct3 == 0b000)
            ss << "beq";
        else if (funct3 == 0b001)
            ss << "bne";
        else if (funct3 == 0b100)
            ss << "blt";
        else if (funct3 == 0b101)
            ss << "bge";
        else if (funct3 == 0b110)
            ss << "bltu";
        else if (funct3 == 0b111)
            ss << "bgeu";
        else
            return "UNKNOWN";
        ss << " x" << rs1 << ",x" << rs2 << "," << imm;
    }
    else if (opcode == 0b0110111 || opcode == 0b0010111) {
        int32_t imm = instruction & 0xFFFFF000;
        ss << (opcode == 0b0110111 ? "lui" : "auipc") << " x" << rd << "," << (imm >> 12);
    }
    else if (opcode == 0b1101111) {
        int32_t imm = ((instruction >> 31) & 0x1) << 20 | ((instruction >> 12) & 0xFF) << 12 |
                      ((instruction >> 20) & 0x1) << 11 | ((instruction >> 21) & 0x3FF) << 1;
        ss << "jal x" << rd << "," << imm;
    }
    else if (opcode == 0b1100111) {
        int32_t imm = instruction >> 20;
        if (funct3 == 0b000)
            ss << "jalr x" << rd << ",x" << rs1 << "," << imm;
        else
            return "UNKNOWN";
    }
    else ss << "UNKNOWN" << std::hex << opcode;
    return ss.str();
}

void writeMachineCode(const std::string& filename, const std::vector<std::pair<uint32_t, uint32_t>>& machineCode, size_t instructionCount, const std::string& inputFile) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open output file for writing: " + filename);
    }

    file << "# ---------------- TEXT SEGMENT ---------------- #\n";
    uint32_t lastTextAddress = 0;
    size_t textInstructions = 0;
    
    for (const auto& [address, code] : machineCode) {
        if (address < riscv::DATA_SEGMENT_START) {
            file << "0x" << std::hex << std::setw(8) << std::setfill('0') << address 
                 << " 0x" << std::setw(8) << std::setfill('0') << code 
                 << " , " << decryptInstruction(code) << "\n";
            lastTextAddress = address;
            textInstructions++;
        }
    }

    file << "\n# ---------------- DATA SEGMENT ---------------- #\n";
    if (textInstructions > 0) {
        file << "0x" << std::hex << std::setw(8) << std::setfill('0') << (lastTextAddress + 4) << " 0x00000000 , <END_OF_TEXT>\n";
    }

    for (const auto& [address, code] : machineCode) {
        if (address >= riscv::DATA_SEGMENT_START) {
            file << "0x" << std::hex << std::setw(8) << std::setfill('0') << address << " 0x" << std::setw(2) << std::setfill('0') << (code & 0xFF) << "\n";
        }
    }

    file.close();
    std::cout << "Machine code written to " << filename << " (" << textInstructions << " instructions, " << (machineCode.size() - textInstructions) << " data entries)" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = (argc == 3) ? argv[2] : (inputFile.find_last_of('.') != std::string::npos ? inputFile.substr(0, inputFile.find_last_of('.')) + ".mc" : inputFile + ".mc");
    
    try {
        std::string programCode = readFile(inputFile);
        if (programCode.empty()) {
            throw std::runtime_error("Input file is empty");
        }
        std::cout << "Read " << programCode.size() << " bytes from " << inputFile << std::endl;

        std::vector<std::vector<riscv::Token>> tokenizedLines = Lexer::tokenize(programCode);
        if (tokenizedLines.empty()) {
            throw std::runtime_error("No valid tokens found in the input file");
        }
        std::cout << "Lexical analysis complete: " << tokenizedLines.size() << " lines processed" << std::endl;

        Parser parser(tokenizedLines);
        if (!parser.parse()) {
            std::cerr << "Error: Parsing failed with " << parser.getErrorCount() << " errors" << std::endl;
            for (const auto& [code, message] : riscv::logs) {
                if (code >= 400) std::cerr << message << std::endl;
            }
            return 1;
        }
        size_t instructionCount = parser.getParsedInstructions().size();
        std::cout << "Parsing complete: " << instructionCount << " instructions found" << std::endl;

        Assembler assembler(parser.getSymbolTable(), parser.getParsedInstructions());
        if (!assembler.assemble()) {
            std::cerr << "Error: Assembly failed with " << assembler.getErrorCount() << " errors" << std::endl;
            for (const auto& [code, message] : riscv::logs) {
                if (code >= 400) std::cerr << message << std::endl;
            }
            return 1;
        }
        std::cout << "Assembly complete: " << assembler.getMachineCode().size() << " machine code entries generated" << std::endl;

        writeMachineCode(outputFile, assembler.getMachineCode(), instructionCount, inputFile);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}