#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uv.h"
#include <asm-generic/socket.h>


void on_close(uv_handle_t *h) {
    free(h);
}

void after_write(uv_write_t* req, int status) {
    if (status < 0) {
        fprintf(stderr, "write to client fail %s\n", uv_strerror(status));
    }

    write_t* wr = (write_t*)req;
    free(wr->b.base);
    free(wr);
}


void echo_read(uv_stream_t* client, ssize_t nread, uv_buf_t* buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "read data error: %s\n", uv_strerror(nread));
        }
        uv_close((uv_handle_t*)client, on_close);
        return;
    }
    
    write_t *wr = malloc(sizeof(write_t));
    char* base = malloc(nread);
    wr->b = uv_buf_init(base, nread);
    memcpy(wr->b.base, buf->base, nread);
    uv_write((uv_write_t*)wr, client, &(wr->b), nread, after_write);
}

void alloc_buf(uv_handle_t* handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "create new connection error %s", uv_strerror(status));
        return;
    }

    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(server->loop, client);
    if ((status = uv_accept(server, (uv_stream_t*)client)) < 0) {
        fprintf(stderr, "accept client error %s\n", uv_strerror(status));
        uv_close((uv_handle_t*)client, on_close);
        return;
    }

    uv_read_start((uv_stream_t*)client, alloc_buf, echo_read);
}


//uv_tcp_t -> init -> bind -> listen -> uv_tcp_t (client) init -> accept -> read_start -> write
void init_loop(void* arg) {
    server_conf* c = (server_conf*)arg;
    printf("sc addr, ip %d, port %d\n", c->addr.sin_addr.s_addr, c->addr.sin_port);
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    //初始化tcp_t
    uv_tcp_t *server = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, server);
    //设置sockopt
    int fd;
    uv_fileno((uv_handle_t*)server, &fd);
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    uv_tcp_nodelay(server, 1);
    uv_tcp_bind(server, &(c->addr), NULL);
    // uv_tcp_nodelay(server, 1);
    //listen
    int r = uv_listen((uv_stream_t*)server, 65535, on_connection);
    if (r < 0) {
        fprintf(stderr, "fail to listen to %s:%d, err : %s\n", c->ip4addr, c->port, uv_strerror(r));
        return;
    } else {
        printf("listen result is %d\n", r);
    }

    uv_run(loop, UV_RUN_DEFAULT);
}

server_conf *new_server_conf(char *ip4addr, int port)
{
    printf("create server conf, ip %s, port %d\n", ip4addr, port);
    server_conf* sc = (server_conf*)malloc(sizeof(server_conf));
    sc->ip4addr = malloc(strlen(ip4addr)+1);
    strcpy(sc->ip4addr, ip4addr);
    sc->port = port;
    int r = uv_ip4_addr(sc->ip4addr, sc->port, &(sc->addr));
    if (r < 0) {
        fprintf(stderr, "fail to convert ip addr : %s\n", uv_strerror(r));
    } else {
        printf("addr after convert, port %d, addr %d\n", sc->addr.sin_port, sc->addr.sin_addr.s_addr);
    }
    return sc;
}
