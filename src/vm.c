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
	BlarbVM_WORD stackIndex = Stack_peek(&vm->stack, 0);
	BlarbVM_WORD regIndex = Stack_peek(&vm->stack, 1);

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
	BlarbVM_WORD bitIndex = Stack_peek(&vm->stack, 2);

	if (bitIndex < 0 || bitIndex >= 32) {
		fprintf(stderr, "Bit index %d out of range [0, 32)\n", bitIndex);
		terminateVM();
	}

	BlarbVM_WORD mask = 1 << bitIndex;
	BlarbVM_WORD firstNandValue = Stack_peek(&vm->stack, firstNandIndex);
	BlarbVM_WORD secondNandValue = Stack_peek(&vm->stack, secondNandIndex);

	BlarbVM_WORD result = ~(firstNandValue & secondNandIndex);

	// Pop args
	Stack_pop(&vm->stack);
	Stack_pop(&vm->stack);
	Stack_pop(&vm->stack);

	Stack_push(&vm->stack, result);
}

void BlarbVM_setBitOnStack(BlarbVM *vm) {
	BlarbVM_WORD destinationIndex = Stack_peek(&vm->stack, 0);
	BlarbVM_WORD bitValue = Stack_peek(&vm->stack, 1);
	BlarbVM_WORD bitIndex = Stack_peek(&vm->stack, 2);

	if (bitValue != 0 && bitValue != 1) {
		fprintf(stderr, "Bit value %d out of range [0, 1]\n", bitValue);
		terminateVM();
	}

	BlarbVM_WORD destinationValue = Stack_peek(&vm->stack, destinationIndex);
	BlarbVM_WORD result = bitValue
		? destinationValue | (1 << bitIndex)
		: destinationValue & ~(1 << bitIndex);
	Stack_set(&vm->stack, destinationIndex, result);

	// Pop args
	Stack_pop(&vm->stack);
	Stack_pop(&vm->stack);
	Stack_pop(&vm->stack);
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

void BlarbVM_addLine(BlarbVM *vm, char *line) {
	if (vm->lineCount % 2 == 0) {
		// Double the size when space runs out - amortized time is O(1)
		vm->lines = vm->lineCount == 0
			? malloc(sizeof(char *) * 1)
			: realloc(vm->lines, sizeof(char *) * vm->lineCount * 2);
	}

	vm->lines[vm->lineCount] = malloc(strlen(line) + 1);
	strcpy(vm->lines[vm->lineCount], line);

	// TODO check for labels

	vm->lineCount++;
}

void BlarbVM_execute(BlarbVM *vm) {
	int lineToExecute = 0;

	// TODO make this access the line pointer register (0)
	while (lineToExecute < vm->lineCount) {
		BlarbVM_executeLine(vm, vm->lines[lineToExecute]);
		lineToExecute++;
	}
}

void BlarbVM_executeLine(BlarbVM *vm, char *line) {
	int i;
	char *it = line;

	// TODO make sure there is whitespace surrounding native ops!

	while (*it) {
		if (*it == ' ' || *it == '\t') {
		} else if (*it >= '0' && *it <= '9') {
			BlarbVM_WORD value = BlarbVM_parseInt(&it);
			Stack_push(&vm->stack, value);
		} else if (*it == '~') {
			BlarbVM_setRegisterFromStack(vm);
		} else if (*it == '$') {
			BlarbVM_pushRegisterToStack(vm);
		} else if (*it == '!') {
			BlarbVM_nandOnStack(vm);
		} else if (*it == '=') {
			BlarbVM_setBitOnStack(vm);
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
	printf("\nBeginning BlarbVM dump:\n");
	printf("\nRegisters:\n");
	int i = 0;
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
}

