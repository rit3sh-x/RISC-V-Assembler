import React, { useEffect, useRef, useCallback } from 'react';

interface PipelineDiagramProps {
    fetchStart?: boolean;
    decodeStart?: boolean;
    executeStart?: boolean;
    memoryStart?: boolean;
    writebackStart?: boolean;
    exEx?: boolean;
    memMem?: boolean;
    memEx?: boolean;
    branchFetch?: boolean;
    branchExecute?: boolean;
    arrowData?: boolean;
    arrowBranch?: boolean;
    arrowFetch?: boolean;
    arrowMemory?: boolean;
    svgData?: string | null;
}

export const PipelineDiagram: React.FC<PipelineDiagramProps> = ({
    fetchStart = false,
    decodeStart = false,
    executeStart = false,
    memoryStart = false,
    writebackStart = false,
    exEx = false,
    memMem = false,
    memEx = false,
    branchFetch = false,
    branchExecute = false,
    arrowData = false,
    arrowBranch = false,
    arrowMemory = false,
    arrowFetch = false,
    svgData = null,
}) => {
    const svgContainerRef = useRef<HTMLDivElement>(null);
    const objectRef = useRef<HTMLObjectElement>(null);

    const updateSvgElements = useCallback(() => {
        if (svgData && svgContainerRef.current) {
            const parser = new DOMParser();
            const svgDoc = parser.parseFromString(svgData, 'image/svg+xml');
            const svgElement = svgDoc.documentElement as unknown as SVGSVGElement;

            svgElement.setAttribute('width', '100%');
            svgElement.setAttribute('height', '100%');
            svgElement.setAttribute('preserveAspectRatio', 'xMidYMid meet');

            const updateOpacity = (id: string, isVisible: boolean) => {
                const element = svgElement.getElementById(id);
                if (element) {
                    element.setAttribute('opacity', isVisible ? '1' : '0');
                }
            };

            const toggleGrayscale = (id: string, isColored: boolean) => {
                const element = svgElement.getElementById(id);
                if (element) {
                    if (isColored) {
                        element.removeAttribute('filter');
                    } else {
                        const filterExists = svgElement.getElementById('grayscaleFilter');
                        if (!filterExists) {
                            const svgNamespace = "http://www.w3.org/2000/svg";
                            const defs = document.createElementNS(svgNamespace, "defs");
                            const filter = document.createElementNS(svgNamespace, "filter");
                            filter.setAttribute("id", "grayscaleFilter");

                            const feColorMatrix = document.createElementNS(svgNamespace, "feColorMatrix");
                            feColorMatrix.setAttribute("type", "matrix");
                            feColorMatrix.setAttribute("values", "0.33 0.33 0.33 0 0 0.33 0.33 0.33 0 0 0.33 0.33 0.33 0 0 0 0 0 1 0");

                            filter.appendChild(feColorMatrix);
                            defs.appendChild(filter);
                            svgElement.insertBefore(defs, svgElement.firstChild);
                        }
                        element.setAttribute('filter', 'url(#grayscaleFilter)');
                    }
                }
            };

            toggleGrayscale('fetch-start', fetchStart);
            toggleGrayscale('decode-start', decodeStart);
            toggleGrayscale('execute-start', executeStart);
            toggleGrayscale('memory-start', memoryStart);
            toggleGrayscale('writeback-start', writebackStart);
            updateOpacity('ex-ex', exEx);
            updateOpacity('mem-mem', memMem);
            updateOpacity('mem-ex', memEx);
            updateOpacity('branch-fetch', branchFetch);
            updateOpacity('branch-execute', branchExecute);
            updateOpacity('arrow-data', arrowData);
            updateOpacity('arrow-branch', arrowBranch);
            updateOpacity('arrow-memory', arrowMemory);
            updateOpacity('arrow-fetch', arrowFetch);

            svgContainerRef.current.innerHTML = '';
            svgContainerRef.current.appendChild(svgElement);

            return;
        }

        if (!objectRef.current) return;
        const svgDoc = objectRef.current.contentDocument;
        if (!svgDoc) return;
        const svgRoot = svgDoc.documentElement;
        if (svgRoot) {
            svgRoot.setAttribute('width', '100%');
            svgRoot.setAttribute('height', '100%');
            svgRoot.setAttribute('preserveAspectRatio', 'xMidYMid meet');
        }

        const updateOpacity = (id: string, isVisible: boolean) => {
            const element = svgDoc.getElementById(id);
            if (element) {
                element.setAttribute('opacity', isVisible ? '1' : '0');
            }
        };

        const toggleGrayscale = (id: string, isColored: boolean) => {
            const element = svgDoc.getElementById(id);
            if (element) {
                if (isColored) {
                    element.removeAttribute('filter');
                } else {
                    const filterExists = svgDoc.getElementById('grayscaleFilter');
                    if (!filterExists) {
                        const svgNamespace = "http://www.w3.org/2000/svg";
                        const defs = svgDoc.createElementNS(svgNamespace, "defs");
                        const filter = svgDoc.createElementNS(svgNamespace, "filter");
                        filter.setAttribute("id", "grayscaleFilter");

                        const feColorMatrix = svgDoc.createElementNS(svgNamespace, "feColorMatrix");
                        feColorMatrix.setAttribute("type", "matrix");
                        feColorMatrix.setAttribute("values", "0.33 0.33 0.33 0 0 0.33 0.33 0.33 0 0 0.33 0.33 0.33 0 0 0 0 0 1 0");

                        filter.appendChild(feColorMatrix);
                        defs.appendChild(filter);
                        if (svgRoot) {
                            svgRoot.insertBefore(defs, svgRoot.firstChild);
                        }
                    }
                    element.setAttribute('filter', 'url(#grayscaleFilter)');
                }
            }
        };

        toggleGrayscale('fetch-start', fetchStart);
        toggleGrayscale('decode-start', decodeStart);
        toggleGrayscale('execute-start', executeStart);
        toggleGrayscale('memory-start', memoryStart);
        toggleGrayscale('writeback-start', writebackStart);
        updateOpacity('ex-ex', exEx);
        updateOpacity('mem-ex', memEx);
        updateOpacity('branch-fetch', branchFetch);
        updateOpacity('branch-execute', branchExecute);
        updateOpacity('arrow-data', arrowData);
        updateOpacity('arrow-branch', arrowBranch);
        updateOpacity('arrow-fetch', arrowFetch);
    }, [
        fetchStart,
        decodeStart,
        executeStart,
        memoryStart,
        writebackStart,
        exEx,
        memEx,
        branchFetch,
        branchExecute,
        arrowData,
        arrowBranch,
        arrowFetch,
        svgData,
        arrowMemory,
        memMem
    ]);

    useEffect(() => {
        if (svgData) {
            updateSvgElements();
            return;
        }

        const objectElement = objectRef.current;
        if (!objectElement) return;

        const handleLoad = () => {
            updateSvgElements();
        };

        objectElement.addEventListener('load', handleLoad);
        updateSvgElements();

        return () => {
            objectElement.removeEventListener('load', handleLoad);
        };
    }, [updateSvgElements, svgData]);

    return (
        <div className="w-full h-full flex items-center justify-center overflow-hidden relative">
            {svgData && (
                <div
                    ref={svgContainerRef}
                    className="h-full flex items-center justify-center"
                    aria-label="Pipeline Diagram"
                />
            )}
        </div>
    );
};