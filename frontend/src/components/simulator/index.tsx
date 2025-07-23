"use client"

import React, { useState, useEffect, useCallback, useMemo } from "react"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { Button } from "@/components/ui/button"
import { RefreshCw, Play, StepForward, ChevronUp, ChevronDown, Hash, SquareCode } from "lucide-react"
import {
    Dialog,
    DialogContent,
    DialogDescription,
    DialogHeader,
    DialogTitle
} from "@/components/ui/dialog"
import { toast } from "sonner";
import { PipelineDiagram } from "./pipeline"
import { ITEMS_PER_PAGE, Stage, StageNames } from "@/constants"
import { byteToSignedDecimal } from "@/lib/utils"
import { SimulatorErrorBoundary } from "./simulator-error-boundary"
import { RegisterTable } from "./register-table"
import { MemoryTable } from "./memory-table"
import { StatsTable } from "./stats-table"
import { PipelineStages } from "./pipeline-stages"
import { useCurrentTheme } from "@/hooks/use-current-theme"
import { useSimulator } from "@/store/zustand"

interface PipelineDiagramInfo {
    ExExForwarding: boolean;
    MemMemForwarding: boolean;
    MemExForwarding: boolean;
    BranchToFetch: boolean;
    ExToBranch: boolean;
}

