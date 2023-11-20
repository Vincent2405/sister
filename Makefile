all: exec

exec: sister.o connection-fork.o request-http.o i4httools.o cmdline.o
	gcc -std=c11 -pedantic -Wall -Werror -D_XOPEN_SOURCE=700 -o exec sister.o connection-fork.o request-http.o i4httools.o cmdline.o

sister.o: sister.c cmdline.o
	gcc -c sister.c -o sister.o

request-http.o: request-http.c cmdline.o
	gcc -c request-http.c -o request-http.o

connection-fork.o: connection-fork.c request-http.o
	gcc -c connection-fork.c -o connection-fork.o
