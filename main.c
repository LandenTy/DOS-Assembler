#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_OUTPUT 65536
#define ORG_OFFSET 0x100
#define MAX_LABELS 256

unsigned char output[MAX_OUTPUT];
int out_pos = 0;

typedef struct {
    char name[32];
    int position;
} Label;

Label labels[MAX_LABELS];
int label_count = 0;

typedef struct {
    char name[32];
    int patch_pos;
    int size;  // 1 for 8-bit offset, 2 for 16-bit offset
} Patch;

Patch patches[MAX_LABELS];
int patch_count = 0;

void emit(unsigned char byte) {
    if (out_pos >= MAX_OUTPUT) {
        fprintf(stderr, "Output too big\n");
        exit(1);
    }
    output[out_pos++] = byte;
}

void emit16(unsigned short word) {
    emit(word & 0xFF);
    emit((word >> 8) & 0xFF);
}

int reg8_code(const char *reg) {
    if (strcmp(reg, "AL") == 0) return 0;
    if (strcmp(reg, "CL") == 0) return 1;
    if (strcmp(reg, "DL") == 0) return 2;
    if (strcmp(reg, "BL") == 0) return 3;
    if (strcmp(reg, "AH") == 0) return 4;
    if (strcmp(reg, "CH") == 0) return 5;
    if (strcmp(reg, "DH") == 0) return 6;
    if (strcmp(reg, "BH") == 0) return 7;
    return -1;
}

unsigned char modrm_byte(int reg, int rm) {
    return 0xC0 | ((reg & 7) << 3) | (rm & 7);
}

void to_upper_str(char *s) {
    for (; *s; ++s) *s = toupper(*s);
}

void trim_trailing_spaces(char *s) {
    int len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = 0;
        len--;
    }
}

void define_label(const char *name) {
    char name_up[32];
    strncpy(name_up, name, sizeof(name_up) - 1);
    name_up[sizeof(name_up) - 1] = 0;
    to_upper_str(name_up);

    for (int i = 0; i < label_count; ++i) {
        if (strcmp(labels[i].name, name_up) == 0) {
            fprintf(stderr, "Duplicate label: %s\n", name_up);
            exit(1);
        }
    }
    if (label_count >= MAX_LABELS) {
        fprintf(stderr, "Too many labels\n");
        exit(1);
    }
    strcpy(labels[label_count].name, name_up);
    labels[label_count].position = out_pos;
    label_count++;
}

int find_label(const char *name) {
    char name_up[32];
    strncpy(name_up, name, sizeof(name_up) - 1);
    name_up[sizeof(name_up) - 1] = 0;
    to_upper_str(name_up);

    for (int i = 0; i < label_count; ++i) {
        if (strcmp(labels[i].name, name_up) == 0)
            return labels[i].position;
    }
    return -1;
}

void add_patch(const char *label, int patch_pos, int size) {
    if (patch_count >= MAX_LABELS) {
        fprintf(stderr, "Too many patches\n");
        exit(1);
    }
    char label_up[32];
    strncpy(label_up, label, sizeof(label_up) - 1);
    label_up[sizeof(label_up) - 1] = 0;
    to_upper_str(label_up);

    strcpy(patches[patch_count].name, label_up);
    patches[patch_count].patch_pos = patch_pos;
    patches[patch_count].size = size;
    patch_count++;
}

void fix_patches() {
    for (int i = 0; i < patch_count; ++i) {
        int target = find_label(patches[i].name);
        if (target == -1) {
            fprintf(stderr, "Undefined label: %s\n", patches[i].name);
            exit(1);
        }
        int patch_pos = patches[i].patch_pos;
        int size = patches[i].size;

        if (size == 1) {
            // 8-bit relative offset (e.g. short jump)
            int offset = target - (patch_pos + 1);
            if (offset < -128 || offset > 127) {
                fprintf(stderr, "Jump out of range for label: %s\n", patches[i].name);
                exit(1);
            }
            output[patch_pos] = (unsigned char)(offset & 0xFF);
        } else if (size == 2) {
            // 16-bit relative offset (For CALL)
            int offset = target - (patch_pos + 2);
            if (offset < -32768 || offset > 32767) {
                fprintf(stderr, "Jump out of 16-bit range for label: %s\n", patches[i].name);
                exit(1);
            }
            output[patch_pos] = (unsigned char)(offset & 0xFF);            // Low byte
            output[patch_pos + 1] = (unsigned char)((offset >> 8) & 0xFF); // High byte
        } else {
            fprintf(stderr, "Invalid patch size: %d\n", size);
            exit(1);
        }
    }
}

