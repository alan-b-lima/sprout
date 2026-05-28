#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "../internal/net.h"
#include "../internal/work.h"
#include "../internal/tcp/server.h"

void sprout_conn_routine(void *);

void server(Addr addr) {
    Server *server = server_new(addr, 16);
    if (server == NULL) {
        printf("failed to start server\n");
        return;
    }

    printf("server listening at ");
    addr_print(addr, stdout);
    printf("\n");

    Group *group = group_new(16);

    while (true) {
        Conn *conn = server_accept(server);
        if (conn == NULL) {
            continue;
        }

        Addr addr = conn_addr(conn);

        printf("new connection ");
        addr_print(addr, stdout);
        printf("\n");

        group_go(group, sprout_conn_routine, conn);
    }

    printf("server shutting down...\n");

    server_close(server);
    group_wait(group);
}

void sprout_conn_routine(void *env) {
    Conn *conn = env;
    char buf[1025];
    int err;

    while (true) {
        size_t n = conn_read(conn, buf, sizeof(buf) - 1);
        if ((err = conn_error(conn)) != 0) {
            printf("server found error %d while reading\n", err);
            goto Exit;
        }
        buf[n] = '\0';

        printf("recv: %s\n", buf);

        conn_write(conn, buf, n);
        if ((err = conn_error(conn)) != 0) {
            printf("server found error %d while writing\n", err);
            goto Exit;
        }
    }

Exit:
    conn_close(conn);
}