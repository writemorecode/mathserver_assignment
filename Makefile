CC = gcc
CFLAGS = -Wall -Werror -pedantic -ggdb3 -O2

all: server client matinvpar kmeanspar

clean: clean_results
	rm -f server client kmeanspar matinvpar test *.so *.o

clean_results:
	rm -rf computed_results/*.txt

server: src/server.c pfd_array.o string_array.o string_utils.o
	$(CC) $(CFLAGS) $^ -o server -fsanitize=address

client: src/client.c string_utils.o string_array.o
	$(CC) $(CFLAGS) $^ -o client -fsanitize=address

matinvpar: src/matinvpar.c matrix.o
	$(CC) -ggdb3 $^ -o matinvpar -pthread -fsanitize=thread

kmeanspar: src/kmeanspar.c
	$(CC) -ggdb3 $^ -o kmeanspar -pthread -fsanitize=thread

string_array.o: src/string_array.c
	$(CC) $(CFLAGS) -c $^ -o $@

pfd_array.o: src/pfd_array.c
	$(CC) $(CFLAGS) -c $^ -o $@

string_utils.o: src/string_utils.c
	$(CC) $(CFLAGS) -c $^ -o $@

matrix.o: src/matrix.c
	$(CC) $(CFLAGS) -c $^ -o $@

