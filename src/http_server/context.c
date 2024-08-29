#include "context.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include "slice.h"
#include "middleware.h"
#include "utilityies.h"

extern slice *global_middleware;

typedef enum ctx_status_e
{
    CTX_INIT,
    CTX_RUNNING,
    CTX_ABORT,
    CTX_FLUSHING,
    CTX_FINISHED,
    CTX_YIELD,
} CTX_STATUS;

struct ctx_s
{
    uv_buf_t rbuf;
    m_list *head;
    m_list *cur;
    m_list *tail;
    uv_stream_t *client;
    uv_buf_t wbuf;
    uv_buf_t fbuf;
    CTX_STATUS status;
    size_t last_buf_len;
    unattach_ctx unattach_method;
    error err;
};

context *new_ctx(uv_stream_t *stream, const uv_buf_t *buf, size_t nread, unattach_ctx unattach_method)
{
    context *ctx = (context *)malloc(sizeof(context));
    char *data = malloc(nread);
    memcpy(data, buf->base, nread);
    ctx->rbuf = uv_buf_init(data, nread);
    ctx->head = (m_list *)malloc(sizeof(m_list));
    ctx->tail = ctx->head;
    ctx->cur = ctx->head;
    ctx->err.code = 0;
    ctx->err.msg = NULL;
    ctx->wbuf.base = NULL;
    ctx->wbuf.len = 0;
    ctx->fbuf.len = 0;
    ctx->fbuf.base = NULL;
    ctx->client = stream;
    ctx->status = CTX_INIT;
    ctx->last_buf_len = 0;
    ctx->unattach_method = unattach_method;
    for (int i = 0; i < slice_len(global_middleware); i++)
    {
        middleware m;
        slice_get(global_middleware, i, (void **)&m);
        _ctx_use(ctx, 1, m);
    }
    return ctx;
}

void ctx_release(context *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    if (ctx->rbuf.base != NULL && ctx->rbuf.len > 0)
    {
        free(ctx->rbuf.base);
    }

    if (ctx->wbuf.base != NULL && ctx->wbuf.len > 0)
    {
        free(ctx->wbuf.base);
    }

    if (ctx->fbuf.base != NULL && ctx->fbuf.len > 0)
    {
        free(ctx->fbuf.base);
    }

    if (ctx->head != NULL)
    {
        while (ctx->head != NULL)
        {
            ctx->cur = ctx->head;
            ctx->head = ctx->head->next;
            free(ctx->cur);
        }
    }

    if (ctx->err.msg != NULL)
    {
        free(ctx->err.msg);
    }

    free(ctx);
}

void ctx_append(context *ctx, const uv_buf_t* buf, size_t nread)
{
    assert(buf->len > 0 && nread > 0);
    ctx->rbuf.base = (char *)realloc(ctx->rbuf.base, ctx->rbuf.len + nread);
    memcpy(ctx->rbuf.base + ctx->rbuf.len, buf->base, nread);
    ctx->last_buf_len = ctx->rbuf.len;
    ctx->rbuf.len += nread;
}

uv_buf_t ctx_read_all(context *ctx)
{
    uv_buf_t r = buf_copy(&ctx->rbuf); 
    //release resource
    free(ctx->rbuf.base);
    ctx->rbuf.base = NULL;
    ctx->rbuf.len = 0;
    return r;
}

void _ctx_use(context *ctx, int argc, ...)
{
    assert(ctx != NULL && ctx->status == CTX_INIT);
    va_list args;
    va_start(args, ctx);
    while (argc--)
    {
        middleware m = va_arg(args, middleware);
        if (m == NULL)
        {
            break;
        }

        m_list *node = (m_list *)malloc(sizeof(m_list));
        node->m = m;
        node->next = NULL;
        ctx->tail->next = node;
        ctx->tail = node;
    }
    va_end(args);
}

