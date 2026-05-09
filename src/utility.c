#include "utility.h"
#include <stdio.h>
#include <stdlib.h>

char *shift(int *argc, char ***argv)
{
    if (*argc <= 0) {
        fprintf(stderr, "[ERROR] not enough arguments\n");
        exit(1);
    }
    char *arg = **argv;
    (*argc)--;
    (*argv)++;
    return arg;
}

void system_run(const char *cmd)
{
    fprintf(stderr, "[CMD] %s\n", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] command exit with code %d\n", ret);
        exit(1);
    }
}
