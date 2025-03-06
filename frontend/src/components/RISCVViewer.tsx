"use client"

import type React from "react"

import { useState, useRef } from "react"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import { Button } from "@/components/ui/button"
import { Upload, Copy, Download, RefreshCw, Play, StepForward, ChevronUp, ChevronDown, Hash } from "lucide-react"

// Add constants at the top of the file
const ITEMS_PER_PAGE = 10

// Define types for our RISC-V simulator
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

// Update the MEMORY_SEGMENTS constant with correct addresses
const MEMORY_SEGMENTS = {
  CODE: "0x00000000",
  DATA: "0x10000000",
  HEAP: "0x10008000",
  STACK: "0x7FFFFFFC",
}

// Add ABI register names mapping
const REGISTER_ABI_NAMES = [
  "zero",
  "ra",
  "sp",
  "gp",
  "tp",
  "t0",
  "t1",
  "t2",
  "s0/fp",
  "s1",
  "a0",
  "a1",
  "a2",
  "a3",
  "a4",
  "a5",
  "a6",
  "a7",
  "s2",
  "s3",
  "s4",
  "s5",
  "s6",
  "s7",
  "s8",
  "s9",
  "s10",
  "s11",
  "t3",
  "t4",
  "t5",
  "t6",
]

// Update the component to include the new features
export default function RiscvViewer() {
  // State for the assembled code
  const [assembledCode, setAssembledCode] = useState<AssembledInstruction[]>([])

  // State for registers (x0-x31)
  const [registers, setRegisters] = useState<Register[]>(
    Array.from({ length: 32 }, (_, i) => ({
      name: `x${i}`,
      value: "00000000",
    })),
  )

  // State for memory cells
  const [memory, setMemory] = useState<MemoryCell[]>(
    Array.from({ length: ITEMS_PER_PAGE }, (_, i) => ({
      address: `0x${(i * 4).toString(16).padStart(8, '0').toUpperCase()}`,
      value: "00000000",
    })),
  )

  // State for active tab
  const [activeTab, setActiveTab] = useState("registers")

  // State for current memory view start index
  const [memoryStartIndex, setMemoryStartIndex] = useState(0)

  // State for display format (hex or decimal)
  const [displayFormat, setDisplayFormat] = useState<"hex" | "decimal">("hex")

  // State for current PC (program counter)
  const [currentPC, setCurrentPC] = useState(0)

  // File input reference
  const fileInputRef = useRef<HTMLInputElement>(null)

  // Add state for pagination
  const [codeStartIndex, setCodeStartIndex] = useState(0)
  const [registerStartIndex, setRegisterStartIndex] = useState(0)

  // Handle file upload
  const handleFileUpload = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (!file) return

    const reader = new FileReader()
    reader.onload = (event) => {
      const content = event.target?.result as string

      // Parse the machine code file
      // This is a simplified example - actual parsing would depend on the file format
      const parsedInstructions = content
        .split("\n")
        .filter((line) => line.trim() !== "")
        .map((line, index) => {
          // Simple parsing logic - in a real app, this would be more complex
          return {
            pc: `0x${(0x10000000 + index * 4).toString(16).toUpperCase()}`,
            machineCode: line.trim().substring(0, 8),
            basicCode: `ADDI x${(index % 31) + 1}, x0, ${index}`, // Example instruction
            originalCode: `# Original code for line ${index + 1}`,
          }
        })

      setAssembledCode(parsedInstructions)
      setCurrentPC(0)

      // Reset registers and memory with default values
      resetRegistersAndMemory()
    }

    reader.readAsText(file)
  }

  // Reset registers and memory to default values
  const resetRegistersAndMemory = () => {
    setRegisters(
      Array.from({ length: 32 }, (_, i) => ({
        name: `x${i}`,
        value: "00000000",
      })),
    )

    setMemory(
      Array.from({ length: ITEMS_PER_PAGE }, (_, i) => ({
        address: `0x${(i * 4).toString(16).padStart(8, '0').toUpperCase()}`,
        value: "00000000",
      })),
    )
  }

  // Trigger file input click
  const triggerFileUpload = () => {
    fileInputRef.current?.click()
  }

  // Step execution - execute one instruction
  const handleStep = () => {
    if (currentPC < assembledCode.length - 1) {
      setCurrentPC(currentPC + 1)

      // Simulate register changes (in a real app, this would execute the actual instruction)
      const updatedRegisters = [...registers]
      const randomRegIndex = Math.floor(Math.random() * 31) + 1 // Don't modify x0
      updatedRegisters[randomRegIndex].value = Math.floor(Math.random() * 0xffffffff)
        .toString(16)
        .padStart(8, "0")
        .toUpperCase()
      setRegisters(updatedRegisters)
    }
  }

  // Run execution - execute all instructions
  const handleRun = () => {
    setCurrentPC(assembledCode.length - 1)

    // Simulate multiple register changes
    const updatedRegisters = [...registers]
    for (let i = 0; i < 5; i++) {
      const randomRegIndex = Math.floor(Math.random() * 31) + 1 // Don't modify x0
      updatedRegisters[randomRegIndex].value = Math.floor(Math.random() * 0xffffffff)
        .toString(16)
        .padStart(8, "0")
        .toUpperCase()
    }
    setRegisters(updatedRegisters)
  }

  // Navigate memory up
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

  // Navigate memory down
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

  // Toggle display format between hex and decimal
  const toggleDisplayFormat = () => {
    setDisplayFormat(displayFormat === "hex" ? "decimal" : "hex")
  }

  // Jump to specific memory segment
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
      })),
    )
  }

  // Format value based on current display format
  const formatValue = (hexValue: string) => {
    if (displayFormat === "hex") {
      return `0x${hexValue.toUpperCase()}`
    } else {
      // Parse the full hex value and convert to decimal
      return parseInt(hexValue, 16).toString()
    }
  }

  // Get visible memory cells based on current start index
  const visibleMemoryCells = () => {
    return memory
  }

  // Add navigation functions for code
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

  // Add navigation functions for registers
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
    <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 bg-white rounded-lg shadow-md p-6">
      {/* Hidden file input */}
      <input type="file" ref={fileInputRef} className="hidden" onChange={handleFileUpload} accept=".bin,.txt,.hex" />

      {/* Left side - Assembled Code */}
      <div className="border rounded-lg overflow-hidden flex flex-col">
        <div className="bg-gray-100 p-4 flex justify-between items-center">
          <div className="flex gap-2">
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

        <div className="overflow-y-auto flex-grow">
          <div className="flex flex-col h-full">
            <div className="overflow-y-auto flex-1">
              <table className="w-full text-sm">
                <thead>
                  <tr className="bg-gray-50 border-b">
                    <th className="py-2 px-4 text-left font-medium text-gray-500">PC</th>
                    <th className="py-2 px-4 text-left font-medium text-gray-500">Machine Code</th>
                    <th className="py-2 px-4 text-left font-medium text-gray-500">Basic Code</th>
                  </tr>
                </thead>
                <tbody>
                  {assembledCode.length > 0 ? (
                    assembledCode.slice(codeStartIndex, codeStartIndex + ITEMS_PER_PAGE).map((instruction, index) => (
                      <tr key={codeStartIndex + index} className={`border-b hover:bg-gray-50 ${codeStartIndex + index === currentPC ? "bg-blue-50" : ""}`}>
                        <td className="py-2 px-4 font-mono">{instruction.pc}</td>
                        <td className="py-2 px-4 font-mono">{instruction.machineCode}</td>
                        <td className="py-2 px-4 font-mono">{instruction.basicCode}</td>
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

            <div className="bg-gray-100 p-2 flex justify-between gap-2 border-t">
              <div className="flex gap-2">
                <Button variant="outline" size="sm" onClick={navigateCodeUp} disabled={codeStartIndex <= 0}>
                  <ChevronUp className="h-4 w-4" />
                </Button>
                <Button variant="outline" size="sm" onClick={navigateCodeDown} disabled={codeStartIndex + ITEMS_PER_PAGE >= assembledCode.length}>
                  <ChevronDown className="h-4 w-4" />
                </Button>
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

      {/* Right side - Registers and Memory */}
      <div className="border rounded-lg overflow-hidden h-[calc(100vh-22rem)]">
        <Tabs defaultValue="registers" onValueChange={setActiveTab} className="h-full flex flex-col">
          <div className="bg-gray-100 p-2 border-b">
            <TabsList className="grid w-full grid-cols-2">
              <TabsTrigger value="registers">Registers</TabsTrigger>
              <TabsTrigger value="memory">Memory</TabsTrigger>
            </TabsList>
          </div>

          <TabsContent value="registers" className="flex-1 flex flex-col data-[state=inactive]:hidden">
            <div className="bg-gray-100 p-2 flex justify-between items-center border-b">
              <div className="text-sm text-gray-500">Register Values</div>
              <Button variant="outline" size="sm" onClick={toggleDisplayFormat}>
                <Hash className="h-4 w-4 mr-2" />
                {displayFormat === "hex" ? "Show Decimal" : "Show Hex"}
              </Button>
            </div>
            <div className="overflow-y-auto flex-1">
              <table className="w-full text-sm">
                <thead>
                  <tr className="bg-gray-50 border-b">
                    <th className="py-2 px-4 text-left font-medium text-gray-500">Register</th>
                    <th className="py-2 px-4 text-left font-medium text-gray-500">Value</th>
                  </tr>
                </thead>
                <tbody>
                  {registers.slice(registerStartIndex, registerStartIndex + ITEMS_PER_PAGE).map((reg, index) => (
                    <tr key={registerStartIndex + index} className="border-b hover:bg-gray-50">
                      <td className="py-2 px-4 font-mono">
                        {reg.name} <span className="text-gray-500">({REGISTER_ABI_NAMES[registerStartIndex + index]})</span>
                      </td>
                      <td className="py-2 px-4 font-mono">{formatValue(reg.value)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
            <div className="bg-gray-100 p-2 flex justify-center items-center border-t">
              <div className="flex gap-2">
                <Button variant="outline" size="sm" onClick={navigateRegistersUp} disabled={registerStartIndex <= 0}>
                  <ChevronUp className="h-4 w-4" />
                </Button>
                <Button variant="outline" size="sm" onClick={navigateRegistersDown} disabled={registerStartIndex + ITEMS_PER_PAGE >= registers.length}>
                  <ChevronDown className="h-4 w-4" />
                </Button>
              </div>
            </div>
          </TabsContent>

          <TabsContent value="memory" className="flex-1 flex flex-col data-[state=inactive]:hidden">
            <div className="bg-gray-100 p-2 flex justify-between items-center border-b">
              <div className="flex gap-2">
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
              <div className="flex gap-2">
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
                  {memory.map((cell, index) => (
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
            <div className="bg-gray-100 p-2 flex justify-center items-center border-t">
              <div className="flex gap-2">
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
            </div>
          </TabsContent>
        </Tabs>
      </div>

      {/* Console output */}
      <div className="col-span-1 lg:col-span-2 border rounded-lg overflow-hidden">
        <div className="bg-gray-100 p-4">
          <h2 className="text-lg font-semibold">Console Output</h2>
        </div>
        <div className="p-4 font-mono text-sm h-32 bg-gray-50 overflow-y-auto">
          {assembledCode.length > 0 ? (
            <>
              Machine code loaded successfully. Ready to simulate.
              <br />
              {currentPC > 0 && `Executed ${currentPC} instruction(s).`}
            </>
          ) : (
            "No code loaded. Upload a RISC-V machine code file to begin."
          )}
        </div>
      </div>
    </div>
  )
}