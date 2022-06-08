#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>

#include "debug.h"
#include "box64stack.h"
#include "x64emu.h"
#include "x64run.h"
#include "x64emu_private.h"
#include "x64run_private.h"
#include "x87emu_private.h"
#include "x64primop.h"
#include "x64trace.h"
#include "wrapper.h"
#include "box64context.h"
#include "librarian.h"

#include <elf.h>
#include "elfloader.h"
#include "elfs/elfloader_private.h"

typedef int32_t (*iFpppp_t)(void*, void*, void*, void*);

x64emu_t* x64emu_fork(x64emu_t* emu, int forktype)
{
    // execute atforks prepare functions, in reverse order
    for (int i=my_context->atfork_sz-1; i>=0; --i)
        if(my_context->atforks[i].prepare)
            EmuCall(emu, my_context->atforks[i].prepare);
    int type = emu->type;
    int v;
    if(forktype==2) {
        iFpppp_t forkpty = (iFpppp_t)emu->forkpty_info->f;
        v = forkpty(emu->forkpty_info->amaster, emu->forkpty_info->name, emu->forkpty_info->termp, emu->forkpty_info->winp);
        emu->forkpty_info = NULL;
    } else
        v = fork();
    if(type == EMUTYPE_MAIN)
        thread_set_emu(emu);
    if(v==EAGAIN || v==ENOMEM) {
        // error...
    } else if(v!=0) {  
        // execute atforks parent functions
        for (int i=0; i<my_context->atfork_sz; --i)
            if(my_context->atforks[i].parent)
                EmuCall(emu, my_context->atforks[i].parent);

    } else if(v==0) {
        // execute atforks child functions
        for (int i=0; i<my_context->atfork_sz; --i)
            if(my_context->atforks[i].child)
                EmuCall(emu, my_context->atforks[i].child);
    }
    R_EAX = v;
    return emu;
}

