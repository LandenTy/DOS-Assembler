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

void add_patch(const char *label, int patch_pos) {
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
    patch_count++;
}

void fix_patches() {
    for (int i = 0; i < patch_count; ++i) {
        int target = find_label(patches[i].name);
        if (target == -1) {
            fprintf(stderr, "Undefined label: %s\n", patches[i].name);
            exit(1);
        }
        int offset = target - (patches[i].patch_pos + 1);
        if (offset < -128 || offset > 127) {
            fprintf(stderr, "Jump out of range for label: %s\n", patches[i].name);
            exit(1);
        }
        output[patches[i].patch_pos] = (unsigned char)(offset & 0xFF);
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

            char *a1 = arg1;
            while (isspace(*a1)) a1++;
            memmove(arg1, a1, strlen(a1) + 1);
            char *a2 = arg2;
            while (isspace(*a2)) a2++;
            memmove(arg2, a2, strlen(a2) + 1);
        } else {
            strncpy(arg1, rest, sizeof(arg1) - 1);
            arg1[sizeof(arg1) - 1] = 0;
            char *a1 = arg1;
            while (isspace(*a1)) a1++;
            memmove(arg1, a1, strlen(a1) + 1);
        }
        to_upper_str(arg1);
        to_upper_str(arg2);
    }

    if (strcmp(instr, "MOV") == 0) {
        int reg_dest = reg8_code(arg1);
        int reg_src = reg8_code(arg2);
        int imm;

        if (reg_dest != -1 && reg_src != -1) {
            // MOV reg8, reg8
            emit(0x88);
            emit(modrm_byte(reg_src, reg_dest));
        }
        else if (reg_dest != -1 && sscanf(arg2, "%i", &imm) == 1 && imm >= 0 && imm <= 255) {
            // MOV reg8, imm8
            emit(0xB0 + reg_dest);
            emit(imm);
        }
        else {
            fprintf(stderr, "Invalid MOV: %s\n", line);
        }
    }
    else if (strcmp(instr, "INT") == 0) {
        int imm = -1;

        // Trim trailing whitespace
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
        add_patch(arg1, out_pos);
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
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input.asm output.com\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) { perror("fopen input"); return 1; }

    char line[256];
    while (fgets(line, sizeof(line), in)) {
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') continue;
        assemble_line(trimmed);
    }
    fclose(in);

    fix_patches();
    write_com(argv[2]);
    write_hex_dump();

    return 0;
}
