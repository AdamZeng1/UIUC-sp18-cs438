CLIENT=http_client
SERVER=http_server

# set up compiler
CC = gcc
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter
CFLAGS_DEBUG   = $(WARNINGS) -g

all: $(CLIENT) $(SERVER)

$(CLIENT): http_client.c
	$(CC) $^ $(CFLAGS_DEBUG) -o $(CLIENT)
$(SERVER): http_server.c
	$(CC) $^ $(CFLAGS_DEBUG) -o $(SERVER)

clean:
	$(RM) $(SERVER) $(CLIENT)
