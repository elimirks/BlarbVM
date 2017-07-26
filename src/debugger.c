#include <stdio.h>
#include "main.h"
#include "debugger.h"

void help();
int hit_breakpoint(BlarbVM *vm, int *bps, int bpc);
void list_breakpoints(int *bps, int bpc);
void push_breakpoint(int **bps, int *bpc, int bp);

void BlarbVM_debugger(BlarbVM *vm) {
    char *c;
    char *buffer;
    size_t  n = 1024;
    buffer = malloc(n);
    strncpy(buffer, " ", 1);

    int *breakpoints = malloc(sizeof(int) * 256); // deal with it
    int breakpoint_count = 0;

    help();

    while (1) {
        printf("(bdb) ");

        if (fgets(buffer, n, stdin) == NULL) break;

        if ((c = strchr(buffer, '\n'))) *c = '\0';

        if (strncmp(buffer, "help", n) == 0) {
            help();
        } else if (strncmp(buffer, "run", n) == 0) {
            while (vm->running) {
                BlarbVM_step(vm);
                BlarbVM_WORD lp = vm->registers[0];
                if (hit_breakpoint(vm, breakpoints, breakpoint_count)) {
                    printf("Hit breakpoint on line %ld\n", lp);
                    break;
                }
            }
            if ( ! vm->running) {
                printf("Hit end of program.\n");
            }
        } else if (strncmp(buffer, "dump", n) == 0) {
            BlarbVM_dumpDebug(vm);
        } else if (strncmp(buffer, "step", n) == 0) {
            BlarbVM_WORD lp = vm->registers[0]; // line pointer
            BlarbVM_step(vm);
            if (vm->running) {
                printf("Executed line %ld\n", lp);
            } else {
                printf("Hit end of program.\n");
            }
        } else if (strncmp(buffer, "status", n) == 0) {
            if (vm->running) {
                printf("VM is still running\n");
            } else {
                printf("Exit status: %d\n", (unsigned char)vm->exitCode);
            }
        } else if (strncmp(buffer, "break", 1) == 0) {
            int line = atoi(&buffer[6]);
            push_breakpoint(&breakpoints, &breakpoint_count, line);
            list_breakpoints(breakpoints, breakpoint_count);
        } else if (strlen(buffer) == 1 && strncmp(buffer, "q", 1) == 0) {
            break;
        } else if (strlen(buffer) > 0) {
            printf("Invalid command: %s\n", buffer);
        }
    }
    free(buffer);
}

void help() {
    printf("Welcome to the Blarb debugger.\n"
           "Available commands:\n"
           "help:    Show this dialog\n"
           "run:     Run the program\n"
           "dump:    Show a Blarb dump\n"
           "step:    Run a step (a single line)\n"
           "break n: Set a breakpoint at line 'n'\n"
           "status:  Get exit status\n"
           "\n");
}

void push_breakpoint(int **bps, int *bpc, int bp) {
    *bps[*bpc] = bp;
    (*bpc)++;
}

void list_breakpoints(int *bps, int bpc) {
    printf("Breakpoints:\n");
    for (int i = 0; i < bpc; i++) {
        printf("%d\n", bps[i]);
    }
}

int hit_breakpoint(BlarbVM *vm, int *bps, int bpc) {
    BlarbVM_WORD lp = vm->registers[0]; // line pointer

    for (int i = 0; i < bpc; i++) {
        if (bps[i] == lp) {
            return 1;
        }
    }
    return 0;
}
