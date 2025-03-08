"use client";

import Link from "next/link";
import Editor from "@/components/Editor";
import Simulator from "@/components/Simulator";
import { Github } from "lucide-react";
import { useState } from "react";

export default function Home() {
  const [activeTab, setActiveTab] = useState<"editor" | "simulator">("editor");
  const [code, setCode] = useState<string>(`0x00000000 0x00000000 addi x1, x0, 10
0x00000000 0x00000014 addi x2, x0, 0x14
0x00000000 0x00000000 add x4, x1, x2`);

  return (
    <main className="h-screen w-screen bg-gray-50 flex justify-center items-center">
      <Link href="https://github.com/rit3sh-x/RISC-V-Aseembler" target="_blank">
        <Github className="fixed top-4 right-4 text-gray-500 hover:text-gray-700 transition-colors" />
      </Link>
      <div className="fixed top-4 mx-auto flex gap-2 bg-white p-2 rounded-lg shadow-md z-[999]">
        <div
          className={`cursor-pointer px-4 py-2 rounded-md transition ${
            activeTab === "editor" ? "bg-gray-200 font-semibold" : "text-gray-500"
          }`}
          onClick={() => setActiveTab("editor")}
        >
          Editor
        </div>
        <div
          className={`cursor-pointer px-4 py-2 rounded-md transition ${
            activeTab === "simulator" ? "bg-gray-200 font-semibold" : "text-gray-500"
          }`}
          onClick={() => setActiveTab("simulator")}
        >
          Simulator
        </div>
      </div>
      <div className="w-[95%] h-[95%]">
        {activeTab === "editor" ? (
          <Editor text={code} setText={setCode} activeTab={activeTab} setActiveTab={setActiveTab} />
        ) : (
          <Simulator text={code} />
        )}
      </div>
    </main>
  );
}