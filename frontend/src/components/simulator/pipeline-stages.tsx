import { StageColors, StageNames } from "@/constants";
import { memo } from "react";

export const PipelineStages = memo(({ activeStates, running }: {
    activeStates: Record<number, { active: boolean, instruction: number }>,
    running: boolean
}) => {
    const stageEntries = Object.entries(StageNames).map(([stageNumStr, stageName]) => {
        const stageNum = parseInt(stageNumStr);
        const stageState = activeStates[stageNum] || { active: false, instruction: 0 };

        return (
            <div
                key={stageName}
                className={`p-2 border rounded ${(stageState.active && running) ? "opacity-100" : "opacity-30"}`}
                style={{ backgroundColor: StageColors[stageName] }}
            >
                <div className="font-semibold text-muted">{stageName}</div>
                <div className="text-xs text-background">
                    {stageState.active ? "Active" : "Idle"}
                </div>
            </div>
        );
    });

    return (
        <div className="grid grid-cols-5 gap-2">
            {stageEntries}
        </div>
    );
});
PipelineStages.displayName = "PipelineStages";