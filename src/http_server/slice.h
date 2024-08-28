#pragma onece

#include <stdlib.h>
typedef struct slice_s slice;

slice* __new_slice__(size_t ele_size, size_t len, size_t cap);
void __slice_append(slice* s, void* ele, size_t esize);
void slice_get(slice* s, size_t i, void** val);
slice* slice_splice(slice* s, int start, int len);
size_t slice_len(slice* s);

#define new_slice_with_cap(typ, cap) __new_slice__(sizeof(typ), 0, cap)
#define slice_append(s, val) __slice_append(s, val, sizeof(val))
#define new_slice(typ) __new_slice__(sizeof(typ), 0, 4)

