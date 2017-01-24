#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "vm.h"

BlarbVM *vm;

int main(int argc, char **argv) {
	if (argc == 1) {
		fprintf(stderr, "Usage: %s path/to/code.blarb\n", argv[0]);
		return 1;
	}

    /*
	vm = BlarbVM_init();

	BlarbVM_loadFile(vm, argv[1]);
	BlarbVM_execute(vm);

	BlarbVM_dumpDebug(vm);
	BlarbVM_destroy(vm);
    */

	return 0;
}

void terminateVM() {
	fprintf(stderr, "Terminating VM.\n");
	BlarbVM_dumpDebug(vm);
	BlarbVM_destroy(vm);
	exit(1);
}

