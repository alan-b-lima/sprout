#include "server.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define MIN_CAP 4

struct Server {
    int      fd;
    Conn *conns;
    uint64_t len;
    uint64_t cap;
};

struct Conn {
    int fd;
};

Conn *server_append(Server *server, int fd);
Addr get_addr(int fd);

Server *server_new(Addr addr, uint64_t cap) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return NULL;
    }

    struct sockaddr_in addr_in = {};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(addr.port);
    addr_in.sin_addr.s_addr = htonl(addr.ipv4);

    int err = bind(fd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
    if (err < 0) {
        return NULL;
    }

    err = listen(fd, (int)cap);
    if (err < 0) {
        return NULL;
    }

    Server *server = calloc(1, sizeof(Server));
    if (server == NULL) {
        return NULL;
    }

    server->fd = fd;
    return server;
}

Addr server_addr(Server *server) {
    return get_addr(server->fd);
}

Conn *server_accept(Server *server) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int fd = accept(server->fd, (struct sockaddr *)&addr, &addrlen);
    if (fd < 0) {
        return NULL;
    }

    Conn *conn = server_append(server, fd);
    if (conn == NULL) {
        close(fd);
        return NULL;
    }

    return conn;
}

Conn *server_append(Server *server, int fd) {
    Conn conn;
    conn.fd = fd;

    for (uint64_t i = 0; i < server->len; i++) {
        if (server->conns[i].fd == -1) {
            server->conns[i] = conn;
            return server->conns + i;
        }
    }

    if (server->cap <= server->len) {
        uint64_t cap = 2 * server->cap;
        if (cap < MIN_CAP) {
            cap = MIN_CAP;
        }

        Conn *conns = calloc(cap, sizeof(Conn));
        if (conns == NULL) {
            return NULL;
        }

        if (server->conns != NULL) {
            memcpy(conns, server->conns, server->len);
            free(server->conns);
        }

        server->conns = conns;
        server->cap = cap;
    }

    Conn *spot = server->conns + server->len;
    *spot = conn;
    server->len++;

    return spot;
}

void server_close(Server *server) {
    for (uint64_t i = 0; i < server->len; i++) {
        if (server->conns[i].fd != -1) {
            close(server->conns[i].fd);
        }
    }
    free(server->conns);

    close(server->fd);
    free(server);
}

Addr conn_addr(Conn *conn) {
    return get_addr(conn->fd);
}

uint64_t conn_write(Conn *conn, const char *buf, uint64_t len);
uint64_t conn_read(Conn *conn, char *buf, uint64_t len);

void conn_close(Conn *conn) {
    close(conn->fd);
    conn->fd = -1;
}

Addr get_addr(int fd) {
    struct sockaddr_in addr_in;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int err = getsockname(fd, (struct sockaddr *)&addr_in, &addrlen);
    if (err < 0) {
        return invalid;
    }

    if (addrlen != sizeof(struct sockaddr_in)) {
        return invalid;
    }

    Addr addr;
    addr.ipv4 = (IPv4)htonl(addr_in.sin_addr.s_addr);
    addr.port = (Port)htons(addr_in.sin_port);

    return addr;
}