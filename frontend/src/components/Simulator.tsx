"use client"

import type React from "react"

import { useState, useRef } from "react"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { Button } from "@/components/ui/button"
import { Upload, Copy, Download, RefreshCw, Play, StepForward, ChevronUp, ChevronDown, Hash } from "lucide-react"

const ITEMS_PER_PAGE = 10

type Register = {
  name: string
  value: string
}

type MemoryCell = {
  address: string
  value: string
}

type AssembledInstruction = {
  pc: string
  machineCode: string
  basicCode: string
}

const MEMORY_SEGMENTS = {
  CODE: "0x00000000",
  DATA: "0x10000000",
  HEAP: "0x10008000",
  STACK: "0x7FFFFFFC",
}

const REGISTER_ABI_NAMES = [
  "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0/fp", "s1",
  "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
  "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
  "t3", "t4", "t5", "t6",
]

type SystemInfo = {
  clockCycles: number
  currentStage: string
  pipelineStatus: {
    IF: {status: string, color: string}
    ID: {status: string, color: string}
    EX: {status: string, color: string}
    MEM: {status: string, color: string}
    WB: {status: string, color: string}
  }
}

export default function Simulator({ text }: { text: string }) {
  const [assembledCode, setAssembledCode] = useState<AssembledInstruction[]>([])
  const [registers, setRegisters] = useState<Register[]>(
    Array.from({ length: 32 }, (_, i) => ({
      name: `x${i}`,
      value: "00000000",
    }))
  )
  const [memory, setMemory] = useState<MemoryCell[]>(
    Array.from({ length: ITEMS_PER_PAGE }, (_, i) => ({
      address: `0x${(i * 4).toString(16).padStart(8, '0').toUpperCase()}`,
      value: "00000000",
    }))
  )
  const [activeTab, setActiveTab] = useState("registers")
  const [memoryStartIndex, setMemoryStartIndex] = useState(0)
  const [displayFormat, setDisplayFormat] = useState<"hex" | "decimal">("hex")
  const [currentPC, setCurrentPC] = useState(0)
  const fileInputRef = useRef<HTMLInputElement>(null)
  const [codeStartIndex, setCodeStartIndex] = useState(0)
  const [registerStartIndex, setRegisterStartIndex] = useState(0)
  const stageColors = {
    IF: "#60A5FA",
    ID: "#34D399",
    EX: "#F59E0B",
    MEM: "#EC4899",
    WB: "#8B5CF6"
  }
  const [systemInfo, setSystemInfo] = useState<SystemInfo>({
    clockCycles: 0,
    currentStage: "IF",
    pipelineStatus: {
      IF: { status: "Idle", color: stageColors.IF },
      ID: { status: "Idle", color: stageColors.ID },
      EX: { status: "Idle", color: stageColors.EX },
      MEM: { status: "Idle", color: stageColors.MEM },
      WB: { status: "Idle", color: stageColors.WB }
    }
  })
  const [consoleOutput, setConsoleOutput] = useState<string[]>([])

  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (!file) return

    const reader = new FileReader()
    reader.onload = (event: ProgressEvent<FileReader>) => {
      try {
        const content = event.target?.result as string
        const lines = content.split("\n").filter(line => line.trim() !== "")
        const parsedInstructions = lines.map((line: string, index: number) => ({
          pc: `0x${(index * 4).toString(16).padStart(8, '0').toUpperCase()}`,
          machineCode: line.trim(),
          basicCode: `Instruction ${index + 1}`,
        }))

        setAssembledCode(parsedInstructions)
        setCurrentPC(0)
        setConsoleOutput(prev => [...prev, "Program loaded successfully"])
      } catch (error) {
        const errorMessage = error instanceof Error ? error.message : 'Unknown error'
        setConsoleOutput(prev => [...prev, `Error loading file: ${errorMessage}`])
      }
    }

    reader.readAsText(file)
  }

  const handleStep = () => {
    if (currentPC < assembledCode.length - 1) {
      setCurrentPC(prev => prev + 1)
      
      // Simulate register changes
      setRegisters(prev => {
        const newRegisters = [...prev]
        const randomRegIndex = Math.floor(Math.random() * 31) + 1 // Skip x0
        const randomValue = Math.floor(Math.random() * 0xFFFFFFFF).toString(16).padStart(8, '0')
        newRegisters[randomRegIndex] = { ...newRegisters[randomRegIndex], value: randomValue }
        return newRegisters
      })
      
      // Simulate memory changes
      setMemory(prev => {
        const newMemory = [...prev]
        const randomMemIndex = Math.floor(Math.random() * ITEMS_PER_PAGE)
        const randomValue = Math.floor(Math.random() * 0xFFFFFFFF).toString(16).padStart(8, '0')
        newMemory[randomMemIndex] = { ...newMemory[randomMemIndex], value: randomValue }
        return newMemory
      })
      
      // Update system info
      setSystemInfo(prev => ({
        ...prev,
        clockCycles: prev.clockCycles + 1,
        currentStage: ["IF", "ID", "EX", "MEM", "WB"][Math.floor(Math.random() * 5)],
        pipelineStatus: {
          IF: { status: "Fetching instruction", color: stageColors.IF },
          ID: { status: "Decoding", color: stageColors.ID },
          EX: { status: "Executing ALU operation", color: stageColors.EX },
          MEM: { status: "Accessing memory", color: stageColors.MEM },
          WB: { status: "Writing back result", color: stageColors.WB }
        }
      }))
      
      setConsoleOutput(prev => [...prev, `Executed instruction at PC: 0x${(currentPC * 4).toString(16).padStart(8, '0')}`])
    }
  }

  const resetRegistersAndMemory = () => {
    setRegisters(
      Array.from({ length: 32 }, (_, i) => ({
        name: `x${i}`,
        value: "00000000",
      }))
    )
    setMemory(
      Array.from({ length: ITEMS_PER_PAGE }, (_, i) => ({
        address: `0x${(i * 4).toString(16).padStart(8, '0').toUpperCase()}`,
        value: "00000000",
      }))
    )
    setCurrentPC(0)
    setSystemInfo({
      clockCycles: 0,
      currentStage: "IF",
      pipelineStatus: {
        IF: { status: "Idle", color: stageColors.IF },
        ID: { status: "Idle", color: stageColors.ID },
        EX: { status: "Idle", color: stageColors.EX },
        MEM: { status: "Idle", color: stageColors.MEM },
        WB: { status: "Idle", color: stageColors.WB }
      }
    })
    setConsoleOutput(["Simulator reset"])
  }

  const handleRun = async () => {
    const originalPC = currentPC
    let currentPc = originalPC
    
    for (let i = 0; i < 10 && currentPc < assembledCode.length - 1; i++) {
      currentPc++
      setCurrentPC(currentPc)
      
      // Simulate register and memory changes
      setRegisters(prev => {
        const newRegisters = [...prev]
        const randomRegIndex = Math.floor(Math.random() * 31) + 1
        const randomValue = Math.floor(Math.random() * 0xFFFFFFFF).toString(16).padStart(8, '0')
        newRegisters[randomRegIndex] = { ...newRegisters[randomRegIndex], value: randomValue }
        return newRegisters
      })
      
      setSystemInfo(prev => ({
        ...prev,
        clockCycles: prev.clockCycles + 1
      }))
      
      setConsoleOutput(prev => [...prev, `Executed instruction at PC: 0x${(currentPc * 4).toString(16).padStart(8, '0')}`])
      
      // Add a small delay to simulate execution
      await new Promise(resolve => setTimeout(resolve, 100))
    }
    
    if (currentPc >= assembledCode.length - 1) {
      setConsoleOutput(prev => [...prev, "Program execution completed"])
    } else {
      setConsoleOutput(prev => [...prev, "Program execution paused"])
    }
  }

  const triggerFileUpload = () => {
    fileInputRef.current?.click()
  }

  const navigateMemoryUp = () => {
    setMemory(prev => {
      const firstAddr = parseInt(prev[0].address, 16)
      if (firstAddr <= 0) return prev
      const newStartAddr = Math.max(0, firstAddr - (ITEMS_PER_PAGE * 4))
      return Array.from({ length: ITEMS_PER_PAGE }, (_, i) => ({
        address: `0x${(newStartAddr + i * 4).toString(16).padStart(8, '0').toUpperCase()}`,
        value: "00000000",
      }))
    })
  }

  const navigateMemoryDown = () => {
    setMemory(prev => {
      const firstAddr = parseInt(prev[0].address, 16)
      if (firstAddr >= 0x80000000 - (ITEMS_PER_PAGE * 4)) return prev
      const newStartAddr = firstAddr + (ITEMS_PER_PAGE * 4)
      return Array.from({ length: ITEMS_PER_PAGE }, (_, i) => ({
        address: `0x${(newStartAddr + i * 4).toString(16).padStart(8, '0').toUpperCase()}`,
        value: "00000000",
      }))
    })
  }

  const toggleDisplayFormat = () => {
    setDisplayFormat(displayFormat === "hex" ? "decimal" : "hex")
  }

  const jumpToSegment = (segment: string) => {
    let baseAddress = 0

    switch (segment) {
      case MEMORY_SEGMENTS.CODE:
        baseAddress = 0x00000000
        break
      case MEMORY_SEGMENTS.DATA:
        baseAddress = 0x10000000
        break
      case MEMORY_SEGMENTS.HEAP:
        baseAddress = 0x10008000
        break
      case MEMORY_SEGMENTS.STACK:
        baseAddress = 0x7ffffffc
        break
    }

    setMemory(
      Array.from({ length: ITEMS_PER_PAGE }, (_, i) => ({
        address: `0x${(baseAddress + i * 4).toString(16).padStart(8, '0').toUpperCase()}`,
        value: "00000000",
      }))
    )
  }

  const formatValue = (hexValue: string) => {
    if (displayFormat === "hex") {
      return `0x${hexValue.toUpperCase()}`
    } else {
      return parseInt(hexValue, 16).toString()
    }
  }

  const navigateCodeUp = () => {
    if (codeStartIndex > 0) {
      setCodeStartIndex(prev => Math.max(0, prev - ITEMS_PER_PAGE))
    }
  }

  const navigateCodeDown = () => {
    if (codeStartIndex + ITEMS_PER_PAGE < assembledCode.length) {
      setCodeStartIndex(prev => Math.min(assembledCode.length - ITEMS_PER_PAGE, prev + ITEMS_PER_PAGE))
    }
  }

  const navigateRegistersUp = () => {
    if (registerStartIndex > 0) {
      setRegisterStartIndex(prev => Math.max(0, prev - ITEMS_PER_PAGE))
    }
  }

  const navigateRegistersDown = () => {
    if (registerStartIndex + ITEMS_PER_PAGE < registers.length) {
      setRegisterStartIndex(prev => Math.min(registers.length - ITEMS_PER_PAGE, prev + ITEMS_PER_PAGE))
    }
  }

  return (
    <div className="w-full h-full p-[2%]">
      <input type="file" ref={fileInputRef} className="hidden" onChange={handleFileUpload} accept=".bin,.txt,.hex,.mc" />

      <div className="grid grid-rows-8 h-full gap-[2%]">
        <div className="row-span-6 grid grid-cols-1 xl:grid-cols-2 gap-[2%]">
          <div className="border rounded-lg overflow-hidden bg-white shadow-sm h-full flex flex-col">
            <div className="bg-gray-100 py-2 px-3 flex justify-between items-center border-b min-h-[8%]">
              <div className="flex gap-2 flex-wrap">
                <Button variant="outline" size="sm" onClick={triggerFileUpload}>
                  <Upload className="h-4 w-4 mr-2" />
                  Upload
                </Button>
                <Button variant="outline" size="sm">
                  <Copy className="h-4 w-4 mr-2" />
                  Copy
                </Button>
                <Button variant="outline" size="sm">
                  <Download className="h-4 w-4 mr-2" />
                  Download
                </Button>
                <Button variant="outline" size="sm" onClick={resetRegistersAndMemory}>
                  <RefreshCw className="h-4 w-4 mr-2" />
                  Reset
                </Button>
              </div>
            </div>

            <div className="flex-1 overflow-hidden">
              <div className="h-full flex flex-col">
                <div className="h-[calc(10*2.5rem)] overflow-y-auto scrollbar-thin scrollbar-thumb-gray-300 scrollbar-track-gray-100">
                  <table className="w-full text-sm table-fixed">
                    <thead className="sticky top-0 bg-white shadow-sm z-10">
                      <tr className="bg-gray-50 border-b">
                        <th className="py-2 px-3 text-left font-medium text-gray-500 w-[25%]">PC</th>
                        <th className="py-2 px-3 text-left font-medium text-gray-500 w-[25%]">Machine Code</th>
                        <th className="py-2 px-3 text-left font-medium text-gray-500 w-[50%]">Basic Code</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-gray-100">
                      {assembledCode.length > 0 ? (
                        assembledCode.map((instruction, index) => (
                          <tr key={index} 
                              className={`hover:bg-gray-50 ${index === currentPC ? "bg-blue-50" : ""} h-10`}
                              onClick={() => setCurrentPC(index)}>
                            <td className="py-2 px-3 font-mono whitespace-nowrap">{instruction.pc}</td>
                            <td className="py-2 px-3 font-mono whitespace-nowrap">{instruction.machineCode}</td>
                            <td className="py-2 px-3 font-mono whitespace-nowrap">{instruction.basicCode}</td>
                          </tr>
                        ))
                      ) : (
                        <tr>
                          <td colSpan={4} className="py-8 text-center text-gray-500">
                            No code loaded. Upload a RISC-V machine code file to view assembled code.
                          </td>
                        </tr>
                      )}
                    </tbody>
                  </table>
                </div>

                <div className="bg-gray-100 py-2 px-3 flex justify-between gap-2 border-t mt-auto min-h-[8%]">
                  <div className="flex items-center gap-2 text-sm text-gray-600">
                    <span>Total Instructions: {assembledCode.length}</span>
                    {currentPC >= 0 && (
                      <span className="text-blue-600">Current: {currentPC + 1}</span>
                    )}
                  </div>
                  <div className="flex gap-2">
                    <Button
                      variant="outline"
                      size="sm"
                      onClick={handleStep}
                      disabled={assembledCode.length === 0 || currentPC >= assembledCode.length - 1}
                    >
                      <StepForward className="h-4 w-4 mr-2" />
                      Step
                    </Button>
                    <Button
                      variant="default"
                      size="sm"
                      onClick={handleRun}
                      disabled={assembledCode.length === 0 || currentPC >= assembledCode.length - 1}
                    >
                      <Play className="h-4 w-4 mr-2" />
                      Run
                    </Button>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <div className="border rounded-lg overflow-hidden bg-white shadow-sm h-full flex flex-col">
            <div className="bg-gray-100 py-2 px-3 border-b min-h-[8%]">
              <h2 className="text-lg font-semibold">System Board</h2>
            </div>
            
            <div className="flex-1 flex flex-col overflow-hidden">
              <Tabs defaultValue="registers" className="flex-1 flex flex-col">
                <TabsList className="grid w-full grid-cols-2 min-h-[8%]">
                  <TabsTrigger value="registers">Registers</TabsTrigger>
                  <TabsTrigger value="memory">Memory</TabsTrigger>
                </TabsList>

                <TabsContent value="registers" className="flex-1 flex flex-col data-[state=inactive]:hidden">
                  <div className="bg-gray-100 py-2 px-3 flex justify-between items-center border-b min-h-[8%]">
                    <div className="flex items-center gap-4">
                      <div className="text-sm text-gray-500">Register Values</div>
                    </div>
                    <div className="flex gap-2">
                      <div className="flex gap-1">
                        <Button 
                          variant="outline" 
                          size="sm" 
                          onClick={navigateRegistersUp}
                          disabled={registerStartIndex <= 0}
                        >
                          <ChevronUp className="h-4 w-4" />
                        </Button>
                        <Button 
                          variant="outline" 
                          size="sm" 
                          onClick={navigateRegistersDown}
                          disabled={registerStartIndex + ITEMS_PER_PAGE >= registers.length}
                        >
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
                    <table className="w-full text-sm table-fixed">
                      <thead>
                        <tr className="bg-gray-50 border-b">
                          <th className="py-2 px-4 text-left font-medium text-gray-500">Register</th>
                          <th className="py-2 px-4 text-left font-medium text-gray-500">Value</th>
                        </tr>
                      </thead>
                      <tbody>
                        {registers.slice(registerStartIndex, registerStartIndex + ITEMS_PER_PAGE).map((reg, index) => (
                          <tr key={registerStartIndex + index} className="border-b hover:bg-gray-50">
                            <td className="py-2 px-3 font-mono">
                              {reg.name} <span className="text-gray-500">({REGISTER_ABI_NAMES[registerStartIndex + index]})</span>
                            </td>
                            <td className="py-2 px-3 font-mono">{formatValue(reg.value)}</td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                </TabsContent>

                <TabsContent value="memory" className="flex-1 flex flex-col data-[state=inactive]:hidden">
                  <div className="bg-gray-100 py-2 px-3 flex flex-col sm:flex-row justify-between items-center gap-2 border-b min-h-[8%]">
                    <div className="flex flex-wrap gap-2">
                      <Button variant="outline" size="sm" onClick={() => jumpToSegment(MEMORY_SEGMENTS.CODE)}>
                        Code
                      </Button>
                      <Button variant="outline" size="sm" onClick={() => jumpToSegment(MEMORY_SEGMENTS.DATA)}>
                        Data
                      </Button>
                      <Button variant="outline" size="sm" onClick={() => jumpToSegment(MEMORY_SEGMENTS.HEAP)}>
                        Heap
                      </Button>
                      <Button variant="outline" size="sm" onClick={() => jumpToSegment(MEMORY_SEGMENTS.STACK)}>
                        Stack
                      </Button>
                    </div>
                    <div className="flex items-center gap-2">
                      <div className="flex gap-1">
                        <Button 
                          variant="outline" 
                          size="sm" 
                          onClick={navigateMemoryUp}
                          disabled={parseInt(memory[0].address, 16) <= 0}
                        >
                          <ChevronUp className="h-4 w-4" />
                        </Button>
                        <Button 
                          variant="outline" 
                          size="sm" 
                          onClick={navigateMemoryDown}
                          disabled={parseInt(memory[0].address, 16) >= 0x80000000 - (ITEMS_PER_PAGE * 4)}
                        >
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
                    <table className="w-full text-sm">
                      <thead>
                        <tr className="bg-gray-50 border-b">
                          <th className="py-2 px-4 text-left font-medium text-gray-500">Address</th>
                          <th className="py-2 px-4 text-left font-medium text-gray-500">+0</th>
                          <th className="py-2 px-4 text-left font-medium text-gray-500">+1</th>
                          <th className="py-2 px-4 text-left font-medium text-gray-500">+2</th>
                          <th className="py-2 px-4 text-left font-medium text-gray-500">+3</th>
                        </tr>
                      </thead>
                      <tbody>
                        {memory.slice(memoryStartIndex, memoryStartIndex + ITEMS_PER_PAGE).map((cell, index) => (
                          <tr key={index} className="border-b hover:bg-gray-50">
                            <td className="py-2 px-4 font-mono">
                              {cell.address > "0x80000000" ? "----------" : cell.address}
                            </td>
                            {cell.address > "0x80000000" ? (
                              <>
                                <td className="py-2 px-4 font-mono">--</td>
                                <td className="py-2 px-4 font-mono">--</td>
                                <td className="py-2 px-4 font-mono">--</td>
                                <td className="py-2 px-4 font-mono">--</td>
                              </>
                            ) : (
                              <>
                                <td className="py-2 px-4 font-mono">
                                  {displayFormat === "hex"
                                    ? cell.value.substring(0, 2)
                                    : Number.parseInt(cell.value.substring(0, 2), 16).toString()}
                                </td>
                                <td className="py-2 px-4 font-mono">
                                  {displayFormat === "hex"
                                    ? cell.value.substring(2, 4)
                                    : Number.parseInt(cell.value.substring(2, 4), 16).toString()}
                                </td>
                                <td className="py-2 px-4 font-mono">
                                  {displayFormat === "hex"
                                    ? cell.value.substring(4, 6)
                                    : Number.parseInt(cell.value.substring(4, 6), 16).toString()}
                                </td>
                                <td className="py-2 px-4 font-mono">
                                  {displayFormat === "hex"
                                    ? cell.value.substring(6, 8)
                                    : Number.parseInt(cell.value.substring(6, 8), 16).toString()}
                                </td>
                              </>
                            )}
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                </TabsContent>
              </Tabs>

              <div className="bg-gray-50 py-2 px-3 border-t mt-auto min-h-[15%]">
                <div className="grid grid-cols-5 gap-2 mb-4">
                  {Object.entries(systemInfo.pipelineStatus).map(([stage, status]) => (
                    <div
                      key={stage}
                      className={`p-2 rounded ${
                        systemInfo.currentStage === stage 
                          ? 'border-2 border-gray-600' 
                          : 'border border-gray-200'
                      }`}
                      style={{ backgroundColor: status.color }}
                    >
                      <div className="font-semibold text-white">{stage}</div>
                      <div className="text-xs text-white/90">{status.status}</div>
                    </div>
                  ))}
                </div>
                <div className="text-sm text-gray-600">
                  Clock Cycles: {systemInfo.clockCycles}
                </div>
              </div>
            </div>
          </div>
        </div>

        <div className="row-span-2 grid grid-cols-1 md:grid-cols-2 gap-[2%]">
          <div className="border rounded-lg overflow-hidden bg-white shadow-sm h-full flex flex-col">
            <div className="bg-gray-100 py-2 px-3 border-b min-h-[15%]">
              <h2 className="text-lg font-semibold">Console Output</h2>
            </div>
            <div className="flex-1 p-3 font-mono text-sm bg-gray-50 overflow-y-auto">
              {consoleOutput.map((line, index) => (
                <div key={index} className="mb-1">{line}</div>
              ))}
              {consoleOutput.length === 0 && (
                <div className="text-gray-500">
                  No code loaded. Upload a RISC-V machine code file to begin.
                </div>
              )}
            </div>
          </div>

          <div className="border rounded-lg overflow-hidden bg-white shadow-sm h-full flex flex-col">
            <div className="bg-gray-100 py-2 px-3 border-b min-h-[15%]">
              <h2 className="text-lg font-semibold">System Information</h2>
            </div>
            <div className="flex-1 p-3 font-mono text-sm bg-gray-50 overflow-y-auto">
              <div className="grid grid-cols-2 gap-4">
                <div>
                  <h3 className="font-semibold mb-2">Memory Segments</h3>
                  <div className="text-xs space-y-1">
                    <div>Code: {MEMORY_SEGMENTS.CODE}</div>
                    <div>Data: {MEMORY_SEGMENTS.DATA}</div>
                    <div>Heap: {MEMORY_SEGMENTS.HEAP}</div>
                    <div>Stack: {MEMORY_SEGMENTS.STACK}</div>
                  </div>
                </div>
                <div>
                  <h3 className="font-semibold mb-2">Pipeline Status</h3>
                  <div className="text-xs space-y-1">
                    <div>Current Stage: {systemInfo.currentStage}</div>
                    <div>Clock Cycles: {systemInfo.clockCycles}</div>
                    <div>Instructions: {assembledCode.length}</div>
                    <div>Current PC: 0x{currentPC.toString(16).toUpperCase()}</div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  )
}