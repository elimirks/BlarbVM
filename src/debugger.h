#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "vm.h"

/**
 * Start the Blarb debugger
 */
void BlarbVM_debugger(BlarbVM *vm);
/**
 * Dump a VM trace.
 * This will output registers and the stack in a human readable format.
 */
void BlarbVM_dumpDebug(BlarbVM *vm);

#endif
