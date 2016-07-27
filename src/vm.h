#ifndef __DATA_STRUCTURES_H__
#define __DATA_STRUCTURES_H__

typedef char BlarbVM_WORD;

typedef struct ByteList {
	struct ByteList *next;
	BlarbVM_WORD value;
} ByteList;

typedef struct BlarbVM {
	ByteList *stack;
	BlarbVM_WORD registers[32];
} BlarbVM;

void Stack_push(ByteList **stack, BlarbVM_WORD value);
BlarbVM_WORD Stack_pop(ByteList **stack);
BlarbVM_WORD Stack_printDebug(ByteList **stack);

void BlarbVM_executeLine(BlarbVM *vm, char *line);
void BlarbVM_dumpDebug(BlarbVM *vm);

#endif

