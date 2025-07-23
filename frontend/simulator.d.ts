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

        constructor(RA?: number, RB?: number, RM?: number, RY?: number, RZ?: number);
    }

    export interface UIResponse {
        isFlushed: boolean;
        isStalled: boolean;
        isDataForwarded: boolean;
        isProgramTerminated: boolean;
    }

    export interface SimulationStats {
        totalCycles: number;
        stallBubbles: number;
        pipelineFlushes: number;
        dataHazards: number;
        controlHazards: number;
        dataHazardStalls: number;
        controlHazardStalls: number;
        aluInstructions: number;
        dataTransferInstructions: number;
        controlInstructions: number;
        cyclesPerInstruction: number;
        instructionsExecuted: number;
        branchPredictionAccuracy: number;
    }

    export interface PipelineDiagramInfo {
        ExExForwarding: boolean;
        MemMemForwarding: boolean;
        MemExForwarding: boolean;
        BranchToFetch: boolean;
        ExToBranch: boolean;
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
        setEnvironment(pipeline: boolean, dataForwarding: boolean): void;
        getInstructionRegisters(): InstructionRegisters;
        getUIResponse(): UIResponse;
        getStats(): SimulationStats;
        getPipelineDiagramInfo(): PipelineDiagramInfo;
    }

    export interface SimulatorModuleInstance {
        Simulator: {
            new(): Simulator;
        };
        Stage: typeof Stage;
        InstructionRegisters: typeof InstructionRegisters;
        UIResponse: typeof UIResponse;
        SimulationStats: typeof SimulationStats;
        PipelineDiagramInfo: typeof PipelineDiagramInfo;
    }

    export type SimulationControls = {
        pipelining: boolean;
        dataForwarding: boolean;
    }

    type MemoryCell = {
        address: string
        value: string
        bytes: string[]
    }
}

declare global {
    type MemoryCell = createSimulator.MemoryCell;
    type SimulationControls = createSimulator.SimulationControls;
    type Simulator = createSimulator.Simulator;
    type SimulatorModuleInstance = createSimulator.SimulatorModuleInstance;
    type Stage = createSimulator.Stage;
    type InstructionRegisters = createSimulator.InstructionRegisters;
    type UIResponse = createSimulator.UIResponse;
    type SimulationStats = createSimulator.SimulationStats;
    type PipelineDiagramInfo = createSimulator.PipelineDiagramInfo;
}

declare function createSimulator(): Promise<createSimulator.SimulatorModuleInstance>;
export = createSimulator;