void assemble_line(const char *line) {
    char copy[256];
    strncpy(copy, line, sizeof(copy));
    copy[sizeof(copy) - 1] = 0;
    copy[strcspn(copy, "\r\n")] = 0;

    if (copy[0] == ';' || strlen(copy) == 0)
        return;

    char *label_colon = strchr(copy, ':');
    if (label_colon) {
        *label_colon = 0;
        define_label(copy);
        return;
    }

    char instr[32] = {0}, arg1[64] = {0}, arg2[64] = {0}, rest[192] = {0};
    int count = sscanf(copy, "%31s %[^\n]", instr, rest);
    if (count < 1) return;

    to_upper_str(instr);

    if (strcmp(instr, "PRINT") == 0) {
        char *p = rest;
        while (isspace(*p)) p++;
        strncpy(arg1, p, sizeof(arg1) - 1);
        arg1[sizeof(arg1) - 1] = 0;
    } else {
        char *comma = strchr(rest, ',');
        if (comma) {
            *comma = 0;
            strncpy(arg1, rest, sizeof(arg1) - 1);
            arg1[sizeof(arg1) - 1] = 0;
            strncpy(arg2, comma + 1, sizeof(arg2) - 1);
            arg2[sizeof(arg2) - 1] = 0;

            // Trim spaces
            while (isspace(*arg1)) memmove(arg1, arg1 + 1, strlen(arg1) + 1);
            while (isspace(*arg2)) memmove(arg2, arg2 + 1, strlen(arg2) + 1);
            trim_trailing_spaces(arg1);
            trim_trailing_spaces(arg2);
        } else {
            strncpy(arg1, rest, sizeof(arg1) - 1);
            arg1[sizeof(arg1) - 1] = 0;

            while (isspace(*arg1)) memmove(arg1, arg1 + 1, strlen(arg1) + 1);
            trim_trailing_spaces(arg1);
        }

        to_upper_str(arg1);
        to_upper_str(arg2);
    }

    if (strcmp(instr, "MOV") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;

        if (reg_dest != -1 && reg_src != -1) {
            emit(0x88);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0xB0 + reg_dest);
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid MOV: %s\n", line);
        }
    }
    else if (strcmp(instr, "INT") == 0) {
        int imm = -1;

        while (isspace(arg1[strlen(arg1) - 1])) {
            arg1[strlen(arg1) - 1] = 0;
        }

        size_t len = strlen(arg1);
        if (len > 1 && (arg1[len - 1] == 'H' || arg1[len - 1] == 'h')) {
            arg1[len - 1] = 0;
            if (sscanf(arg1, "%x", &imm) != 1) imm = -1;
        } else if (strncmp(arg1, "0X", 2) == 0) {
            if (sscanf(arg1 + 2, "%x", &imm) != 1) imm = -1;
        } else {
            if (sscanf(arg1, "%d", &imm) != 1) imm = -1;
        }

        if (imm >= 0 && imm <= 255) {
            emit(0xCD);
            emit(imm);
        } else {
            fprintf(stderr, "Invalid INT operand: %s\n", arg1);
        }
    }
    else if (strcmp(instr, "JMP") == 0) {
        emit(0xEB);
        add_patch(arg1, out_pos, 1);
        emit(0);
    }
    else if (strcmp(instr, "PRINT") == 0) {
        if (arg1[0] != '"' || arg1[strlen(arg1) - 1] != '"') {
            fprintf(stderr, "Expected quoted string in PRINT\n");
            return;
        }

        arg1[strlen(arg1) - 1] = 0;
        const char *text = arg1 + 1;

        emit(0xB4); emit(0x09);

        unsigned short str_offset = out_pos + 5 + ORG_OFFSET;
        emit(0xBA); emit16(str_offset);

        emit(0xCD); emit(0x21);

        while (*text) emit(*text++);
        emit('$');
    }
    else if (strcmp(instr, "ADD") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;

        if (reg_dest != -1 && reg_src != -1) {
            emit(0x00);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0x80);
            emit(modrm_byte(0, reg_dest));
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid ADD: %s\n", line);
        }
    }
    else if (strcmp(instr, "SUB") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;

        if (reg_dest != -1 && reg_src != -1) {
            emit(0x28);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0x80);
            emit(modrm_byte(5, reg_dest));
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid SUB: %s\n", line);
        }
    }
    else if (strcmp(instr, "CMP") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;

        if (reg_dest != -1 && reg_src != -1) {
            emit(0x38);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0x80);
            emit(modrm_byte(7, reg_dest));
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid CMP: %s\n", line);
        }
    }
    else if (strcmp(instr, "INC") == 0) {
        int reg = reg8_code(arg1);
        if (reg != -1) {
            emit(0xFE);
            emit(modrm_byte(0, reg));
        }
        else {
            fprintf(stderr, "Invalid INC: %s\n", line);
        }
    }
    else if (strcmp(instr, "DEC") == 0) {
        int reg = reg8_code(arg1);
        if (reg != -1) {
            emit(0xFE);
            emit(modrm_byte(1, reg));
        }
        else {
            fprintf(stderr, "Invalid DEC: %s\n", line);
        }
    }
    else if (strcmp(instr, "DIV") == 0) {
        int reg = reg8_code(arg1);
        if (reg != -1) {
            emit(0xF6);
            emit(modrm_byte(6, reg));
        }
        else {
            fprintf(stderr, "Invalid DIV: %s\n", line);
        }
    }
    else if (strcmp(instr, "AND") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;
        if (reg_dest != -1 && reg_src != -1) {
            emit(0x20);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0x80);
            emit(modrm_byte(4, reg_dest));
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid AND: %s\n", line);
        }
    }
    else if (strcmp(instr, "OR") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;
        if (reg_dest != -1 && reg_src != -1) {
            emit(0x08);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0x80);
            emit(modrm_byte(1, reg_dest));
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid OR: %s\n", line);
        }
    }
    else if (strcmp(instr, "XOR") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;
        if (reg_dest != -1 && reg_src != -1) {
            emit(0x30);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0x80);
            emit(modrm_byte(6, reg_dest));
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid XOR: %s\n", line);
        }
    }
    else if (strcmp(instr, "NOT") == 0) {
        int reg = reg8_code(arg1);
        if (reg != -1) {
            emit(0xF6);
            emit(modrm_byte(2, reg));
        }
        else {
            fprintf(stderr, "Invalid NOT: %s\n", line);
        }
    }
    else if (strcmp(instr, "PUSH") == 0) {
        int reg = reg8_code(arg1);
        if (reg != -1) {
            emit(0x50 + reg);
        } else {
            fprintf(stderr, "Invalid PUSH: %s\n", line);
        }
    }
    else if (strcmp(instr, "POP") == 0) {
        int reg = reg8_code(arg1);
        if (reg != -1) {
            emit(0x58 + reg);
        } else {
            fprintf(stderr, "Invalid POP: %s\n", line);
        }
    }
    else if (strcmp(instr, "CALL") == 0) {
        emit(0xE8);
        add_patch(arg1, out_pos, 2);
        emit(0);
        emit(0);
    }
    else if (strcmp(instr, "RET") == 0) {
        emit(0xC3);
    }
    else if (strcmp(instr, "SHL") == 0 || strcmp(instr, "SAL") == 0) {
        int reg = reg8_code(arg1);
        int imm;
        if (reg != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0xC0);
            emit(modrm_byte(4, reg)); // /4 = SHL
            emit(imm);
        } else {
            fprintf(stderr, "Invalid SHL: %s\n", line);
        }
    }
    else if (strcmp(instr, "SHR") == 0) {
        int reg = reg8_code(arg1);
        int imm;
        if (reg != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0xC0);
            emit(modrm_byte(5, reg)); // /5 = SHR
            emit(imm);
        } else {
            fprintf(stderr, "Invalid SHR: %s\n", line);
        }
    }
    else if (strcmp(instr, "ROL") == 0) {
        int reg = reg8_code(arg1);
        int imm;
        if (reg != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0xC0);
            emit(modrm_byte(0, reg)); // /0 = ROL
            emit(imm);
        } else {
            fprintf(stderr, "Invalid ROL: %s\n", line);
        }
    }
    else if (strcmp(instr, "ROR") == 0) {
        int reg = reg8_code(arg1);
        int imm;
        if (reg != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            emit(0xC0);
            emit(modrm_byte(1, reg)); // /1 = ROR
            emit(imm);
        } else {
            fprintf(stderr, "Invalid ROR: %s\n", line);
        }
    }
    else if (
        strcmp(instr, "JE") == 0 || strcmp(instr, "JZ") == 0 ||
        strcmp(instr, "JNE") == 0 || strcmp(instr, "JNZ") == 0 ||
        strcmp(instr, "JC") == 0 || strcmp(instr, "JNC") == 0 ||
        strcmp(instr, "JG") == 0 || strcmp(instr, "JNLE") == 0 ||
        strcmp(instr, "JGE") == 0 || strcmp(instr, "JNL") == 0 ||
        strcmp(instr, "JL") == 0 || strcmp(instr, "JNGE") == 0 ||
        strcmp(instr, "JLE") == 0 || strcmp(instr, "JNG") == 0) {

        unsigned char opcode = 0;

        if (strcmp(instr, "JE") == 0 || strcmp(instr, "JZ") == 0) opcode = 0x74;
        else if (strcmp(instr, "JNE") == 0 || strcmp(instr, "JNZ") == 0) opcode = 0x75;
        else if (strcmp(instr, "JC") == 0) opcode = 0x72;
        else if (strcmp(instr, "JNC") == 0) opcode = 0x73;
        else if (strcmp(instr, "JG") == 0 || strcmp(instr, "JNLE") == 0) opcode = 0x7F;
        else if (strcmp(instr, "JGE") == 0 || strcmp(instr, "JNL") == 0) opcode = 0x7D;
        else if (strcmp(instr, "JL") == 0 || strcmp(instr, "JNGE") == 0) opcode = 0x7C;
        else if (strcmp(instr, "JLE") == 0 || strcmp(instr, "JNG") == 0) opcode = 0x7E;

        emit(opcode);
        add_patch(arg1, out_pos, 1);
        emit(0);
    }
    else if (strcmp(instr, "EXIT") == 0) {
        emit(0xB4); emit(0x4C);  // MOV AH, 4Ch
        emit(0xCD); emit(0x21);  // INT 21h
    }
    else {
        fprintf(stderr, "Unknown instruction: %s\n", line);
    }
}

