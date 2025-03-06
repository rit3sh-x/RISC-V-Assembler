'use client';

import React, { useState } from 'react';
import MonacoEditor from '@monaco-editor/react';
import { editor } from 'monaco-editor';

const Editor = () => {
  const [code, setCode] = useState<string>(`# Write your RISC-V code here
# Example:
addi x1, x0, 10  # Set x1 to 10
addi x2, x0, 0x14  # Set x2 to 20 (hex)
add x4, x1, x2   # x4 = x1 + x2
`);

  const handleEditorDidMount = (editor: editor.IStandaloneCodeEditor, monaco: typeof import('monaco-editor')) => {
    console.log('Editor mounted, registering RISC-V assembly language');

    monaco.languages.register({ id: 'riscv-assembly' });
    monaco.languages.setMonarchTokensProvider('riscv-assembly', {
      tokenizer: {
        root: [
          [/^\s*(addi|add|sub|mul|div|rem|and|or|xor|sll|srl|sra|lw|sw|lb|sb|jal|jalr|beq|bne|blt|bge|li)\b/, 'keyword'],
          [/\b(x(?:[0-9]|[12]\d|3[01])|zero|ra|sp|gp|tp|t[0-6]|s[0-1]|a[0-7]|s[2-9]|s1[0-1])\b/, 'variable'],
          [/\b0x[0-9a-fA-F]+\b/, 'number.hex'],
          [/\b-?\d+\b/, 'number'],
          [/#.*/, 'comment'],
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
        { token: 'comment', foreground: '808080' },
        { token: 'type.identifier', foreground: 'FF6600', fontStyle: 'bold' },
      ],
      colors: {
        'editor.background': '#FFFFFF',
        'editor.lineHighlightBackground': '#F0F0F0',
      },
    });

    editor.updateOptions({ theme: 'riscv-theme' });
    console.log('Theme and language configured');
  };

  const handleEditorChange = (value: string | undefined) => {
    if (value !== undefined) {
      setCode(value);
    }
  };

  return (
    <div className="flex flex-col h-full p-4">
      <div className="flex-grow rounded-2xl shadow-lg overflow-hidden border border-gray-300">
        <MonacoEditor
          height="100%"
          language={'riscv-assembly'}
          theme="riscv-theme"
          value={code}
          onChange={handleEditorChange}
          onMount={handleEditorDidMount}
          options={{
            minimap: { enabled: false },
            scrollBeyondLastLine: false,
            fontSize: 14,
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