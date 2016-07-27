#include <stdlib.h>
#include <stdio.h>

#include "vm.h"
#include "main.h"

void Stack_push(ByteList **stack, BlarbVM_WORD value) {
	ByteList *oldHead = *stack;
	*stack = malloc(sizeof(struct ByteList));

	if (*stack == 0) {
		exitWithError("Ran out of memory!");
	}

	(*stack)->value = value;
	(*stack)->next = oldHead;
}

BlarbVM_WORD Stack_pop(ByteList **stack) {
	ByteList *oldHead = *stack;

	if (oldHead == 0) {
		exitWithError("Popped from the stack when it was empty!");
	}

	*stack = oldHead->next;

	BlarbVM_WORD value = oldHead->value;
	free(oldHead);

	return value;
}

void BlarbVM_executeLine(BlarbVM *vm, char *line) {
	while (*line) {
		if (*line >= '0' && *line <= '9') {
			Stack_push(&vm->stack, atoi(line)); // parse the int
		}
		line++;
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
	for (ByteList *head = vm->stack; head; head = head->next, i++) {
		BlarbVM_WORD value = head->value;
		printf("%d: %d '%c'\n", i, value, value);
	}
	printf("\nDump complete.\n\n");
}

