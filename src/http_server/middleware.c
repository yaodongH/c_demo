#include "middleware.h"
#include "uv.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "slice.h"
#include "context.h"

slice* global_middleware = NULL;

void __init_middlewares(int argc, ...){
    if (global_middleware == NULL) {
        global_middleware = new_slice_with_cap(middleware, 10);
    }

    va_list args;
    va_start(args, argc);
    for (int i=0; i<argc; i++) {
        middleware m = va_arg(args, middleware);
        slice_append(global_middleware, m);
    }

    va_end(args);
}