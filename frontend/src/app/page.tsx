"use client";

import RiscvViewer from "@/components/RISCVViewer";
import Link from "next/link";
import Editor from "@/components/Editor";
import { Github } from "lucide-react";
import { useState } from "react";

export default function Home() {
  const [activeTab, setActiveTab] = useState<"editor" | "simulator">("editor");

  return (
    <main className="h-screen w-screen bg-gray-50 flex justify-center items-center">
      <Link href="https://github.com/rit3sh-x/RISC-V-Aseembler" target="_blank">
        <Github className="absolute top-4 right-4 text-gray-500 hover:text-gray-700 transition-colors" />
      </Link>
      <div className="absolute top-4 mx-auto flex gap-2 bg-white p-2 rounded-lg shadow-md">
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

      {/* Render the selected component */}
      <div className="w-[90%] h-[90%]">
        {activeTab === "editor" ? <Editor /> : <RiscvViewer />}
      </div>
    </main>
  );
}