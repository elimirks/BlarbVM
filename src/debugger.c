#include <stdio.h>
#include <math.h>
#include "main.h"
#include "debugger.h"

void help();
void print_current_vm_line(BlarbVM *vm);
void print_loaded_files(BlarbVM *vm);
int hit_breakpoint(BlarbVM *vm, int bpc);
void print_breakpoints(int bpc);

void list_current_context(BlarbVM *vm);

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
void exec_command(BlarbVM *vm, char *command);

#define MAX_BREAKPOINTS (256)
static BlarbVM_WORD breakpoints[MAX_BREAKPOINTS];

#define INPUT_BUFFER_LEN (1024)
static char input_buffer[INPUT_BUFFER_LEN];

enum {
      DISPLAY_UNSIGNED_INTEGER = 0,
      DISPLAY_SIGNED_INTEGER,
      DISPLAY_HEX,
      DISPLAY_BINARY,
};

static int dump_display_mode = DISPLAY_UNSIGNED_INTEGER;

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
            if (vm->running) {
                print_current_vm_line(vm);
            } else {
                printf("Hit end of program.\n");
            }
            BlarbVM_step(vm);
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
        } else if (strncmp(input_buffer, "files", INPUT_BUFFER_LEN) == 0) {
            printf("Blarb files loaded:\n");
            print_loaded_files(vm);
        } else if (strncmp(input_buffer, "exec ", 5) == 0) {
            char *command = &input_buffer[5];
            printf("Executing `%s`...\n", command);
            exec_command(vm, command);
        } else if (strncmp(input_buffer, "list", 4) == 0) {
            list_current_context(vm);
        } else if (strncmp(input_buffer, "display ", 8) == 0) {
            char *display_type = &input_buffer[8];

            if (strncmp(display_type, "int", 3) == 0) {
                dump_display_mode = DISPLAY_SIGNED_INTEGER;
            } else if (strncmp(display_type, "uint", 4) == 0) {
                dump_display_mode = DISPLAY_UNSIGNED_INTEGER;
            } else if (strncmp(display_type, "hex", 3) == 0) {
                dump_display_mode = DISPLAY_HEX;
            } else if (strncmp(display_type, "binary", 6) == 0) {
                dump_display_mode = DISPLAY_BINARY;
            } else {
                printf("Invalid display type.\n"
                       "Please choose uint, int, hex, or binary\n");
            }
        } else if (strlen(input_buffer) > 0) {
            printf("Invalid command: %s\n", input_buffer);
        }
    }
}

