#include "slice.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

typedef struct slice_s {
    char* v;
    size_t cap;
    size_t len;
    size_t esize;
} slice;

//TODO:考虑下所有权管理问题
slice* __new_slice__(size_t ele_size, size_t len, size_t cap) {
    assert(len <= cap);
    slice *s = (slice*) malloc(sizeof(slice));
    s->v = malloc(ele_size*cap);
    s->len = len;
    s->cap = cap;
    s->esize = ele_size;
    printf("esize of new slice : %d\n", s->esize);
    return s;
}

void __slice_append(slice* s, void* ele, size_t esize) {
    assert(s != NULL && s->v != NULL && ele != NULL);
    // assert(esize == s->esize);

    if (s->len+1 > s->cap) {
        s->v = (char*)realloc(s->v, s->cap * 2);
        s->cap *= 2;
    }

    char* ptr = (char*)&ele;
    memcpy(s->v + s->len * s->esize, ptr, s->esize);
    s->len += 1;
}


void slice_get(slice* s, size_t i, void** val) {
    assert(i >= 0 && i < s->len);
    assert(s != NULL);
    memcpy(val, s->v + i*s->esize, s->esize);
}

// TODO: 使用集约化的内存管理手段以避免悬垂指针
slice* slice_splice(slice* s, int start, int len) {
    assert(s != NULL);
    assert(start < s->len && start + len <= s->len);
    slice* ns = (slice*) malloc(sizeof(slice));
    ns->v = s->v + len*s->esize;
    ns->cap = s->cap - start;
    ns->len = len;
    ns->esize = s->esize;
    return ns;
}

size_t slice_len(slice* s) {
    assert(s != NULL);
    return s->len;
}