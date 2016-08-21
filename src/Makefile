CC = gcc
CFLAGS = -std=c11 -Wall -O3

OBJ = table.o parser.o

pear:$(OBJ) pear.c
	$(CC) $(CFLAGS) -o pear $(OBJ) pear.c

tag:
	ctags --sort=yes -f .tags *.c *.h

clean:
	rm *.o pear .tags*