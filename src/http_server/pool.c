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


typedef union {
    size_t cap;
    char capBytes[sizeof(size_t)];
} bufcap; 

#define BUF_HEAD_LEN sizeof(size_t)+2

uv_buf_t buf_create_with_head(size_t cap) {
    bufcap bc = {.cap = cap};
    char *base = (char*)malloc(cap+2+sizeof(size_t));
    *base = 0x2b;
    *(base+1)=0x2b;
    memcpy(base+2, bc.capBytes, sizeof(size_t));
    return uv_buf_init(base+BUF_HEAD_LEN, cap);
}

uv_buf_t buf_realloc(uv_buf_t b, size_t cap) {
    assert(cap > 0);
    //TODO:针对由外部分配的buf
    // if (*(base - BUF_HEAD_LEN) != 0x2b && *(base - BUF_HEAD_LEN +1)!=0x2b) { 
    // }
    bufcap bc;
    memcpy(bc.capBytes, b.base - sizeof(size_t), sizeof(size_t));
    if(bc.cap > cap) {
        b.len = cap;
        return b;
    }

    char * base = b.base - BUF_HEAD_LEN;
    base = (char*)realloc(base, cap + BUF_HEAD_LEN);
    bufcap bc1 = {.cap = cap};
    memcpy(base +2, bc1.capBytes, sizeof(size_t));
    return uv_buf_init(base, cap);
}

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
            node->buf = buf_create_with_head(cap); 
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
            cur->buf = buf_realloc(cur->buf, size);
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
    size_t cap = buf.len;
    if (*(buf.base-BUF_HEAD_LEN) == 0x2b && *(buf.base-BUF_HEAD_LEN+1) == 0x2b) {
        bufcap bc = {.cap = 0};
        memcpy(bc.capBytes, buf.base-BUF_HEAD_LEN+2, sizeof(size_t));
        cap = bc.cap;
    }
    buf.len = cap;
    memset(buf.base, 0, buf.len);

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
            free(cur->buf.base-BUF_HEAD_LEN);
        }
        prev = cur;
        cur = cur->next;
        free(prev);
    }

    free(pool->head);
    free(pool);
}