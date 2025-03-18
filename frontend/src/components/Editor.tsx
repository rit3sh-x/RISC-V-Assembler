'use client';

import React, { useEffect } from 'react';
import MonacoEditor from '@monaco-editor/react';
import { editor } from 'monaco-editor';
import { Play, FileUp } from 'lucide-react';

interface EditorProps {
  text: string;
  setText: (text: string) => void;
  setActiveTab: (tab: "editor" | "simulator") => void;
}

const Editor = ({ text, setText, setActiveTab }: EditorProps) => {

  useEffect(() => {
    const savedCode = localStorage.getItem('riscvMachineCode');
    if (savedCode) {
      setText(savedCode);
    }
  }, [setText, setActiveTab]);

  const handleEditorDidMount = (editor: editor.IStandaloneCodeEditor, monaco: typeof import('monaco-editor')) => {
    monaco.languages.register({ id: 'riscv-assembly' });
    monaco.languages.setMonarchTokensProvider('riscv-assembly', {
      tokenizer: {
        root: [
          [/^\s*(addi|add|sub|mul|div|rem|and|or|xor|sll|srl|sra|lw|sw|lb|sb|jal|jalr|beq|bne|blt|bge|li)\b/, 'keyword'],
          [/\b(x(?:[0-9]|[12]\d|3[01])|zero|ra|sp|gp|tp|t[0-6]|s[0-1]|a[0-7]|s[2-9]|s1[0-1])\b/, 'variable'],
          [/\b0x[0-9a-fA-F]+\b/, 'number.hex'],
          [/\b-?\d+\b/, 'number'],
          [/^[a-zA-Z_]\w*:/, 'type.identifier'],
        ],
      },
    });

    monaco.editor.defineTheme('riscv-theme', {
      base: 'vs',
      inherit: true,
      rules: [
        { token: 'keyword', foreground: '0000FF', fontStyle: 'bold' },
        { token: 'variable', foreground: '008800' },
        { token: 'number', foreground: '666633' },
        { token: 'number.hex', foreground: 'FF8C00' },
        { token: 'type.identifier', foreground: 'FF6600', fontStyle: 'bold' },
      ],
      colors: {
        'editor.background': '#FFFFFF',
        'editor.lineHighlightBackground': '#F0F0F0',
      },
    });

    editor.updateOptions({ theme: 'riscv-theme' });
  };

  const handleEditorChange = (value: string | undefined) => {
    if (value !== undefined) {
      setText(value);
      localStorage.setItem('riscvMachineCode', value);
    }
  };

  const handleFileUpload = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (e) => {
        const content = e.target?.result as string;
        setText(content);
        localStorage.setItem('riscvMachineCode', content);
      };
      reader.readAsText(file);
    }
  };

  return (
    <div className="flex flex-col h-full p-4">
      <div className="flex justify-start gap-2 mb-4">
        <label className="flex items-center px-4 py-2 bg-blue-500 text-white rounded cursor-pointer hover:bg-blue-600 transition-colors">
          <FileUp className="w-5 h-5 mr-2" />
          Upload Code
          <input
            type="file"
            accept=".s,.asm,.txt,.mc"
            onChange={handleFileUpload}
            className="hidden"
          />
        </label>
      </div>

      <div className="flex-grow rounded-2xl shadow-lg overflow-hidden border border-gray-300">
        <MonacoEditor
          height="100%"
          language="riscv-assembly"
          theme="riscv-theme"
          value={text}
          onChange={handleEditorChange}
          onMount={handleEditorDidMount}
          options={{
            minimap: { enabled: false },
            scrollBeyondLastLine: false,
            fontSize: 16,
            cursorBlinking: 'blink',
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

export default Editor;