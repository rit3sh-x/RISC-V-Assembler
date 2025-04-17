declare namespace createSimulator {
  export enum Stage {
    FETCH = 0,
    DECODE = 1,
    EXECUTE = 2,
    MEMORY = 3,
    WRITEBACK = 4
  }

  export class InstructionRegisters {
    RA: number;
    RB: number;
    RM: number;
    RY: number;
    RZ: number;
  
    constructor(RA = 0, RB = 0, RM = 0, RY = 0, RZ = 0) {
      this.RA = RA;
      this.RB = RB;
      this.RM = RM;
      this.RY = RY;
      this.RZ = RZ;
    }
  }


  export interface UIResponse {
    isFlushed: boolean;
    isStalled: boolean;
    isDataForwarded: boolean;
    isProgramTerminated: boolean;
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
    getUIResponse(): UIResponse;
  }

  export interface SimulatorModuleInstance {
    Simulator: {
      new(): Simulator;
    };
    Stage: typeof Stage;
    InstructionRegisters: typeof InstructionRegisters;
    UIResponse: typeof UIResponse;
  }
}

declare function createSimulator(): Promise<createSimulator.SimulatorModuleInstance>;
export = createSimulator;