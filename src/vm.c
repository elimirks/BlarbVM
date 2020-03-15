#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>

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

void BlarbVM_explicitSetRegisterFromStack(BlarbVM *vm, token *t) {
    BlarbVM_WORD stackIndex = t->vals[0] - 2;
    BlarbVM_WORD regIndex   = t->vals[1];
	vm->registers[regIndex] = BlarbVM_peekOnStack(vm, stackIndex);
}

void BlarbVM_setRegisterFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = BlarbVM_peekOnStack(vm, 1);
	BlarbVM_WORD regIndex = BlarbVM_peekOnStack(vm, 0);

	vm->registers[regIndex] = BlarbVM_peekOnStack(vm, stackIndex);

	// Pop args
	BlarbVM_popFromStack(vm);
	BlarbVM_popFromStack(vm);
}

void BlarbVM_runImportsInRegion(BlarbVM *vm,
                                BlarbVM_WORD start,
                                BlarbVM_WORD end) {
    for (BlarbVM_WORD i = start; i < end; i++) {
        for (token *t = vm->lines[i]; ; t++) {
            if (t->type == INCLUDE) {
                BlarbVM_executeLine(vm, vm->lines[i]);
                break;
            } else if (t->type == NEWLINE) {
                // Stop executing once there is a line that doesn't include
                // E.g., includes must be at the top, otherwise they're dynamic
                return;
            }
        }
    }
}

void BlarbVM_expandSystemIncludePath(char *fileName) {
    char expandedFileName[PATH_MAX + 1];

    if (fileName[0] == '@') {
        snprintf(expandedFileName, sizeof(expandedFileName), "%s/%s.blarb",
                BLARB_LIBRARY_PATH, &fileName[1]);
    } else {
        snprintf(expandedFileName, sizeof(expandedFileName), "%s.blarb",
                fileName);
    }

    strcpy(fileName, expandedFileName);
}

void BlarbVM_includeFileOnStack(BlarbVM *vm) {
	char fileName[PATH_MAX + 1];
	size_t size = 0;

	char c;
	while ((c = (char)BlarbVM_popFromStack(vm))) {
		fileName[size++] = c;

        if (size > sizeof(fileName) - 2) {
            fprintf(stderr, "Max include file name length is %lu.",
                    sizeof(fileName));
            BlarbVM_exit(vm, 1);
            return;
        }
	}
	fileName[size] = '\0';

    BlarbVM_expandSystemIncludePath(fileName);

    BlarbVM_WORD includeOffset = vm->lineCount;
    BlarbVM_loadFile(vm, fileName);
    BlarbVM_runImportsInRegion(vm, includeOffset, vm->lineCount);
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

void BlarbVM_explicitPushRegisterToStack(BlarbVM *vm, token *t) {
    BlarbVM_WORD regIndex = t->val;
    BlarbVM_WORD regValue = vm->registers[regIndex];
    BlarbVM_pushToStack(vm, regValue);
}

void BlarbVM_pushRegisterToStack(BlarbVM *vm) {
    BlarbVM_WORD regIndex = BlarbVM_peekOnStack(vm, 0);
    BlarbVM_WORD regValue = vm->registers[regIndex];

    // Pop args
    BlarbVM_popFromStack(vm);

    BlarbVM_pushToStack(vm, regValue);
}

void BlarbVM_nandIndicesOnStack(BlarbVM *vm, BlarbVM_WORD first,
                                BlarbVM_WORD second) {
    vm->nandCount++;

    BlarbVM_WORD firstNandValue  = BlarbVM_peekOnStack(vm, first);
    BlarbVM_WORD secondNandValue = BlarbVM_peekOnStack(vm, second);

    BlarbVM_WORD result = ~(firstNandValue & secondNandValue);
    BlarbVM_setOnStack(vm, second, result);
}

void BlarbVM_explicitNandOnStack(BlarbVM *vm, token *t) {
    // Simulate as if the args were on the stack
    BlarbVM_WORD first  = t->vals[1] - 2;
    BlarbVM_WORD second = t->vals[0] - 2;
    BlarbVM_nandIndicesOnStack(vm, first, second);
}

void BlarbVM_nandOnStack(BlarbVM *vm) {
    BlarbVM_WORD first  = BlarbVM_peekOnStack(vm, 0);
    BlarbVM_WORD second = BlarbVM_peekOnStack(vm, 1);

    BlarbVM_nandIndicesOnStack(vm, first, second);

    // Pop args
    BlarbVM_popFromStack(vm);
    BlarbVM_popFromStack(vm);
}

// Returns true if the conditional succeeded
int BlarbVM_explicitConditionalFromStack(BlarbVM *vm, token *line) {
    BlarbVM_WORD stackIndex = line->val;

    // Special case, in case the blarb program is trying to do useless things.
    if (stackIndex == 0) {
        return 0;
    }

    // Simulate the arg as if it were on the stack 
    stackIndex--;

    BlarbVM_WORD conditionalValue = BlarbVM_peekOnStack(vm, stackIndex);
    return conditionalValue != 0;
}
int BlarbVM_conditionalFromStack(BlarbVM *vm) {
    BlarbVM_WORD stackIndex = BlarbVM_peekOnStack(vm, 0);
    BlarbVM_WORD conditionalValue = BlarbVM_peekOnStack(vm, stackIndex);
    BlarbVM_popFromStack(vm);
    return conditionalValue != 0;
}

void BlarbVM_explicitPopOnStack(BlarbVM *vm, BlarbVM_WORD popAmount) {
    while (popAmount >= 1) {
        if (vm->stack_top == 0) {
            fprintf(stderr, "Tried popping more stack elements than avalaible\n");
            terminateVM();
        }
        BlarbVM_popFromStack(vm);
        popAmount--;
    }
}

void BlarbVM_popOnStack(BlarbVM *vm) {
    BlarbVM_WORD popAmount = BlarbVM_popFromStack(vm);
    BlarbVM_explicitPopOnStack(vm, popAmount);
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
        case EXPLICIT_REG_GET:
            BlarbVM_explicitPushRegisterToStack(vm, line);
            break;
        case STACK_POP:
            BlarbVM_popOnStack(vm);
            break;
        case EXPLICIT_STACK_POP:
            BlarbVM_explicitPopOnStack(vm, line->val);
            break;
        case NAND:
            BlarbVM_nandOnStack(vm);
            break;
        case EXPLICIT_NAND:
            BlarbVM_explicitNandOnStack(vm, line);
            break;
        case CONDITION:
            if ( ! BlarbVM_conditionalFromStack(vm)) {
                return;
            }
            break;
        case EXPLICIT_CONDITION:
            if ( ! BlarbVM_explicitConditionalFromStack(vm, line)) {
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

