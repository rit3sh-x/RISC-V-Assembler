#include "lexer.hpp"
#include "temp/parser.hpp"
#include <iostream>

int main() {
    const std::string filename = "input.asm";
    std::vector<std::vector<Token>> tokens = Lexer::Tokenizer(filename);
    if (tokens.empty()) {
        std::cout << "No tokens generated. Check if '" << filename << "' exists or contains errors." << std::endl;
        return 1;
    }
    
    // std::cout << "\nTokenized Output from " << filename << ":\n";
    for (const auto& lineTokens : tokens) {
        std::cout << "Line " << lineTokens[0].lineNumber << ": ";
        for (const Token& token : lineTokens) {
            std::cout << "[" << Lexer::getTokenTypeName(token.type) << ", \"" << token.value  << "\"] ";
        }
        std::cout << std::endl;
    }

    Parser parser(tokens);
    if (!parser.parse()) {
        std::cout << "\nParsing failed due to " << parser.getErrorCount() << " error(s)." << std::endl;
        return 1;
    }

    std::cout << "\n";
    parser.printSymbolTable();
    std::cout << "\n";
    parser.printParsedInstructions();
    return 0;
}