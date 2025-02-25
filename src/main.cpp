#include "lexer.hpp"
#include <iostream>

int main() {
    const std::string filename = "input.asm";
    std::vector<std::vector<Token>> tokens = Lexer::Tokenizer(filename);
    if (tokens.empty()) {
        std::cout << "No tokens generated. Check if '" << filename << "' exists or contains errors." << std::endl;
        return 1;
    }
    std::cout << "\nTokenized Output from " << filename << ":\n";
    for (size_t line = 0; line < tokens.size(); ++line) {
        std::cout << "Line " << tokens[line][0].lineNumber << ": ";
        for (const Token& token : tokens[line]) {
            std::cout << "[" << Lexer::getTokenTypeName(token.type) 
                      << ", \"" << token.value << "\"] ";
        }
        std::cout << std::endl;
    }

    return 0;
}