import { useState, useEffect } from 'react';
import type { Simulator, SimulatorModuleInstance } from '@/types/simulator';

interface WindowWithSimulator extends Window {
  createSimulator?: () => Promise<SimulatorModuleInstance>;
}

export const useSimulator = () => {
  const [simulator, setSimulator] = useState<Simulator | null>(null);
  const [module, setModule] = useState<SimulatorModuleInstance | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    async function initSimulator() {
      try {
        const script = document.createElement('script');
        script.src = '/wasm/simulator.js';
        script.async = true;
        document.body.appendChild(script);
        await new Promise<void>((resolve) => {
          script.onload = () => resolve();
        });

        const windowWithSim = window as WindowWithSimulator;
        const createSimulatorFn = windowWithSim.createSimulator;

        if (!createSimulatorFn) {
          throw new Error('createSimulator function not found on window');
        }

        const moduleInstance = await createSimulatorFn();
        const simulatorInstance = new moduleInstance.Simulator();

        setModule(moduleInstance);
        setSimulator(simulatorInstance);
        setLoading(false);
      } catch (err) {
        console.error('Simulator initialization error:', err);
        setError(err instanceof Error ? err.message : 'Failed to load simulator');
        setLoading(false);
      }
    }
    initSimulator();
    return () => {
      const script = document.querySelector('script[src="/wasm/simulator.js"]');
      if (script) document.body.removeChild(script);
    };
  }, []);

  return { simulator, module, loading, error };
};