#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "../internal/net.h"
#include "../internal/tcp/server.h"

void *sprout_conn_routine(void *);

void server(Addr addr) {
    Server *server = server_new(addr, 16);
    if (server == NULL) {
        printf("failed to start server\n");
        return;
    }

    printf("server listening at ");
    addr_print(addr, stdout);
    printf("\n");

    pthread_t threads[16];

    while (true) {
        Conn *conn = server_accept(server);
        if (conn == NULL) {
            continue;
        }

        Addr addr = conn_addr(conn);

        printf("new connection ");
        addr_print(addr, stdout);
        printf("\n");

        pthread_create(&threads[0], NULL, sprout_conn_routine, conn);
    }

    printf("server shutting down...\n");
    server_close(server);
}

void *sprout_conn_routine(void *env) {
    Conn *conn = env;

    printf("%p", conn);
    conn_close(conn);

    return NULL;
}