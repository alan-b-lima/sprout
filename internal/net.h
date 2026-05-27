#ifndef SPROUT_NET_H
#define SPROUT_NET_H

#include <stdint.h>
#include <stdio.h>

typedef uint32_t Port;
typedef uint32_t IPv4;

typedef struct Addr {
    IPv4 ipv4;
    Port port;
} Addr;

extern Addr invalid;

Addr addr_parse(const char *string);
int  addr_print(Addr addr, FILE *fd);

#endif