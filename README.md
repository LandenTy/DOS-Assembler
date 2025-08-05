# DOS-Assembler
An educational 16-bit assembler for MS-DOS COM programs. Designed for 386+ PCs and built with DJGPP, this project assembles x86 instructions to raw machine code without relying on external tools. Made to further my understanding of low-level systems and compilers.

### Supported Instructions

| Instruction      | Format              | Notes |
|------------------|---------------------|-------|
| `MOV`            | `reg8, reg8`        | 8-bit register to register |
| `MOV`            | `reg8, imm8`        | Immediate value (0–255) to 8-bit register |
| `INT`            | `imm8`              | Software interrupt (supports `INT 20H`, `INT 33`, etc.) |
| `JMP`            | `label`             | Short jump (±127 bytes) to a label |
| `PRINT`          | `"string"`          | Emits DOS-compatible string using `INT 21h / AH=09h` |
| `EXIT`           | —                   | Halts program using `INT 21h / AH=4Ch` |
| `label:`         | —                   | Label for use with `JMP` and flow control |

- Comments (`;`) are supported
- Whitespace-insensitive
- Only 8-bit registers currently: `AL, CL, DL, BL, AH, CH, DH, BH`
