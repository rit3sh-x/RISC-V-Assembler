'use client';

import React, { useEffect, useRef, useState } from 'react';
import MonacoEditor from '@monaco-editor/react';
import { editor } from 'monaco-editor';
import { FileUp } from 'lucide-react';

interface EditorProps {
  text: string;
  setText: (text: string) => void;
}

const Editor = ({ text, setText }: EditorProps) => {
  const editorRef = useRef<editor.IStandaloneCodeEditor | null>(null);
  const [isEditorReady, setIsEditorReady] = useState(false);

  useEffect(() => {
    const savedCode = localStorage.getItem('riscvMachineCode');
    if (savedCode) {
      setText(savedCode);
    }
  }, [setText]);

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

  const handleEditorDidMount = (editor: editor.IStandaloneCodeEditor, monaco: typeof import('monaco-editor')) => {
    editorRef.current = editor;

    monaco.languages.register({ id: 'riscv-assembly' });
    monaco.languages.setMonarchTokensProvider('riscv-assembly', {
      tokenizer: {
        root: [
          [/^\s*(add|sub|mul|div|rem|and|or|xor|sll|slt|sra|srl|addi|andi|ori|lb|ld|lh|lw|jalr|sb|sw|sd|sh|beq|bne|bge|blt|auipc|lui|jal)\b/, 'keyword'],
          [/\b(x(?:[0-9]|[12]\d|3[01])|zero|ra|sp|gp|tp|t[0-6]|s[0-1]|a[0-7]|s[2-9]|s1[0-1]|fp)\b/, 'variable'],
          [/\b0x[0-9a-fA-F]+\b/, 'number.hex'],
          [/\b0b[01]+\b/, 'number.binary'],
          [/\b-?\d+\b/, 'number'],
          [/^[a-zA-Z_]\w*:/, 'type.identifier'],
          [/\b(jal|beq|bne|blt|bge|bltu|bgeu)\s+(?:[^,]+,\s*)?([a-zA-Z_]\w*)/, ['keyword', 'type.identifier']],
          [/,\s*([a-zA-Z_]\w*)(?=\s|$)/, 'type.identifier'],
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
    
    monaco.editor.defineTheme('riscv-theme', {
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
      },
    });
  
    monaco.editor.setModelLanguage(editor.getModel()!, 'riscv-assembly');
    monaco.editor.setTheme('riscv-theme');
    setIsEditorReady(true);
    editor.focus();
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
            accept=".s,.asm,.txt"
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