#ifndef __DATA_STRUCTURES_H__
#define __DATA_STRUCTURES_H__

#include <stdint.h>

typedef uint32_t BlarbVM_WORD;

typedef struct ByteList {
	struct ByteList *next;
	BlarbVM_WORD value;
} ByteList;

typedef struct LabelPointer {
	char *name;
	int line;
} LabelPointer;

typedef struct BlarbVM {
	ByteList *stack;
	BlarbVM_WORD registers[8];
	char **lines;
	int lineCount;
	LabelPointer *labelPointers;
	int labelPointerCount;
} BlarbVM;

void Stack_push(ByteList **stack, BlarbVM_WORD value);
BlarbVM_WORD Stack_pop(ByteList **stack);
BlarbVM_WORD Stack_printDebug(ByteList **stack);

void BlarbVM_loadFile(BlarbVM *vm, char *fileName);

/**
 * @param line The line to add - will be free'd in BlarbVM_destroy
 */
void BlarbVM_addLine(BlarbVM *vm, char *line);
void BlarbVM_execute(BlarbVM *vm);
// Outside VM code, this is used for debugging (script injection)
void BlarbVM_executeLine(BlarbVM *vm, char *line);

void BlarbVM_addFile(BlarbVM *vm, char *name, FILE *file);

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

#endif