class InstructionRegisters {
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

interface SimulationStats {
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

interface UIResponse {
    isFlushed: boolean;
    isStalled: boolean;
    isDataForwarded: boolean;
    isProgramTerminated: boolean;
}

interface SimulatorProps {
    text: string;
    simulatorInstance: Simulator;
    controls: SimulationControls;
    pipelineSvg: string | null;
    pipelineLightSvg: string | null;
}

const MEMORY_SEGMENTS = {
    CODE: "0x00000000",
    DATA: "0x10000000",
    STACK: "0x7FFFFFFC",
    END: "0x80000000",
}

interface TextMapEntry {
    first: number;
    second: string;
}

const StageColorClasses: { [key: string]: string } = {
    [StageNames[Stage.FETCH]]: 'bg-blue-500',
    [StageNames[Stage.DECODE]]: 'bg-yellow-500',
    [StageNames[Stage.EXECUTE]]: 'bg-red-500',
    [StageNames[Stage.MEMORY]]: 'bg-green-500',
    [StageNames[Stage.WRITEBACK]]: 'bg-purple-500',
};


export function Simulator({
    text,
    simulatorInstance,
    controls,
    pipelineSvg,
    pipelineLightSvg
}: SimulatorProps) {
    const currentTheme = useCurrentTheme();
    const { isRunning, setIsRunning } = useSimulator();

    const [registers, setRegisters] = useState<number[]>(Array.from({ length: 32 }, () => 0));
    const [cycles, setCycles] = useState<number>(0);
    const [dataMap, setDataMap] = useState<Record<string, number>>({});
    const [textMap, setTextMap] = useState<Record<string, { first: number, second: string }>>({});
    const [terminal, setTerminal] = useState<Record<string, string>>({});
    const [activeTab, setActiveTab] = useState<string>("registers");
    const [consoleActiveTab, setConsoleActiveTab] = useState<string>("pipeline");

    const [activeStates, setActiveStates] = useState<Record<number, { active: boolean, instruction: number }>>({
        [Stage.FETCH]: { active: false, instruction: 0 },
        [Stage.DECODE]: { active: false, instruction: 0 },
        [Stage.EXECUTE]: { active: false, instruction: 0 },
        [Stage.MEMORY]: { active: false, instruction: 0 },
        [Stage.WRITEBACK]: { active: false, instruction: 0 }
    });
    const [pipelineRegisters, setPipelineRegisters] = useState<InstructionRegisters>(new InstructionRegisters());
    const [isHex, setIsHex] = useState<boolean>(true);
    const [instruction, setInstruction] = useState<number>(0);
    const [stalls, setStalls] = useState<number>(0);
    const [memoryStartIndex, setMemoryStartIndex] = useState<number>(0);
    const [memoryEntries, setMemoryEntries] = useState<MemoryCell[]>([]);
    const [displayFormat, setDisplayFormat] = useState<"hex" | "decimal">("hex");
    const [registerStartIndex, setRegisterStartIndex] = useState(0);
    const [isDialogOpen, setIsDialogOpen] = useState(false);
    const [dialogMessage, setDialogMessage] = useState({
        title: "",
        description: ""
    });
    const [uiResponse, setUIResponse] = useState<UIResponse>({
        isFlushed: false,
        isStalled: false,
        isDataForwarded: false,
        isProgramTerminated: false
    });
    const [stats, setStats] = useState<SimulationStats>({
        totalCycles: 0,
        stallBubbles: 0,
        pipelineFlushes: 0,
        dataHazards: 0,
        controlHazards: 0,
        dataHazardStalls: 0,
        controlHazardStalls: 0,
        aluInstructions: 0,
        dataTransferInstructions: 0,
        controlInstructions: 0,
        cyclesPerInstruction: 0,
        instructionsExecuted: 0,
        branchPredictionAccuracy: 0
    });
    const [pipelineDiag, setPipelineDiag] = useState<PipelineDiagramInfo>({
        ExExForwarding: false,
        MemMemForwarding: false,
        MemExForwarding: false,
        BranchToFetch: false,
        ExToBranch: false
    });

    const showTerminalErrorDialog = useCallback((errorMessage: string) => {
        setDialogMessage({
            title: "Error 404: Not Found",
            description: errorMessage
        });
        setIsDialogOpen(true);
        if (simulatorInstance) {
            simulatorInstance.reset();
        }
    }, [simulatorInstance]);

    const updateSimulatorState = useCallback(() => {
        if (!simulatorInstance) return;

        setUIResponse(simulatorInstance.getUIResponse());

        const newTerminal = simulatorInstance.getLogs();
        setTerminal(newTerminal);

        setRegisters(simulatorInstance.getRegisters());
        setCycles(simulatorInstance.getCycles());
        setDataMap(simulatorInstance.getDataMap());
        setTextMap(simulatorInstance.getTextMap());
        setStalls(simulatorInstance.getStalls());
        setActiveStates(simulatorInstance.getActiveStages());
        setIsRunning(simulatorInstance.isRunning());
        setPipelineRegisters(simulatorInstance.getInstructionRegisters());
        setInstruction(simulatorInstance.getPC());
        setStats(simulatorInstance.getStats());
        setPipelineDiag(simulatorInstance.getPipelineDiagramInfo());

        if (newTerminal["404"]) {
            setTimeout(() => showTerminalErrorDialog(newTerminal["404"]), 0);
        }
    }, [simulatorInstance, showTerminalErrorDialog, setIsRunning]);

    const memoizedUpdateMemoryEntries = useMemo(() => {
        return () => {
            const entries = Array.from({ length: ITEMS_PER_PAGE }, (_, index) => {
                const addressNum = memoryStartIndex + index * 4;
                const address = addressNum.toString();
                const addressString = addressNum.toString(16).padStart(8, '0').toUpperCase();

                if (addressNum >= parseInt(MEMORY_SEGMENTS.END, 16)) {
                    return {
                        address: "----------",
                        value: "00000000",
                        bytes: ["--", "--", "--", "--"]
                    };
                }

                let value = 0;
                let found = false;
                let bytesDisplay = ["00", "00", "00", "00"];

                if (addressNum >= 0x00000000 && addressNum <= 0x0FFFFFFC && textMap[address]) {
                    value = textMap[address].first;
                    found = true;
                }
                else if (addressNum >= 0x10000000 && addressNum <= 0x80000000) {
                    const byteAddresses = [
                        addressNum,
                        addressNum + 1,
                        addressNum + 2,
                        addressNum + 3
                    ];
                    const bytes = byteAddresses.map(addr => {
                        const addrStr = addr.toString();
                        return dataMap[addrStr] !== undefined ? dataMap[addrStr] : 0;
                    });
                    if (bytes.some(b => b !== 0)) {
                        found = true;
                        value = (bytes[3] << 0) | (bytes[2] << 8) | (bytes[1] << 16) | (bytes[0] << 24);
                        bytesDisplay = [
                            bytes[3], bytes[2], bytes[1], bytes[0]
                        ].map(byte => {
                            let byteValue = byte;
                            if (byteValue < 0) {
                                byteValue = byteValue & 0xFF;
                            }
                            const byteHex = byteValue.toString(16).padStart(2, '0').toUpperCase();
                            const byteDecimal = isHex ? byteHex : byteToSignedDecimal(byteValue);
                            return isHex ? byteHex : byteDecimal.toString();
                        });
                    }
                }
                let valueHex;
                if (value < 0) {
                    valueHex = (value >>> 0).toString(16).padStart(8, '0').toUpperCase();
                } else {
                    valueHex = value.toString(16).padStart(8, '0').toUpperCase();
                }
                return {
                    address: addressString,
                    value: valueHex,
                    bytes: found ? bytesDisplay : ["00", "00", "00", "00"]
                };
            });
            setMemoryEntries(entries);
        };
    }, [memoryStartIndex, isHex, textMap, dataMap]);

    const handleStep = useCallback(() => {
        simulatorInstance.step();
        updateSimulatorState();
    }, [simulatorInstance, updateSimulatorState]);

    const resetRegistersAndMemory = useCallback(() => {
        simulatorInstance.reset();
        updateSimulatorState();
        memoizedUpdateMemoryEntries();
    }, [simulatorInstance, updateSimulatorState, memoizedUpdateMemoryEntries]);

    const handleRun = useCallback(() => {
        simulatorInstance.run();
        updateSimulatorState();
    }, [simulatorInstance, updateSimulatorState]);

    useEffect(() => {
        if (uiResponse.isFlushed) {
            toast.info("Pipeline Flushed", { description: "The pipeline has been flushed." });
        }
    }, [uiResponse.isFlushed]);

    useEffect(() => {
        if (uiResponse.isStalled) {
            toast.warning("Pipeline Stalled", { description: "The pipeline has encountered a stall." });
        }
    }, [uiResponse.isStalled]);

    useEffect(() => {
        if (uiResponse.isDataForwarded) {
            toast.success("Data Forwarded", { description: "Data forwarding has occurred in the pipeline." });
        }
    }, [uiResponse.isDataForwarded]);

    useEffect(() => {
        if (uiResponse.isProgramTerminated) {
            toast.error("Program Terminated", { description: "The program has terminated execution." });
        }
    }, [uiResponse.isProgramTerminated]);

    const handleAssemble = useCallback(() => {
        const success = simulatorInstance.loadProgram(text);
        if (success) {
            toast.success("Program Loaded", { description: "RISC-V program loaded successfully.", });
        } else {
            toast.error("Load Failed", { description: "Failed to load program: " });
        }
        updateSimulatorState();
    }, [text, simulatorInstance, updateSimulatorState]);

    useEffect(() => {
        updateSimulatorState();
    }, [updateSimulatorState]);

    useEffect(() => {
        memoizedUpdateMemoryEntries();
    }, [memoizedUpdateMemoryEntries]);

    useEffect(() => {
        if (!simulatorInstance) return;
        simulatorInstance.setEnvironment(
            controls.pipelining,
            controls.dataForwarding
        );
        updateSimulatorState();
    }, [controls.pipelining, controls.dataForwarding, simulatorInstance, updateSimulatorState]);

    const navigateMemoryUp = () => {
        setMemoryStartIndex(prev => Math.max(0, prev - ITEMS_PER_PAGE * 4));
    };

    const navigateMemoryDown = () => {
        setMemoryStartIndex(prev => {
            const maxAddr = parseInt(MEMORY_SEGMENTS.END, 16) - (ITEMS_PER_PAGE * 4);
            return Math.min(maxAddr, prev + ITEMS_PER_PAGE * 4);
        });
    };

    const toggleDisplayFormat = () => {
        setDisplayFormat(displayFormat === "hex" ? "decimal" : "hex");
        setIsHex(!isHex);
    };

    const jumpToSegment = (segment: string) => {
        let baseAddress = 0;
        switch (segment) {
            case MEMORY_SEGMENTS.CODE: baseAddress = 0x00000000; break;
            case MEMORY_SEGMENTS.DATA: baseAddress = 0x10000000; break;
            case MEMORY_SEGMENTS.STACK: baseAddress = 0x7ffffffc; break;
        }
        setMemoryStartIndex(baseAddress);
    };

    const navigateRegistersUp = () => {
        if (registerStartIndex > 0) {
            setRegisterStartIndex(prev => Math.max(0, prev - ITEMS_PER_PAGE));
        }
    };

    const navigateRegistersDown = () => {
        if (registerStartIndex + ITEMS_PER_PAGE < registers.length) {
            setRegisterStartIndex(prev => Math.min(registers.length - ITEMS_PER_PAGE, prev + ITEMS_PER_PAGE));
        }
    };

    const renderTerminalOutput = () => {
        if (Object.entries(terminal).length === 0) {
            return isRunning ? (
                <div className="text-muted-foreground">
                    Nothing to log.
                </div>
            ) : (
                <div className="text-muted-foreground">
                    No code loaded. Assemble RISC-V machine code to begin.
                </div>
            );
        }

        return Object.entries(terminal).map(([key, value], index) => {
            switch (key) {
                case '400':
                    return (
                        <div key={index} className="mb-1 text-destructive">
                            {value}
                        </div>
                    );
                case '300':
                    return (
                        <div key={index} className="mb-1 text-orange-500">
                            {value}
                        </div>
                    );
                case '200':
                    return (
                        <div key={index} className="mb-1 text-green-600">
                            {value}
                        </div>
                    );
                default:
                    return (
                        <div key={index} className="mb-1">
                            {value}
                        </div>
                    );
            }
        });
    };

    return (
        <SimulatorErrorBoundary>
            <div className="w-full h-full bottom-0">
                <Dialog open={isDialogOpen} onOpenChange={setIsDialogOpen}>
                    <DialogContent>
                        <DialogHeader>
                            <DialogTitle>{dialogMessage.title}</DialogTitle>
                            <DialogDescription>{dialogMessage.description}</DialogDescription>
                        </DialogHeader>
                        <Button onClick={() => setIsDialogOpen(false)}>Close</Button>
                    </DialogContent>
                </Dialog>
                <div className="grid grid-rows-8 h-full gap-2">
                    <div className="row-span-6 grid grid-cols-1 xl:grid-cols-2 gap-2">
                        <div className="border rounded-lg overflow-hidden bg-card shadow-sm h-full flex flex-col">
                            <div className="bg-muted p-3 flex justify-between items-center border-b">
                                <div className="flex gap-2 flex-wrap">
                                    <Button variant="outline" size="sm" onClick={handleAssemble}>
                                        <SquareCode className="h-4 w-4 mr-2" />
                                        Assemble
                                    </Button>
                                    <Button variant="outline" size="sm" onClick={resetRegistersAndMemory}>
                                        <RefreshCw className="h-4 w-4 mr-2" />
                                        Reset
                                    </Button>
                                </div>
                            </div>

                            <div className="flex-1 overflow-hidden">
                                <div className="h-full flex flex-col">
                                    <div className="flex-1 overflow-y-auto scrollbar-thin scrollbar-thumb-muted-foreground scrollbar-track-muted">
                                        <table className="w-full text-sm table-fixed">
                                            <thead className="sticky top-0 bg-card shadow-sm">
                                                <tr className="bg-muted border-b">
                                                    <th className="py-2 px-3 text-left font-medium text-muted-foreground w-[25%]">PC</th>
                                                    <th className="py-2 px-3 text-left font-medium text-muted-foreground w-[25%]">Machine Code</th>
                                                    <th className="py-2 px-3 text-left font-medium text-muted-foreground w-[50%]">Basic Code</th>
                                                </tr>
                                            </thead>
                                            <tbody className="divide-y divide-border">
                                                {Object.keys(textMap).length > 0 ? (
                                                    Object.entries(textMap as Record<string, TextMapEntry>).map(([key, instObj]) => {
                                                        const instructionAddress = parseInt(key);

                                                        const activeStagesForInstruction = Object.entries(activeStates)
                                                            .filter(([, stageInfo]) => stageInfo.active && stageInfo.instruction === instructionAddress)
                                                            .map(([stageKey]) => parseInt(stageKey));

                                                        let rowClass = "hover:bg-muted/50 h-10 transition-colors duration-300";
                                                        if (activeStagesForInstruction.length > 0) {
                                                            const mostAdvancedStage = Math.max(...activeStagesForInstruction) as Stage;
                                                            const stageName = StageNames[mostAdvancedStage];
                                                            const colorClass = StageColorClasses[stageName];
                                                            rowClass += ` ${colorClass} text-primary-foreground`;
                                                        }

                                                        return (
                                                            <tr key={key} className={rowClass}>
                                                                <td className="py-2 px-3 font-mono whitespace-nowrap">
                                                                    0x{instructionAddress.toString(16).padStart(8, '0').toUpperCase()}
                                                                </td>
                                                                <td className="py-2 px-3 font-mono whitespace-nowrap">
                                                                    0x{instObj.first.toString(16).padStart(8, '0').toUpperCase()}
                                                                </td>
                                                                <td className="py-2 px-3 font-mono whitespace-nowrap">
                                                                    {instObj.second}
                                                                </td>
                                                            </tr>
                                                        );
                                                    })
                                                ) : (
                                                    <tr>
                                                        <td colSpan={3} className="py-8 text-center text-muted-foreground">
                                                            No code loaded. Assemble a RISC-V program to begin.
                                                        </td>
                                                    </tr>
                                                )}
                                            </tbody>
                                        </table>
                                    </div>

                                    <div className="bg-muted p-3 flex justify-between gap-2 border-t mt-auto">
                                        <div className="flex items-center gap-2 text-sm text-muted-foreground">
                                            <span>Total Instructions: {Object.keys(textMap).length ?? 0}</span>
                                        </div>
                                        <div className="flex gap-2">
                                            <Button
                                                variant="outline"
                                                size="sm"
                                                onClick={handleStep}
                                                disabled={!isRunning}
                                            >
                                                <StepForward className="h-4 w-4 mr-2" />
                                                Step
                                            </Button>
                                            <Button
                                                variant="default"
                                                size="sm"
                                                onClick={handleRun}
                                                disabled={!isRunning}
                                            >
                                                <Play className="h-4 w-4 mr-2" />
                                                Run
                                            </Button>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>

                        <div className="border rounded-lg overflow-hidden bg-card shadow-sm h-full flex flex-col">
                            <div className="bg-muted p-3 border-b">
                                <h2 className="text-lg font-semibold text-right">System Board</h2>
                            </div>

                            <div className="flex-1 flex flex-col overflow-hidden">
                                <Tabs
                                    defaultValue="registers"
                                    className="flex-1 flex flex-col"
                                    onValueChange={(value) => setActiveTab(value)}
                                >
                                    <TabsList className="grid w-full grid-cols-3">
                                        <TabsTrigger value="registers">Registers</TabsTrigger>
                                        <TabsTrigger value="memory">Memory</TabsTrigger>
                                        <TabsTrigger value="stats">Stats</TabsTrigger>
                                    </TabsList>

                                    <TabsContent value="registers" className="flex-1 flex flex-col data-[state=inactive]:hidden">
                                        <div className="bg-muted p-3 flex justify-between items-center border-b">
                                            <div className="flex items-center gap-4">
                                                <div className="text-sm text-muted-foreground">Register Values</div>
                                            </div>
                                            <div className="flex gap-2">
                                                <div className="flex gap-1">
                                                    <Button variant="outline" size="icon" onClick={navigateRegistersUp} disabled={registerStartIndex <= 0} >
                                                        <ChevronUp className="h-4 w-4" />
                                                    </Button>
                                                    <Button variant="outline" size="icon" onClick={navigateRegistersDown} disabled={registerStartIndex + ITEMS_PER_PAGE >= registers.length} >
                                                        <ChevronDown className="h-4 w-4" />
                                                    </Button>
                                                </div>
                                                <Button variant="outline" size="sm" onClick={toggleDisplayFormat}>
                                                    <Hash className="h-4 w-4 mr-2" />
                                                    {displayFormat === "hex" ? "Show Decimal" : "Show Hex"}
                                                </Button>
                                            </div>
                                        </div>
                                        <div className="overflow-y-auto flex-1">
                                            <RegisterTable
                                                registers={registers}
                                                registerStartIndex={registerStartIndex}
                                                isHex={isHex}
                                            />
                                        </div>
                                    </TabsContent>

                                    <TabsContent value="memory" className="flex-1 flex flex-col data-[state=inactive]:hidden">
                                        <div className="bg-muted p-3 flex flex-col sm:flex-row justify-between items-center gap-2 border-b">
                                            <div className="flex flex-wrap gap-2">
                                                <Button variant="outline" size="sm" onClick={() => jumpToSegment(MEMORY_SEGMENTS.CODE)}>
                                                    Code
                                                </Button>
                                                <Button variant="outline" size="sm" onClick={() => jumpToSegment(MEMORY_SEGMENTS.DATA)}>
                                                    Data
                                                </Button>
                                                <Button variant="outline" size="sm" onClick={() => jumpToSegment(MEMORY_SEGMENTS.STACK)}>
                                                    Stack
                                                </Button>
                                            </div>
                                            <div className="flex items-center gap-2">
                                                <div className="flex gap-1">
                                                    <Button variant="outline" size="icon" onClick={navigateMemoryUp} disabled={memoryStartIndex <= 0} >
                                                        <ChevronUp className="h-4 w-4" />
                                                    </Button>
                                                    <Button variant="outline" size="icon" onClick={navigateMemoryDown} disabled={memoryStartIndex >= parseInt(MEMORY_SEGMENTS.END, 16) - (ITEMS_PER_PAGE * 4)} >
                                                        <ChevronDown className="h-4 w-4" />
                                                    </Button>
                                                </div>
                                                <Button variant="outline" size="sm" onClick={toggleDisplayFormat}>
                                                    <Hash className="h-4 w-4 mr-2" />
                                                    {displayFormat === "hex" ? "Decimal" : "Hex"}
                                                </Button>
                                            </div>
                                        </div>
                                        <div className="overflow-y-auto flex-1">
                                            <MemoryTable entries={memoryEntries} />
                                        </div>
                                    </TabsContent>

                                    <TabsContent value="stats" className="flex-1 flex flex-col data-[state=inactive]:hidden">
                                        <div className="bg-muted p-3 flex justify-between items-center border-b">
                                            <div className="text-sm text-muted-foreground">Simulation Statistics</div>
                                        </div>
                                        <div className="overflow-y-auto flex-1 p-4">
                                            <StatsTable stats={{
                                                "Total Cycles": stats.totalCycles,
                                                "Instructions Executed": stats.instructionsExecuted,
                                                "CPI (Cycles Per Instruction)": Number(Number(stats.cyclesPerInstruction).toFixed(2)),
                                                "Data-Transfer Instructions": stats.dataTransferInstructions,
                                                "ALU Instructions": stats.aluInstructions,
                                                "Control Instructions": stats.controlInstructions,
                                                "Stalls/Bubbles": stats.stallBubbles,
                                                "Data Hazards": stats.dataHazards,
                                                "Control Hazards": stats.controlHazards,
                                                "Pipeline flushes": stats.pipelineFlushes,
                                                "Stalls due to Data Hazards": stats.dataHazardStalls,
                                                "Stalls due to Control Hazards": stats.controlHazardStalls,
                                                "Branch Prediction Accuracy": Number(Number(stats.branchPredictionAccuracy).toFixed(2))
                                            }} />
                                        </div>
                                    </TabsContent>
                                </Tabs>

                                {activeTab !== "stats" && (
                                    <div className="bg-muted p-3 border-t mt-auto">
                                        <PipelineStages activeStates={activeStates} running={isRunning} />
                                    </div>
                                )}
                            </div>
                        </div>
                    </div>

                    <div className="row-span-2 grid grid-cols-1 md:grid-cols-2 gap-2">
                        <div className="border rounded-lg overflow-hidden bg-card shadow-sm h-full flex flex-col">
                            <div className="bg-muted p-3 border-b">
                                <div className="flex justify-between items-center">
                                    <h2 className="text-lg font-semibold">Output</h2>
                                    <div className="flex gap-1">
                                        <Button
                                            variant={consoleActiveTab === "pipeline" ? "default" : "outline"}
                                            size="sm"
                                            onClick={() => setConsoleActiveTab("pipeline")}
                                        >
                                            Pipeline
                                        </Button>
                                        <Button
                                            variant={consoleActiveTab === "console" ? "default" : "outline"}
                                            size="sm"
                                            onClick={() => setConsoleActiveTab("console")}
                                        >
                                            Console
                                        </Button>
                                    </div>
                                </div>
                            </div>
                            <div className="flex-1 p-3 font-mono text-sm bg-muted/50 overflow-y-auto h-full">
                                <div className="h-full w-full flex items-center justify-center text-muted-foreground">
                                    {consoleActiveTab === "console" ? (
                                        renderTerminalOutput()
                                    ) : (
                                        <PipelineDiagram
                                            fetchStart={activeStates[Stage.FETCH].active}
                                            decodeStart={activeStates[Stage.DECODE].active}
                                            executeStart={activeStates[Stage.EXECUTE].active}
                                            memoryStart={activeStates[Stage.MEMORY].active}
                                            writebackStart={activeStates[Stage.WRITEBACK].active}
                                            exEx={pipelineDiag.ExExForwarding}
                                            memMem={pipelineDiag.MemMemForwarding}
                                            memEx={pipelineDiag.MemExForwarding}
                                            branchFetch={pipelineDiag.BranchToFetch}
                                            branchExecute={pipelineDiag.ExToBranch}
                                            arrowData={pipelineDiag.ExExForwarding || pipelineDiag.MemExForwarding}
                                            arrowBranch={pipelineDiag.ExToBranch}
                                            arrowMemory={pipelineDiag.MemMemForwarding}
                                            arrowFetch={pipelineDiag.BranchToFetch}
                                            svgData={currentTheme === "dark" ? pipelineLightSvg : pipelineSvg}
                                        />
                                    )}
                                </div>
                            </div>
                        </div>
                        <div className="border rounded-lg overflow-hidden bg-card shadow-sm h-full flex flex-col">
                            <div className="bg-muted p-3 border-b">
                                <h2 className="text-lg font-semibold">System Information</h2>
                            </div>
                            <div className="flex-1 p-3 font-mono text-sm bg-muted/50 overflow-y-auto">
                                <div className="grid grid-cols-2 gap-4">
                                    <div>
                                        <h3 className="font-semibold mb-2 text-foreground">System Variables</h3>
                                        <div className="text-xs space-y-1 text-muted-foreground">
                                            <div>RA: {`0x${((pipelineRegisters["RA"] ?? 0) >>> 0).toString(16).toUpperCase().padStart(8, '0')}`}</div>
                                            <div>RB: {`0x${((pipelineRegisters["RB"] ?? 0) >>> 0).toString(16).toUpperCase().padStart(8, '0')}`}</div>
                                            <div>RM: {`0x${((pipelineRegisters["RM"] ?? 0) >>> 0).toString(16).toUpperCase().padStart(8, '0')}`}</div>
                                            <div>RY: {`0x${((pipelineRegisters["RY"] ?? 0) >>> 0).toString(16).toUpperCase().padStart(8, '0')}`}</div>
                                        </div>
                                    </div>
                                    <div>
                                        <h3 className="font-semibold mb-2 text-foreground">&nbsp;</h3>
                                        <div className="text-xs space-y-1 text-muted-foreground">
                                            <div>Stalls: {!isRunning ? 0 : stalls}</div>
                                            <div>Clock Cycles: {cycles ?? 0}</div>
                                            <div>PC: 0x{(instruction && isRunning) ?
                                                ((instruction) >>> 0).toString(16).toUpperCase().padStart(8, '0') :
                                                '00000000'}</div>
                                            <div>RZ: {`0x${((pipelineRegisters["RZ"] ?? 0) >>> 0).toString(16).toUpperCase().padStart(8, '0')}`}</div>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </SimulatorErrorBoundary>
    )
}