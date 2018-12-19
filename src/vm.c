#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "vm.h"
#include "main.h"

size_t BlarbVM_exit(BlarbVM *vm, size_t exitCode) {
    vm->running = 0;
    vm->exitCode = exitCode;
    return 0;
}

void BlarbVM_pushToStack(BlarbVM *vm, BlarbVM_WORD value) {
	if (vm->stack_top == BLARB_STACK_SIZE - 1) {
        fprintf(stderr, "Blarb Stack Overflow!\n");
        terminateVM();
	}
    vm->stack[(vm->stack_top)++] = value;
}

BlarbVM_WORD BlarbVM_popFromStack(BlarbVM *vm) {
    if (vm->stack_top == 0) {
		fprintf(stderr, "Popped from the stack when it was empty!\n");
		terminateVM();
    }
    return vm->stack[--(vm->stack_top)];
}

void BlarbVM_setOnStack(BlarbVM *vm, BlarbVM_WORD index, BlarbVM_WORD value) {
	if (index >= vm->stack_top) {
		fprintf(stderr, "Tried setting over the stack limit: %lu\n", index);
		terminateVM();
	}

    vm->stack[vm->stack_top - 1 - index] = value;
}

BlarbVM_WORD BlarbVM_peekOnStack(BlarbVM *vm, BlarbVM_WORD index) {
	if (index >= vm->stack_top) {
		fprintf(stderr, "Tried setting over the stack limit: %lu\n", index);
		terminateVM();
	}

    return vm->stack[vm->stack_top - 1 - index];
}

// Sets the line pointer register to the line of the given label
// Also pushes the current line to the stack
// Implicit: "0 $ #labelName 0 2 ~ 1 ^"
void BlarbVM_jumpToLabel(BlarbVM *vm, char *name) {
    LabelPointer *label;
    HASH_FIND_STR(vm->labelPointers, name, label);

    if (label) {
        // -1 b.c. the line pointer increments
        vm->registers[0] = label->line - 1;
    } else  {
        fprintf(stderr, "Label not found: %s\n", name);
        terminateVM();
    }
}

void BlarbVM_pushStringLiteralToStack(BlarbVM *vm, char *line) {
	// Push the string on the stack 'backwards'
	for (int i = strlen(line); i >= 0; i--) {
		BlarbVM_pushToStack(vm, (BlarbVM_WORD)line[i]);
	}
}

void BlarbVM_setRegisterFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = BlarbVM_peekOnStack(vm, 1);
	BlarbVM_WORD regIndex = BlarbVM_peekOnStack(vm, 0);

	vm->registers[regIndex] = BlarbVM_peekOnStack(vm, stackIndex);

	// Pop args
	BlarbVM_popFromStack(vm);
	BlarbVM_popFromStack(vm);
}

/*
 * Note: I know I know, I should really be using a set.
 * ... but a lookup table is good enough for now.
 * I don't want to worry about premature optimizations :)
 */
int BlarbVM_shouldLoadFileName(BlarbVM *vm, char *fileName, size_t len) {
    // Ignore duplicates
    for (size_t i = 0; i < vm->loadedFileNameCount; i++) {
        if (strncmp(fileName, vm->loadedFileNames[i], len) == 0) {
            return 0;
        }
    }

    // Add the file to the list.

    vm->loadedFileNameCount++;
    vm->loadedFileNames = realloc(vm->loadedFileNames, vm->loadedFileNameCount);

    char **new = &vm->loadedFileNames[vm->loadedFileNameCount - 1];
    *new = malloc(len);
    strncpy(*new, fileName, len);

    return 1;
}

void BlarbVM_includeFileOnStack(BlarbVM *vm) {
	char fileName[512];
	size_t size = 0;

	char c;
	while ((c = (char)BlarbVM_popFromStack(vm))) {
		fileName[size++] = c;

        if (size > sizeof(fileName) - 2) {
            fprintf(stderr, "Make include file name length is %lu.",
                    sizeof(fileName));
            BlarbVM_exit(vm, 1);
            return;
        }
	}
	fileName[size] = '\0';

    if (BlarbVM_shouldLoadFileName(vm, fileName, size + 1)) {
        BlarbVM_loadFile(vm, fileName);
    }
}

size_t BlarbVM_brk(BlarbVM *vm, size_t newEnd) {
    if (newEnd) {
        // Resize the heap
        vm->heapSize = newEnd;
        vm->heap = realloc(vm->heap, vm->heapSize);
    }
    return vm->heapSize;
}

size_t BlarbVM_systemCallFromStack(BlarbVM *vm) {
	const BlarbVM_WORD num = BlarbVM_popFromStack(vm);
	BlarbVM_WORD args[6];
	args[0] = BlarbVM_popFromStack(vm);
	args[1] = BlarbVM_popFromStack(vm);
	args[2] = BlarbVM_popFromStack(vm);
	args[3] = BlarbVM_popFromStack(vm);
	args[4] = BlarbVM_popFromStack(vm);
	args[5] = BlarbVM_popFromStack(vm);

    // Refer to http://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/
    // Many syscalls must map the virtual Blarb addresses to real VM heap address
    switch (num) {
    case 12:
        return BlarbVM_brk(vm, args[0]);
    case 60:
        return BlarbVM_exit(vm, args[0]);
    default:
        return BlarbVM_performSyscall(vm, num, args);
    }
}

