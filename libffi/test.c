#include <stdio.h>
#include <ffi.h> // libffi

void h(int (*fun)(int))
{
    printf("fun(42) = %d\n", fun(42));
}

void g(ffi_cif *cif, void *ret, void *args[], void *context)
{
    int x = *(int *)context;
    int y = *(int *)args[0];
    *(int *)ret = x + y;
}

int main()
{
    ffi_closure *closure;
    void *fun_p;

    closure = ffi_closure_alloc(sizeof(ffi_closure), &fun_p);

    if (closure) {
        ffi_cif cif;
        ffi_type *args[1];
        args[0] = &ffi_type_sint;

        if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_sint, args) == FFI_OK) {
            int x = -5;
            if (ffi_prep_closure_loc(closure, &cif, g, &x, fun_p) == FFI_OK) {
                int (*fun)(int) = (int (*)(int))fun_p;
                printf("fun(77) = %d\n", fun(77));
                h(fun);
            }
        }
    }

    ffi_closure_free(closure);
}
