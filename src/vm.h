#ifndef __DATA_STRUCTURES_H__
#define __DATA_STRUCTURES_H__

#include <stdint.h>
#include <stddef.h>
#include "uthash.h"

typedef size_t BlarbVM_WORD;

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
	BlarbVM_WORD *stack;
    int stack_size;
    int stack_top;
	BlarbVM_WORD registers[8];
	token **lines;
	int lineCount;
	LabelPointer *labelPointers;
} BlarbVM;

void Stack_push(BlarbVM *vm, BlarbVM_WORD value);
BlarbVM_WORD Stack_pop(BlarbVM *vm);

void BlarbVM_execute(BlarbVM *vm);

/**
 * Dump a VM trace.
 * This will output registers and the stack in a human readable format.
 */
void BlarbVM_dumpDebug(BlarbVM *vm);

/**
 * Create a new VM.
 */
BlarbVM * BlarbVM_init();
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

