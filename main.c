#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typef unsigned char byte;

#define mode32 (mode == 32)
#define mode64 (mode == 64)

#define write_asm(x) if(gen_sources) {fputs(x, fp);}
#define writef_asm(...) if(gen_sources) {fprintf(fp, __VA_ARGS__);}

#define write_byte(x) if(!gen_sources) {fwrite(&x, sizeof(byte), 1, fp);}

#define warn(x) printf(x); \
    if (werror) {          \
        return 1;          \
    }

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

    // todo: automatize
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

    write_asm("section .data\ncells:\n");
    writef_asm("    times %i db 0\n\n", memsize);
    writef_asm("section .text\n    global _start\n\n_start:\n    mov ecx, cells+%i\n\n", cell_off);
    for (int i = 0; i < memsize; i ++) {
        write_byte(0x00);
    }
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
    return 0;
}
