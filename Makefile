CFLAGS := -O3 -DNDEBUG -pg # -ansi -pedantic

ALL: um

um: um.c
	cc $(CFLAGS) $< -o $@

test: um
	./um given/sandmark.umz | diff -u given/sandmark-output.txt - | awk '{ if (NR < 10) print $0 }'