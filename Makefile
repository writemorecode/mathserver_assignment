CC = gcc
CFLAGS = -Wall -Werror -pedantic -ggdb3

clean: clean_results
	rm -f server client kmeanspar matinvpar test *.so *.o

clean_results:
	rm -rf computed_results/*.txt

server: src/server.c libarray.so libstringutils.so
	$(CC) $(CFLAGS) -L . src/server.c -larray -lstringutils -o server -fsanitize=address

client: src/client.c libstringutils.so
	$(CC) $(CFLAGS) -L . src/client.c -lstringutils -o client -fsanitize=address

matinvpar: src/matinvpar.c libmatrix.so
	$(CC) -ggdb3 $^ -o matinvpar -pthread

kmeanspar: src/kmeanspar.c
	$(CC) -ggdb3 $^ -o kmeanspar -pthread -fsanitize=thread

%.o: src/%.c
	$(CC) $(CFLAGS) $^ -c -fPIC -o $@

libarray.so: string_array.o pfd_array.o
	$(CC) $(CFLAGS) -shared $^ -o $@

libstringutils.so: string_utils.o
	$(CC) $(CFLAGS) -shared $^ -o $@

libmatrix.so: matrix.o
	$(CC) $(CFLAGS) -shared $^ -o $@

testing: src/testing.c
	$(CC) $(CFLAGS) -fsanitize=address $^ -o $@
