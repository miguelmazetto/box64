#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include <stdint.h>

typedef struct x64emu_s x64emu_t;

uint64_t RunFunction(box64context_t *context, uintptr_t fnc, int nargs, ...);
// use emu state to run function
uint64_t RunFunctionWithEmu(x64emu_t *emu, int QuitOnLongJump, uintptr_t fnc, int nargs, ...);

#endif //__CALLBACK_H__