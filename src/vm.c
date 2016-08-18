#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
void BlarbVM_jumpToLabel(BlarbVM *vm, char *line) {
	char name[256];
	int i;

	for (i = 0; line[i] >= 'a' && line[i] <= 'z'; i++) {
		name[i] = line[i];
	}
	name[i] = '\0';

	for (i = 0; i < vm->labelPointerCount; i++) {
		if (strcmp(vm->labelPointers[i].name, name) == 0) {
			Stack_push(&vm->stack, vm->registers[0]); // return address
			vm->registers[0] = vm->labelPointers[i].line;
			return;
		}
	}
	fprintf(stderr, "Label not found: %s\n", name);
	terminateVM();
}

BlarbVM_WORD BlarbVM_parseInt(char **line) {
	// FIXME error handling!
	char value[16];
	int i;
	for (i = 0; **line >= '0' && **line <= '9'; i++, (*line)++) {
		value[i] = **line;
	}
	value[i] = '\0';
	return atoi(value);
}

void BlarbVM_setRegisterFromStack(BlarbVM *vm) {
	BlarbVM_WORD stackIndex = Stack_peek(&vm->stack, 1);
	BlarbVM_WORD regIndex = Stack_peek(&vm->stack, 0);

	vm->registers[regIndex] = Stack_peek(&vm->stack, stackIndex);

	// Pop args
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
	BlarbVM_WORD conditionalValue = Stack_peek(&vm->stack, 0);
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

void BlarbVM_addLabelPointer(BlarbVM *vm, char *name, int line) {
	if (vm->labelPointerCount % 2 == 0) {
		// Double the size when space runs out - amortized time is O(1)
		vm->labelPointers = vm->labelPointerCount == 0
			? malloc(sizeof(LabelPointer) * 2)
			: realloc(vm->labelPointers, sizeof(LabelPointer) * vm->labelPointerCount * 2);
	}

	LabelPointer *newLabel = &vm->labelPointers[vm->labelPointerCount];
	newLabel->name = malloc(sizeof(char) * strlen(name));
	strcpy(newLabel->name, name);
	newLabel->line = line;

	vm->labelPointerCount++;
}

void BlarbVM_loadFile(BlarbVM *vm, char *fileName) {
	FILE *fp = fopen(fileName, "r");
	if ( ! fp) {
		perror("fopen");
		terminateVM();
	}

	char line[256];
	while (fgets(line, sizeof(line), fp)) {
		BlarbVM_addLine(vm, line);
	}

	fclose(fp);
}

void BlarbVM_addLine(BlarbVM *vm, char *line) {
	if (vm->lineCount % 2 == 0) {
		// Double the size when space runs out - amortized time is O(1)
		vm->lines = vm->lineCount == 0
			? malloc(sizeof(char *) * 1)
			: realloc(vm->lines, sizeof(char *) * vm->lineCount * 2);
	}

	vm->lines[vm->lineCount] = malloc(strlen(line) + 1);
	strcpy(vm->lines[vm->lineCount], line);

	// Add label on lines beginning with #
	// TODO: ignore spaces and include capital alphabets
	// TODO: error handling. What if the label already exists?
	if (strlen(line) > 0 && line[0] == '#') {
		char name[256];

		int i;
		for (i = 1; line[i] >= 'a' && line[i] <= 'z'; i++) {
			name[i - 1] = line[i];
		}
		name[i - 1] = '\0';

		BlarbVM_addLabelPointer(vm, name, vm->lineCount);
	}

	vm->lineCount++;
}

void BlarbVM_execute(BlarbVM *vm) {
	BlarbVM_WORD *lineToExecute = &(vm->registers[0]); // line pointer
	while (*lineToExecute < vm->lineCount && *lineToExecute >= 0) {
		BlarbVM_executeLine(vm, vm->lines[*lineToExecute]);
		(*lineToExecute)++;
	}
}

void BlarbVM_executeLine(BlarbVM *vm, char *line) {
	// Don't run labels!
	if (line && strlen(line) > 0 && line[0] == '#') {
		return;
	}

	int i;
	char *it = line;

	// TODO make sure there is whitespace surrounding native ops!

	while (*it) {
		if (*it <= ' ' || *it == '\t') {
		} else if (*it >= 'a' && *it <= 'z') {
			BlarbVM_jumpToLabel(vm, it);
			return;
		} else if (*it == ';') {
			return; // Blarb comment
		} else if (*it >= '0' && *it <= '9') {
			BlarbVM_WORD value = BlarbVM_parseInt(&it);
			Stack_push(&vm->stack, value);
		} else if (*it == '~') {
			BlarbVM_setRegisterFromStack(vm);
		} else if (*it == '$') {
			BlarbVM_pushRegisterToStack(vm);
		} else if (*it == '!') {
			BlarbVM_nandOnStack(vm);
		} else if (*it == '?') {
			if ( ! BlarbVM_conditionalFromStack(vm)) {
				return;
			}
		} else if (*it == '^') {
			BlarbVM_popOnStack(vm);
		} else {
			fprintf(stderr, "Invalid syntax '%c', %d: %s\n", *it, i, line);
			terminateVM();
		}

		if (*it) {
			it++;
			i++;
		}
	}
}

void BlarbVM_dumpDebug(BlarbVM *vm) {
	int i;

	printf("\nBeginning BlarbVM dump:\n");

	printf("\nDefined labels:\n");
	for (i = 0; i < vm->labelPointerCount; i++) {
		LabelPointer *label = &vm->labelPointers[i];
		printf("%d: %s\n", label->line, label->name);
	}

	printf("\nRegisters:\n");
	for (i = 0; i < sizeof(vm->registers); i++) {
		BlarbVM_WORD value = vm->registers[i];
		printf("%d: %d '%c'\n", i, value, value);
	}
	printf("\nStack:\n");

	i = 0;
	for (ByteList *head = vm->stack; head; head = head->next, i++) {
		BlarbVM_WORD value = head->value;
		printf("%d: %d '%c'\n", i, value, value);
	}
	printf("\nDump complete.\n\n");
}

BlarbVM * BlarbVM_init() {
	BlarbVM *vm = malloc(sizeof(BlarbVM));
	memset(vm, 0, sizeof(BlarbVM));
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

	for (int i = 0; i < vm->labelPointerCount; i++) {
		free(vm->labelPointers[i].name);
	}
	free(vm->labelPointers);
}

