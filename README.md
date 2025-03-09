# RISC-V Assembler

## Overview
This project is a RISC-V assembler written in C++. The assembler translates RISC-V assembly language code into machine code that can be executed by a RISC-V processor. This project is currently in the development stage, focusing on building the core functionality of the assembler.

## Authors
- **Ritesh Kumar**  
  Entry No: 2023CSB1153  
  Ritesh is responsible for the overall project management and integration of various components.

- **Ruhaan Choudhary**  
  Entry No: 2023CSB1156  
  Ruhaan specializes in software development and has a strong background in C++. He is working on the implementation of the assembler's parsing and code generation modules.

- **Sumit Yadav**  
  Entry No: 2023CSB1167  
  Sumit has a deep understanding of assembly languages and compiler design. He is focusing on the instruction set architecture and ensuring the assembler correctly translates assembly instructions into machine code.

## Development Stage
The project is currently in the development stage, with the primary focus on creating a functional C++ assembler for the RISC-V architecture. The team is working on the following key components:
- **Lexical Analysis**: Tokenizing the input assembly code.
- **Syntax Analysis**: Parsing the tokens to generate an abstract syntax tree (AST).
- **Code Generation**: Translating the AST into RISC-V machine code.
- **Error Handling**: Implementing robust error detection and reporting mechanisms.

## Getting Started
To get started with the project, clone the repository and follow the instructions to set up the development environment.

```bash
git clone https://github.com/rit3sh-x/RISC-V-Aseembler
cd RISC-V-Aseembler
```

## Running the Program
Follow these steps to compile and run the assembler:

1. **Navigate to the project directory**:
    ```bash
    cd /path/to/RISC-V-Assembler
    ```

2. **Compile the assembler**:
    ```bash
    g++ -o riscv_assembler ./src/assembler.cpp
    ```

3. **Run the assembler**:
    ```bash
    ./riscv_assembler input_file.asm output_file.mc
    ```

    Replace `input_file.asm` with the path to your RISC-V assembly file and `output_file.mc` with the desired output file name.