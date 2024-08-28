#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uv.h"
#include "server.h"
#include "middleware.h"
#include "context.h"

void echo_read(context* ctx) {
    uv_buf_t buf = ctx_read_all(ctx);
    if (buf.len == 0) {
        ctx_pause(ctx);
        return;
    }

    ctx_write(ctx, buf.base, buf.len);
    ctx_flush(ctx);
}

//uv_tcp_t -> init -> bind -> listen -> uv_tcp_t (client) init -> accept -> read_start -> write
int main() {
    init_middleware(echo_read);
    server_conf* sc = new_server_conf("0.0.0.0", 9999);
    int cpu_count = uv_available_parallelism();
    printf("cpu count %d\n", cpu_count);
    uv_thread_t *thread_ids;
    thread_ids = (uv_thread_t*)calloc(cpu_count, sizeof(uv_thread_t));
    for (int i=0; i<cpu_count; i++) {
        printf("create %dth thread\n", i+1);
        uv_thread_create(&(thread_ids[i]), init_loop, sc);
    }
    printf("finish to create thread\n ");
    for (int i=0; i<cpu_count; i++) {
        uv_thread_join(&(thread_ids[i]));
    }
    return 0;
}