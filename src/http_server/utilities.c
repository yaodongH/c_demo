#pragma once

#include "utilityies.h"
#include "uv.h"
#include <stdlib.h>
#include <memory.h>

uv_buf_t buf_copy(uv_buf_t* src) {
    uv_buf_t b;
    b.base = NULL;
    b.len = 0;
    if (src->base == NULL || src->len == 0) {
        return b;
    }

    char* ptr = (char*)malloc(src->len);
    memcpy(ptr, src->base, src->len);
    b = uv_buf_init(ptr, src->len);
    return b;
}