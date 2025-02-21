#include <iostream>
#include <fstream>
#include "lexer.hpp"

int main() {
    std::string filename = "input.asm";
    std::vector<std::vector<Token>> tokenizedOutput = Lexer::Tokenizer(filename);

    for (const auto& lineTokens : tokenizedOutput) {
        for (const auto& token : lineTokens) {
            std::cout << token.value << " (" << Lexer::getTokenTypeName(token.type) << ") ";
        }
        std::cout << std::endl;
    }
    return 0;
}