void write_com(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen"); exit(1); }
    fwrite(output, 1, out_pos, f);
    fclose(f);
    printf("Wrote %d bytes to %s\n", out_pos, filename);
}

void write_hex_dump() {
    FILE *f = fopen("hex.txt", "w");
    if (!f) {
        perror("fopen hex.txt");
        return;
    }
    for (int i = 0; i < out_pos; i++) {
        fprintf(f, "%02X ", output[i]);
        if ((i + 1) % 16 == 0)
            fprintf(f, "\n");
    }
    if (out_pos % 16 != 0)
        fprintf(f, "\n");
    fclose(f);
    printf("Wrote hex dump to hex.txt\n");
}

int main(int argc, char **argv) {
    int do_hex_dump = 0;
    char *input_file = NULL;
    char *output_file = NULL;

    // Check FLags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--hexdump") == 0) {
            do_hex_dump = 1;
        } else if (strcmp(argv[i], "--help") == 0 ||
                   strcmp(argv[i], "-h") == 0 ||
                   strcmp(argv[i], "/help") == 0) {
            printf("Usage: %s input.asm [output.com] [options]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --hexdump        Write hex dump to hex.txt after assembling\n");
            printf("  --help, -h       Show this help message\n");
            printf("  /help            Same as --help (Windows-style)\n");
            printf("\nExample:\n");
            printf("  %s source.asm out.com --hexdump\n", argv[0]);
            return 0;
        } else if (!input_file) {
            input_file = argv[i];
        } else if (!output_file) {
            output_file = argv[i];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: input file is required.\n");
        fprintf(stderr, "Use --help to see usage.\n");
        return 1;
    }

    if (!output_file) {
        output_file = "output.com";  // default output filename
    }

    FILE *in = fopen(input_file, "r");
    if (!in) {
        perror("fopen input");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), in)) {
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') continue;
        assemble_line(trimmed);
    }
    fclose(in);

    fix_patches();
    write_com(output_file);

    if (do_hex_dump)
        write_hex_dump();

    return 0;
}
