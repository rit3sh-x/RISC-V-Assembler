import { memo } from "react";

export const MemoryTable = memo(({ entries }: { entries: MemoryCell[] }) => (
    <table className="w-full text-sm">
        <thead>
            <tr className="bg-muted border-b">
                <th className="py-2 px-4 text-left font-medium text-muted-foreground">Address</th>
                <th className="py-2 px-4 text-left font-medium text-muted-foreground">+3</th>
                <th className="py-2 px-4 text-left font-medium text-muted-foreground">+2</th>
                <th className="py-2 px-4 text-left font-medium text-muted-foreground">+1</th>
                <th className="py-2 px-4 text-left font-medium text-muted-foreground">+0</th>
            </tr>
        </thead>
        <tbody>
            {entries.map((entry, index) => (
                <tr key={index} className="border-b hover:bg-accent">
                    <td className="py-2 px-4 font-mono">{entry.address === '----------' ? entry.address : `0x${entry.address}`}</td>
                    <td className="py-2 px-4 font-mono">{entry.bytes[0]}</td>
                    <td className="py-2 px-4 font-mono">{entry.bytes[1]}</td>
                    <td className="py-2 px-4 font-mono">{entry.bytes[2]}</td>
                    <td className="py-2 px-4 font-mono">{entry.bytes[3]}</td>
                </tr>
            ))}
        </tbody>
    </table>
));
MemoryTable.displayName = "MemoryTable";