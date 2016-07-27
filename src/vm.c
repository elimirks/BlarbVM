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
}

void BlarbVM_pushRegisterToStack(BlarbVM *vm) {
	BlarbVM_WORD regIndex = Stack_peek(&vm->stack, 0);
	BlarbVM_WORD regValue = vm->registers[regIndex];
	Stack_push(&vm->stack, regValue);
}

void BlarbVM_executeLine(BlarbVM *vm, char *line) {
	int i;
	char *it = line;

	while (*it) {
		if (*it == ' ' || *it == '\t') {
		} else if (*it >= '0' && *it <= '9') {
			BlarbVM_WORD value = BlarbVM_parseInt(&it);
			Stack_push(&vm->stack, value);
		} else if (*it == '~') {
			BlarbVM_setRegisterFromStack(vm);
			// TODO make sure there is whitespace!
		} else if (*it == '$') {
			BlarbVM_pushRegisterToStack(vm);
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

