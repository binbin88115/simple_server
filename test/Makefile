
CFLAGS = -g -O2 -Wall -Wextra
CC = gcc

all: test_socket test

test_socket: test_client test_server

% : %.o
	$(CC) $(CFLAGS) -o $@ $^	

# test_client: test_client.o
# 	$(CC) $(CFLAGS) -o $@ $^	

# test_server: test_server.o
# 	$(CC) $(CFLAGS) -o $@ $^	

# test: test.o
# 	$(CC) $(CFLAGS) -o $@ $^	

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^ 

clean:
	@rm -rf *.o *.dSYM test_client test_server test