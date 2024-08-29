#pragma once

#include "utilityies.h"
#include "uv.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

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


uv_buf_t buf_create(size_t s) {
    assert(s > 0);
    char* base = (char*)malloc(s);
    return uv_buf_init(base, s); 
}