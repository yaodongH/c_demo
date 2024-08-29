#include "unity/unity.h"
#include <stdlib.h>
#include <string.h>
#include "slice.h"
#include "uv.h"
#include "pool.h"

void setUp() {

}

void tearDown() {

}

void test_slice_append() {
    slice* s = new_slice(int);
    slice_append(s, 890384758);
    TEST_ASSERT_EQUAL(1, slice_len(s));
    int v; 
    slice_get(s, 0, &v);
    TEST_ASSERT_EQUAL(890384758, v);
}

void test_ptr_slice() {
    char* str = "an";
    printf("size of an %d\n", sizeof(str));
    slice* s = new_slice(char*);
    slice_append(s, "abcdefg");
    slice_append(s, "kdieogjk");
    TEST_ASSERT_EQUAL(2, slice_len(s));
    char* p;
    slice_get(s, 1, &p);
    TEST_ASSERT_EQUAL(0, strcmp("kdieogjk", p));
}

void test_sizeof() {
    char *p = "sdfsdfsfsdfsdfsdf";
    size_t s = sizeof(&p);
    short sh = 123;
    size_t ss = sizeof(&sh);
    TEST_ASSERT_EQUAL(sizeof(char*), s);
    TEST_ASSERT_EQUAL(sizeof(short), ss);
}


void test_bufpool() {
    bufpool*p = bp_init(DEFAULT_BUF_CAP, 100);
    uv_buf_t b = bp_get_buf(p, 128);
    TEST_ASSERT_EQUAL(4096, b.len);
    TEST_ASSERT_TRUE(b.base != NULL);
    bp_release_buf(p, b);
    bp_destroy(p);
}



int main(int argc, char const *argv[])
{
    UNITY_BEGIN();
    // test_slice_append();
    // test_ptr_slice();
    test_bufpool();
    // test_sizeof();
    UNITY_END();
    return 0;
}
