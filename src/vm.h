#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>
#include <stddef.h>
#include "uthash.h"

typedef size_t BlarbVM_WORD;

#define BLARB_STACK_SIZE (16384)

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
} token_t;

typedef struct token {
    token_t type;
    union {
        BlarbVM_WORD val;
        char *str;
    };
} token;

typedef struct LabelPointer {
	char name[64];
	int line;
    UT_hash_handle hh;
} LabelPointer;

typedef struct BlarbVM {
	void *heap;
	size_t heapSize;
	BlarbVM_WORD stack[BLARB_STACK_SIZE];
    BlarbVM_WORD stack_top;
	BlarbVM_WORD registers[8];
	token **lines;
	BlarbVM_WORD lineCount;
	LabelPointer *labelPointers;
    int running;
    int exitCode;
} BlarbVM;

void BlarbVM_pushToStack(BlarbVM *vm, BlarbVM_WORD value);

/**
 * Pushes the given string to the stack.
 */
void BlarbVM_pushStackArg(BlarbVM *vm, const char *s);

void BlarbVM_step(BlarbVM *vm); // Steps through a single line of blarb
void BlarbVM_execute(BlarbVM *vm);

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

void BlarbVM_loadFile(BlarbVM *vm, char *fileName);

/**
 * @param line The line to add - will be free'd in BlarbVM_destroy
 */
void BlarbVM_addLine(BlarbVM *vm, token *line);
#endif
