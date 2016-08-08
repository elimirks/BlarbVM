#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "vm.h"

BlarbVM *vm;

int main(int argc, char **argv) {
	vm = BlarbVM_init();

	BlarbVM_addLine(vm, "42 5 2 ~");
	BlarbVM_addLine(vm, "65 5 $ 12 4 1 4 3 !");
	BlarbVM_execute(vm);

	BlarbVM_dumpDebug(vm);
	BlarbVM_destroy(vm);

	return 0;
}

void terminateVM() {
	fprintf(stderr, "Terminating VM.\n");
	BlarbVM_dumpDebug(vm);
	BlarbVM_destroy(vm);
	exit(1);
}

