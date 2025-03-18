"use client";

import Link from "next/link";
import Editor from "@/components/Editor";
import Simulator from "@/components/Simulator";
import { Github, Loader2, AlertCircle, AlertTriangle } from 'lucide-react';
import { useState } from "react";
import { useSimulator } from '@/hooks/useSimulator';
import Script from 'next/script';
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { useRouter } from "next/navigation";

declare global {
  interface Window {
    simulatorScriptLoaded?: boolean;
  }
}

export default function Landing() {
  const router = useRouter();
  const { simulator, loading, error } = useSimulator();
  const [activeTab, setActiveTab] = useState<"editor" | "simulator">("editor");
  const [code, setCode] = useState<string>(`0x00000000 0x00000000 addi x1, x0, 10
0x00000000 0x00000014 addi x2, x0, 0x14
0x00000000 0x00000000 add x4, x1, x2`);

  const handleScriptLoad = () => {
    window.simulatorScriptLoaded = true;
    window.dispatchEvent(new Event('simulator-ready'));
  };

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
        <div className="h-screen flex items-center justify-center bg-gray-50 p-4">
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
        <div className="h-screen flex items-center justify-center bg-gray-50 p-4">
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
        <div className="h-screen flex items-center justify-center bg-gray-50 p-4">
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
    <main className="h-screen w-screen bg-gray-50 flex justify-center items-center">
      {scriptElement}
      <Link href="https://github.com/rit3sh-x/RISC-V-Aseembler" target="_blank">
        <Github className="fixed top-4 right-4 text-gray-500 hover:text-gray-700 transition-colors" />
      </Link>
      <div className="fixed top-4 mx-auto flex gap-2 bg-white p-2 rounded-lg shadow-md z-[999]">
        <div
          className={`cursor-pointer px-4 py-2 rounded-md transition ${activeTab === "editor" ? "bg-gray-200 font-semibold" : "text-gray-500"
            }`}
          onClick={() => {
            setActiveTab("editor");
          }}
        >
          Editor
        </div>
        <div
          className={`cursor-pointer px-4 py-2 rounded-md transition ${activeTab === "simulator" ? "bg-gray-200 font-semibold" : "text-gray-500"
            }`}
          onClick={() => {
            setActiveTab("simulator");
          }}
        >
          Simulator
        </div>
      </div>
      <div className="w-[95%] h-[95%]">
        {activeTab === "editor" ? (
          <Editor text={code} setText={setCode} setActiveTab={setActiveTab} />
        ) : (
          <Simulator
            text={code}
            simulatorInstance={simulator}
          />
        )}
      </div>
    </main>
  );
}