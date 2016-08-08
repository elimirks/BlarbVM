#ifndef __DATA_STRUCTURES_H__
#define __DATA_STRUCTURES_H__

typedef char BlarbVM_WORD;

typedef struct ByteList {
	struct ByteList *next;
	BlarbVM_WORD value;
} ByteList;

/* TODO: In the future, use labels as a precompiler construct
typedef struct LabelPointer {
	char *name;
	BlarbVM_WORD line;
} LabelPointer;
*/

typedef struct BlarbVM {
	ByteList *stack;
	BlarbVM_WORD registers[16];
	char **lines;
	int lineCount;
	// LabelPointer *labelPointers;
} BlarbVM;

void Stack_push(ByteList **stack, BlarbVM_WORD value);
BlarbVM_WORD Stack_pop(ByteList **stack);
BlarbVM_WORD Stack_printDebug(ByteList **stack);

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

