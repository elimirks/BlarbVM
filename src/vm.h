#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>
#include <stddef.h>
#include "uthash.h"

typedef size_t BlarbVM_WORD;

#define BLARB_STACK_SIZE (16384)

// A massive lookup table for virtual memory
extern int SYSCALL_POINTER_TABLE[329][6];

// Token types
typedef enum {
    // Note: There are no strings, since they are parsed while scanning
    INTEGER = 1,
    LABEL_CALL,
    LABEL,
    INCLUDE,
    REG_STORE,
    REG_GET,
    STACK_POP,
    NAND,
    CONDITION,
    SYS_CALL,
    HEAP_SWAP,
    // These symbols get abstracted to the parser
    NEWLINE,
    STR,
    CHR,
    // Optimized operations
    EXPLICIT_STACK_POP,
    EXPLICIT_NAND,
    EXPLICIT_REG_GET,
    EXPLICIT_CONDITION,
} token_t;

typedef struct token {
    token_t type;
    union {
        BlarbVM_WORD val;
        char *str;
        // For optimizing parameters, we sometimes store multiple values
        BlarbVM_WORD vals[6];
    };
} token;

typedef struct LabelPointer {
	char name[64];
	int line;
    UT_hash_handle hh;
} LabelPointer;

typedef struct LineDebugInfo {
    char *fileName;
    BlarbVM_WORD line;
} LineDebugInfo;

typedef struct BlarbVM {
	void *heap;
	size_t heapSize;
	BlarbVM_WORD stack[BLARB_STACK_SIZE];
    BlarbVM_WORD stack_top;
	BlarbVM_WORD registers[8];
	token **lines;
	LineDebugInfo *linesDebug;
	BlarbVM_WORD lineCount;
	LabelPointer *labelPointers;
    int running;
    int exitCode;
    // For fun analytics
    size_t nandCount;
    // To keep track of loaded files. It shouldn't include twice!
    char **loadedFileNames;
    size_t loadedFileNameCount;
} BlarbVM;

void BlarbVM_pushToStack(BlarbVM *vm, BlarbVM_WORD value);

/**
 * Pushes the given string to the stack.
 */
void BlarbVM_pushStackArg(BlarbVM *vm, const char *s);

/**
 * Steps through a single line of blarb
 */
void BlarbVM_step(BlarbVM *vm);

/**
 * Continues running the VM.
 */
void BlarbVM_execute(BlarbVM *vm);

/**
 * Executes a single given line
 */
void BlarbVM_executeLine(BlarbVM *vm, token *line);

/**
 * Dump a VM trace.
 * This will output registers and the stack in a human readable format.
 */
void BlarbVM_dumpDebug(BlarbVM *vm);

/**
 * Create a new VM.
 */
void BlarbVM_init(BlarbVM *vm);

/**
 * Destroy an existing VM.
 */
void BlarbVM_destroy(BlarbVM *vm);

/**
 * Scans a line into tokens.
 *
 * Make sure you set yyin before calling this!
 */
token * BlarbVM_scanLine(BlarbVM *vm);

/**
 * Loads a blarb file into the VM, to be run. Can be dynamic.
 */
void BlarbVM_loadFile(BlarbVM *vm, char *fileName);

/**
 * @param line The line to add - will be free'd in BlarbVM_destroy
 */
void BlarbVM_addLine(BlarbVM *vm, token *line);

/**
 * Executes the given syscall number. Refer to the 64 bit linux syscall table.
 */
size_t BlarbVM_performSyscall(BlarbVM *vm, BlarbVM_WORD num, BlarbVM_WORD *args);
#endif
