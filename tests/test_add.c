#include "add.h"
#include "unity/unity.h"
#include <stdlib.h>
#include <string.h>

void setUp() {

}

void tearDown() {

}

void test_add() {
    TEST_ASSERT_TRUE(add(2, 3) == 5);
}


void test_arr_bytes() {
    char* bytes = (char*) malloc(10*sizeof(int));
    
    int i = 7890;
    char* p = (char*)&i;
    
    memcpy(bytes, p, sizeof(int));

    char* pp = malloc(sizeof(int));

    memcpy(pp, bytes, sizeof(int));

    int n;
    memcpy(&n, pp, sizeof(int));
    TEST_ASSERT_EQUAL(7890, n);
}

int main(int argc, char const *argv[])
{
    UNITY_BEGIN();
    RUN_TEST(test_add);
    RUN_TEST(test_arr_bytes);
    UNITY_END();
    return 0;
}
