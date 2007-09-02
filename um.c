#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

/* types */
typedef unsigned int uint;

/* we need to store lengths of collections */
typedef struct _um_arr {
	size_t len;
	uint *a;
} um_arr;


/* operators are found in the high nibble */
#define um_op(n) (n >> 28)

/* extract segment values */
#define um_seg_c(n) (n & 07)                        /* segment C */
#define um_seg_b(n) ((n & 070) >> 3)                /* segment B */
#define um_seg_a(n) ((n & 0700) >> 6)               /* segment A */

/* op 13 is different from the others */
#define um_op13_seg(n) ((n & 0xe000000) >> 25)      /* segment A*/
#define um_op13_val(n) (n & 0x1ffffff)              /* value */

/* A bit like realloc, but when _increasing_ the array, 
   initialize new pointers to NULL. */
um_arr **um_ppuirealloc(um_arr **p, size_t *old, size_t new)
{
	int i;
	p = realloc(p, new * sizeof(um_arr));
	assert(p != NULL);
	
	/* init new pointers to NULL */
	for (i = 0; (*old + i) < new; i++) {
		p[ *old + i ] = NULL;
	}

	/* update the passed-in count of elements */
	*old = new;
	return p;
}

um_arr *um_uicalloc(size_t size)
{
	um_arr *arr = malloc(sizeof(um_arr));
	assert(arr != NULL);

	
	arr->a = calloc(size, sizeof(uint));
	assert(arr->a != NULL);

	arr->len = size;	
	return arr;
}

um_arr *um_read_scroll(char *name)
{
	um_arr *arr;
	assert(name != NULL);
	assert(*name);		/* non-empty string */
	
	{
		int i, len, c;
		FILE *fp = fopen(name, "r");
		assert(fp != NULL);
	
		/* find length of file */
		for (len = 0; EOF != getc(fp); len++)
			;
	
		/* allocate array of ints big enough */
		arr = um_uicalloc(len / 4 + 1);
		
		rewind(fp);
		
		/* read bytestream and write into array of ints */
		for (i = 0; (c = getc(fp)) != EOF; i++) {
			assert(c <= 255);
			assert(c >= 0);
			arr->a[ i / 4 ] += c >> (3 - i % 4);
		}
		printf("i: %u, len: %u\n", i, len);
		assert(len == i);
	}
	return arr;
}		
		
		
#if !UM_TEST
int main(int argc, char **argv)
{
    uint r[8] = { 0 };
    size_t mlen = 0;
    um_arr **m = um_ppuirealloc(NULL, &mlen, 5);
    uint finger = 0;
	
	m[0] = um_read_scroll(argv[1]);

    for (;;) {
		assert(finger < m[0]->len);
    	uint p = m[0]->a[finger];
    	uint a = um_seg_a(p);
    	uint b = um_seg_b(p);
    	uint c = um_seg_c(p);
    	uint op = um_op(p);
        printf("op: %u, a: %u, b: %u, c: %u\n", op, a, b, c);
        switch (op) {
            case 0: /* Conditional Move. */

                break;

            case 1: /* Array Index. */

                break;

            case 2: /* Array Amendment. */

                break;

            case 3: /* Addition. */

                break;

            case 4: /* Multiplication. */

                break;

            case 5: /* Division. */

                break;

            case 6: /* Not-And. */

                break;

            case 7: /* Halt. */

				puts("Exiting");
                exit(0);
                break;

            case 8: /* Allocation. */

                break;

            case 9: /* Abandonment. */

                break;

            case 10: /* Output. */

                break;

            case 11: /* Input. */

                break;

            case 12: /* Load Program. */

                break;

            case 13: /* Orthography. */

                break;

            default:
            	fprintf(stderr, "illegal operator encounted: '%u'\n", op);
            	exit(EXIT_FAILURE);
            	break;
		}
		finger++;
    }

	return 0;
}

# else

int main(void)
{
	size_t mlen = 0;
	um_arr **m = um_ppuirealloc(NULL, &mlen, 5);
 	assert(m[0] == NULL);
	assert(m[4] == NULL);
	assert(mlen == 5);
	
 	assert(um_op(0xf0000000) == 15);
   	assert(um_seg_c(0xffffffff) == 7);
    assert(um_seg_b(0xffffffff) == 7);
    assert(um_seg_a(0xffffffff) == 7);
    assert(um_op13_seg(0xffffffff) == 7);
    assert(um_op13_val(0xffffffff) == 33554431);
	puts("all ok");
    return 0;
}

#endif
