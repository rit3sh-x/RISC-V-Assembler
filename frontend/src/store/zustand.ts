import { create } from 'zustand';
import { createJSONStorage, persist } from 'zustand/middleware';
import { useCallback } from 'react';
import { useShallow } from 'zustand/react/shallow';
import { initialBoilerplate } from '@/constants';

type TabType = 'editor' | 'simulator';

interface AppState {
    currentTab: TabType;
    machineCode: string;
    isRunning: boolean;
    pipelining: boolean;
    dataForwarding: boolean;
    setCurrentTab: (tab: TabType) => void;
    updateMachineCode: (code: string) => void;
    resetState: () => void;
    setIsRunning: (running: boolean) => void;
    setPipelining: (value: boolean) => void;
    setDataForwarding: (value: boolean) => void;
}

const useAppStore = create<AppState>()(
    persist(
        (set) => ({
            currentTab: 'editor',
            machineCode: initialBoilerplate,
            isRunning: false,
            pipelining: true,
            dataForwarding: true,
            setCurrentTab: (tab) =>
                set((state) => ({ ...state, currentTab: tab })),
            updateMachineCode: (code) =>
                set((state) => ({ ...state, machineCode: code })),
            resetState: () =>
                set(() => ({
                    currentTab: 'editor',
                    machineCode: initialBoilerplate,
                    isRunning: false,
                    pipelining: true,
                    dataForwarding: true,
                })),
            setIsRunning: (running) =>
                set((state) => ({ ...state, isRunning: running })),
            setPipelining: (value: boolean) =>
                set((state) => ({ ...state, pipelining: value })),
            setDataForwarding: (value: boolean) =>
                set((state) => ({ ...state, dataForwarding: value })),
        }),
        {
            name: 'app-store',
            storage: createJSONStorage(() => localStorage),
            partialize: (state) => ({
                currentTab: state.currentTab,
                machineCode: state.machineCode,
                pipelining: state.pipelining,
                dataForwarding: state.dataForwarding,
            }),
        }
    )
);

export const useSimulator = () => {
    const {
        currentTab,
        machineCode,
        isRunning,
        pipelining,
        dataForwarding,
    } = useAppStore(
        useShallow((state) => ({
            currentTab: state.currentTab,
            machineCode: state.machineCode,
            isRunning: state.isRunning,
            pipelining: state.pipelining,
            dataForwarding: state.dataForwarding,
        }))
    );

    const setCurrentTab = useAppStore((state) => state.setCurrentTab);
    const setMachineCode = useAppStore((state) => state.updateMachineCode);
    const setIsRunning = useAppStore((state) => state.setIsRunning);
    const setPipelining = useAppStore((state) => state.setPipelining);
    const setDataForwarding = useAppStore((state) => state.setDataForwarding);

    const handleSetCurrentTab = useCallback(
        (tab: TabType) => setCurrentTab(tab),
        [setCurrentTab]
    );
    const handleSetMachineCode = useCallback(
        (code: string) => setMachineCode(code),
        [setMachineCode]
    );
    const handleSetIsRunning = useCallback(
        (running: boolean) => setIsRunning(running),
        [setIsRunning]
    );
    const handleSetPipelining = useCallback(
        (value: boolean) => setPipelining(value),
        [setPipelining]
    );
    const handleSetDataForwarding = useCallback(
        (value: boolean) => setDataForwarding(value),
        [setDataForwarding]
    );

    return {
        currentTab,
        machineCode,
        setCurrentTab: handleSetCurrentTab,
        setMachineCode: handleSetMachineCode,
        isRunning,
        setIsRunning: handleSetIsRunning,
        pipelining,
        setPipelining: handleSetPipelining,
        dataForwarding,
        setDataForwarding: handleSetDataForwarding,
    };
};