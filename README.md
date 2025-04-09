# 🚀 RISC-V Assembler & Simulator

## 📋 Overview
This project consists of a RISC-V assembler and simulator written in C++. The assembler translates RISC-V assembly language code into machine code, and the simulator executes this machine code in a virtual RISC-V environment. The project includes a web-based frontend built with NextJS that leverages WebAssembly to run the simulator in browsers.

## 👨‍💻 Authors
- **Ritesh Kumar**  
  Entry No: 2023CSB1153  
  Ritesh is responsible for the overall project management, frontend development with NextJS, and integration of WebAssembly with the web interface.

- **Ruhaan Choudhary**  
  Entry No: 2023CSB1156  
  Ruhaan specializes in software development and has a strong background in C++. He is responsible for WebAssembly compilation and the implementation of the assembler's parsing and code generation modules.

- **Sumit Yadav**  
  Entry No: 2023CSB1167  
  Sumit has a deep understanding of assembly languages and compiler design. He is focusing on the instruction set architecture and ensuring the assembler correctly translates assembly instructions into machine code.

## 🧩 Components

### 1. 🔄 Assembler (src/assembler.cpp)
The assembler translates RISC-V assembly code into machine code. It provides:

- Lexical analysis to tokenize the assembly code
- Parsing of assembly instructions into an intermediate representation
- Symbol table management for labels and addresses
- Generation of executable machine code
- Support for all basic RISC-V instructions (R-type, I-type, S-type, B-type, U-type, J-type)
- Machine code output with clear formatting of text and data segments
- Debugging information including instruction decoding

The assembler outputs a readable machine code file (.mc) that includes both hexadecimal instruction codes and their assembly equivalents.

### 2. 💻 Simulator (src/simulator.cpp)
The simulator executes RISC-V machine code in a virtual environment. It provides:

- Instruction-by-instruction execution
- Register and memory state monitoring
- Console output for debugging
- Step-by-step execution with detailed logging

The simulator can be used both as a standalone C++ application and as a WebAssembly module in the web frontend.

### 3. 🌐 NextJS Web Frontend
The project includes a modern web-based interface built with NextJS that allows users to:
- Write and edit RISC-V assembly code
- Assemble the code to machine code
- Simulate the execution with visual feedback
- Observe register and memory states during execution

The frontend uses React for the UI components and WebAssembly to run the C++ simulator directly in the browser, providing near-native performance.

## 🏗️ Architecture

The RISC-V Assembler and Simulator follows a modular architecture:

```
RISC-V-Assembler/
├── src/                     # Core C++ implementation
│   ├── assembler.cpp        # Assembler implementation
│   ├── assembler.hpp        # Assembler class definitions
│   ├── simulator.cpp        # Simulator implementation
│   ├── simulator.hpp        # Simulator class definitions
│   ├── types.hpp            # Core types and constants
│   ├── lexer.hpp            # Lexical analyzer for tokenizing assembly
│   ├── parser.hpp           # Parser for processing tokens
│   └── execution.hpp        # Execution logic for simulation
├── wasm/
│   |── wasm.cpp             # WebAssembly bindings for browser integration
├   |── assembler.hpp        # Assembler class definitions
│   ├── simulator.hpp        # Simulator class definitions
│   ├── types.hpp            # Core types and constants
│   ├── lexer.hpp            # Lexical analyzer for tokenizing assembly
│   ├── execution.hpp        # Execution logic for simulation
│   └── parser.hpp           # Parser for processing tokens
└── frontend/
    ├── public/
    │   ├── simulator.js     # Compiled WebAssembly JavaScript glue
    │   └── simulator.wasm   # Compiled WebAssembly binary
    ├── src/
    │   ├── app/             # NextJS app directory (pages, layouts)
    │   ├── components/      # Reusable UI components
    │   ├── hooks/           # Custom React hooks
    │   ├── lib/             # Utility functions and libraries
    │   └── types/           # TypeScript type definitions for simulator
    ├── package.json         # Project dependencies and scripts
    ├── tsconfig.json        # TypeScript configuration
    └── next.config.js       # NextJS configuration
```

The architecture follows a pipeline approach:
1. **Source Code** → Assembly file (.asm)
2. **Assembly** → Machine code (.mc) via assembler.cpp
3. **Simulation** → Execution via simulator.cpp with register/memory state tracking
4. **Visualization** → Web interface displaying execution state

## 🔍 Approach

### Simulator Core Design (simulator.hpp)

The simulator implements a RISC-V processor core with the following key components:

1. **Register File**: 
   - 32 general-purpose registers (x0-x31)
   - x0 hardwired to zero
   - Special registers for Program Counter (PC)

2. **Memory System**:
   - Segmented memory with text and data sections
   - 4KB page-aligned addressing
   - Support for byte, half-word, and word access

