ALL: um

um: um.c
	cc $< -o $@

test: um
	./um given/sandmark.umz | diff -u given/sandmark-output.txt - | awk '{ if (NR < 10) print $0 }'