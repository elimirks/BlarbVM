#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "vm.h"
#include "main.h"

void Stack_push(BlarbVM *vm, BlarbVM_WORD value) {
	if (vm->stack_top == vm->stack_size) {
		// Double the size when space runs out - amortized time is O(1)
        vm->stack_size *= 2;
		vm->stack = realloc(vm->stack, sizeof(BlarbVM_WORD) * vm->stack_size);
        if (vm->stack == NULL) {
            fprintf(stderr, "Ran out of memory!\n");
            terminateVM();
        }
	}

    vm->stack[vm->stack_top] = value;
    vm->stack_top += 1;
}

BlarbVM_WORD Stack_pop(BlarbVM *vm) {
    if (vm->stack_top == 0) {
		fprintf(stderr, "Popped from the stack when it was empty!\n");
		terminateVM();
    }
    if (vm->stack_top < (vm->stack_size / 2) - 1) {
		// Preventing memory leakage, if we aren't using much of the stack anymore
        vm->stack_size /= 2;
		vm->stack = realloc(vm->stack, sizeof(BlarbVM_WORD) * vm->stack_size);
        if (vm->stack == NULL) {
            fprintf(stderr, "Ran out of memory!\n");
            terminateVM();
        }
    }

    vm->stack_top -= 1;
    return vm->stack[vm->stack_top];
}

void Stack_set(BlarbVM *vm, BlarbVM_WORD index, BlarbVM_WORD value) {
	if (index < 0 || index >= vm->stack_top) {
		fprintf(stderr, "Tried setting over the stack limit: %d\n", index);
		terminateVM();
	}

    vm->stack[vm->stack_top - 1 - index] = value;
}

BlarbVM_WORD Stack_peek(BlarbVM *vm, BlarbVM_WORD index) {
	if (index < 0 || index >= vm->stack_top) {
		fprintf(stderr, "Tried setting over the stack limit: %d\n", index);
		terminateVM();
	}

    return vm->stack[vm->stack_top - 1 - index];
}

// Sets the line pointer register to the line of the given label
// Also pushes the current line to the stack
// Implicit: "0 $ #labelName 0 2 ~ 1 ^"
void BlarbVM_jumpToLabel(BlarbVM *vm, char *name) {
    LabelPointer *lab;
    HASH_FIND_STR(vm->labelPointers, name, lab);

    if (lab) {
        Stack_push(vm, vm->registers[0]); // return address
        // -1 b.c. the line pointer increments
        vm->registers[0] = lab->line - 1;
    } else  {
        fprintf(stderr, "Label not found: %s\n", name);
        terminateVM();
    }
}

void BlarbVM_pushStringLiteralToStack(BlarbVM *vm, char *line) {
	// Push the string on the stack 'backwards'
	for (int i = strlen(line); i >= 0; i--) {
		Stack_push(vm, (BlarbVM_WORD)line[i]);
	}
}

void BlarbVM_setRegisterFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = Stack_peek(vm, 1);
	BlarbVM_WORD regIndex = Stack_peek(vm, 0);

	vm->registers[regIndex] = Stack_peek(vm, stackIndex);

	// Pop args
	Stack_pop(vm);
	Stack_pop(vm);
}

void BlarbVM_includeFileOnStack(BlarbVM *vm) {
	char fileName[256];
	size_t size = 0;

	char c;
	while ((c = (char)Stack_pop(vm))) {
		fileName[size++] = c;
	}
	fileName[size] = '\0';

	BlarbVM_loadFile(vm, fileName);
}

