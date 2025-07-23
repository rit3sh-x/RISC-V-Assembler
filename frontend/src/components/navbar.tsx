import { useSimulator } from "@/store/zustand";
import { FileUp, Github } from "lucide-react";
import { Sidebar } from "./sidebar";
import { useEffect, useState } from "react";
import { cn } from "@/lib/utils";
import { buttonVariants } from "@/components/ui/button";
import { ToggleTheme } from "./toggle-theme";
import Link from "next/link";

export const Navbar = () => {
    const { currentTab, setCurrentTab, setMachineCode, isRunning, setDataForwarding, setPipelining, dataForwarding, pipelining } = useSimulator();
    const [isOpen, setIsOpen] = useState(false);

    const handlePipelining = (enabled: boolean) => {
        setPipelining(enabled);
    };

    const handleDataForwarding = (enabled: boolean) => {
        setDataForwarding(enabled);
    };

    const handleSidebarOpenChange = (open: boolean) => {
        setIsOpen(open);
    };

    const handleFileUpload = (event: React.ChangeEvent<HTMLInputElement>) => {
        const file = event.target.files?.[0];
        if (file) {
            const reader = new FileReader();
            reader.onload = (e) => {
                const content = e.target?.result as string;
                setMachineCode(content);
            };
            reader.readAsText(file);
        }
    };

    useEffect(() => { }, [currentTab]);

    return (
        <div className="relative w-full h-16">
            <div className="absolute w-full h-full flex justify-center items-center z-20 pointer-events-none">
                <div className={cn(
                    "flex gap-2 bg-muted p-2 rounded-lg shadow-md pointer-events-auto"
                )}>
                    <button
                        className={cn(
                            buttonVariants({ variant: currentTab === "editor" ? "secondary" : "ghost" }),
                            "w-24 bg-background",
                            currentTab === "editor" && "font-semibold"
                        )}
                        onClick={() => setCurrentTab("editor")}
                    >
                        Editor
                    </button>
                    <button
                        className={cn(
                            buttonVariants({ variant: currentTab === "simulator" ? "secondary" : "ghost" }),
                            "w-24 bg-background",
                            currentTab === "simulator" && "font-semibold"
                        )}
                        onClick={() => setCurrentTab("simulator")}
                    >
                        Simulator
                    </button>
                </div>
            </div>
            <div className="w-full px-8 flex flex-row justify-between h-full items-center z-10 relative">
                <div className="flex justify-start">
                    {currentTab === "editor" ? (
                        <label className={cn(
                            buttonVariants({ variant: "default" }),
                            "flex items-center cursor-pointer"
                        )}>
                            <FileUp className="size-4 mr-2" />
                            Upload Code
                            <input
                                type="file"
                                accept=".s,.asm,.txt"
                                onChange={handleFileUpload}
                                className="hidden"
                            />
                        </label>
                    ) : (
                        <Sidebar
                            controls={{ dataForwarding, pipelining }}
                            onPipeliningChange={handlePipelining}
                            onDataForwardingChange={handleDataForwarding}
                            isOpen={isOpen}
                            onOpenChange={handleSidebarOpenChange}
                            isRunning={isRunning}
                        />
                    )}
                </div>
                <div className="flex h-full items-center justify-center gap-6">
                    <ToggleTheme />
                    <Link href="https://github.com/rit3sh-x/RISC-V-Aseembler" target="_blank">
                        <Github className="size-5 text-muted-foreground hover:text-foreground transition-colors" />
                    </Link>
                </div>
            </div>
        </div>
    );
};