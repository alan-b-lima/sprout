#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../internal/net.h"
#include "server.c"

const char SERVER[] = "server";
const char CLIENT[] = "client";

const char LOCALHOST[] = "127.0.0.1:3232";

void client(Addr);

int main(int argc, const char **argv) {
    if (argc < 3) {
        printf(
            "usage:\n"
            "    sprout server <ipv4>:<port>\n"
            "    sprout client <ipv4>:<port>\n"
        );

        return 0;
    }

    Addr addr = addr_parse(argv[2]);

    if (strncmp(SERVER, argv[1], sizeof(SERVER) / sizeof(char)) == 0) {
        server(addr);
        return 0;
    }

    if (strncmp(CLIENT, argv[1], sizeof(CLIENT) / sizeof(char)) == 0) {
        client(addr);
        return 0;
    }

    fprintf(stderr, "unknown mode '%s'\n", argv[1]);
    return 1;
}

void client(Addr) {
    printf("client\n");
}