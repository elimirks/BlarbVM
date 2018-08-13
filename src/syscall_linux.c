#ifdef __linux__
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "vm.h"

// Don't worry, I wrote a script to generate this :)
// Refer to http://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/
int SYSCALL_POINTER_TABLE[329][6] = {
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 1, 1, 1, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 0, 0, 1, 0 },
                                     { 0, 1, 0, 0, 1, 1 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 0, 0, 0, 1, 1, 0 },
                                     { 0, 0, 1, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 1, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 1, 1, 0, 1, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 1, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 1, 1, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 0, 1, 1, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 1, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 1, 1, 0, 0, 0, 0 },
                                     { 1, 0, 0, 1, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 1, 0 },
                                     { 0, 1, 0, 1, 1, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 0, 0, 1, 0, 1, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 1, 1, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 1, 0, 1, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 1, 1, 1, 1 },
                                     { 1, 0, 1, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 1, 1, 1, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 0, 0, 1, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 1, 1, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 1, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 1, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 1, 1, 0, 0 },
                                     { 0, 1, 1, 1, 0, 0 },
                                     { 0, 1, 1, 1, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 1, 1, 1, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 0, 1, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 1, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 1, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 1, 1, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 0, 0, 0, 0, 0 },
                                     { 0, 1, 0, 1, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 },
                                     { 0, 1, 0, 0, 0, 0 }
};

// Translates memory addresses for syscalls (because virtual memory)
void BlarbVM_translateArgs(BlarbVM *vm, BlarbVM_WORD num, BlarbVM_WORD *args) {
    const BlarbVM_WORD heapAddr = (BlarbVM_WORD)vm->heap;
    
    for (int i = 0; i < 6; i++) {
        if (SYSCALL_POINTER_TABLE[num][i]) {
            args[i] += heapAddr;
        }
    }
}

size_t BlarbVM_performSyscall(BlarbVM *vm, BlarbVM_WORD num, BlarbVM_WORD *args) {
	BlarbVM_translateArgs(vm, num, args);

	size_t ret;
	if ((ret = syscall(num, args[0], args[1], args[2], args[3], args[4], args[5])) == (size_t)-1) {
		fprintf(stderr, "Syscall args: %lu, %lu, %lu, %lu, %lu, %lu, %lu\n",
                num, args[0], args[1], args[2], args[3], args[4], args[5]);
		perror("syscall");
	}
	return ret;
}

#endif