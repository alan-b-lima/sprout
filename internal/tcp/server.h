#ifndef SPROUT_TCP_SERVER_H
#define SPROUT_TCP_SERVER_H

#include <stddef.h>

#include "../net.h"

typedef struct Server Server;
typedef struct Conn Conn;

Server *server_new(Addr addr, size_t cap);
Addr    server_addr(Server *server);
Conn   *server_accept(Server *server);
void    server_close(Server *server);

Addr   conn_addr(Conn *conn);
size_t conn_write(Conn *conn, const char *buf, size_t len);
size_t conn_read(Conn *conn, char *buf, size_t len);
int    conn_error(Conn *conn);
void   conn_close(Conn *conn);

#endif