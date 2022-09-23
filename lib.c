#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/errno.h>
#include <sys/mman.h> // mmap, mprotect, munmap
#include <unistd.h> // getpagesize
#if defined(__APPLE__)
#include <pthread.h> // pthread_jit_write_protect_np
#include <libkern/OSCacheControl.h> // sys_icache_invalidate
#endif
#include "lib.h"

extern void thunk(void);

static uint32_t *emit_movimm64(uint32_t *inst, uint32_t Xd, uint64_t value)
{
    {
        uint32_t sf = 1; // 64-bit
        uint32_t hw = 0;
        uint32_t imm16 = value & 0xffff;
        *inst++ = 0x52800000 | (sf << 31) | (hw << 21) | (imm16 << 5) | Xd; // MOVZ Xd, #<value & 0xffff>
    }
    if ((value >> 16) & 0xffff) {
        uint32_t sf = 1; // 64-bit
        uint32_t hw = 1; // LSL #16
        uint32_t imm16 = (value >> 16) & 0xffff;
        *inst++ = 0x72800000 | (sf << 31) | (hw << 21) | (imm16 << 5) | Xd; // MOVK Xd, #<(value >> 16) & 0xffff>, LSL #16
    }
    if ((value >> 32) & 0xffff) {
        uint32_t sf = 1; // 64-bit
        uint32_t hw = 2; // LSL #32
        uint32_t imm16 = (value >> 32) & 0xffff;
        *inst++ = 0x72800000 | (sf << 31) | (hw << 21) | (imm16 << 5) | Xd; // MOVK Xd, #<(value >> 32) & 0xffff>, LSL #32
    }
    if ((value >> 48) & 0xffff) {
        uint32_t sf = 1; // 64-bit
        uint32_t hw = 3; // LSL #48
        uint32_t imm16 = (value >> 48) & 0xffff;
        *inst++ = 0x72800000 | (sf << 31) | (hw << 21) | (imm16 << 5) | Xd; // MOVK Xd, #<(value >> 48) & 0xffff>, LSL #48
    }
    return inst;
}

static void *alloc_executable(size_t _len)
{
    size_t len = getpagesize();
#if defined(__APPLE__)
    const int flags = MAP_ANON | MAP_PRIVATE | MAP_JIT;
#else
    const int flags = MAP_ANON | MAP_PRIVATE;
#endif
    void *mem = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC, flags, -1, 0);
    if (mem == MAP_FAILED) {
        int e = errno;
        fprintf(stderr, "mem = %p (failed), errno = %d (%s)\n", mem, e, strerror(e));
        return NULL;
    }
    return mem;
}

struct info {
    size_t stack_space;
    FP target;
    void *userdata;
    size_t n;
    enum arg_type argtypes[];
};

static inline size_t align_up_z(size_t x, int i)
{
    size_t mask = (1 << i) - 1;
    return (x + mask) & ~mask;
}

FP make_closure(FP target, void *userdata, size_t n, const enum arg_type argtypes[n])
{
    void *mem = alloc_executable(sizeof(uint32_t) * 9);
    uint32_t *inst = mem;
    assert(inst != NULL);
    struct info *info = malloc(sizeof(struct info) + sizeof(enum arg_type) * n);
    assert(info != NULL);
    {
        size_t space = 0;
        size_t ngrn = 0; // The Next General-purpose Register Number
        size_t nsrn = 0; // The Next SIMD and Floating-point Register Number
        for (size_t i = 0; i < n; ++i) {
#if defined(__APPLE__)
#define NSAA_ALIGN(i) i
#else
#define NSAA_ALIGN(i) 3
#endif
            switch (argtypes[i]) {
            case I8:
                if (ngrn < 7) {
                    ++ngrn;
                } else {
                    space = align_up_z(space, NSAA_ALIGN(0));
                    space += 1;
                }
                break;
            case I16:
                if (ngrn < 7) {
                    ++ngrn;
                } else {
                    space = align_up_z(space, NSAA_ALIGN(1));
                    space += 2;
                }
                break;
            case I32:
                if (ngrn < 7) {
                    ++ngrn;
                } else {
                    space = align_up_z(space, NSAA_ALIGN(1));
                    space += 4;
                }
                break;
            case I64:
            case PTR:
                if (ngrn < 7) {
                    ++ngrn;
                } else {
                    space = align_up_z(space, 3);
                    space += 8;
                }
            case F32:
                if (nsrn < 8) {
                    ++nsrn;
                } else {
                    space = align_up_z(space, NSAA_ALIGN(2));
                    space += 4;
                }
                break;
            case F64:
                if (nsrn < 8) {
                    ++nsrn;
                } else {
                    space = align_up_z(space, 3);
                    space += 8;
                }
                break;
            }
#undef NSAA_ALIGN
#undef NSAA_SIZE
        }
        info->stack_space = align_up_z(space, 4); // 16 bytes-aligned
    }
    info->target = target;
    info->userdata = userdata;
    info->n = n;
    memcpy(info->argtypes, argtypes, n * sizeof(enum arg_type));
#if defined(__APPLE__)
    pthread_jit_write_protect_np(0); // Make the memory writable
#endif
    /*
    *inst++ = 0xD4200000; // BRK #0
    *inst++ = 0xD4200000 | (1 << 5); // BRK #1
    *inst++ = 0xD4200000 | (2 << 5); // BRK #2
    */
    inst = emit_movimm64(inst, 16, (uint64_t)info); // MOV X16, #<info>
    inst = emit_movimm64(inst, 17, (uint64_t)thunk); // MOV X17, #<thunk>
    *inst++ = 0xD61F0000 | (17 << 5); // BR X17
#if defined(__APPLE__)
    pthread_jit_write_protect_np(1); // Make the memory executable
    sys_icache_invalidate(mem, (char *)inst - (char *)mem);
#else
    __builtin___clear_cache((void *)mem, (void *)inst);
#endif
    return (FP)mem;
}

