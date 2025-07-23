"use client";

import { Editor } from "./monaco";
import { Simulator } from "./simulator";
import { Loader2, AlertCircle, AlertTriangle } from 'lucide-react';
import { useState, useEffect } from "react";
import { useSimulator } from "@/hooks/use-simulator";
import Script from 'next/script';
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { useRouter } from "next/navigation";
import { useSimulator as useStateMachine } from "@/store/zustand";

declare global {
    interface Window {
        simulatorScriptLoaded?: boolean;
    }
}

export type SimulationControls = {
    pipelining: boolean;
    dataForwarding: boolean;
}

export default function Landing() {
    const router = useRouter();
    const { currentTab, machineCode, setMachineCode, dataForwarding, pipelining } = useStateMachine();
    const { simulator, loading, error } = useSimulator();
    const [pipelineSvg, setPipelineSvg] = useState<string | null>(null);
    const [pipelineLightSvg, setPipelineLightSvg] = useState<string | null>(null);

    const handleScriptLoad = () => {
        window.simulatorScriptLoaded = true;
        window.dispatchEvent(new Event('simulator-ready'));
    };

    useEffect(() => {
        const fetchSvg = async () => {
            try {
                const response = await fetch('/pipeline.svg');
                if (response.ok) {
                    const svgText = await response.text();
                    setPipelineSvg(svgText);
                } else {
                    console.error('Failed to load pipeline SVG');
                }
            } catch (error) {
                console.error('Error loading pipeline SVG:', error);
            }
            try {
                const response = await fetch('/pipeline-light.svg');
                if (response.ok) {
                    const svgText = await response.text();
                    setPipelineLightSvg(svgText);
                } else {
                    console.error('Failed to load pipeline SVG');
                }
            } catch (error) {
                console.error('Error loading pipeline SVG:', error);
            }
        };

        fetchSvg();
    }, []);

    const scriptElement = (
        <Script
            src="/wasm/simulator.js"
            strategy="afterInteractive"
            onLoad={handleScriptLoad}
        />
    );

    if (loading) {
        return (
            <>
                {scriptElement}
                <div className="h-screen flex items-center justify-center bg-background p-4">
                    <Card className="w-full max-w-md shadow-lg">
                        <CardContent className="p-6">
                            <div className="flex flex-col items-center justify-center space-y-4 py-8">
                                <Loader2 className="h-12 w-12 text-gray-700 animate-spin" />
                                <h3 className="text-xl font-medium text-center">Loading RISC-V simulator...</h3>
                                <p className="text-gray-500 text-center">
                                    Please wait while we initialize the WebAssembly environment
                                </p>
                            </div>
                        </CardContent>
                    </Card>
                </div>
            </>
        );
    }

    if (error) {
        return (
            <>
                {scriptElement}
                <div className="h-screen flex items-center justify-center bg-background p-4">
                    <Card className="w-full max-w-md border-red-300 shadow-lg">
                        <CardContent className="p-6">
                            <div className="flex flex-col items-center justify-center space-y-4 py-8">
                                <div className="rounded-full bg-red-100 p-3">
                                    <AlertCircle className="h-10 w-10 text-red-500" />
                                </div>
                                <h3 className="text-xl font-medium text-center">Error loading simulator</h3>
                                <p className="text-red-500 text-center">{error}</p>
                                <Button
                                    onClick={() => router.refresh()}
                                    variant="outline"
                                    className="mt-4 border-gray-300"
                                >
                                    Retry
                                </Button>
                            </div>
                        </CardContent>
                    </Card>
                </div>
            </>
        );
    }

    if (!simulator) {
        return (
            <>
                {scriptElement}
                <div className="h-screen flex items-center justify-center bg-background p-4">
                    <Card className="w-full max-w-md border-amber-300 shadow-lg">
                        <CardContent className="p-6">
                            <div className="flex flex-col items-center justify-center space-y-4 py-8">
                                <div className="rounded-full bg-amber-100 p-3">
                                    <AlertTriangle className="h-10 w-10 text-amber-500" />
                                </div>
                                <h3 className="text-xl font-medium text-center">Simulator not available</h3>
                                <p className="text-gray-600 text-center">
                                    The RISC-V simulator could not be initialized. This might be due to
                                    browser compatibility issues or missing WebAssembly support.
                                </p>
                                <Button
                                    onClick={() => router.refresh()}
                                    variant="outline"
                                    className="mt-4 border-gray-300"
                                >
                                    Try again
                                </Button>
                            </div>
                        </CardContent>
                    </Card>
                </div>
            </>
        );
    }

    return (
        <main className="w-full bg-background flex justify-center items-center h-full">
            <div className="w-full px-8 py-2 h-full">
                {currentTab === "editor" ? (
                    <Editor text={machineCode} setText={setMachineCode} />
                ) : (
                    <Simulator
                        text={machineCode}
                        simulatorInstance={simulator}
                        controls={{ dataForwarding, pipelining }}
                        pipelineSvg={pipelineSvg}
                        pipelineLightSvg={pipelineLightSvg}
                    />
                )}
            </div>
        </main>
    );
}