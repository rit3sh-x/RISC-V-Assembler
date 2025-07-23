import { ITEMS_PER_PAGE, REGISTER_ABI_NAMES } from "@/constants";
import { wordToSignedDecimal } from "@/lib/utils";
import { memo } from "react";

export const RegisterTable = memo(({
    registers,
    registerStartIndex,
    isHex
}: {
    registers: number[],
    registerStartIndex: number,
    isHex: boolean
}) => (
    <table className="w-full text-sm table-fixed">
        <thead>
            <tr className="bg-muted border-b">
                <th className="py-2 px-4 text-left font-medium text-muted-foreground">Register</th>
                <th className="py-2 px-4 text-left font-medium text-muted-foreground">Value</th>
            </tr>
        </thead>
        <tbody>
            {registers.slice(registerStartIndex, registerStartIndex + ITEMS_PER_PAGE).map((reg, index) => {
                let hexValue;
                if (reg < 0) {
                    hexValue = (reg >>> 0).toString(16).padStart(8, '0');
                } else {
                    hexValue = reg.toString(16).padStart(8, '0');
                }
                const decimalValue = wordToSignedDecimal(reg >>> 0);

                return (
                    <tr key={registerStartIndex + index} className="border-b hover:bg-accent">
                        <td className="py-2 px-3 font-mono">
                            {`x${registerStartIndex + index}`} <span className="text-muted-foreground">({REGISTER_ABI_NAMES[registerStartIndex + index]})</span>
                        </td>
                        <td className="py-2 px-3 font-mono">
                            {isHex ? `0x${hexValue}` : decimalValue}
                        </td>
                    </tr>
                );
            })}
        </tbody>
    </table>
));
RegisterTable.displayName = "RegisterTable";