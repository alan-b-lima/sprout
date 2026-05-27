#include "net.h"

#include <stdlib.h>

Addr invalid = (Addr){ .ipv4 = 0, .port = -1 };

Addr addr_parse(const char *string) {
    Addr addr = { 0 };

    char *cursor = (char *)string;
    for (int i = 0; i < 4; i++) {
        uint64_t octect = strtoull(cursor, &cursor, 10);
        if (octect > UINT8_MAX) {
            return invalid;
        }
        if (i < 3 && *cursor != '.') {
            return invalid;
        }
        if (i == 3 && *cursor != ':') {
            return invalid;
        }

        addr.ipv4 = (addr.ipv4 << 8) | (IPv4)octect;
        cursor++;
    }

    uint64_t port = strtoull(cursor, &cursor, 10);
    if (port > UINT16_MAX) {
        return invalid;
    }
    addr.port = (Port)port;

    if (*cursor != '\0') {
        return invalid;
    }

    return addr;
}

int addr_print(Addr addr, FILE *fd) {
    return fprintf(fd, "%d.%d.%d.%d:%d",
        (addr.ipv4 >> 0x18) & 0xFF,
        (addr.ipv4 >> 0x10) & 0xFF,
        (addr.ipv4 >> 0x08) & 0xFF,
        (addr.ipv4 >> 0x00) & 0xFF,
        addr.port
    );
}
