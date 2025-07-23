import React, { useState } from "react";
import { Button } from "../ui/button";
import { ErrorBoundary } from "react-error-boundary";

interface ErrorBoundaryProps {
    children: React.ReactNode;
}

export function SimulatorErrorBoundary({ children }: ErrorBoundaryProps) {
    const [error, setError] = useState<Error | null>(null);

    if (error) {
        return (
            <div className="flex flex-col items-center justify-center h-full p-4">
                <h2 className="text-xl font-semibold text-red-600 mb-2">Something went wrong</h2>
                <p className="text-gray-600 mb-4">{error.message}</p>
                <Button
                    variant="outline"
                    onClick={() => setError(null)}
                >
                    Try again
                </Button>
            </div>
        );
    }

    return (
        <ErrorBoundary
            fallbackRender={({ error }) => {
                setError(error);
                return null;
            }}
        >
            {children}
        </ErrorBoundary>
    );
}
