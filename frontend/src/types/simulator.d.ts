declare namespace createSimulator {
  export enum Stage {
    FETCH = 0,
    DECODE = 1,
    EXECUTE = 2,
    MEMORY = 3,
    WRITEBACK = 4
  }

  export interface InstructionRegisters {
    RA: number;
    RB: number;
    RM: number;
    RY: number;
    RZ: number;
  }

  export interface Simulator {
    loadProgram(input: string): boolean;
    step(): boolean;
    run(): void;
    reset(): void;
    getRegisters(): number[];
    getPC(): number;
    getCycles(): number;
    getDataMap(): Record<string, number>;
    getTextMap(): Record<string, { first: number, second: string }>;
    getLogs(): Record<string, string>;
    isRunning(): boolean;
    getActiveStages(): Record<number, { active: boolean, instruction: number }>;
    getStalls(): number;
    setEnvironment(pipeline: boolean, dataForwarding: boolean, followedInstruction?: number): void;
    getInstructionRegisters(): InstructionRegisters;
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