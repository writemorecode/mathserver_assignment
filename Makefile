CC = gcc
CFLAGS = -Wall -Werror -Wextra -pedantic -ggdb3 -O2 -fsanitize=address

all: server client matinvpar kmeanspar 

clean:
	rm -f server client kmeanspar matinvpar test *.so *.o computed_results/*.txt

server: src/server.c pfd_array.o string_array.o string_utils.o net.o command_handler.o fork_handler.o poll_handler.o epoll_handler.o socket.o
	$(CC) $(CFLAGS) $^ -o server

client: src/client.c string_utils.o string_array.o net.o
	$(CC) $(CFLAGS) $^ -o client

matinvpar: src/matinvpar.c matrix.o
	$(CC) $(CFLAGS) $^ -o matinvpar -pthread

kmeanspar: src/kmeanspar.c
	$(CC) $(CFLAGS) $^ -o kmeanspar -pthread

%.o: src/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

