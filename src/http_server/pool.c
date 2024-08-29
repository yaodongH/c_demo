#include "pool.h"
#include "uv.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "utilityies.h"
//TODO: 限制pool大小，确保不会导致内存爆了
struct freelist;

struct freelist
{
    uv_buf_t buf;
    size_t cap;
    struct freelist *next;
};

struct bufpool_s
{
    /* data */
    uv_mutex_t *m;
    // 需保证freelist是按cap从大到小排列的
    struct freelist *head;
};

bufpool *bp_init(size_t cap, int count)
{
    bufpool *p = (bufpool *)malloc(sizeof(bufpool));
    p->m = (uv_mutex_t *)malloc(sizeof(uv_mutex_t));
    uv_mutex_init_recursive(p->m);
    p->head = (struct freelist *)malloc(sizeof(struct freelist));
    if (count > 0)
    {
        struct freelist *fp = p->head;
        for (int i = 0; i < count; i++)
        {
            struct freelist *node = (struct freelist *)malloc(sizeof(struct freelist));
            node->buf = buf_create(cap); 
            node->cap = cap;
            node->next = NULL;
            fp->next = node;
            fp = node;
        }
    }
    else
    {
        p->head->next = NULL;
    }
    return p;
}

uv_buf_t bp_get_buf(bufpool *pool, size_t size)
{
    uv_mutex_lock(pool->m);
    struct freelist *prev = pool->head;
    struct freelist *cur = pool->head->next;
    uv_buf_t r;
    r.base = NULL;
    if (cur != NULL && cur->cap >= size)
    {
        r = cur->buf;
        prev->next = cur->next;
        free(cur); //TODO: freelist本身也可以做slab
    }

    if (r.base == NULL)
    {
        cur = pool->head->next;
        if (cur != NULL)
        {
            cur->buf.base = realloc(cur->buf.base, size);
            cur->buf.len = size;
            r = cur->buf;
            pool->head->next = cur->next;
            free(cur);
        } else {
            r =  buf_create(size); 
        }
    }
    uv_mutex_unlock(pool->m);
    return r;
}

void bp_release_buf(bufpool *pool, uv_buf_t buf) {
    uv_mutex_lock(pool->m);
    struct freelist* prev = pool->head;
    struct freelist* cur = pool->head->next;
    //确保内存更大的在前面
    while(cur != NULL && cur->cap > buf.len) {
        prev = cur;
        cur = cur->next;
    }

    struct freelist* node = (struct freelist*)malloc(sizeof(struct freelist));
    memset(buf.base, 0, buf.len);
    node->buf = buf;
    node->cap = buf.len;
    prev->next = node;
    node->next = cur;
    uv_mutex_unlock(pool->m);
}

void bp_destroy(bufpool *pool) {
    //TODO:要怎么确保Mutex没人用了？
    free(pool->m);
    struct freelist* prev = pool->head;
    struct freelist* cur = pool->head->next;
    while (cur != NULL) {
        if (cur->buf.len > 0) {
            free(cur->buf.base);
        }
        prev = cur;
        cur = cur->next;
        free(prev);
    }

    free(pool->head);
    free(pool);
}