'use client';

import { useEffect, useRef, useState } from 'react';
import MonacoEditor from '@monaco-editor/react';
import { editor } from 'monaco-editor';
import { useCurrentTheme } from '@/hooks/use-current-theme';
import { Skeleton } from './ui/skeleton';

interface EditorProps {
    text: string;
    setText: (text: string) => void;
}

export const Editor = ({ text, setText }: EditorProps) => {
    const editorRef = useRef<editor.IStandaloneCodeEditor | null>(null);
    const [isEditorReady, setIsEditorReady] = useState(false);
    const currentTheme = useCurrentTheme();

    useEffect(() => {
        if (editorRef.current && isEditorReady) {
            const resizeEditor = () => {
                if (editorRef.current) {
                    editorRef.current.layout();
                }
            };
            resizeEditor();
            window.addEventListener('resize', resizeEditor);
            return () => {
                window.removeEventListener('resize', resizeEditor);
            };
        }
    }, [isEditorReady]);

    useEffect(() => {
        if (editorRef.current && isEditorReady) {
            const themeToUse = currentTheme === 'dark' ? 'riscv-dark-theme' : 'riscv-light-theme';
            editorRef.current.updateOptions({ theme: themeToUse });
        }
    }, [currentTheme, isEditorReady]);

    const defineThemes = (monaco: typeof import('monaco-editor')) => {
        monaco.editor.defineTheme('riscv-light-theme', {
            base: 'vs',
            inherit: true,
            rules: [
                { token: 'keyword', foreground: '0000FF', fontStyle: 'bold' },
                { token: 'variable', foreground: 'FF0000' },
                { token: 'number', foreground: 'FF4500' },
                { token: 'number.hex', foreground: 'FF8C00' },
                { token: 'number.binary', foreground: 'FFA500' },
                { token: 'type.identifier', foreground: '00B7EB', fontStyle: 'bold' },
                { token: 'directive', foreground: 'FF69B4', fontStyle: 'italic' },
                { token: 'comment', foreground: '008000', fontStyle: 'italic' },
                { token: 'string', foreground: '800080' },
                { token: 'string.escape', foreground: '9932CC' },
                { token: 'string.invalid', foreground: 'FF0000', fontStyle: 'underline' },
            ],
            colors: {
                'editor.background': '#FFFFFF',
                'editor.lineHighlightBackground': '#F0F0F0',
                'editorCursor.foreground': '#0000FF',
                'editor.selectionBackground': '#ADD8E6',
                'editorLineNumber.foreground': '#999999',
            },
        });

        monaco.editor.defineTheme('riscv-dark-theme', {
            base: 'vs-dark',
            inherit: true,
            rules: [
                { token: 'keyword', foreground: '569CD6', fontStyle: 'bold' },
                { token: 'variable', foreground: 'FF6B6B' },
                { token: 'number', foreground: 'FFB86C' },
                { token: 'number.hex', foreground: 'F1FA8C' },
                { token: 'number.binary', foreground: 'FFD93D' },
                { token: 'type.identifier', foreground: '50FA7B', fontStyle: 'bold' },
                { token: 'directive', foreground: 'FF79C6', fontStyle: 'italic' },
                { token: 'comment', foreground: '6272A4', fontStyle: 'italic' },
                { token: 'string', foreground: 'F1FA8C' },
                { token: 'string.escape', foreground: 'BD93F9' },
                { token: 'string.invalid', foreground: 'FF5555', fontStyle: 'underline' },
            ],
            colors: {
                'editor.background': '#1E1E1E',
                'editor.lineHighlightBackground': '#2D2D30',
                'editorCursor.foreground': '#FFFFFF',
                'editor.selectionBackground': '#264F78',
                'editorLineNumber.foreground': '#858585',
            },
        });
    };

    const handleEditorDidMount = async (editor: editor.IStandaloneCodeEditor, monaco: typeof import('monaco-editor')) => {
        editorRef.current = editor;

        monaco.languages.register({ id: 'riscv-assembly' });
        monaco.languages.setMonarchTokensProvider('riscv-assembly', {
            tokenizer: {
                root: [
                    [/\b(add|sub|mul|div|rem|and|or|xor|sll|slt|sra|srl|addi|andi|ori|lb|ld|lh|lw|jalr|sb|sw|sd|sh|beq|bne|bge|blt|auipc|lui|jal)\b/, 'keyword'],
                    [/\b(x[0-9]|x[1-2][0-9]|x3[0-1]|zero|ra|sp|gp|tp|t[0-6]|s[0-1]|a[0-7]|s[2-9]|s1[0-1]|fp)\b/, 'variable'],
                    [/\b0x[0-9a-fA-F]+\b/, 'number.hex'],
                    [/\b0b[01]+\b/, 'number.binary'],
                    [/\b-?\d+\b/, 'number'],
                    [/^[a-zA-Z_]\w*:/, 'type.identifier'],
                    [/\b(jal|beq|bne|blt|bge|bltu|bgeu)\s+([a-zA-Z_]\w*)/, ['keyword', 'type.identifier']],
                    [/\b(jal|beq|bne|blt|bge|bltu|bgeu)\s+\w+\s*,\s*\w+\s*,\s*([a-zA-Z_]\w*)/, ['keyword', { token: '', next: '@pop' }, 'type.identifier']],
                    [/\.(text|data|byte|half|word|dword|asciz|asciiz)\b/, 'directive'],
                    [/#.*$/, 'comment'],
                    [/"([^"\\]|\\.)*$/, 'string.invalid'],
                    [/"/, { token: 'string.quote', bracket: '@open', next: '@string' }],
                ],
                string: [
                    [/[^\\"]+/, 'string'],
                    [/\\./, 'string.escape'],
                    [/"/, { token: 'string.quote', bracket: '@close', next: '@pop' }],
                ],
            },
        });

        defineThemes(monaco);

        monaco.editor.setModelLanguage(editor.getModel()!, 'riscv-assembly');
        const initialTheme = currentTheme === 'dark' ? 'riscv-dark-theme' : 'riscv-light-theme';
        monaco.editor.setTheme(initialTheme);

        editor.focus();
        setIsEditorReady(true);
    };

    const handleEditorChange = (value: string | undefined) => {
        if (value !== undefined) {
            setText(value);
        }
    };

    return (
        <div className="rounded-2xl h-full w-full shadow-lg overflow-hidden border border-border relative">
            {!isEditorReady && <EditorSkeleton />}
            <div className={`h-full w-full ${!isEditorReady ? 'opacity-0' : 'opacity-100'} transition-opacity duration-200`}>
                <MonacoEditor
                    height="100%"
                    language="riscv-assembly"
                    theme={currentTheme === 'dark' ? 'riscv-dark-theme' : 'riscv-light-theme'}
                    value={text}
                    onChange={handleEditorChange}
                    onMount={handleEditorDidMount}
                    loading={<EditorSkeleton />}
                    options={{
                        minimap: { enabled: false },
                        scrollBeyondLastLine: false,
                        fontSize: 16,
                        wordWrap: 'on',
                        lineNumbers: 'on',
                        renderLineHighlight: 'all',
                        scrollbar: {
                            vertical: 'auto',
                            horizontal: 'auto',
                            verticalScrollbarSize: 6,
                            horizontalScrollbarSize: 6,
                        },
                        roundedSelection: true,
                        automaticLayout: true,
                        padding: {
                            top: 16,
                        },
                    }}
                />
            </div>
        </div>
    );
};

const EditorSkeleton = () => {
    const [lineWidths, setLineWidths] = useState<string[]>([]);

    useEffect(() => {
        setLineWidths(
            Array.from({ length: 8 }, () =>
                `${Math.floor(Math.random() * 41) + 10}%`
            )
        );
    }, []);

    return (
        <div className="absolute inset-0 rounded-2xl h-full w-full shadow-lg overflow-hidden border border-border flex items-center justify-center bg-background font-mono z-10">
            <Skeleton className="w-full h-full" />
            <div className="absolute top-8 left-8 right-8 flex flex-col gap-3 pointer-events-none">
                {lineWidths.map((width, i) => (
                    <div key={i} className="flex items-center gap-3">
                        <div
                            className="w-6 text-right text-muted-foreground select-none"
                            style={{ opacity: 0.7 }}
                        >
                            {i + 1}
                        </div>
                        <div
                            className="h-5 bg-border animate-pulse rounded"
                            style={{ width }}
                        />
                    </div>
                ))}
            </div>
        </div>
    );
};