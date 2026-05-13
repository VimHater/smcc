#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *shift(int *argc, char ***argv)
{
    if (*argc <= 0) {
        if (stderr_is_tty())
            fprintf(stderr, ANSI_RED "[ERROR]" ANSI_RESET " not enough arguments\n");
        else
            fprintf(stderr, "[ERROR] not enough arguments\n");
        exit(1);
    }
    char *arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}

int stderr_is_tty(void)
{
    return isatty(STDERR_FILENO);
}

void system_run(const char *cmd)
{
    if (stderr_is_tty())
        fprintf(stderr, ANSI_DIM "[CMD]" ANSI_RESET " %s\n", cmd);
    else
        fprintf(stderr, "[CMD] %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        if (stderr_is_tty())
            fprintf(stderr, ANSI_RED"[ERROR]" ANSI_RESET" command exit with code %d\n", ret);
        else
            fprintf(stderr, "[ERROR] command exit with code %d\n", ret);
        exit(1);
    }
}
