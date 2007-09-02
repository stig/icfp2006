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

#define nand(a, b) (~(a) | ~(b))
#define mod(n) ((n) % 0xffffffff)

/* operators are found in the high nibble */
#define um_op(n) (n >> 28)

/* extract segment values */
#define um_seg_c(n) (n & 07)                        /* segment C */
#define um_seg_b(n) ((n & 070) >> 3)                /* segment B */
#define um_seg_a(n) ((n & 0700) >> 6)               /* segment A */

/* op 13 is different from the others */
#define um_op13_seg(n) ((n & 0xe000000) >> 25)      /* segment A */
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

void um_free(um_arr *p)
{
	assert(p != NULL);
	assert(p->a != NULL);
	free(p->a);
	free(p);
}

void um_out(uint c)
{
	assert(c <= 255);
	putchar(c);
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
			arr->a[ i / 4 ] <<= 8;
			arr->a[ i / 4 ] += c;
		}
		/* make sure the last int is shifted correctly */
		arr->a[ i / 4] <<= (i % 4) * 8;
//		printf("i: %u, len: %u\n", i, len);
		assert(len == i);
	}
	return arr;
}
		
#if !UM_TEST
int main(int argc, char **argv)
{
    uint r[8] = { 0 };
    size_t mlen = 0;
    um_arr **m = um_ppuirealloc(NULL, &mlen, 1);
    uint finger = 0;
	
	m[0] = um_read_scroll(argv[1]);

    for (;;) {
		assert(finger < m[0]->len);
		
    	uint p = m[0]->a[finger++];
    	uint a = um_seg_a(p);
    	uint b = um_seg_b(p);
    	uint c = um_seg_c(p);
    	uint op = um_op(p);
        switch (op) {
            case 0: /* Conditional Move. */

				if (r[c]) {
					r[a] = r[b];
				}
                break;

            case 1: /* Array Index. */

				assert(r[b] < mlen);
				assert(m[ r[b] ] != NULL);
				assert(m[ r[b] ]->len > r[c]);
				
				r[a] = m[ r[b] ]->a[ r[c] ];
                break;

            case 2: /* Array Amendment. */

				assert(r[a] < mlen);
				assert(m[ r[a] ] != NULL);
				assert(m[ r[a] ]->len > r[b]);
				
				m[ r[a] ]->a[ r[b] ] = r[c];
                break;

            case 3: /* Addition. */
				
				r[a] = mod(r[b] + r[c]);
                break;

            case 4: /* Multiplication. */

				r[a] = mod(r[b] * r[c]);
                break;

            case 5: /* Division. */

				assert(r[c]);	/* don't devide by zero */
				r[a] = (uint)r[b] / (uint)r[c];
                break;

            case 6: /* Not-And. */

				r[a] = nand(r[b], r[c]);
                break;

            case 7: /* Halt. */

				puts("Exiting");
                exit(0);
                break;

            case 8: /* Allocation. */
            
            	{
            		uint i;
            		for (i = 0; i < mlen; i++)
            			if (!m[i])
            				break;

            		if (i == mlen) {
            			m = um_ppuirealloc(m, &mlen, mlen * 2);
            		}
            		
            		assert(m[i] == NULL);
            		m[i] = um_uicalloc( r[c] );
            		r[b] = i;
            	}
                break;

            case 9: /* Abandonment. */

				um_free(m[ r[c] ]);
				m[ r[c] ] = NULL;
                break;

            case 10: /* Output. */

				um_out(r[c]);
                break;

            case 11: /* Input. */

				puts("Input unhandled");
                break;

            case 12: /* Load Program. */
            
            	if (r[b]) {
            		uint i;
	            	um_free(m[0]);
            		um_arr *a = m[ r[b] ];
	            	m[0] = um_uicalloc(a->len);

	            	for (i = 0; i < a->len; i++) {
	            		m[0]->a[i] = a->a[i];
	            	}
	            }
				finger = r[c];

                break;

            case 13: /* Orthography. */

				r[ um_op13_seg(p) ] = um_op13_val(p);
                break;

            default:
            	fprintf(stderr, "illegal operator encounted: '%u'\n", op);
            	exit(EXIT_FAILURE);
            	break;
		}
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
	assert(nand(0xfffffffa, 0xfffffff9) == 7);
	
	puts("all ok");
    return 0;
}

#endif
