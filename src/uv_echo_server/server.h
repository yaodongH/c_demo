#pragma once
#include <sys/socket.h>
#include "uv.h"

typedef struct server_conf_s
{
    char* ip4addr;
    int port;
    struct sockaddr_in addr;
} server_conf;


typedef struct write_t
{
    uv_write_t w;
    uv_buf_t b;
} write_t;

void init_loop(void* arg);
server_conf* new_server_conf(char* ip4addr, int port);
