#pragma once

#include "uv.h"
#include "middleware.h"
#include "utilityies.h"

struct ctx_s;

// typedef struct wr write_t;

typedef void (*middleware)(struct ctx_s* ctx);
typedef void (*unattach_ctx)(struct ctx_s* ctx);

typedef struct m_list_s {
    struct m_list_s* next;
    middleware m;  
} m_list;

typedef struct error_s {
    char* msg;
    int code;
} error;

typedef struct ctx_s context;

context* new_ctx(uv_stream_t* client, const uv_buf_t* buf, size_t nread, unattach_ctx unattach_method);
m_list* ctx_next(context* ctx);
void ctx_abort(context* ctx, int err_code, const char* err_msg);
void ctx_run(context* ctx);
void _ctx_use(context* ctx, int argc, ...);
void ctx_write(context* ctx, char* buf, int len);
void ctx_flush(context* ctx);
void ctx_done(context* ctx);
void ctx_pause(context* ctx);
void ctx_append(context* ctx, const uv_buf_t* buf, size_t nread);
uv_stream_t* ctx_get_client(context* ctx);
uv_buf_t ctx_read_all(context* ctx);

#define ctx_use(ctx, ...) _ctx_use(ctx, NARG(__VA_ARGS__), __VA_ARGS__)

#define ERR_FLUSH -1
#define ERR_FLUSH_MSG "fail to flush data to client"