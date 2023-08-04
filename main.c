#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

typedef int8_t s_byte;
typedef int16_t s_word;
typedef int32_t s_dword;
typedef int64_t s_qword;

#define mode32 (mode == 32)
#define mode64 (mode == 64)

#define write_asm(x) if(gen_sources) {fputs(x, fp);}
#define writef_asm(...) if(gen_sources) {fprintf(fp, __VA_ARGS__);}


#define write_byte(x) if(!gen_sources) {fwrite(&((byte)x), sizeof(byte), 1, fp);}
#define write_sbyte(x) if(!gen_sources) {fwrite(&((s_byte)x), sizeof(byte), 1, fp);}

#define write_word(x) if(!gen_sources) {fwrite(&((word)x), sizeof(word), 1, fp);}
#define write_sword(x) if(!gen_sources) {fwrite(&((s_word)x), sizeof(s_word), 1, fp);}

#define write_dword(x) if(!gen_sources) {fwrite(&((dword)x), sizeof(dword), 1, fp);}
#define write_sdword(x) if(!gen_sources) {fwrite(&((s_dword)x), sizeof(s_dword), 1, fp);}

#define write_qword(x) if(!gen_sources) {fwrite(&((qword)x), sizeof(qword), 1, fp);}
#define write_sqword(x) if(!gen_sources) {fwrite(&((s_qword)x), sizeof(s_qword), 1, fp);}


#define warn(x) printf(x); \
    if (werror) {          \
        return 1;          \
    }

struct SymbolDef {
    size_t addr;
    char *name;
};

enum SymbolRefType {
    // unsigned integers
    // abs = "absolute"
    abs16,
    abs24,
    abs32,
    abs64,

    // 2s complement signed integers
    // rel = "relative"
    rel8,
    rel16,
    rel32
};

struct SymbolRef {
    size_t pos;               /* position of the reference (in the file) (to be overwritten) */
    enum SymbolRefType type;  /* type of the reference*/
    char *name;
};

// thats 100% what macros are supposed to do
// why shouldn't it?
// ps: ote don't kill me, thanks
#define add_sy_ref(pos, type, name) if (!gen_sources) { \
        sy_refs = realloc(sy_refs, sizeof(struct SymbolRef *) * (sy_refs_am + 1)); \
        struct SymbolRef *ref = malloc(sizeof(struct SymbolRef)); \
        sy_refs[sy_refs_am++] = ref; \
        ref->name = name; \
        ref->type = type; \
        ref->pos = pos; \
    }

// ote look away for some lines, thanks

// OTE LOOK AWAAAAY
// **OTE STOP LOOKING**

// okay i think ote isn't looking

// (had to do it again)
// (OTE IM SORRY BUT DONT LOOK)
// okay
#define add_sy_def(addr, name) if (!gen_sources) { \
        sy_defs = realloc(sy_defs, sizeof(struct SymbolDef *) * (sy_refs_am + 1)); \
        // i should use a macro for ^that buuuuut
        // ....yeah
        struct SymbolDef *sy = malloc(sizeof(struct SymbolDef)); \

