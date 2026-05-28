#include "server.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../slice.h"

#define SPROUT_MIN_CAP 4

struct Server {
    int         fd;
    Addr        addr;
    slice(Conn) conns;
};

struct Conn {
    int  fd;
    Addr addr;
    int  err;
};

Conn *server_append__(Server *server, Conn conn);
Addr  addr_make__(int fd);
Addr  addr_get__(struct sockaddr_in, socklen_t);

Server *server_new(Addr addr, size_t cap) {
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
    server->addr = addr_make__(fd);
    return server;
}

Addr server_addr(Server *server) {
    return server->addr;
}

Conn *server_accept(Server *server) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int fd = accept(server->fd, (struct sockaddr *)&addr, &addrlen);
    if (fd < 0) {
        return NULL;
    }

    Conn c;
    c.fd = fd;
    c.addr = addr_get__(addr, addrlen);

    Conn *conn = server_append__(server, c);
    if (conn == NULL) {
        close(fd);
        return NULL;
    }

    return conn;
}

Conn *server_append__(Server *server, Conn conn) {
    for (size_t i = 0; i < len(Conn, server->conns); i++) {
        Conn *slot = &at(Conn, server->conns, i);
        if (slot->fd == -1) {
            *slot = conn;
            return slot;
        }
    }

    server->conns = append(Conn, server->conns, conn);
    return &at(Conn, server->conns, len(Conn, server->conns) - 1);
}

void server_close(Server *server) {
    for (size_t i = 0; i < len(Conn, server->conns); i++) {
        Conn conn = at(Conn, server->conns, i);
        if (conn.fd != -1) {
            close(conn.fd);
        }
    }

    release(Conn, server->conns);
    close(server->fd);
    free(server);
}

Addr conn_addr(Conn *conn) {
    return conn->addr;
}

size_t conn_write(Conn *conn, const char *buf, size_t len) {
    if (len <= 0) {
        conn->err = 0;
        return 0;
    }

    int n = write(conn->fd, buf, len);
    if (n < 0) {
        conn->err = errno;
        return 0;
    }

    conn->err = 0;
    return n;
}

size_t conn_read(Conn *conn, char *buf, size_t len) {
    if (len <= 0) {
        conn->err = 0;
        return 0;
    }

    int n = read(conn->fd, buf, len);
    if (n == 0) {
        conn->err = EOF;
        return 0;
    }
    if (n < 0) {
        conn->err = errno;
        return 0;
    }

    conn->err = 0;
    return n;
}

int conn_error(Conn *conn) {
    return conn->err;
}

void conn_close(Conn *conn) {
    close(conn->fd);
    conn->fd = -1;
}

Addr addr_make__(int fd) {
    struct sockaddr_in addr_in;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int err = getsockname(fd, (struct sockaddr *)&addr_in, &addrlen);
    if (err < 0) {
        return invalid;
    }

    return addr_get__(addr_in, addrlen);
}

Addr addr_get__(struct sockaddr_in addr_in, socklen_t addrlen) {
    if (addrlen != sizeof(struct sockaddr_in)) {
        return invalid;
    }

    Addr addr;
    addr.ipv4 = (IPv4)htonl(addr_in.sin_addr.s_addr);
    addr.port = (Port)htons(addr_in.sin_port);

    return addr;
}