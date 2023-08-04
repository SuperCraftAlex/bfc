// ======================================================================================
// ======================= B R A I N F U C K   C O M P I L E R ==========================
// ======================================================================================
//
// (written by alex aka alex_s168)
//
// supported operating systems:
// - linux
//
// supported architectures:
// - 32bit x86
//
// supported brainfuck variations:
// - brainfuck
//
// credits:
// - ote aka otesunki (help with x86)
//
// !WARNING¡ this file contains commented-out comments!
// !DISCLAIMER¡ this is the least efficient code i ever wrote
// !DISCLAIMER¡ i know that macros are not used like this
//

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


#define SDWORD_MAX 2147483647;
#define SWORD_MAX 32768;
#define SBYTE_MAX 127;

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


#define warn(x) printf("%s", x); \
    if (werror) {          \
        return 1;          \
    }

#define error(x) printf("%s", x); return 1;

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
    unsigned int off;	      /* absolute offset */

    /* only resolve this symbol if condition is true or when is_cond_rel == 0 */
    unsigned int is_cond_rel;
    int c_rel_backw_max;
    int c_rel_baclw_min:
    int c_rel_forw_min;
    int c_rel_forw_max;
    unsigned int remove_left_ifn_c;  /* if the condition is false, it undos x bytes from left */
    unsigned int remove_right_ifn_c; /* if the condition is false, it undos x bytes from the right */
};

// thats 100% what macros are supposed to do
// why shouldn't it?
// ps: ote don't kill me, thanks
#define add_sy_ref_RAW \
    sy_refs = realloc(sy_refs, sizeof(struct SymbolRef *) * (sy_refs_am + 1)); \
    struct SymbolRef *ref = malloc(sizeof(struct SymbolRef)); \
    sy_refs[sy_refs_am++] = ref; \
    ref->name = name; \
    ref->type = type; \
    ref->pos = pos; \
    ref->off = off;


#define add_sy_ref(pos, type, name, off) if (!gen_sources) { \
        add_sy_ref_RAW \
        ref->is_cond_rel = 0; \
    }

#define add_sy_ref_cond(pos, type, name, off, backw_max, backw_min, forw_min, forw_max, leftrem, rightrem) if (!gen_sources) { \
        add_sy_ref_RAW \
        ref->is_cond_rel = 1; \
        ref->c_rel_backw_max = backw_max; \
        ref->c_rel_backw_min = backw_min; \
        ref->c_rel_forw_min = forw_min; \
        ref->c_rel_forw_max = forw_max; \
        ref->remove_left_ifn_c = leftrem; \
        ref->remove_right_ifn_c = rightrem; \
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
        /* i should use a macro for ^that buuuuut */ \
        /* ....yeah */ \
        struct SymbolDef *sy = malloc(sizeof(struct SymbolDef)); \
        sy_defs[sy_defs_am++] = sy; \
        sy->name = name; \
        sy->addr = arddr; \
    }

// OTE YOU CAN LOOK AGAIN!

// is this macro okay?
#define write_bytes(x, amount) for (size_t x = 0; x < amount; x++) { write_byte(x); }

enum Arch {
    x86,
    arm,
    riscv
};

