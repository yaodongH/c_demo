#pragma once
#include "uv.h"

typedef struct bufpool_s bufpool;

uv_buf_t* get_buf(size_t size);
void release_buf(uv_buf_t* buf);
