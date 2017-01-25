#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "vm.h"

static BlarbVM *vm;

void abortWithUsage(char *arg0) {
    fprintf(stderr, "Usage: %s [--debug] path/to/code.blarb\n", arg0);
    exit(1);
}

int main(int argc, char **argv) {
    int debug = 0;

    char *fileName = NULL;
    // Possibly load multiple files
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--debug", 7) == 0) {
            debug = 1;
        // Filename was already specified
        } else if (fileName) {
            abortWithUsage(argv[0]);
        } else {
            fileName = argv[i];
        }
    }

	vm = BlarbVM_init();
    if ( ! fileName) {
        abortWithUsage(argv[0]);
    }

    BlarbVM_loadFile(vm, fileName);
    BlarbVM_execute(vm);
    if (debug) {
        BlarbVM_dumpDebug(vm);
    }
	BlarbVM_destroy(vm);

	return 0;
}

void terminateVM() {
	fprintf(stderr, "Terminating VM.\n");
	BlarbVM_dumpDebug(vm);
	BlarbVM_destroy(vm);
	exit(1);
}