void help() {
    printf("Welcome to the Blarb debugger.\n"
           "Available commands:\n"
           "help:             Show this dialog\n"
           "run:              Run the program\n"
           "dump:             Show a Blarb dump\n"
           "step:             Run a step (a single line)\n"
           "break n:          Set a breakpoint at line 'n'\n"
           "status:           Get exit status\n"
           "nands:            Get the amount of NANDS performed\n"
           "files:            Prints which Blarb files have been loaded\n"
           "exec <cmd>:       Executed the given Blarb command\n"
           "list:             List the blarb code around the current line pointer\n"
           "display <mode>:   Change the dump display mode (uint, int, hex, or binary)\n"
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

void exec_command(BlarbVM *vm, char *command) {
    extern FILE *yyin;
    extern char *yyfilename;

    // In-memory file descriptor, for yylex
    char fileContent[INPUT_BUFFER_LEN];
    FILE *fp = fmemopen(fileContent, INPUT_BUFFER_LEN, "w+");
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
}

void print_loaded_files(BlarbVM *vm) {
    for (size_t i = 0; i < vm->loadedFileNameCount; i++) {
        printf("%s\n", vm->loadedFileNames[i]);
    }
}

void print_current_vm_line(BlarbVM *vm) {
    BlarbVM_WORD lp = vm->registers[0]; // line pointer
    LineDebugInfo *info = &vm->linesDebug[lp];
    printf("Executed %s:%ld\n", info->fileName, info->line);
}

void print_token_line(token *line) {
    for (token *t = line; t->type != NEWLINE; t++) {
        switch (t->type) {
        case INTEGER:
            printf("%lu ", t->val);
            break;
        case LABEL_CALL:
            printf("%s ", t->str);
            break;
        case INCLUDE:
            printf("@ ");
            break;
        case REG_STORE:
            printf("~ ");
            break;
        case REG_GET:
            printf("$ ");
            break;
        case STACK_POP:
            printf("^ ");
            break;
        case NAND:
            printf("! ");
            break;
        case CONDITION:
            printf("? ");
            break;
        case SYS_CALL:
            printf("%% ");
            break;
        case HEAP_SWAP:
            printf("= ");
            break;
        case STR:
            printf("\"%s\" ", t->str);
            break;
        case CHR:
            printf("\'%c\' ", (char)t->val);
            break;
        case EXPLICIT_STACK_POP:
            printf("%lu ^ ", t->val);
            break;
        case EXPLICIT_NAND:
            printf("%lu %lu ! ", t->vals[0], t->vals[1]);
            break;
        case EXPLICIT_REG_GET:
            printf("%lu $ ", t->val);
            break;
        case EXPLICIT_CONDITION:
            printf("%lu ? ", t->val);
            break;
        case LABEL:
            printf("#%s ", t->str);
            break;
        default:
            fprintf(stderr, "Unrecognized token type\n");
            exit(1);
        }
    }
}

// Asumes little endian
void printBits(size_t const size, void const * const ptr) {
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i = size-1; i >= 0; i--) {
        for (j = 7; j>=0; j--) {
            byte = (b[i] >> j) & 1;
            fprintf(stderr, "%u", byte);
        }
    }
}

void displayWord(unsigned long index, BlarbVM_WORD word) {
    switch (dump_display_mode) {
    case DISPLAY_BINARY:
        fprintf(stderr, "%lu: ", index);
        printBits(sizeof(word), &word);
        fprintf(stderr, " \n");
        break;
    case DISPLAY_UNSIGNED_INTEGER:
        fprintf(stderr, "%lu: %lu \n", index, word);
        break;
    case DISPLAY_SIGNED_INTEGER:
        fprintf(stderr, "%lu: %ld \n", index, word);
        break;
    case DISPLAY_HEX:
        fprintf(stderr, "%lu: %016lx\n", index, word);
        break;
    default:
        fprintf(stderr, "Invalid word display mode\n");
        exit(1);
        break;
    }
}

void BlarbVM_dumpDebug(BlarbVM *vm) {
	unsigned long i;

	fprintf(stderr, "\nBeginning BlarbVM dump:\n");

	fprintf(stderr, "\nRegisters:\n");
	for (i = 0; i < sizeof(vm->registers) / sizeof(BlarbVM_WORD); i++) {
		BlarbVM_WORD value = vm->registers[i];

        if (i == 0) {
            // Register 0 is the line pointer, so let's output some useful info

            BlarbVM_WORD lp = vm->registers[0];

            if (lp < vm->lineCount) {
                LineDebugInfo *info = &vm->linesDebug[lp];
                fprintf(stderr, "%lu: %lx (%s:%lu)\n",
                        i, value, info->fileName, info->line);
            } else {
                fprintf(stderr, "%lu: %lx (invalid line pointer)\n",
                        i, value);
            }
        } else {
            displayWord(i, value);
        }
	}
	fprintf(stderr, "\nStack:\n");

	for (i = 0; i < vm->stack_top; i++) {
		BlarbVM_WORD value = vm->stack[i];
        displayWord(i, value);
	}

	fprintf(stderr, "\nHeap (%lu):\n", vm->heapSize);
	for (i = 0; i < vm->heapSize; i++) {
		char byteValue = ((char*)vm->heap)[i];
		fprintf(stderr, "%lx: %08x\n", i, byteValue);
	}

	fprintf(stderr, "\nDump complete.\n\n");
}

void list_current_context(BlarbVM *vm) {
    BlarbVM_WORD lp = vm->registers[0];

    BlarbVM_WORD start = lp < 10 ? 0 : lp - 10;
    BlarbVM_WORD end   = lp + 10 > vm->lineCount ? vm->lineCount : lp + 10;

    for (BlarbVM_WORD i = start; i < end; i++) {
        LineDebugInfo *info = &vm->linesDebug[i];
        printf("%s:%-4lu -> %-8lu", info->fileName, info->line, i);
        if (i == lp) {
            printf("* ");
        } else {
            printf("  ");
        }
        print_token_line(vm->lines[i]);
        printf("\n");
    }
}