m_list *ctx_next(context *ctx)
{
    assert(ctx != NULL && ctx->status == CTX_RUNNING);
    ctx->cur = ctx->cur->next;
    if (ctx->cur == NULL)
    {
        return NULL;
    }

    ctx->cur->m(ctx);
    return ctx->cur;
}

void ctx_run(context *ctx)
{
    if (ctx->cur == ctx->head) {
        ctx->cur = ctx->cur->next;
    }
    ctx->status = CTX_RUNNING;
    while (ctx->cur != NULL)
    {
        ctx->cur->m(ctx);
        if (ctx->status == CTX_YIELD)
        {
            break;
        }
        ctx->cur = ctx->cur->next;
    }

    if (ctx->cur == NULL) {
        ctx_done(ctx);
    }
}

void ctx_pause(context *ctx)
{
    ctx->status = CTX_YIELD;
}

void ctx_abort(context *ctx, int err_code, const char *err_msg)
{
    ctx->cur = NULL;
    ctx->err.code = err_code;
    ctx->err.msg = (char *)malloc(strlen(err_msg) + 1);
    strcpy(ctx->err.msg, err_msg);
}

void ctx_write(context *ctx, char *base, int len)
{
    assert(len > 0 && base != NULL);

    uv_buf_t *buf = &(ctx->wbuf);
    if (ctx->status == CTX_FLUSHING)
    {
        buf = &(ctx->fbuf);
    }
    if (buf->base == NULL)
    {
        char *b = (char *)malloc(len);
        *buf = uv_buf_init(b, len);
        memcpy(buf->base, base, len);
        return;
    }

    buf->base = (char *)realloc(buf->base, buf->len + len);
    memcpy(buf->base + buf->len, base, len);
    buf->len += len;
}

struct wr
{
    uv_write_t w;
    context *ctx;
    CTX_STATUS old;
    uv_buf_t buf;
};

void __ctx_write_cb(uv_write_t *req, int status)
{
    struct wr *w = (struct wr *)req;
    if (status)
    {
        w->ctx->err.code = status;
        const char *err_msg = uv_strerror(status);
        w->ctx->err.msg = (char *)malloc(strlen(err_msg) + 1);
        strcpy(w->ctx->err.msg, err_msg);
    }

    if (w->ctx->status == CTX_FLUSHING)
    {
        w->ctx->status = w->old;
    }

    if (w->ctx->fbuf.len > 0)
    {
        ctx_write(w->ctx, w->ctx->fbuf.base, w->ctx->fbuf.len);
        free(w->ctx->fbuf.base);
        w->ctx->fbuf.base = NULL;
        w->ctx->fbuf.len = 0;
    }

    if (w->ctx->status == CTX_FINISHED)
    {
        ctx_done(w->ctx);
    }

    //TODO:失败后是否重新传buf??此处直接释放对应的内存，后续可考虑扩展性的容错hook
    free(w->buf.base);
}

void ctx_flush(context *ctx)
{
    if (ctx->client == NULL || ctx->wbuf.len == 0 || ctx->status == CTX_FLUSHING)
    {
        return;
    }

    struct wr *w = (struct wr *)malloc(sizeof(struct wr));
    w->ctx = ctx;
    w->old = ctx->status;
    ctx->status = CTX_FLUSHING;
    w->buf = ctx->wbuf;
    uv_write((uv_write_t *)w, ctx->client, &w->buf, w->buf.len, __ctx_write_cb);
    ctx->wbuf.base = NULL;
    ctx->wbuf.len = 0;
}

void ctx_done(context *ctx)
{
    CTX_STATUS old = ctx->status;
    ctx->status = CTX_FINISHED;
    if (ctx->wbuf.len > 0)
    {
        ctx_flush(ctx);
    }

    if (ctx->unattach_method != NULL) {
        ctx->unattach_method(ctx);
    }

    if (old != CTX_FLUSHING)
    {
        ctx_release(ctx);
    }
}

uv_stream_t* ctx_get_client(context* ctx) {
    return ctx->client;
}