void BlarbVM_setHeapValueFromStack(BlarbVM *vm) {
    const BlarbVM_WORD heapAddressIndex = BlarbVM_peekOnStack(vm, 0);
	const BlarbVM_WORD valueIndex = BlarbVM_peekOnStack(vm, 1);

	BlarbVM_WORD heapAddress = (BlarbVM_WORD)vm->heap + BlarbVM_peekOnStack(vm, heapAddressIndex);
	const BlarbVM_WORD value = BlarbVM_peekOnStack(vm, valueIndex);

    const char old_value = *((char *)heapAddress);
	// Set the value in memory
	*((char *)heapAddress) = value;
    BlarbVM_setOnStack(vm, valueIndex, old_value);

	BlarbVM_popFromStack(vm);
	BlarbVM_popFromStack(vm);
}

void BlarbVM_pushRegisterToStack(BlarbVM *vm) {
	BlarbVM_WORD regIndex = BlarbVM_peekOnStack(vm, 0);
	BlarbVM_WORD regValue = vm->registers[regIndex];

	// Pop args
	BlarbVM_popFromStack(vm);

	BlarbVM_pushToStack(vm, regValue);
}

void BlarbVM_nandOnStack(BlarbVM *vm) {
	BlarbVM_WORD firstNandIndex = BlarbVM_peekOnStack(vm, 0);
	BlarbVM_WORD secondNandIndex = BlarbVM_peekOnStack(vm, 1);

	BlarbVM_WORD firstNandValue = BlarbVM_peekOnStack(vm, firstNandIndex);
	BlarbVM_WORD secondNandValue = BlarbVM_peekOnStack(vm, secondNandIndex);

	BlarbVM_WORD result = ~(firstNandValue & secondNandValue);
	BlarbVM_setOnStack(vm, secondNandIndex, result);

	// Pop args
	BlarbVM_popFromStack(vm);
	BlarbVM_popFromStack(vm);
}

// Returns true if the conditional is a success
int BlarbVM_conditionalFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = BlarbVM_peekOnStack(vm, 0);
	BlarbVM_WORD conditionalValue = BlarbVM_peekOnStack(vm, stackIndex);
	BlarbVM_popFromStack(vm);
	return conditionalValue != 0;
}

void BlarbVM_popOnStack(BlarbVM *vm) {
	BlarbVM_WORD popAmount = BlarbVM_popFromStack(vm);

	while (popAmount >= 1) {
		if (vm->stack_top == 0) {
			fprintf(stderr, "Tried popping more stack elements than avalaible\n");
			terminateVM();
		}
		BlarbVM_popFromStack(vm);
		popAmount--;
	}
}

void BlarbVM_executeLine(BlarbVM *vm, token *line) {
    while (line->type != NEWLINE) {
        switch (line->type) {
        case INTEGER:
            BlarbVM_pushToStack(vm, line->val);
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
            vm->nandCount++;
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
                BlarbVM_pushToStack(vm, result);
            }
            break;
        case HEAP_SWAP:
            BlarbVM_setHeapValueFromStack(vm);
            break;
        // Ignore these token types
        case LABEL:
        case CHR:
        case NEWLINE:
          break;
        }
        line++;
    }
}

void BlarbVM_pushStackArg(BlarbVM *vm, const char *s) {
    BlarbVM_pushToStack(vm, 0);
    for (int i = strlen(s) - 1; i >= 0; --i) {
        BlarbVM_pushToStack(vm, (BlarbVM_WORD)s[i]);
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
	unsigned long i;

	fprintf(stderr, "\nBeginning BlarbVM dump:\n");

	fprintf(stderr, "\nRegisters:\n");
	for (i = 0; i < sizeof(vm->registers) / sizeof(BlarbVM_WORD); i++) {
		BlarbVM_WORD value = vm->registers[i];
		fprintf(stderr, "%lu: %lx \n", i, value);
	}
	fprintf(stderr, "\nStack:\n");

	for (i = 0; i < vm->stack_top; i++) {
		BlarbVM_WORD value = vm->stack[i];
		fprintf(stderr, "%lu: %lx\n", i, value);
	}

	fprintf(stderr, "\nHeap (%lu):\n", vm->heapSize);
	for (i = 0; i < vm->heapSize; i++) {
		char byteValue = ((char*)vm->heap)[i];
		fprintf(stderr, "%lx: %08x\n", i, byteValue);
	}

	fprintf(stderr, "\nDump complete.\n\n");
}

void BlarbVM_init(BlarbVM *vm) {
	memset(vm, 0, sizeof(BlarbVM));
	vm->heap = malloc(1); // Give it some random address to start with
    vm->stack_top = 0;
    vm->exitCode = 0;
    vm->running = 1;
    vm->loadedFileNameCount = 0;
}

void BlarbVM_destroy(BlarbVM *vm) {
	while (vm->stack_top) {
		BlarbVM_popFromStack(vm);
	}

	for (BlarbVM_WORD i = 0; i < vm->lineCount; i++) {
		free(vm->lines[i]);
	}
	free(vm->lines);

    LabelPointer *current, *tmp; // tmp is needed in uthash
    HASH_ITER(hh, vm->labelPointers, current, tmp) {
        HASH_DEL(vm->labelPointers, current);
        free(current);
    }
}

