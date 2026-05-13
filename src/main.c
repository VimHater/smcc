#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ASSEMBLER "clang"

int main(int argc, char **argv)
{
    char *program = shift(&argc, &argv);
    char *input = NULL;
    char *output = "a.out";

    while (argc > 0) {
        char *arg = shift(&argc, &argv);
        if (strncmp(arg, "-o", 256) == 0) {
            output = shift(&argc, &argv);
        } else {
            input = arg;
        }
    }

    if (!input) {
        fprintf(stderr, "usage: %s <file.c> [-o output]\n", program);
        return 1;
    }

    // Read input file
    FILE *f = fopen(input, "r");
    if (!f) {
        if (stderr_is_tty())
            fprintf(stderr, ANSI_RED"[ERROR]" ANSI_RESET" cannot open '%s'\n", input);
        else
            fprintf(stderr, "[ERROR] cannot open '%s'\n", input);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *src = malloc(len + 1);
    fread(src, 1, len, f);
    src[len] = '\0';
    fclose(f);

    Lexer lexer;
    lexer_init(&lexer, src);
    ASTTree *tree = parse(&lexer);

    char asm_path[256];
    snprintf(asm_path, sizeof(asm_path), "%s.s", output);
    FILE *asm_file = freopen(asm_path, "w", stdout);
    if (!asm_file) {
        if (stderr_is_tty())
            fprintf(stderr, ANSI_RED"[ERROR]" ANSI_RESET" cannot create '%s'\n", asm_path);
        else
            fprintf(stderr, "[ERROR] cannot create '%s'\n", asm_path);
        return 1;
    }
    codegen(tree);
    fflush(asm_file);
    fclose(asm_file);

    FILE *asm_read = fopen(asm_path, "r");
    if (asm_read) {
        char line[256];
        if (stderr_is_tty()) {
            fprintf(stderr, ANSI_CYAN ANSI_BOLD "======== GENERATED ASM ========" ANSI_RESET "\n");
            while (fgets(line, sizeof(line), asm_read))
                fprintf(stderr, ANSI_DIM "%s" ANSI_RESET, line);
            fprintf(stderr, ANSI_CYAN ANSI_BOLD "===============================" ANSI_RESET "\n");
        } else {
            fprintf(stderr, "======== GENERATED ASM ========\n");
            while (fgets(line, sizeof(line), asm_read))
                fprintf(stderr, "%s", line);
            fprintf(stderr, "===============================\n");
        }
        fclose(asm_read);
    }

    // Assemble with clang
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        ASSEMBLER
        " --target=mips-unknown-linux-gnu"
        " -nostdlib"
        " -static"
        " -fuse-ld=lld -Wl,-e,__start"
        " %s -o %s",
        asm_path, output);
    system_run(cmd);

    free(src);
    return 0;
}
