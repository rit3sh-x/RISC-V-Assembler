import { Button } from "@/components/ui/button"
import { Card } from "@/components/ui/card"
import { Menu, X } from "lucide-react"
import { Switch } from "@/components/ui/switch"
import { cn } from "@/lib/utils";

interface SidebarProps {
    controls: SimulationControls;
    onPipeliningChange: (enabled: boolean) => void;
    onDataForwardingChange: (enabled: boolean) => void;
    isOpen: boolean;
    onOpenChange: (open: boolean) => void;
    isRunning: boolean;
}

export function Sidebar({
    controls,
    onPipeliningChange,
    onDataForwardingChange,
    isOpen,
    onOpenChange,
    isRunning
}: SidebarProps) {
    const toggleSwitch = (key: keyof SimulationControls, value?: boolean) => {
        const newValue = value !== undefined ? value : !controls[key]

        switch (key) {
            case 'pipelining':
                onPipeliningChange(newValue)
                break
            case 'dataForwarding':
                onDataForwardingChange(newValue)
                break
        }
    }

    return (
        <>
            {isOpen && (
                <div
                    className="absolute inset-0 transition-opacity z-10"
                    onClick={() => onOpenChange(false)}
                />
            )}

            <Button
                variant="ghost"
                size="icon"
                className={cn(
                    "top-4 left-4 transition-opacity absolute",
                    "text-muted-foreground hover:text-foreground",
                    isRunning ? "opacity-30 pointer-events-none" : "",
                    isOpen ? "opacity-0" : ""
                )}
                disabled={isOpen || isRunning}
                onClick={() => onOpenChange(true)}
                style={{ position: "absolute", zIndex: 2147483647 }}
            >
                <Menu className="h-6 w-6" />
            </Button>

            <div
                className={cn(
                    "fixed inset-y-0 left-0 transition-transform duration-300 ease-in-out z-[999]",
                    isOpen ? "translate-x-0" : "-translate-x-full"
                )}
            >
                <Card
                    className="w-64 p-4 h-screen flex flex-col gap-4 overflow-y-auto"
                    style={{
                        background: "var(--card)",
                        color: "var(--foreground)",
                        borderRadius: "var(--radius)"
                    }}
                >
                    <div className="flex justify-between items-center mb-4">
                        <h2 className="text-lg font-semibold">Simulation Controls</h2>
                        <Button
                            variant="ghost"
                            size="icon"
                            onClick={() => onOpenChange(false)}
                            className="text-muted-foreground hover:text-foreground"
                        >
                            <X className="h-4 w-4" />
                        </Button>
                    </div>

                    <div className="space-y-6 divide-y">
                        <div className="space-y-2 pt-4">
                            <div className="flex items-center justify-between">
                                <span className="text-sm font-medium">Pipelining</span>
                                <Switch
                                    checked={controls.pipelining}
                                    onCheckedChange={(checked) => toggleSwitch('pipelining', checked)}
                                    className="data-[state=checked]:bg-primary"
                                />
                            </div>
                            <p className="text-sm text-muted-foreground pb-4">
                                Enable/disable pipelined execution. When disabled, works like a non-pipelined processor.
                            </p>
                        </div>

                        <div className="space-y-2 pt-4">
                            <div className="flex items-center justify-between">
                                <span className="text-sm font-medium">Data Forwarding</span>
                                <Switch
                                    checked={controls.dataForwarding && controls.pipelining}
                                    onCheckedChange={(checked) => toggleSwitch('dataForwarding', checked)}
                                    className="data-[state=checked]:bg-primary"
                                />
                            </div>
                            <p className="text-sm text-muted-foreground">
                                Enable data forwarding in pipeline. When disabled, pipeline works with stalling.
                            </p>
                        </div>
                    </div>
                </Card>
            </div>
        </>
    )
}