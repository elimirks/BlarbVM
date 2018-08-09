#ifdef __APPLE__
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "vm.h"

size_t BlarbVM_performSyscall(BlarbVM *vm, BlarbVM_WORD num, BlarbVM_WORD *args) {
	const BlarbVM_WORD heapAddr = (BlarbVM_WORD)vm->heap;

	switch (num) {
		case 0:
			return read(args[0], (void*)(args[1] + heapAddr), args[2]);
		case 1:
			return write(args[0], (void*)(args[1] + heapAddr), args[2]);
		case 2:
			return open((char *)(args[0] + heapAddr), args[1], args[2]);
		case 3:
			return close(args[0]);
		default:
			fprintf(stderr, "Syscall %lu isn't supported on MacOS... yet.\n", num);
			exit(1);
	}
}
#endif
