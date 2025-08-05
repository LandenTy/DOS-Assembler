# DOS-Assembler
An educational 16-bit assembler for MS-DOS COM programs. Designed for 386+ PCs and built with DJGPP, this project assembles x86 instructions to raw machine code without relying on external tools. Made to further my understanding of low-level systems and compilers.

### Supported Instructions

| Instruction      | Format              | Notes |
|------------------|---------------------|-------|
| `MOV`            | `reg8, reg8`        | 8-bit register to register |
| `MOV`            | `reg8, imm8`        | Immediate value (0–255) to 8-bit register |
| `INT`            | `imm8`              | Software interrupt (supports `INT 20H`, `INT 33`, etc.) |
| `JMP`            | `label`             | Short jump (±127 bytes) to a label |
| `JE`, `JZ`       | `label`             | Jump if equal / zero |
| `JNE`, `JNZ`     | `label`             | Jump if not equal / not zero |
| `JC`             | `label`             | Jump if carry |
| `JNC`            | `label`             | Jump if not carry |
| `JG`, `JNLE`     | `label`             | Jump if greater (signed) |
| `JGE`, `JNL`     | `label`             | Jump if greater or equal (signed) |
| `JL`, `JNGE`     | `label`             | Jump if less (signed) |
| `JLE`, `JNG`     | `label`             | Jump if less or equal (signed) |
| `CALL`           | `label`             | Call a label (relative offset, 2 bytes) |
| `RET`            | —                   | Return from call |
| `PRINT`          | `"string"`          | Emits DOS-compatible string using `INT 21h / AH=09h` |
| `EXIT`           | —                   | Halts program using `INT 21h / AH=4Ch` |
| `ADD`            | `reg8, reg8`        | 8-bit register addition |
| `ADD`            | `reg8, imm8`        | Add immediate to 8-bit register |
| `SUB`            | `reg8, reg8`        | 8-bit register subtraction |
| `SUB`            | `reg8, imm8`        | Subtract immediate from 8-bit register |
| `CMP`            | `reg8, reg8`        | 8-bit register comparison |
| `CMP`            | `reg8, imm8`        | Compare register with immediate |
| `INC`            | `reg8`              | Increment register |
| `DEC`            | `reg8`              | Decrement register |
| `DIV`            | `reg8`              | Unsigned divide by register (AL / reg) |
| `AND`            | `reg8, reg8`        | Bitwise AND |
| `AND`            | `reg8, imm8`        | Bitwise AND with immediate |
| `OR`             | `reg8, reg8`        | Bitwise OR |
| `OR`             | `reg8, imm8`        | Bitwise OR with immediate |
| `XOR`            | `reg8, reg8`        | Bitwise XOR |
| `XOR`            | `reg8, imm8`        | Bitwise XOR with immediate |
| `NOT`            | `reg8`              | Bitwise NOT |
| `PUSH`           | `reg8`              | Push 8-bit register to stack |
| `POP`            | `reg8`              | Pop 8-bit value from stack to register |
| `SHL`, `SAL`     | `reg8, imm8`        | Logical/arithmetic shift left |
| `SHR`            | `reg8, imm8`        | Logical shift right |
| `ROL`            | `reg8, imm8`        | Rotate left |
| `ROR`            | `reg8, imm8`        | Rotate right |
| `label:`         | —                   | Label for use with `JMP`, `CALL`, etc. |

- Comments (`;`) are supported
- Whitespace-insensitive
- Only 8-bit registers currently: `AL, CL, DL, BL, AH, CH, DH, BH`
- Most closely resembles **Intel x86 Real Mode (16-bit)** assembly language
