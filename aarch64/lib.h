#ifndef LIB_H
#define LIB_H

#include <stddef.h>

typedef void (*FP)(void);

enum arg_type {
    I8, /* int8, word8 */
    I16, /* int16, word16 */
    I32, /* int32, word32 */
    I64, /* int64, word64 */
    PTR, /* pointer */
    F32, /* real32 */
    F64 /* real64 */
};

FP make_closure(FP target, void *userdata, size_t n, const enum arg_type argtypes[n]);

#endif
