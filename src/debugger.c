#include <stdio.h>
#include "main.h"
#include "debugger.h"

void help();
int hit_breakpoint(BlarbVM *vm, int bpc);
void print_breakpoints(int bpc);
/**
 * Pushes a breakpoint if it exists. Pops otherwise.
 *
 * @param bpc The break point count.
 * @param bp  The break to add.
 */
void pushpop_breakpoint(int *bpc, BlarbVM_WORD bp);

/**
 * Executes the given command, interactively.
 */
void exec_command(BlarbVM *vm, char *command, size_t len);

#define MAX_BREAKPOINTS (256)
static BlarbVM_WORD breakpoints[MAX_BREAKPOINTS];

#define INPUT_BUFFER_LEN (1024)
static char input_buffer[INPUT_BUFFER_LEN];

void BlarbVM_debugger(BlarbVM *vm) {
    char *c;
    strncpy(input_buffer, " ", 1);

    int breakpoint_count = 0;

    help();

    while (1) {
        printf("(bdb) ");

        if (fgets(input_buffer, INPUT_BUFFER_LEN, stdin) == NULL) break;

        if ((c = strchr(input_buffer, '\n'))) *c = '\0';

        if (strncmp(input_buffer, "help", INPUT_BUFFER_LEN) == 0) {
            help();
        } else if (strncmp(input_buffer, "run", INPUT_BUFFER_LEN) == 0) {
            while (vm->running) {
                BlarbVM_step(vm);
                BlarbVM_WORD lp = vm->registers[0];
                if (hit_breakpoint(vm, breakpoint_count)) {
                    printf("Hit breakpoint on line %ld\n", lp);
                    break;
                }
            }
            if ( ! vm->running) {
                printf("Hit end of program.\n");
            }
        } else if (strncmp(input_buffer, "dump", INPUT_BUFFER_LEN) == 0) {
            BlarbVM_dumpDebug(vm);
        } else if (strncmp(input_buffer, "step", INPUT_BUFFER_LEN) == 0) {
            BlarbVM_WORD lp = vm->registers[0]; // line pointer
            BlarbVM_step(vm);
            if (vm->running) {
                printf("Executed line %ld\n", lp);
            } else {
                printf("Hit end of program.\n");
            }
        } else if (strncmp(input_buffer, "status", INPUT_BUFFER_LEN) == 0) {
            if (vm->running) {
                printf("VM is still running\n");
            } else {
                printf("Exit status: %d\n", (unsigned char)vm->exitCode);
            }
        } else if (strncmp(input_buffer, "break", 5) == 0) {
            BlarbVM_WORD line = atoi(&input_buffer[6]);

            if (breakpoint_count == MAX_BREAKPOINTS - 1) {
                printf("Too many breakpoints :(\n");
            }
            if (line != 0) {
                pushpop_breakpoint(&breakpoint_count, line);
            }
            print_breakpoints(breakpoint_count);
        } else if (strlen(input_buffer) == 1 && strncmp(input_buffer, "q", 1) == 0) {
            break;
        } else if (strncmp(input_buffer, "nands", INPUT_BUFFER_LEN) == 0) {
            printf("NANDs performed: %lu\n", vm->nandCount);
        } else if (strncmp(input_buffer, "exec ", 5) == 0) {
            char *command = &input_buffer[5];
            printf("Executing `%s`...\n", command);
            exec_command(vm, command, INPUT_BUFFER_LEN);
        } else if (strlen(input_buffer) > 0) {
            printf("Invalid command: %s\n", input_buffer);
        }
    }
}

void help() {
    printf("Welcome to the Blarb debugger.\n"
           "Available commands:\n"
           "help:       Show this dialog\n"
           "run:        Run the program\n"
           "dump:       Show a Blarb dump\n"
           "step:       Run a step (a single line)\n"
           "break n:    Set a breakpoint at line 'n'\n"
           "status:     Get exit status\n"
           "nands:      Get the amount of NANDS performed\n"
           "exec <cmd>: Executed the given Blarb command\n"
           "\n");
}

void remove_breakpoint(int *bpc, int index) {
    (*bpc)--;
    for (int i = index; i < *bpc; i++) {
        breakpoints[i] = breakpoints[i + 1];
    }
}

void pushpop_breakpoint(int *bpc, BlarbVM_WORD bp) {
    for (int i = 0; i < *bpc; i++) {
        if (breakpoints[i] == bp) {
            remove_breakpoint(bpc, i);
            return;
        }
    }

    breakpoints[*bpc] = bp;
    (*bpc)++;
}

void print_breakpoints(int bpc) {
    printf("Breakpoints:\n");
    for (int i = 0; i < bpc; i++) {
        printf("%lu\n", breakpoints[i]);
    }
}

int hit_breakpoint(BlarbVM *vm, int bpc) {
    BlarbVM_WORD linePointer = vm->registers[0];

    for (int i = 0; i < bpc; i++) {
        if (breakpoints[i] == linePointer) {
            return 1;
        }
    }
    return 0;
}

void exec_command(BlarbVM *vm, char *command, size_t len) {
    extern FILE *yyin;
    extern char *yyfilename;

    // Create a temporary in-memory file descriptor, for yylex
    char *fileContent = malloc(sizeof(char) * len);
    FILE *fp = fmemopen(fileContent, len + 1, "w+");
    fprintf(fp, "%s", command);
    rewind(fp);

    yyin = fp;
    yyfilename = "bdbexec.blarb";

    token *line = BlarbVM_scanLine(vm);

    if (line == 0) {
        printf("Nothing to execute.\n");
    } else {
        BlarbVM_executeLine(vm, line);
        free(line);
    }

    yyin = stdin;
    fclose(fp);
    free(fileContent);
}