int main(int argc, char **argv) {
    int werror = 0;

    int used_out_syscall = 0;
    int used_in_syscall = 0;

    if (argc < 3) {
        printf("Usage: bfc (-m(32|64)) (-s[mem size]) (-S) (-o[cell starting offset]) [infile] [outfile (nasm)]\n");
        return 1;
    }

    int memsize = 100;
    int mode = 32;
    int gen_sources = 0;
    int cell_off = -1;

    for (int i = 0; i < argc-2; i ++) {
        char *x = argv[i+1];
        char c = x[1];

        if (c == 'm') {
            mode = atoi(x+2);
        }
        else if (c == 's') {
            memsize = atoi(x+2);
        }
        else if (c == 'S') {
            gen_sources = 1;
        }
        else if (c == 'o') {
            cell_off = atoi(x+2);
        }
    }

    if (cell_off < 0) {
        cell_off = memsize / 2;
    }

    FILE *fp;

    if (mode != 32) {
        printf("Unsupported mode! Supported: 32\n");
        return 1;
    }

    fp = fopen(argv[argc-2], "r");
    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    char *buff = malloc(size);
    fread(buff, size, size, fp);
    fclose(fp);

    char stemp[20];
    int stempo = 0;
    int is_parsing_stemp = 0;
    for (int i = 0; i < (int)size) {
        char c = buff[i];
        if (c == '%) {
            is_parsing_stemp = !is_parsing_stemp;
            if (!is_parsing_stemp) {
                stempo = 0;
                int varam = strcspn(stemp, "0123456789");
                char old = stemp[varam];
                stemp[varam] = 0;

                if (!strcmp(stemp, "cells")) {
                    stemp[varam] = old;
                    memsize = atoi(stemp+varam);
                }

                else if (!strcmp(stemp, "cello")) {
                    stemp[varam] = old;
                    cell_off = atoi(stemp+varam);
                }

                memset(stemp, 0, 20);
            }
            continue;
        }
        if (is_parsing_stemp) {
            stemp[stempo] = c;
            stempo ++;
        }
    }

    // strip comments
    char *buff2 = malloc(size);
    int y = 0;
    for (size_t i = 0; i < size; i++) {
        char c = buff[i];
        if ( c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',' || c == '[' || c == ']') {
            buff2[y] = c;
            y ++;
        }
    }
    free(buff);
    buff = realloc(buff2, y);
    buff[y] = 0;
    size = y;

    char *fn = argv[argc-1];
    fp = fopen(fn, "w");
    int localid = 0;

    struct SymbolDef *sy_defs = malloc(1 * sizeof(*sys_defs));
    size_t sy_defs_am = 0;

    struct SymbolRef *sy_refs = malloc(1 * sizeof(*sys_defs));
    size_t sy_refs_am = 0;

    write_asm("section .data\ncells:\n");
    writef_asm("    times %i db 0\n\n", memsize);
    writef_asm("section .text\n    global _start\n\n_start:\n    mov ecx, cells+%i\n\n", cell_off);

    // data section
    for (int i = 0; i < memsize; i ++) {
        write_byte(0x00);
    }
    // code section


    for (size_t i = 0; i < size; i++) {
        char c = buff[i];
        if (c == '\n') {
            continue;
        }
        if (mode32) {
            if (c == '>') {         // increment pt
                write_asm("    inc ecx\n");
            }
            else if (c == '<') {    // decrement pt
                write_asm("    dec ecx\n");
            }
            else if (c == '+') {    // inc cell at pt
                int am = (int)strspn(buff + i, "+");
                if (am > 1) {
                    writef_asm("    add byte [ecx], %i\n", am);
                    i += am-1;
                    continue;
                } else {
                    write_asm("    inc byte [ecx]\n");
                }
            }
            else if (c == '-') {    // dec cell at pt
                int am = (int)strspn(buff + i, "-");
                if (am > 1) {
                    writef_asm("    sub byte [ecx], %i\n", am);
                    i += am-1;
                    continue;
                } else {
                    write_asm("    dec byte [ecx]\n");
                }
            }
            else if (c == '.') {    // output char cell at pt
                used_out_syscall = 1;
                write_asm("    call putchar\n");
            }
            else if (c == ',') {    // input char cell at pt
                used_in_syscall = 1;
                write_asm("    call getchar\n");
            }
            else if (c == '[') {    // jumps after the corresponding ']' instruction if the cell val is 0
                int corr = -1;
                int ind = 0;
                for (size_t x = i + 1; x < size; x ++) {
                    if (buff[x] == '[') {
                        ind ++;
                    }
                    if (buff[x] == ']') {
                        if (!ind) {
                            corr = (int) x;
                            break;
                        }
                        else {
                            ind --;
                        }
                    }
                }

                if (corr > 0) {
                    write_asm("    xor eax, eax\n");
                    write_asm("    mov al, [ecx]\n");
                    write_asm("    test eax, eax\n");
                    writef_asm("    jz close_%i\n", corr);
                    writef_asm("open_%i:\n", corr);
                }
                else {
                    warn("warning: unclosed square brackets!\n");
                }
            }
            else if (c == ']') {    // jumps after the corresponding '[' instruction if the cell val is not 0
                write_asm("    xor eax, eax\n");
                write_asm("    mov al, [ecx]\n");
                write_asm("    test eax, eax\n");
                writef_asm("    jnz open_%i\n", (int)i);
                writef_asm("close_%i:\n", (int)i);
            }
            else {
                continue;
            }
        }
    }
    // exit syscall
    if (mode32) {
        fputs("end:\n    mov eax,1\n    mov ebx,0\n    int 0x80\n\n", fp);
        if (used_in_syscall) {
            // generate read(0, pt, 1) syscall
            write_asm("getchar:\n");
            write_asm("    mov eax, 3\n");
            write_asm("    mov ebx, 0\n");
            write_asm("    mov edx, 1\n");
            write_asm("    push ecx\n");
            write_asm("    int 0x80\n");
            write_asm("    pop ecx\n");
            write_asm("    test eax, eax\n");
            writef_asm("    jnz .L%i\n", localid);
            write_asm("    mov byte [ecx], 0\n");
            writef_asm(".L%i:\n", localid);
            write_asm("    ret\n\n");
            localid ++;
        }
        if (used_out_syscall) {
            // generate write(1, pt, 1) syscall
            write_asm("putchar:\n");
            write_asm("    mov eax, 4\n");
            write_asm("    mov ebx, 1\n");
            write_asm("    mov edx, 1\n");
            write_asm("    push ecx\n");
            write_asm("    int 0x80\n");
            write_asm("    pop ecx\n");
            write_asm("    ret\n\n");
        }
    }
    fclose(fp);
    free(buff);

    if (!gen_sources) {
        // todo: convert file with name fn to elf
    }

    // todo: free sy_refs and sy_defs
    return 0;
}
