CFLAGS := -std=c99 -O3 -DNDEBUG -pedantic # -pg

ALL: um

um: um.c
	cc $(CFLAGS) $< -o $@

test: um
	time ./um given/sandmark.umz 
#	./um given/sandmark.umz | diff -u given/sandmark-output.txt - | awk '{ if (NR < 10) print $0 }'
