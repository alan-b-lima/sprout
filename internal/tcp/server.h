#ifndef SPROUT_TCP_SERVER_H
#define SPROUT_TCP_SERVER_H

#include <stdint.h>
#include "../net.h"

typedef struct Server Server;
typedef struct Conn Conn;

Server *server_new(Addr addr, uint64_t cap);
Addr    server_addr(Server *server);
Conn   *server_accept(Server *server);
void    server_close(Server *server);

Addr     conn_addr(Conn *conn);
uint64_t conn_write(Conn *conn, const char *buf, uint64_t len);
uint64_t conn_read(Conn *conn, char *buf, uint64_t len);
void     conn_close(Conn *conn);

#endif