extern int errno;
void x64Int3(x64emu_t* emu)
{
    if(Peek(emu, 0)=='S' && Peek(emu, 1)=='C') // Signature for "Out of x86 door"
    {
        R_RIP += 2;
        uintptr_t addr = Fetch64(emu);
        if(addr==0) {
            //printf_log(LOG_INFO, "%p:Exit x86 emu (emu=%p)\n", *(void**)(R_ESP), emu);
            emu->quit=1; // normal quit
        } else {
            RESET_FLAGS(emu);
            wrapper_t w = (wrapper_t)addr;
            addr = Fetch64(emu);
            /* This party can be used to trace only 1 specific lib (but it is quite slow)
            elfheader_t *h = FindElfAddress(my_context, *(uintptr_t*)(R_ESP));
            int have_trace = 0;
            if(h && strstr(ElfName(h), "libMiles")) have_trace = 1;*/
            if(box64_log>=LOG_DEBUG /*|| have_trace*/) {
                pthread_mutex_lock(&emu->context->mutex_trace);
                int tid = GetTID();
                char buff[256] = "\0";
                char buff2[64] = "\0";
                char buff3[64] = "\0";
                char *tmp;
                int post = 0;
                int perr = 0;
                uint64_t *pu64 = NULL;
                const char *s = NULL;
                s = GetNativeName((void*)addr);
                if(addr==(uintptr_t)PltResolver) {
                    snprintf(buff, 256, "%s", " ... ");
                } else if (!strcmp(s, "__open") || !strcmp(s, "open") || !strcmp(s, "open ") || !strcmp(s, "open64")) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\", %d (,%d))", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)", (int)(R_ESI), (int)(R_EDX));
                    perr = 1;
                } else if (!strcmp(s, "shm_open")) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\", %d, %d)", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)", (int)(R_ESI), (int)(R_EDX));
                    perr = 1;
                } else if (!strcmp(s, "fopen") || !strcmp(s, "fopen64")) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\", \"%s\")", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)", (char*)(R_RSI));
                    perr = 2;
                } else if (!strcmp(s, "__openat64") || !strcmp(s, "openat64") || !strcmp(s, "__openat64_2")) {
                    tmp = (char*)(R_RSI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(%d, \"%s\", %d (,%d))", tid, *(void**)(R_RSP), s, (int)R_EDI, (tmp)?tmp:"(nil)", (int)(R_EDX), (int)(R_ECX));
                    perr = 1;
                } else if (strstr(s, "mkdir")==s) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\", %d)", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)", (int)(R_ESI));
                    perr = 1;
                } else if (strstr(s, "opendir")==s) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\")", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)");
                    perr = 2;
                } else if (!strcmp(s, "read")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(%d, %p, %zu)", tid, *(void**)(R_RSP), s, R_EDI, (void*)R_RSI, R_RDX);
                    perr = 1;
                } else if (!strcmp(s, "write")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(%d, %p, %zu)", tid, *(void**)(R_RSP), s, R_EDI, (void*)R_RSI, R_RDX);
                    perr = 1;
                } else if (strstr(s, "access")==s) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\", 0x%x)", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)", R_ESI);
                    perr = 1;
                } else if (!strcmp(s, "lseek64")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(%d, %ld, %d)", tid, *(void**)(R_RSP), s, (int)R_EDI, (int64_t)R_RSI, (int)R_EDX);
                    perr = 1;
                } else if (!strcmp(s, "lseek")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(%d, %ld, %d)", tid, *(void**)(R_RSP), s, (int)R_EDI, (int64_t)R_RSI, (int)R_EDX);
                    perr = 1;
                } else if (strstr(s, "puts")==s) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\")", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)");
                } else if (strstr(s, "strlen")==s) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\")", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)");
                } else if (strstr(s, "strcmp")==s) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\", \"%s\")", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)", (char*)R_RSI);
                } else if (strstr(s, "getenv")==s) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\")", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)");
                } else if (!strcmp(s, "poll")) {
                    struct pollfd* pfd = (struct pollfd*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(%p[%d/%d/%d, ...], %d, %d)", tid, *(void**)(R_RSP), s, pfd, pfd->fd, pfd->events, pfd->revents, R_ESI, R_EDX);
                } else if (strstr(s, "__printf_chk")) {
                    tmp = (char*)(R_RSI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(%d, \"%s\" (,%p))", tid, *(void**)(R_RSP), s, R_EDI, (tmp)?tmp:"(nil)", (void*)(R_RDX));
                } else if (strstr(s, "__snprintf_chk")) {
                    tmp = (char*)(R_R8);
                    pu64 = (uint64_t*)R_RDI;
                    post = 3;
                    snprintf(buff, 255, "%04d|%p: Calling %s(%p, %zu, %d, %zu, \"%s\" (,%p))", tid, *(void**)(R_RSP), s, (void*)R_RDI, R_RSI, R_EDX, R_RCX, (tmp)?tmp:"(nil)", (void*)(R_R9));
                } else if (!strcmp(s, "snprintf")) {
                    tmp = (char*)(R_RDX);
                    pu64 = (uint64_t*)R_RDI;
                    post = 3;
                    snprintf(buff, 255, "%04d|%p: Calling %s(%p, %zu, \"%s\" (,%p))", tid, *(void**)(R_RSP), s, (void*)R_RDI, R_RSI, (tmp)?tmp:"(nil)", (void*)(R_RCX));
                } else if (!strcmp(s, "getcwd")) {
                    post = 2;
                    snprintf(buff, 255, "%04d|%p: Calling %s(%p, %zu)", tid, *(void**)(R_RSP), s, (void*)R_RDI, R_RSI);
                } else if (!strcmp(s, "ftok")) {
                    tmp = (char*)(R_RDI);
                    perr = 1;
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\", %d)", tid, *(void**)(R_RSP), s, tmp?tmp:"nil", R_ESI);
                } else if (!strcmp(s, "glXGetProcAddress") || !strcmp(s, "SDL_GL_GetProcAddress") || !strcmp(s, "glXGetProcAddressARB")) {
                    tmp = (char*)(R_RDI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(\"%s\")", tid, *(void**)(R_RSP), s, (tmp)?tmp:"(nil)");
                } else if (!strcmp(s, "glLabelObjectEXT")) {
                    tmp = (char*)(R_RCX);
                    snprintf(buff, 255, "%04d|%p: Calling %s(0x%x, %d, %d, \"%s\")", tid, *(void**)(R_RSP), s, R_EDI, R_ESI, R_ECX, (tmp)?tmp:"(nil)");
                } else if (!strcmp(s, "glGetStringi")) {
                    post = 2;
                    snprintf(buff, 255, "%04d|%p: Calling %s(0x%x, %d)", tid, *(void**)(R_RSP), s, R_EDI, R_ESI);
                } else if (!strcmp(s, "glFramebufferTexture2D")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(0x%x, 0x%x, 0x%x, %u, %d)", tid, *(void**)(R_RSP), s, R_EDI, R_ESI, R_EDX, R_ECX, R_R8d);
                } else if (!strcmp(s, "glTexSubImage2D")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(0x%x, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p)", tid, *(void**)(R_RSP), s, R_EDI, R_ESI, R_EDX, R_ECX, R_R8d, R_R9d, *(uint32_t*)(R_RSP+8), *(uint32_t*)(R_RSP+16), *(void**)(R_RSP+24));
                } else if (!strcmp(s, "glCompressedTexSubImage2D")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(0x%x, %d, %d, %d, %d, %d, 0x%x, %d, %p)", tid, *(void**)(R_RSP), s, R_EDI, R_ESI, R_EDX, R_ECX, R_R8d, R_R9d, *(uint32_t*)(R_RSP+8), *(uint32_t*)(R_RSP+16), *(void**)(R_RSP+24));
                } else if (!strcmp(s, "glVertexAttribPointer")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(%u, %d, 0x%x, %d, %d, %p)", tid, *(void**)(R_RSP), s, R_EDI, R_ESI, R_EDX, R_ECX, R_R8d, (void*)R_R9);
                } else if (!strcmp(s, "glDrawElements")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(0x%x, %d, 0x%x, %p)", tid, *(void**)(R_RSP), s, R_EDI, R_ESI, R_EDX, (void*)R_RCX);
                } else if (!strcmp(s, "glUniform4fv")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(%d, %d, %p[%g/%g/%g/%g...])", tid, *(void**)(R_RSP), s, R_EDI, R_ESI, (void*)R_RDX, ((float*)(R_RDX))[0], ((float*)(R_RDX))[1], ((float*)(R_RDX))[2], ((float*)(R_RDX))[3]);
                } else if (!strcmp(s, "mmap64") || !strcmp(s, "mmap")) {
                    snprintf(buff, 255, "%04d|%p: Calling %s(%p, %lu, 0x%x, 0x%x, %d, %ld)", tid, *(void**)(R_RSP), s, 
                        (void*)R_RDI, R_RSI, (int)(R_RDX), (int)R_RCX, (int)R_R8, R_R9);
                } else if (!strcmp(s, "sscanf")) {
                    tmp = (char*)(R_RSI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(%p, \"%s\" (,%p))", tid, *(void**)(R_RSP), s, (void*)R_RDI, (tmp)?tmp:"(nil)", (void*)(R_RDX));
                } else if (!strcmp(s, "XCreateWindow")) {
                    tmp = (char*)(R_RSI);
                    snprintf(buff, 255, "%04d|%p: Calling %s(%p, %p, %d, %d, %u, %u, %u, %d, %u, %p, 0x%lx, %p)", tid, *(void**)(R_RSP), s, 
                        (void*)R_RDI, (void*)R_RSI, (int)R_EDX, (int)R_ECX, R_R8d, R_R9d, 
                        (uint32_t)*(uint64_t*)(R_RSP+8), (int)*(uint64_t*)(R_RSP+16), 
                        (uint32_t)*(uint64_t*)(R_RSP+24), (void*)*(uint64_t*)(R_RSP+32), 
                        (unsigned long)*(uint64_t*)(R_RSP+40), (void*)*(uint64_t*)(R_RSP+48));
                } else {
                    snprintf(buff, 255, "%04d|%p: Calling %s(0x%lX, 0x%lX, 0x%lX, ...)", tid, *(void**)(R_RSP), s, R_RDI, R_RSI, R_RDX);
                }
                printf_log(LOG_NONE, "%s =>", buff);
                pthread_mutex_unlock(&emu->context->mutex_trace);
                w(emu, addr);   // some function never come back, so unlock the mutex first!
                pthread_mutex_lock(&emu->context->mutex_trace);
                if(post)
                    switch(post) { // Only ever 2 for now...
                    case 1: snprintf(buff2, 63, " [%llu sec %llu nsec]", pu64?pu64[0]:~0ull, pu64?pu64[1]:~0ull);
                            break;
                    case 2: snprintf(buff2, 63, "(%s)", R_RAX?((char*)R_RAX):"nil");
                            break;
                    case 3: snprintf(buff2, 63, "(%s)", pu64?((char*)pu64):"nil");
                            break;
                    case 4: snprintf(buff2, 63, " (%f)", ST0.d);
                            break;
                    case 5: {
                            uint32_t* p = (uint32_t*)R_RAX; // uint64_t? (case never used)
                            if(p)
                                snprintf(buff2, 63, " size=%ux%u, pitch=%u, pixels=%p", p[2], p[3], p[4], p+5);
                            else
                                snprintf(buff2, 63, "NULL Surface");
                        }
                        break;
                }
                if(perr==1 && ((int)R_EAX)<0)
                    snprintf(buff3, 63, " (errno=%d:\"%s\")", errno, strerror(errno));
                else if(perr==2 && R_EAX==0)
                    snprintf(buff3, 63, " (errno=%d:\"%s\")", errno, strerror(errno));
                printf_log(LOG_NONE, " return 0x%lX%s%s\n", R_RAX, buff2, buff3);
                pthread_mutex_unlock(&emu->context->mutex_trace);
            } else
                w(emu, addr);
        }
        return;
    }
    if(0 && my_context->signals[SIGTRAP])
        raise(SIGTRAP);
    else
        printf_log(LOG_INFO, "%04d|Warning, ignoring unsupported Int 3 call @%p\n", GetTID(), (void*)R_RIP);
    //emu->quit = 1;
}

int GetTID()
{
    return syscall(SYS_gettid);
}
