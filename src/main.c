#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "vm.h"

int main(int argc, char **argv) {
	BlarbVM vm;
	memset(&vm, 0, sizeof(BlarbVM));

	BlarbVM_executeLine(&vm, "1 2 3");
	BlarbVM_dumpDebug(&vm);

	printf("%d\n", Stack_pop(&vm.stack));
	printf("%d\n", Stack_pop(&vm.stack));

	printf("Success!\n");
	return 0;
}

void exitWithError(char *msg) {
	fprintf(stderr, "Error: %s\n", msg);
	exit(1);
}

