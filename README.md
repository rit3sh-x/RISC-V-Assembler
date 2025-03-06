# RISC-V Assembler

## Overview
This project implements a RISC-V assembler in C++, translating RISC-V assembly language into machine code. Our implementation focuses on clarity, robustness, and adherence to the RISC-V specification while providing detailed debugging information.

## Technical Approach

### Memory Layout
We follow a standard RISC-V memory layout with predefined segment boundaries:
- Text Segment: Starts at 0x00000000
- Data Segment: Starts at 0x10000000
- Heap: Starts at 0x10008000
- Stack: Starts at 0x7FFFFDC

### Two-Pass Assembly
Our assembler uses a two-pass approach:
1. **First Pass**: 
   - Builds symbol table
   - Resolves labels
   - Calculates addresses for all instructions and data
2. **Second Pass**:
   - Generates machine code
   - Resolves forward references
   - Performs error checking

### Machine Code Generation
Each instruction is encoded according to its type (R, I, S, SB, U, UJ) following the RISC-V specification:
- R-type: add, sub, sll, slt, sltu, xor, srl, sra, or, and
- I-type: addi, slti, sltiu, xori, ori, andi, lb, lh, lw, lbu, lhu
- S-type: sb, sh, sw
- SB-type: beq, bne, blt, bge, bltu, bgeu
- U-type: lui, auipc
- UJ-type: jal

### Output Format
Our .mc file output follows a specific format for clarity and debugging:
```
<address> <machine_code> , <assembly_instruction> # <opcode-func3-func7-rd-rs1-rs2-immediate>
```
Example:
```
0x0 0x003100B3 , add x1,x2,x3 # 0110011-000-0000000-00001-00010-00011-NULL
```

### Special Features

#### Text Segment End Marker (0xDEADBEEF)
We use 0xDEADBEEF as a special marker at the end of the text segment for several reasons:
1. Easy visual identification of text segment boundary
2. Debugging aid to ensure proper segment separation
3. Common practice in systems programming for marking memory boundaries
4. Helps detect potential overflow into data segment

#### NULL Fields
We use "NULL" in the output format to explicitly indicate undefined or unused fields in different instruction types, making it easier to:
- Debug instruction encoding
- Verify correct instruction formatting
- Maintain consistency with tools like Venus

### Error Handling
The assembler implements robust error checking:
- Invalid instructions
- Register out of range
- Immediate value out of range
- Invalid labels
- Memory alignment issues
- Segment boundary violations

## Authors
- **Ritesh Kumar** (2023CSB1153)  
  Project Lead & Core Architecture

- **Ruhaan Choudhary** (2023CSB1156)  
  Parser Implementation & Code Generation

- **Sumit Yadav** (2023CSB1167)  
  Instruction Encoding & Error Handling

## Getting Started

### Building the Project
```bash
g++ -o riscv_assembler assembler.cpp
```

### Running the Assembler
```bash
./riscv_assembler input_file.asm output_file.mc
```

### Input Assembly Format
- Labels must end with ':'
- One instruction per line
- Comments start with '#'
- Supports all basic RISC-V integer instructions
- Data directives: .word, .byte, .string

### Output Format
The generated .mc file contains:
1. Data segment with addresses and values
2. Text segment with detailed instruction encoding
3. Clear separation between segments
4. Human-readable instruction decomposition
