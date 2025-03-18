declare namespace createSimulator {
  export interface Simulator {
    loadProgram(input: string): boolean;
    step(): boolean;
    run(): void;
    parseInstructions(instHex: number): string;
    getRegisters(): number[];
    getPC(): number;
    getCycles(): number;
    getDataMap(): Record<string, number>;
    getTextMap(): Record<string, { first: number, second: string }>;
    getMemoryChanges(): Record<string, number>;
    getConsoleOutput(): Record<string, string>;
    getPipelineRegisters(): Record<string, number>;
    getCurrentStage(): number;
    isRunning(): boolean;
    getLastError(): string;
    clearError(): void;
  }

  export enum Stage {
    FETCH = 0,
    DECODE = 1,
    EXECUTE = 2,
    MEMORY = 3,
    WRITEBACK = 4
  }

  export interface SimulatorModuleInstance {
    Simulator: {
      new(): Simulator;
    };
    Stage: typeof Stage;
  }
}

declare function createSimulator(): Promise<createSimulator.SimulatorModuleInstance>;
export = createSimulator;