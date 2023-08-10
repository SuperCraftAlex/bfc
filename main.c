// ============================================================================================
// ======================= B R A I N F U C K   C O M P I L E R ================================
// ============================================================================================
//
// (written by alex aka alex_s168)
//
// supported operating systems:
// - linux
//
// supported architectures:
// - 32bit x86
//
// supported brainfuck variations/extensions:
// - brainfuck
//
// credits:
// - ote aka otesunki (help with x86; help with c; [a lot of other shit i already forgot])
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
#include <stdarg.h>


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


#define write_x_raw(datatype, x) {fwrite((datatype *) &x, sizeof(datatype), 1, fp);}

void write_byte_raw(FILE *fp, byte x) {write_x_raw(byte, x);}
void write_sbyte_raw(FILE *fp, s_byte x) {write_x_raw(s_byte, x);}

void write_word_raw(FILE *fp, word x) {write_x_raw(word, x);}
void write_sword_raw(FILE *fp, s_word x) {write_x_raw(s_word, x);}

void write_dword_raw(FILE *fp, dword x) {write_x_raw(dword, x);}
void write_sdword_raw(FILE *fp, s_dword x) {write_x_raw(s_dword, x);}

void write_qword_raw(FILE *fp, qword x) {write_x_raw(qword, x);}
void write_sqword_raw(FILE *fp, s_qword x) {write_x_raw(s_qword, x);}

#define write_byte(x) if(!gen_sources) {write_byte_raw(fp, (byte)x);}
#define write_sbyte(x) if(!gen_sources) {write_sbyte_raw(fp, (s_byte)x);}

#define write_word(x) if(!gen_sources) {write_byte_raw(fp, (word)x);}
#define write_sword(x) if(!gen_sources) {write_sbyte_raw(fp, (s_word)x);}

#define write_dword(x) if(!gen_sources) {write_dword_raw(fp, (dword)x);}
#define write_sdword(x) if(!gen_sources) {write_sdword_raw(fp, (s_dword)x);}

#define write_qword(x) if(!gen_sources) {write_qword_raw(fp, (qword)x);}
#define write_sqword(x) if(!gen_sources) {write_sqword_raw(fp, (s_qword)x);}


#define warn(x) printf("warning: %s\n", x); if (werror) { return 1; };

#define error(x) printf("error: %s\n", x); return 1;

struct SymbolDef {
    size_t addr;
    char *name;
};

enum SymbolRefType {
    // unsigned integers
    // abs = "absolute"
    ref_abs16,
    ref_abs24,
    ref_abs32,
    ref_abs64,

    // 2s complement signed integers
    // rel = "relative"
    ref_rel8,
    ref_rel16,
    ref_rel32
};

struct SymbolRef {
    size_t pos;               /* position of the reference (in the file) (to be overwritten) */
    enum SymbolRefType type;  /* type of the reference*/
    char *name;
    int off;	              /* absolute*/

    /* only resolve this symbol if condition is true or when is_cond_rel == 0 */
    unsigned int is_cond_rel;
    int c_rel_backw_max;
    int c_rel_backw_min;
    int c_rel_forw_min;
    int c_rel_forw_max;
    unsigned int remove_left_ifn_c;  /* if the condition is false, it undos x bytes from left */
    unsigned int remove_right_ifn_c; /* if the condition is false, it undos x bytes from the right */
};

struct SymbolRef *add_sy_ref_func(struct SymbolRef ***sy_refs, size_t *sy_refs_am, size_t i_pos, enum SymbolRefType i_type, char *i_name, int i_off) {
    *sy_refs = realloc(*sy_refs, sizeof(struct SymbolRef *) * (*sy_refs_am + 1));
    struct SymbolRef *ref = malloc(sizeof(struct SymbolRef));
    (*sy_refs)[*sy_refs_am] = ref;
    ref->name = i_name;
    ref->type = i_type;
    ref->pos = i_pos;
    ref->off = i_off;
    (*sy_refs_am) ++;
    ref->is_cond_rel = 0;
    return ref;
}

#define add_sy_ref(pos, type, name, off) if (!gen_sources) { \
        add_sy_ref_func(&sy_refs, &sy_refs_am, pos, type, name, off); \
    }

#define add_sy_ref_cond(pos, type, name, off, backw_max, backw_min, forw_min, forw_max, leftrem, rightrem) if (!gen_sources) { \
        struct SymbolRef *ref = add_sy_ref_func(&sy_refs, &sy_refs_am, pos, type, name, off); \
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
#define add_sy_def(i_addr, i_name) if (!gen_sources) { \
        sy_defs = realloc(sy_defs, sizeof(struct SymbolDef *) * (sy_refs_am + 1)); \
        /* i should use a macro for ^that buuuuut */ \
        /* ....yeah */ \
        struct SymbolDef *sy = malloc(sizeof(struct SymbolDef)); \
        sy_defs[sy_defs_am] = sy; \
        sy->name = i_name; \
        sy->addr = i_addr; \
        sy_defs_am ++; \
    }

