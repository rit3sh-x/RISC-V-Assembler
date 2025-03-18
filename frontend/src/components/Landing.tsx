"use client";

import Link from "next/link";
import Editor from "@/components/Editor";
import Simulator from "@/components/Simulator";
import { Github } from "lucide-react";
import { useState, useEffect } from "react";
import { useSimulator } from '@/hooks/useSimulator';
import Script from 'next/script';

declare global {
  interface Window {
    simulatorScriptLoaded?: boolean;
  }
}

const Landing = () => {
  const { simulator, loading, error } = useSimulator();
  const [activeTab, setActiveTab] = useState<"editor" | "simulator">("editor");
  const [code, setCode] = useState<string>(`0x00000000 0x00000000 addi x1, x0, 10
0x00000000 0x00000014 addi x2, x0, 0x14
0x00000000 0x00000000 add x4, x1, x2`);

  useEffect(() => {
    const savedTab = localStorage.getItem('activeTab') as "editor" | "simulator" | null;
    if (savedTab) {
      setActiveTab(savedTab);
    } else {
      localStorage.setItem('activeTab', 'editor');
    }
  }, []);

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

  if (loading) return (
    <>
      {scriptElement}
      <div className="h-screen flex items-center justify-center">
        <div className="text-lg font-medium">Loading RISC-V simulator...</div>
      </div>
    </>
  );

  if (error) return (
    <>
      {scriptElement}
      <div className="h-screen flex items-center justify-center">
        <div className="text-red-500">Error loading simulator: {error}</div>
      </div>
    </>
  );

  if (!simulator) return (
    <>
      {scriptElement}
      <div className="h-screen flex items-center justify-center">
        <div className="text-amber-500">Simulator not available</div>
      </div>
    </>
  );

  return (
    <main className="h-screen w-screen bg-gray-50 flex justify-center items-center">
      {scriptElement}
      <Link href="https://github.com/rit3sh-x/RISC-V-Aseembler" target="_blank">
        <Github className="fixed top-4 right-4 text-gray-500 hover:text-gray-700 transition-colors" />
      </Link>
      <div className="fixed top-4 mx-auto flex gap-2 bg-white p-2 rounded-lg shadow-md z-[999]">
        <div
          className={`cursor-pointer px-4 py-2 rounded-md transition ${
            activeTab === "editor" ? "bg-gray-200 font-semibold" : "text-gray-500"
          }`}
          onClick={() => {
            setActiveTab("editor");
            localStorage.setItem('activeTab', 'editor');
          }}
        >
          Editor
        </div>
        <div
          className={`cursor-pointer px-4 py-2 rounded-md transition ${
            activeTab === "simulator" ? "bg-gray-200 font-semibold" : "text-gray-500"
          }`}
          onClick={() => {
            setActiveTab("simulator");
            localStorage.setItem('activeTab', 'simulator');
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
};

export default Landing;