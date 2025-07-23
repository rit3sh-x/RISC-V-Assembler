"use client";

import { useTheme } from "next-themes";
import { useCurrentTheme } from "@/hooks/use-current-theme";
import { Sun, Moon } from "lucide-react";
import { useEffect, useState } from "react";

export const ToggleTheme = () => {
    const { setTheme } = useTheme();
    const currentTheme = useCurrentTheme();
    const [mounted, setMounted] = useState(false);

    useEffect(() => {
        setMounted(true);
    }, []);

    const handleToggle = () => {
        if (currentTheme === "dark") {
            setTheme("light");
        } else {
            setTheme("dark");
        }
    };

    if (!mounted) return null;

    return (
        <button onClick={handleToggle} aria-label="Toggle theme" className="text-muted-foreground hover:text-foreground transition-colors">
            {currentTheme === "dark" && <Moon size={20} />}
            {currentTheme === "light" && <Sun size={20} />}
        </button>
    );
};