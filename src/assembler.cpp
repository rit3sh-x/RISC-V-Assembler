#include "assembler.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "types.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

int main(int argc, char* argv[]) {
    try {
        std::cout << "RISC-V Assembler starting...\n";
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <input_file> <optional_output_file>\n";
            return 1;
        }
        std::string inputFile = argv[1];
        std::string outputFile = (argc == 3) ? argv[2] : inputFile + ".mc";
        auto tokens = Lexer::Tokenizer(inputFile);
        if (tokens.empty()) {
            std::cerr << "Lexical analysis failed: No tokens generated\n";
            return 1;
        }
        Parser parser(tokens);
        if (!parser.parse()) {
            std::cerr << "Error parsing file: " << inputFile << std::endl;
            return 2;
        }
        Assembler assembler(parser);
        if (!assembler.assemble()) {
            std::cerr << "Error assembling code. Found " << assembler.getErrorCount() << " errors." << std::endl;
            return 3;
        }
        if (!assembler.writeToFile(outputFile)) {
            std::cerr << "Error writing to output file: " << outputFile << std::endl;
            return 4;
        }
        std::cout << "Assembly successful. Output written to: " << outputFile << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}