size_t BlarbVM_brk(BlarbVM *vm, size_t newEnd) {
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

size_t BlarbVM_exit(BlarbVM *vm, size_t exitCode) {
    vm->running = 0;
    vm->exitCode = exitCode;
}

size_t BlarbVM_systemCallFromStack(BlarbVM *vm) {
	BlarbVM_WORD arg[6];
	arg[0] = Stack_pop(vm);
	arg[1] = Stack_pop(vm);
	arg[2] = Stack_pop(vm);
	arg[3] = Stack_pop(vm);
	arg[4] = Stack_pop(vm);
	arg[5] = Stack_pop(vm);

    switch (arg[0]) {
    case 12: return BlarbVM_brk(vm, arg[1]);
    case 60: return BlarbVM_exit(vm, arg[1]);
    }

	size_t ret;
	if ((ret = syscall(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5])) == -1) {
		fprintf(stderr, "Syscall args: %d, %d, %d, %d, %d, %d\n",
			arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
		perror("syscall");
	}
	return ret;
}

void BlarbVM_setHeapValueFromStack(BlarbVM *vm) {
	BlarbVM_WORD heapAddressIndex = Stack_peek(vm, 0);
	BlarbVM_WORD valueIndex = Stack_peek(vm, 1);

	BlarbVM_WORD heapAddress = Stack_peek(vm, heapAddressIndex);
	BlarbVM_WORD value = Stack_peek(vm, valueIndex);

    char old_value = *((char *)heapAddress);
	// Set the value in memory
	*((char *)heapAddress) = value;
    Stack_set(vm, valueIndex, old_value);

	Stack_pop(vm);
	Stack_pop(vm);
}

void BlarbVM_pushRegisterToStack(BlarbVM *vm) {
	BlarbVM_WORD regIndex = Stack_peek(vm, 0);
	BlarbVM_WORD regValue = vm->registers[regIndex];

	// Pop args
	Stack_pop(vm);

	Stack_push(vm, regValue);
}

void BlarbVM_nandOnStack(BlarbVM *vm) {
	BlarbVM_WORD firstNandIndex = Stack_peek(vm, 0);
	BlarbVM_WORD secondNandIndex = Stack_peek(vm, 1);

	BlarbVM_WORD firstNandValue = Stack_peek(vm, firstNandIndex);
	BlarbVM_WORD secondNandValue = Stack_peek(vm, secondNandIndex);

	BlarbVM_WORD result = ~(firstNandValue & secondNandValue);
	Stack_set(vm, secondNandIndex, result);

	// Pop args
	Stack_pop(vm);
	Stack_pop(vm);
}

// Returns true if the conditional is a success
int BlarbVM_conditionalFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = Stack_peek(vm, 0);
	BlarbVM_WORD conditionalValue = Stack_peek(vm, stackIndex);
	Stack_pop(vm);
	return conditionalValue != 0;
}

void BlarbVM_popOnStack(BlarbVM *vm) {
	BlarbVM_WORD popAmount = Stack_pop(vm);

	while (popAmount >= 1) {
		if (vm->stack_top == 0) {
			fprintf(stderr, "Tried popping more stack elements than avalaible\n");
			terminateVM();
		}
		Stack_pop(vm);
		popAmount--;
	}
}

void BlarbVM_executeLine(BlarbVM *vm, token *line) {
    while (line->type != NEWLINE) {
        switch (line->type) {
        case INTEGER:
            Stack_push(vm, line->val);
            break;
        case LABEL_CALL:
			BlarbVM_jumpToLabel(vm, line->str);
            // Returning b.c. it should ignore all other tokens on this line
            return;
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
                Stack_push(vm, result);
            }
            break;
        case HEAP_SWAP:
			BlarbVM_setHeapValueFromStack(vm);
            break;
        }
        line++;
    }
}

void BlarbVM_step(BlarbVM *vm) {
	BlarbVM_WORD *lineToExecute = &(vm->registers[0]); // line pointer
	if (*lineToExecute < vm->lineCount) {
		BlarbVM_executeLine(vm, vm->lines[*lineToExecute]);
		(*lineToExecute)++;
	} else {
        vm->running = 0;
    }
}

void BlarbVM_execute(BlarbVM *vm) {
    while (vm->running) {
        BlarbVM_step(vm);
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

	for (i = 0; i < vm->stack_top; i++) {
		BlarbVM_WORD value = vm->stack[i];
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
    vm->stack = malloc(sizeof(BlarbVM_WORD));
    vm->stack_size = 1;
    vm->stack_top = 0;
    vm->exitCode = 0;
    vm->running = 1;
	return vm;
}

void BlarbVM_destroy(BlarbVM *vm) {
	while (vm->stack_top) {
		Stack_pop(vm);
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

