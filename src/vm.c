#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "vm.h"
#include "main.h"

void Stack_push(ByteList **stack, BlarbVM_WORD value) {
	ByteList *oldHead = *stack;
	*stack = malloc(sizeof(struct ByteList));

	if (*stack == 0) {
		fprintf(stderr, "Ran out of memory!\n");
		terminateVM();
	}

	(*stack)->value = value;
	(*stack)->next = oldHead;
}

BlarbVM_WORD Stack_pop(ByteList **stack) {
	ByteList *oldHead = *stack;

	if (oldHead == 0) {
		fprintf(stderr, "Popped from the stack when it was empty!\n");
		terminateVM();
	}

	*stack = oldHead->next;

	BlarbVM_WORD value = oldHead->value;
	free(oldHead);

	return value;
}

void Stack_set(ByteList **stack, BlarbVM_WORD index, BlarbVM_WORD value) {
	ByteList *head = *stack;

	while (index > 0 && head) {
		head = head->next;
		index--;
	}

	if ( ! head) {
		fprintf(stderr, "Tried setting over the stack limit: %d\n", index);
		terminateVM();
	}

	head->value = value;
}

BlarbVM_WORD Stack_peek(ByteList **stack, BlarbVM_WORD index) {
	ByteList *head = *stack;

	while (index > 0 && head) {
		head = head->next;
		index--;
	}

	if ( ! head) {
		fprintf(stderr, "Tried peeking over the stack limit: %d\n", index);
		terminateVM();
	}

	return head->value;
}

// Sets the line pointer register to the line of the given label
// Also pushes the current line to the stack
// Implicit: "0 $ #labelName 0 2 ~ 1 ^"
void BlarbVM_jumpToLabel(BlarbVM *vm, char *name) {
    LabelPointer *lab;
    HASH_FIND_STR(vm->labelPointers, name, lab);

    if (lab) {
        Stack_push(&vm->stack, vm->registers[0]); // return address
        vm->registers[0] = lab->line;
    } else  {
        fprintf(stderr, "Label not found: %s\n", name);
        terminateVM();
    }
}

void BlarbVM_pushStringLiteralToStack(BlarbVM *vm, char *line) {
	Stack_push(&vm->stack, 0);
	// Push the string on the stack 'backwards'
	for (int i = strlen(line); i >= 0; i--) {
		Stack_push(&vm->stack, (BlarbVM_WORD)line[i]);
	}
}

void BlarbVM_setRegisterFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = Stack_peek(&vm->stack, 1);
	BlarbVM_WORD regIndex = Stack_peek(&vm->stack, 0);

	vm->registers[regIndex] = Stack_peek(&vm->stack, stackIndex);

	// Pop args
	Stack_pop(&vm->stack);
	Stack_pop(&vm->stack);
}

void BlarbVM_includeFileOnStack(BlarbVM *vm) {
	char fileName[256];
	size_t size = 0;

	char c;
	while (c = (char)Stack_pop(&vm->stack)) {
		fileName[size++] = c;
	}
	fileName[size] = '\0';

	BlarbVM_loadFile(vm, fileName);
}

size_t BlarbVM_systemCallFromStack(BlarbVM *vm) {
	BlarbVM_WORD arg[6];
	arg[0] = Stack_pop(&vm->stack);
	arg[1] = Stack_pop(&vm->stack);
	arg[2] = Stack_pop(&vm->stack);
	arg[3] = Stack_pop(&vm->stack);
	arg[4] = Stack_pop(&vm->stack);
	arg[5] = Stack_pop(&vm->stack);

	// Custom BRK to internalize the VM heap
	if (arg[0] == 12) {
		size_t newEnd = arg[1];

		if (newEnd) {
			size_t currentEnd = (size_t)vm->heap + vm->heapSize;
			// If the break point is before the beggining of the heap (bad)
			if (newEnd < (size_t)vm->heap) {
				return currentEnd; // Do nothing (as per linux brk implementation)
			}
			// Resize the heap
			vm->heapSize = newEnd - (size_t)vm->heap;
			vm->heap = realloc(vm->heap, vm->heapSize);
		}
		return (size_t)(vm->heap) + vm->heapSize;
	}

	size_t ret;
	if ((ret = syscall(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5])) == -1) {
		fprintf(stderr, "Syscall args: %d, %d, %d, %d, %d, %d",
			arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
		perror("syscall");
	}
	return ret;
}

void BlarbVM_setHeapValueFromStack(BlarbVM *vm) {
	BlarbVM_WORD heapAddressIndex = Stack_peek(&vm->stack, 0);
	BlarbVM_WORD valueIndex = Stack_peek(&vm->stack, 1);

	BlarbVM_WORD heapAddress = Stack_peek(&vm->stack, heapAddressIndex);
	BlarbVM_WORD value = Stack_peek(&vm->stack, valueIndex);

	// Set the value in memory
	*((char *)heapAddress) = value;

	Stack_pop(&vm->stack);
	Stack_pop(&vm->stack);
}

