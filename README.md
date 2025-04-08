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

### 1. 💻 Simulator (src/simulator.cpp)
The simulator executes RISC-V machine code in a virtual environment. It provides:

- Instruction-by-instruction execution
- Register and memory state monitoring
- Console output for debugging
- Step-by-step execution with detailed logging

The simulator can be used both as a standalone C++ application and as a WebAssembly module in the web frontend.

### 2. 🌐 NextJS Web Frontend
The project includes a modern web-based interface built with NextJS that allows users to:
- Write and edit RISC-V assembly code
- Assemble the code to machine code
- Simulate the execution with visual feedback
- Observe register and memory states during execution

The frontend uses React for the UI components and WebAssembly to run the C++ simulator directly in the browser, providing near-native performance.

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

## ⚙️ Running the Components

### 💻 Simulator
1. **Compile the simulator**:
    ```bash
    g++ -o riscv_simulator ./src/main.cpp
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