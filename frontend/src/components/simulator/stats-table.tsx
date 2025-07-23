import { memo } from "react";

export const StatsTable = memo(({ stats }: { stats: Record<string, number> }) => (
    <table className="w-full text-sm">
        <tbody>
            {Object.entries(stats).map(([key, value], index) => (
                <tr
                    key={index}
                    className={index % 2 === 0 ? "bg-muted" : "bg-background"}
                >
                    <td className="py-2 px-4 font-medium text-foreground">{key}</td>
                    <td className="py-2 px-4 pr-6 font-mono text-right text-foreground">{value}</td>
                </tr>
            ))}
        </tbody>
    </table>
));
StatsTable.displayName = "StatsTable";