static inline unsigned char *align_up(unsigned char *ptr, int i)
{
    uintptr_t mask = (1 << i) - 1;
    return (unsigned char *)(((uintptr_t)ptr + mask) & ~mask);
}

FP adjust_args(uint64_t intreg[8], unsigned char *stack_new, unsigned char *stack_old, const struct info *info)
{
    uint64_t newreg[8];
    size_t ngrn = 0; // The Next General-purpose Register Number
    size_t nsrn = 0; // The Next SIMD and Floating-point Register Number
    unsigned char *nsaa_old = stack_old; // The Next Stacked Argument Address (old)
    unsigned char *nsaa_new = stack_new; // The Next Stacked Argument Address (new)
    newreg[0] = (uint64_t)info->userdata;
    for (size_t i = 0; i < info->n; ++i) {
#if defined(__APPLE__)
#define NSAA_ALIGN(i) i
#else
#define NSAA_ALIGN(i) 3
#endif
        switch (info->argtypes[i]) {
        case I8:
            if (ngrn < 7) {
                newreg[ngrn + 1] = intreg[ngrn];
                ++ngrn;
            } else if (ngrn == 7) {
                nsaa_new = align_up(nsaa_new, NSAA_ALIGN(0));
                *nsaa_new = (unsigned char)intreg[ngrn];
                nsaa_new += 1;
                ngrn = 8;
                //asm("brk #0");
            } else {
                nsaa_new = align_up(nsaa_new, NSAA_ALIGN(0));
                nsaa_old = align_up(nsaa_old, NSAA_ALIGN(0));
                *nsaa_new = *nsaa_old;
                nsaa_new += 1;
                nsaa_old += 1;
            }
            break;
        case I16:
            if (ngrn < 7) {
                newreg[ngrn + 1] = intreg[ngrn];
                ++ngrn;
            } else if (ngrn == 7) {
                nsaa_new = align_up(nsaa_new, NSAA_ALIGN(1));
                *(uint16_t *)nsaa_new = (uint16_t)intreg[ngrn];
                nsaa_new += 2;
                ngrn = 8;
                //asm("brk #1");
            } else {
                nsaa_new = align_up(nsaa_new, NSAA_ALIGN(1));
                nsaa_old = align_up(nsaa_old, NSAA_ALIGN(1));
                *(uint16_t *)nsaa_new = *(uint16_t *)nsaa_old;
                nsaa_new += 2;
                nsaa_old += 2;
            }
            break;
        case I32:
            if (ngrn < 7) {
                newreg[ngrn + 1] = intreg[ngrn];
                ++ngrn;
            } else if (ngrn == 7) {
                nsaa_new = align_up(nsaa_new, NSAA_ALIGN(2));
                *(uint32_t *)nsaa_new = (uint32_t)intreg[ngrn];
                nsaa_new += 4;
                ngrn = 8;
                //asm("brk #2");
            } else {
                nsaa_new = align_up(nsaa_new, NSAA_ALIGN(2));
                nsaa_old = align_up(nsaa_old, NSAA_ALIGN(2));
                *(uint32_t *)nsaa_new = *(uint32_t *)nsaa_old;
                nsaa_new += 4;
                nsaa_old += 4;
            }
            break;
        case I64:
        case PTR:
            if (ngrn < 7) {
                newreg[ngrn + 1] = intreg[ngrn];
                ++ngrn;
            } else if (ngrn == 7) {
                nsaa_new = align_up(nsaa_new, 3);
                *(uint64_t *)nsaa_new = (uint64_t)intreg[ngrn];
                nsaa_new += 8;
                ngrn = 8;
                //asm("brk #3");
            } else {
                nsaa_new = align_up(nsaa_new, 3);
                nsaa_old = align_up(nsaa_old, 3);
                *(uint64_t *)nsaa_new = *(uint64_t *)nsaa_old;
                nsaa_new += 8;
                nsaa_old += 8;
            }
            break;
        case F32:
            if (nsrn < 8) {
                ++nsrn;
            } else {
                nsaa_new = align_up(nsaa_new, NSAA_ALIGN(2));
                nsaa_old = align_up(nsaa_old, NSAA_ALIGN(2));
                *(uint32_t *)nsaa_new = *(uint32_t *)nsaa_old;
                nsaa_new += 4;
                nsaa_old += 4;
            }
            break;
        case F64:
            if (nsrn < 8) {
                ++nsrn;
            } else {
                nsaa_new = align_up(nsaa_new, 3);
                nsaa_old = align_up(nsaa_old, 3);
                *(uint64_t *)nsaa_new = *(uint64_t *)nsaa_old;
                nsaa_new += 8;
                nsaa_old += 8;
            }
            break;
        }
#undef NSAA_ALIGN
#undef NSAA_SIZE
    }
    memcpy(intreg, newreg, sizeof(uint64_t) * 8);
    return info->target;
}
