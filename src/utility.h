#ifndef UTILITY_H
#define UTILITY_H

char *shift(int *argc, char ***argv);

void system_run(const char *cmd);

int stderr_is_tty(void);

#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"
#define ANSI_RESET   "\033[0m"

#endif
