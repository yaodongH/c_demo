#pragma once
#include "uv.h"

#define DEFAULT_BUF_CAP 4096

typedef struct bufpool_s bufpool;

bufpool* bp_init(size_t cap, int count);
uv_buf_t bp_get_buf(bufpool* pool, size_t size);
void bp_release_buf(bufpool* pool, uv_buf_t buf);
void bp_destroy(bufpool* pool);