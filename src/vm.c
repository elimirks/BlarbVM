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

BlarbVM_WORD BlarbVM_parseInt(char **line) {
	// FIXME error handling!
	char value[16];
	int i;
	for (i = 0; **line >= '0' && **line <= '9'; i++, (*line)++) {
		value[i] = **line;
	}
	value[i] = '\0';
	printf("Val: %d\n", atoi(value));
	return atoi(value);
}

void BlarbVM_executeLine(BlarbVM *vm, char *line) {
	int i;
	char *it = line;

	while (*it) {
		printf("l: %s\n", it);
		if (*it == ' ' || *it == '\t') {
		} else if (*it >= '0' && *it <= '9') {
			BlarbVM_WORD value = BlarbVM_parseInt(&it);
			Stack_push(&vm->stack, value);
		} else if (*it == '~') {
			// TODO plop into a register
		} else {
			fprintf(stderr, "Invalid syntax '%c', %d: %s\n", *it, i, line);
			exitWithError("Syntax error");
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