// OTE YOU CAN LOOK AGAIN!

// is this macro okay?
#define write_bytes(x, amount) for (size_t i = 0; i < (size_t)amount; i++) { write_byte(x); }

enum target_arch {
    tg_x86,
    tg_arm,
    tg_riscv
};

char *strformat(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char *buff = malloc(strlen(format) * 100);

    vsprintf(buff, format, args);

    va_end(args);
    return buff;
}


int gen_exit_syscall(int mode, enum target_arch arch, FILE *fp, int gen_sources, int *localid,
                     byte code) {
    if (mode == 32 && arch == tg_x86) {
        write_asm("    mov eax, 1\n");
        writef_asm("    mov ebx, %i\n", code);
        write_asm("    int 0x80\n");

        write_byte(0xb8);           /* mov */
        write_dword(1);
        write_byte(0xbb);           /* mov */
        write_dword(code);
        write_byte(0xcd);           /* int */
        write_byte(0x80);
        return 0;
    }
    // gcc wants me to use localid
    // so i use it:
    (*localid) = *localid;
    return 1;
}

// requires pointer to text to be in the cell pointer register
// preserves cell pointer register
// stream 0 is stdin
// if end-of-file: move 0 into cell at cell pointer register
int gen_read_syscall(int mode, enum target_arch arch, FILE *fp, int gen_sources, int *localid,
                     dword stream, size_t amount) {
    if (mode == 32 && arch == tg_x86) {
        write_asm("    mov eax, 3\n");
        writef_asm("    mov ebx, %iu\n", stream);
        writef_asm("    mov edx, %i\n", amount);
        write_asm("    push ecx\n");
        write_asm("    int 0x80\n");
        write_asm("    pop ecx\n");
        write_asm("    test eax, eax\n");
        writef_asm("    jnz .L%i\n", *localid);
        write_asm("    mov byte [ecx], 0\n");
        writef_asm(".L%i\n", *localid);

        write_byte(0xB8);            /* mov */
        write_dword(3);
        write_byte(0xB9);            /* mov */
        write_dword(stream);
        write_byte(0xC0);            /* mov */
        write_dword(amount);
        write_byte(0x51);            /* push */
        write_byte(0xCD);            /* int */
        write_byte(0x80);
        write_byte(0x59);            /* pop */
        write_byte(0x85);            /* test */
        write_byte(0xc0);
        write_byte(0x66);            /* 16 bit; jnz rel16 */
        write_byte(0x0F);
        write_byte(0x85);
        write_word(3);
        write_byte(0xc6);            /* mov */
        write_byte(0x01);
        write_byte(0x00);

        if (gen_sources)
            (*localid) ++;
        return 0;
    }
    return 1;
}

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
    enum target_arch arch = tg_x86;

    for (int i = 0; i < argc-2; i ++) {
        char *x = argv[i+1];
        char c = x[1];

        if (c == 'a') {
            char *a = x+2;
            if (!strcmp(a, "x86")) { arch = tg_x86; }
            else if (!strcmp(a, "arm")) { arch = tg_arm; }
            else if (!strcmp(a, "riscv")) { arch = tg_riscv; }
            else { error("Architecture not supported!"); }
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

    if (arch != tg_x86) {
        error("Architecture not supported yet!");
    }

    if (cell_off < 0) {
        cell_off = memsize / 2;
        warn("Automatically set the cell offset");
    }

    if (mode != 32) {
        error("Unsupported mode! Supported: 32");
    }

    FILE *fp;
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
    for (int i = 0; i < (int)size; i ++) {
        char c = buff[i];
        if (c == '%') {
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
    if (fp == NULL) {
        error("error writing file");
        return 1;
    }
    int localid = 0;

    struct SymbolDef **sy_defs = malloc(1 * sizeof(struct SymbolDef *));
    size_t sy_defs_am = 0;

    struct SymbolRef **sy_refs = malloc(1 * sizeof(struct SymbolDef *));
    size_t sy_refs_am = 0;

    write_asm("section .data\ncells:\n");
    writef_asm("    times %i db 0\n\n", memsize);
    writef_asm("section .text\n    global _start\n\n_start:\n    mov ecx, cells+%i\n\n", cell_off);

    // data section
    add_sy_def(ftell(fp), "cells");
    write_bytes(0x00, memsize);
    // code section
    add_sy_def(ftell(fp), "_start");
    write_byte(0xb9);                                    /* mov */
    add_sy_ref(ftell(fp), ref_abs32, "cells", cell_off);
    write_dword(0);

    for (size_t i = 0; i < size; i++) {
        char c = buff[i];
        putchar(c);
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

		    write_byte(0x80);    /* addb */
                    write_byte(0x01);
                    write_byte(am);
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
                    write_byte(am);
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

                // this one byte is only requiered in 32 bit mode
                // doesnt work in 64 bits at all (i think)
                // in 16 bit mode, not requiered
                write_byte(0x66);	 /* 16 bit; call rel16/32 */
                write_byte(0xe8);
                add_sy_ref_cond(ftell(fp), ref_rel16, "putchar", 0, SWORD_MAX, 0, 0, SWORD_MAX, 2, 0);
                // in case the offset is bigger than signed 16 bit
                write_byte(0xe8); 	 /* call rel16/32 */
                add_sy_ref_cond(ftell(fp), ref_rel32, "putchar", 0, SDWORD_MAX, SWORD_MAX, SWORD_MAX, SDWORD_MAX, 1, 2);
                write_dword(0);
            }
            else if (c == ',') {    // input char cell at pt
                write_asm("    call getchar\n");
                write_byte(0x66);	 /* 16 bit; call rel16/32 */
                write_byte(0xe8);
                add_sy_ref_cond(ftell(fp), ref_rel16, "getchar", 0, SWORD_MAX, 0, 0, SWORD_MAX, 2, 0);
                write_byte(0xe8); 	 /* call rel16/32 */
                add_sy_ref_cond(ftell(fp), ref_rel32, "getchar", 0, SDWORD_MAX, SWORD_MAX, SWORD_MAX, SDWORD_MAX, 1, 2);
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

                if (corr > 0) {
                    write_asm("    cmp byte [ecx], 0\n");
                    writef_asm("    je close_%i\n", corr);
                    writef_asm("open_%i:\n", corr);

		    write_byte(0x80);     /* cmp */
                    write_byte(0x39);
                    write_byte(0x00);

                    write_byte(0x66);     /* 16 bit; je rel16 */
                    write_byte(0x0F);
                    write_byte(0x84);
                    add_sy_ref_cond(ftell(fp), ref_rel16, strformat("close_%i", corr), 0, SWORD_MAX, 0, 0, SWORD_MAX, 3, 0);
                    write_byte(0x0F); 	  /* je rel32 */
                    write_byte(0x84);
                    add_sy_ref_cond(ftell(fp), ref_rel32, strformat("close_%i", corr), 0, SDWORD_MAX, SWORD_MAX, SWORD_MAX, SDWORD_MAX, 2, 2);
                    write_dword(0);

                    add_sy_def(ftell(fp), strformat("open_%i", corr));
                }
                else {
                    error("unclosed square brackets!\n");
                }
            }
            else if (c == ']') {    // jumps after the corresponding '[' instruction if the cell val is not 0
                write_asm("    cmp byte [ecx], 0\n");
                writef_asm("    jnz open_%i\n", (int)i);
                writef_asm("close_%i:\n", (int)i);

                write_byte(0x80);         /* cmp */
                write_byte(0x39);
                write_byte(0x00);

                write_byte(0x66);         /* 16 bit; jnz rel16 */
                write_byte(0x0F);
                write_byte(0x85);
                add_sy_ref_cond(ftell(fp), ref_rel16, strformat("open_%i", i), 0, SWORD_MAX, 0, 0, SWORD_MAX, 3, 0);
                write_byte(0x0F); 	  /* jnz rel32 */
                write_byte(0x85);
                add_sy_ref_cond(ftell(fp), ref_rel32, strformat("open_%i", i), 0, SDWORD_MAX, SWORD_MAX, SWORD_MAX, SDWORD_MAX, 2, 2);
                write_dword(0);

                add_sy_def(ftell(fp), strformat("close_%i", i));

            }
            else {
                continue;
            }
        }
    }
    if (mode32) {
        // generate exit(0) syscall
        write_asm("end:\n");
        add_sy_def(ftell(fp), "end");
        gen_exit_syscall(mode, arch, fp, gen_sources, &localid, 0);

        if (used_in_syscall) {
            // generate read(0, pt, 1) syscall
            write_asm("getchar:\n");
            add_sy_def(ftell(fp), "getchar");
            gen_read_syscall(mode, arch, fp, gen_sources, &localid, 0, 1);
            write_asm("    ret\n");
            write_byte(0xC3);             /* ret */
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

    free(buff);

    // todo: resolve symbols

    fclose(fp);

    if (!gen_sources) {
        // todo: convert file with name [fn] to elf
    }

    for (size_t i = 0; i < sy_refs_am; i++)
        free(sy_refs[i]);

    for (size_t i = 0; i < sy_defs_am; i++)
        free(sy_defs[i]);

    free(sy_refs);
    free(sy_defs);
    return 0;
}
