
CC = g++
CFLAGS = -g -O2 -Wall -std=c++11

all: main client

main: main.o socket_t.o socket_server.o socket_poll.o app.o
	$(CC) $(CFLAGS) -o $@ $^

client: client.o socket_t.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	@rm -rf main client *.dSYM *.o