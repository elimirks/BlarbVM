#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "vm.h"

BlarbVM vm;

int main(int argc, char **argv) {
	memset(&vm, 0, sizeof(BlarbVM));

	BlarbVM_executeLine(&vm, "42 5 2 ~ 65 5 $");
	BlarbVM_dumpDebug(&vm);

	return 0;
}

void terminateVM() {
	fprintf(stderr, "Terminating VM.\n");
	BlarbVM_dumpDebug(&vm);
	exit(1);
}