int main(int argc, char **argv) {
    int werror = 0;

    int used_out_syscall = 0;
    int used_in_syscall = 0;

    if (argc < 3) {
        printf("Usage: bfc (-a(x86|arm|riscv)) (-m(32|64)) (-s[mem size]) (-S) (-o[cell starting offset]) [infile] [outfile (nasm)]\n");
        return 1;
    }

    int memsize = 100;
    int mode = 32;
    int gen_sources = 0;
    int cell_off = -1;
    enum Arch arch = Arch.x86;

    for (int i = 0; i < argc-2; i ++) {
        char *x = argv[i+1];
        char c = x[1];

        if (c == 'a') {
            char *a = x+2;
            if (!strcmp(a, "x86"))
                arch = Arch.x86;
            else if (!strcmp(a, "arm"))
                arch = Arch.arm;
            else if (!strcmp(a, "riscv"))
                arch = Arch.riscv;
            else
                error("Architecture not supported!\n");
        }
        else if (c == 'm') {
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

    if (arch != Arch.x86)
        error("Architecture not supported yet!\n");

    if (cell_off < 0)
        cell_off = memsize / 2;

    FILE *fp;

    if (mode != 32)
        error("Unsupported mode! Supported: 32\n");

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
    add_sy_def(ftell(), "cells");
    write_bytes(0x00, memsize);
    // code section
    add_sy_def(ftell(), "_start");
    write_byte(0xb9);                               	         /* mov ecx, IMMEDIATE-DWORD  */
    add_sy_ref(ftell(), SymbolRefType.abs32, "cells", cell_off);
    write_dword(0);

    for (size_t i = 0; i < size; i++) {
        char c = buff[i];
        if (c == '\n') {
            continue;
        }
        if (mode32) {
            if (c == '>') {         // increment pt
                write_asm("    inc ecx\n");
                write_byte(0x41);
            }
            else if (c == '<') {    // decrement pt
                write_asm("    dec ecx\n");
                write_byte(0x49);
            }
            else if (c == '+') {    // inc cell at pt
                int am = (int)strspn(buff + i, "+");
                if (am > 1) {
                    writef_asm("    add byte [ecx], %i\n", am);
		    // too lazy to look at coders32 sooo
                    // yeah

                    // OMG WHAT IF I JUST RUN IT TROUGHT NASM AND THEN DO OBJDUM???
                    // big brain move
		    write_byte(0x80);    /* addb */
                    write_byte(0x01);
                    write_byte((byte)am);
                    i += am-1;
                    continue;
                } else {
                    write_asm("    inc byte [ecx]\n");
                    write_byte(0xfe);    /* incb */
                    write_byte(0x01);
                }
            }
            else if (c == '-') {    // dec cell at pt
                int am = (int)strspn(buff + i, "-");
                if (am > 1) {
                    writef_asm("    sub byte [ecx], %i\n", am);
                    write_byte(0x80);    /* subb */
                    write_byte(0x29);
                    write_byte((byte)am);
                    i += am-1;
                    continue;
                } else {
                    write_asm("    dec byte [ecx]\n");
                    write_byte(0xfe);    /* decb */
                    write_byte(0x09)
                }
            }
            else if (c == '.') {    // output char cell at pt
                used_out_syscall = 1;
                write_asm("    call putchar\n");
                // // how the fuck do i implement relative calls and jumps?
                // // anywaaaays
                // write_byte(0xe8);        /* call */
                // add_sy_ref(ftell(), SymbolRefType.abs32, "putchar", 0);

                // partially implemented cool shit, so now i can do:
                // this one byte is only requiered in 32 bit mode
                // doesnt work in 64 bits at all (i think)
                // in 16 bit mode, not requiered
                write_byte(0x66);	 /* 16 bit; call rel16/32 */
                write_byte(0xe8);
                add_sy_ref_cond(ftell(), SymbolRefType.rel16, "putchar", SWORD_MAX, 0, 0, SWORD_MAX, 2, 0);
                // in case the offset is bigger than signed 16 bit
                write_byte(0xe8); 	 /* call rel16/32 */
                add_sy_ref_cond(ftell(), SymbolRefType.rel32, "putchar", SDWORD_MAX, SWORD_MAX+1, SWORD_MAX+1, SDWORD_MAX, 1, 2);
                write_dword(0);
            }
            else if (c == ',') {    // input char cell at pt
                write_asm("    call getchar\n");
                write_byte(0x66);	 /* 16 bit; call rel16/32 */
                write_byte(0xe8);
                add_sy_ref_cond(ftell(), SymbolRefType.rel16, "getchar", SWORD_MAX, 0, 0, SWORD_MAX, 2, 0);
                write_byte(0xe8); 	 /* call rel16/32 */
                add_sy_ref_cond(ftell(), SymbolRefType.rel32, "getchar", SDWORD_MAX, SWORD_MAX+1, SWORD_MAX+1, SDWORD_MAX, 1, 2);
                write_dword(0);
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

		// the fuck did i do  here????
                if (corr > 0) {
                    write_asm("    xor eax, eax\n");
                    write_asm("    mov al, [ecx]\n");
                    write_asm("    test eax, eax\n");
                    writef_asm("    jz close_%i\n", corr);
                    writef_asm("open_%i:\n", corr);
                    // okaaay so
                    // NASM WORK PLEAAASE
		    write_byte(0x31);     /* xor */
                    write_byte(0xc0);
                    write_byte(0x8a);     /* mov */
                    write_byte(0x01);
                    write_byte(0x85);     /* test */
                    write_byte(0xc0);
                    // wait
                    // just realised that i didnt even write this
                    //  ~~ote gave it to me~~

		    // // just realised i HAVE TO implement a system to do relative jumps for stuff like je...
                    // okay implemented it
                    
                }
                else {
                    error("unclosed square brackets!\n");
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

    for (size_t i = 0; i < sy_refs_am)
        free(sy_refs[i]);

    for (size_t i = 0; i < sy_defs_am)
        free(sy_defs[i]);

    free(sy_refs);
    free(sy_defs);
    return 0;
}