void BlarbVM_pushRegisterToStack(BlarbVM *vm) {
	BlarbVM_WORD regIndex = Stack_peek(&vm->stack, 0);
	BlarbVM_WORD regValue = vm->registers[regIndex];

	// Pop args
	Stack_pop(&vm->stack);

	Stack_push(&vm->stack, regValue);
}

void BlarbVM_nandOnStack(BlarbVM *vm) {
	BlarbVM_WORD firstNandIndex = Stack_peek(&vm->stack, 0);
	BlarbVM_WORD secondNandIndex = Stack_peek(&vm->stack, 1);

	BlarbVM_WORD firstNandValue = Stack_peek(&vm->stack, firstNandIndex);
	BlarbVM_WORD secondNandValue = Stack_peek(&vm->stack, secondNandIndex);

	BlarbVM_WORD result = ~(firstNandValue & secondNandValue);
	Stack_set(&vm->stack, secondNandIndex, result);

	// Pop args
	Stack_pop(&vm->stack);
	Stack_pop(&vm->stack);
}

// Returns true if the conditional is a success
int BlarbVM_conditionalFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = Stack_peek(&vm->stack, 0);
	BlarbVM_WORD conditionalValue = Stack_peek(&vm->stack, stackIndex);
	Stack_pop(&vm->stack);
	return conditionalValue != 0;
}

void BlarbVM_popOnStack(BlarbVM *vm) {
	BlarbVM_WORD popAmount = Stack_pop(&vm->stack);

	while (popAmount >= 1) {
		Stack_pop(&vm->stack);
		popAmount--;

		if ( ! &vm->stack) {
			fprintf(stderr, "Tried popping more stack elements than avalaible\n");
			terminateVM();
		}
	}
}

void BlarbVM_executeLine(BlarbVM *vm, token *line) {
    while (line->type != NEWLINE) {
        switch (line->type) {
        case INTEGER:
            Stack_push(&vm->stack, line->val);
            break;
        case FUNCTION_CALL:
			BlarbVM_jumpToLabel(vm, line->str);
            break;
        case STR:
            BlarbVM_pushStringLiteralToStack(vm, line->str);
            break;
        case INCLUDE:
			BlarbVM_includeFileOnStack(vm);
            break;
        case REG_STORE:
			BlarbVM_setRegisterFromStack(vm);
            break;
        case REG_GET:
			BlarbVM_pushRegisterToStack(vm);
            break;
        case STACK_POP:
			BlarbVM_popOnStack(vm);
            break;
        case NAND:
			BlarbVM_nandOnStack(vm);
            break;
        case CONDITION:
			if ( ! BlarbVM_conditionalFromStack(vm)) {
				return;
			}
            break;
        case SYS_CALL:
            {
                BlarbVM_WORD result = BlarbVM_systemCallFromStack(vm);
                Stack_push(&vm->stack, result);
            }
            break;
        case MEM_SET:
			BlarbVM_setHeapValueFromStack(vm);
            break;
        }
        line++;
    }
}

void BlarbVM_execute(BlarbVM *vm) {
	BlarbVM_WORD *lineToExecute = &(vm->registers[0]); // line pointer
	while (*lineToExecute < vm->lineCount && *lineToExecute >= 0) {
		BlarbVM_executeLine(vm, vm->lines[*lineToExecute]);
		(*lineToExecute)++;
	}
}

void BlarbVM_dumpDebug(BlarbVM *vm) {
	int i;

	fprintf(stderr, "\nBeginning BlarbVM dump:\n");

	fprintf(stderr, "\nRegisters:\n");
	for (i = 0; i < sizeof(vm->registers) / sizeof(BlarbVM_WORD); i++) {
		BlarbVM_WORD value = vm->registers[i];
		fprintf(stderr, "%d: %d \n", i, value);
	}
	fprintf(stderr, "\nStack:\n");

	i = 0;
	for (ByteList *head = vm->stack; head; head = head->next, i++) {
		BlarbVM_WORD value = head->value;
		fprintf(stderr, "%d: %lld\n", i, value);
	}

	fprintf(stderr, "\nHeap (%d):\n", vm->heapSize);
	for (i = 0; i < vm->heapSize; i++) {
		char byteValue = ((char*)vm->heap)[i];
		fprintf(stderr, "%d: %lld\n", (size_t)vm->heap + i, byteValue);
	}

	fprintf(stderr, "\nDump complete.\n\n");
}

BlarbVM * BlarbVM_init() {
	BlarbVM *vm = malloc(sizeof(BlarbVM));
	memset(vm, 0, sizeof(BlarbVM));
	vm->heap = malloc(1); // Give it some random address to start with
	return vm;
}

void BlarbVM_destroy(BlarbVM *vm) {
	while (vm->stack) {
		Stack_pop(&vm->stack);
	}

	for (int i = 0; i < vm->lineCount; i++) {
		free(vm->lines[i]);
	}
	free(vm->lines);

    LabelPointer *current, *tmp; // tmp is needed in uthash
    HASH_ITER(hh, vm->labelPointers, current, tmp) {
        HASH_DEL(vm->labelPointers, current);
        free(current);
    }
}

