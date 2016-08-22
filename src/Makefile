CC = gcc
CFLAGS = -std=c99 -Wall -O3

OBJ = util.o btree.o table.o db.o parser.o

pear:$(OBJ) pear.c
	$(CC) $(CFLAGS) -o pear $(OBJ) pear.c

tag:
	ctags --sort=yes -f .tags *.c *.h

clean:
	rm *.o pear .tags*
