#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "vm.h"
#include "debugger.h"

// Flags
#define DEBUG 1
#define DEBUGGER 2

static BlarbVM *vm;

void abortWithUsage(char *arg0) {
    fprintf(stderr, "Usage: %s [--debug|--debugger] path/to/code.blarb\n", arg0);
    exit(1);
}

int main(int argc, char **argv) {
    int flags = 0;

    char *fileName = NULL;
    // Possibly load multiple files
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--debugger", 10) == 0) {
            flags |= DEBUGGER;
        } else if (strncmp(argv[i], "--debug", 7) == 0) {
            flags |= DEBUG;
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
    } else if ((flags & DEBUG) && (flags & DEBUGGER)) {
        printf("Invalid options: Can't have both --debug and --debugger\n");
        abortWithUsage(argv[0]);
    }

    BlarbVM_loadFile(vm, fileName);

    if (flags & DEBUGGER) {
        BlarbVM_debugger(vm);
    } else {
        BlarbVM_execute(vm);
        if (flags & DEBUG) {
            BlarbVM_dumpDebug(vm);
        }
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

