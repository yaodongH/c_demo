#pragma once
#include <sys/socket.h>
#include "uv.h"

#define INIT_BUFPOOL_LEN 1000
#define INIT_BUFPOOL_CAP 4096

typedef struct server_context_s server_context;
typedef struct client_context_s client_context;

typedef struct server_conf_s
{
    char* ip4addr;
    int port;
    struct sockaddr_in addr;
} server_conf;


// typedef struct write_t
// {
//     uv_write_t w;
//     uv_buf_t b;
// } write_t;

void init_loop(void* arg);
server_conf* new_server_conf(char* ip4addr, int port);