3. **Instruction Pipeline**:
   - 5-stage pipeline: Fetch, Decode, Execute, Memory, Write-back
   - Pipeline hazard detection and resolution
   - Optional data forwarding to minimize stalls

4. **Execution Model**:
   - Instruction decoding using bit-field extraction
   - ALU operations for arithmetic and logical computations
   - Control flow handling for branches and jumps
   - Memory operations for loads and stores

5. **Instruction Set Support**:
   - R-type: `add`, `sub`, `sll`, `slt`, `sltu`, `xor`, `srl`, `sra`, `or`, `and`
   - I-type: `addi`, `slti`, `sltiu`, `xori`, `ori`, `andi`, `slli`, `srli`, `srai`, `lb`, `lh`, `lw`, `lbu`, `lhu`, `jalr`
   - S-type: `sb`, `sh`, `sw`
   - B-type: `beq`, `bne`, `blt`, `bge`, `bltu`, `bgeu`
   - U-type: `lui`, `auipc`
   - J-type: `jal`

The simulator can operate in two modes:
1. **Interactive Mode**: Step-through execution with state visualization
2. **Batch Mode**: Complete program execution with final state reporting

Performance optimizations include:
- Configurable data forwarding to reduce data hazards
- Branch prediction options to minimize control hazards
- Memory caching for faster data access

The WebAssembly integration exposes the core simulator functionality through a clean JavaScript API, allowing the web frontend to control execution flow and visualize processor state in real-time.

### Assembler Implementation (assembler.cpp)

The assembler translates RISC-V assembly code into machine code through several stages:

1. **Lexical Analysis**:
   - Tokenizes the assembly code into meaningful components
   - Distinguishes between opcodes, registers, immediates, labels, and directives
   - Handles special cases like string literals and comments
   - Reports syntax errors with line numbers for easier debugging

2. **Parsing**:
   - Two-pass algorithm to resolve forward references
   - First pass builds the symbol table and allocates addresses for instructions and data
   - Second pass resolves label references and validates instruction operands
   - Generates parsed instruction objects with validated operands

3. **Code Generation**:
   - Converts parsed instructions into binary machine code
   - Handles different instruction formats (R-type, I-type, S-type, B-type, U-type, J-type)
   - Properly encodes immediate values, addressing modes, and branch offsets
   - Organizes code into text and data segments

4. **Output Generation**:
   - Creates a readable machine code file (.mc)
   - Includes both hexadecimal instruction codes and their assembly equivalents
   - Clearly delineates text and data segments
   - Provides metadata about assembled instructions

The assembler handles special cases such as:
- Pseudo-instructions and their expansion
- Relative addressing for branch and jump instructions
- Proper alignment of instructions and data
- Error checking for invalid operands or out-of-range values
- Data directive processing for different sizes (.byte, .half, .word, .dword)
- String literal handling with proper null termination

The machine code output format is designed to be easily readable by humans while also being suitable for loading into the simulator.

## 📁 Project Files

The project is structured across multiple files, each responsible for specific functionality:

### 1. 📄 types.hpp
The core types and constants used across the project. This file defines:
- Memory segment addresses and sizes
- Register count and instruction size constants
- Enums for instruction types, token types, and pipeline stages
- Data structures for branch prediction and instruction nodes
- RISC-V instruction encodings for different instruction formats (R-type, I-type, etc.)
- Utility functions for encoding/decoding instructions

### 2. 📄 lexer.hpp
The lexical analyzer that converts source code text into tokens. Key features:
- Breaking assembly code into tokens (opcodes, registers, immediates, etc.)
- Handling comments and string literals
- Validating tokens and reporting syntax errors
- Supporting all RISC-V register names and mnemonics

### 3. 📄 parser.hpp 
The parser that transforms tokens into structured representations. Functionality includes:
- Two-pass parsing to resolve forward references
- Symbol table management for labels and data
- Handling directives for different memory segments
- Semantic analysis of instructions and operands
- Building parsed instruction objects for the assembler

### 4. 📄 execution.hpp
Core execution logic for instruction simulation. Contains:
- Functions for each pipeline stage (fetch, decode, execute, memory, writeback)
- Memory access and register file management
- Instruction decoding and execution
- Branch prediction and handling
- Helper functions for simulation statistics

### 5. 📄 assembler.hpp
Responsible for transforming parsed instructions into machine code. Features:
- Encoding for all supported RISC-V instruction formats
- Code generation for both text and data segments
- Error checking and validation
- Address resolution for labels and branches

## 🔌 WebAssembly Integration
The project uses Emscripten to compile the C++ simulator to WebAssembly, enabling it to run in web browsers:

```bash
emcc -O2 -s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME="createSimulator" -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 --bind -I. wasm/wasm.cpp -o frontend/simulator.js
```

