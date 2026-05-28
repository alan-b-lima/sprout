mkdir -p bin
gcc cmd/main.c internal/net.c internal/work.c internal/slice.c internal/tcp/server.c -Wall -Wextra -Werror -g -o bin/sprout