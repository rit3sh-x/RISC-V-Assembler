#include "assembler.hpp"
#include "lexer.hpp"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    auto tokens = Lexer::Tokenizer(inputFile);
    if (tokens.empty()) {
        std::cerr << "Lexical analysis failed: No tokens generated\n";
        return 1;
    }

    Parser parser(tokens);
    if (!parser.parse()) {
        std::cerr << "Parsing failed with " << parser.getErrorCount() << " errors\n";
        return 1;
    }

    Assembler assembler(parser);
    if (!assembler.assemble()) {
        std::cerr << "Assembly failed with " << assembler.getErrorCount() << " errors\n";
        return 1;
    }

    if (!assembler.writeToFile(outputFile)) {
        std::cerr << "Failed to write output file: " << outputFile << "\n";
        return 1;
    }
    std::cout << "Assembly completed successfully. Output written to: " << outputFile << "\n";
    return 0;
} 