This command:
- Compiles `simulator.cpp` to WebAssembly
- Generates JavaScript bindings with `--bind`
- Modularizes the output for clean integration with NextJS
- Optimizes the code with `-O2` for better performance

To integrate the WebAssembly module with NextJS, the compiled files should be placed in the public directory of the NextJS project.

## 🚦 Getting Started
To get started with the project, clone the repository and follow the instructions to set up the development environment.

```bash
git clone https://github.com/rit3sh-x/RISC-V-Aseembler
cd RISC-V-Aseembler
```

## 🔧 Troubleshooting

### Emscripten Setup Issues
If you encounter any issues with Emscripten installation or usage:

1. **Installing Emscripten**:
   - Follow the official installation guide: https://emscripten.org/docs/getting_started/downloads.html
   - For Windows users, consider using the emsdk through Git Bash or WSL

2. **Common Emscripten Errors**:
   - Missing LLVM: Make sure to run `emsdk install latest` and `emsdk activate latest`
   - Environment variables: Run `emsdk_env.bat` (Windows) or `source ./emsdk_env.sh` (Linux/Mac)
   - For WebAssembly compilation issues: Check the [Emscripten Compiler Frontend docs](https://emscripten.org/docs/tools_reference/emcc.html)

### C++ Compiler Setup
This project uses C++17 features. Make sure your compiler supports them:

#### 1. **Windows**

- **MinGW-w64** or **MSVC** recommended
- For **MinGW**, use the [SourceForge UCRT64 build](https://sourceforge.net/projects/mingw-w64/)
- For **MSVC**, install [Visual Studio](https://visualstudio.microsoft.com/downloads/) with C++ support
- Alternatively, you can use **MSYS2**, which provides a convenient package manager and development environment. To get started with MSYS2, follow these steps:
  1. Download and install MSYS2 from the [official website](https://www.msys2.org/).
  2. After installation, open the MSYS2 shell and run the following commands to update the package database and core system packages:
     ```bash
     pacman -Syu
     ```
  3. Install the necessary build tools:
     ```bash
     pacman -S mingw-w64-ucrt-x86_64-gcc
     ```

#### 2. **Linux**

- Install `g++` with `sudo apt install g++` (Debian/Ubuntu) or equivalent
- Ensure version 7.0+ with `g++ --version`

#### 3. **macOS**

- Install Clang through Xcode Command Line Tools: `xcode-select --install`
- Or use Homebrew: `brew install llvm`

For additional help with project-specific issues, please open an issue on the GitHub repository.

## ⚙️ Running the Components

### 🔄 Assembler
1. **Compile the assembler**:
    ```bash
    g++ -o riscv_assembler ./src/assembler.cpp
    ```

2. **Run the assembler**:
    ```bash
    ./riscv_assembler <input_file.asm> [output_file.mc]
    ```

3. **Command-line arguments**:
    - `input_file.asm`: Required. The RISC-V assembly source file
    - `output_file.mc`: Optional. The output machine code file. If not specified, uses `<input_file>.mc`

4. **Example usage**:
    ```bash
    ./riscv_assembler program.asm output.mc
    ```
    This will assemble the program.asm file and write the machine code to output.mc.

### 💻 Simulator
1. **Compile the simulator**:
    ```bash
    g++ -o riscv_simulator ./src/simulator.cpp
    ```

2. **Run the simulator**:
    ```bash
    ./riscv_simulator [options]
    ```

3. **Command-line options**:
    ```
    -p, --pipeline             Print full pipeline state each cycle
    -d, --data-forwarding      Enable data forwarding
    -r, --registers            Print register values
    -l, --pipeline-regs        Print pipeline register values only
    -b, --branch-predict       Enable branch prediction
    -a, --auto                 Run simulation automatically (non-interactive)
    -f, --follow NUM           Track specific instruction by number
    -i, --input FILE           Specify input assembly file (default: input.asm)
    -h, --help                 Display the help message
    ```

4. **Example usage**:
    ```bash
    ./riscv_simulator -i program.asm -r -d -a
    ```
    This will run the simulator with the program.asm file, enable data forwarding, print register values, and run in automatic mode.

### 🌐 NextJS Web Frontend
1. **Compile the simulator to WebAssembly**:
    ```bash
    emcc ./src/simulator.cpp -o public/simulator.js --bind -s MODULARIZE=1 -s EXPORT_NAME="createSimulator" -O2
    ```

2. **Navigate to the frontend directory**:
    ```bash
    cd frontend
    ```

3. **Install dependencies**:
    ```bash
    npm install
    ```

4. **Start the development server**:
    ```bash
    npm run dev
    ```

5. **Open your browser** and navigate to `http://localhost:3000` to access the web interface.

To build for production:
```bash
npm run build
npm run start
```