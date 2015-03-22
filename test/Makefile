
CFLAGS = -O2 -Wall -Wextra
CC = gcc

test: test.o
	$(CC) $(CFLAGS) -o $@ $^	

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^ 

clean:
	@rm -f *.o