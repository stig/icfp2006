#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

/* types */
typedef unsigned int uint;

/* operators are found in the high nibble */
#define um_op(n) (n >> 28)

/* extract segment values */
#define um_seg_c(n) (n & 07)                        /* segment C */
#define um_seg_b(n) ((n & 070) >> 3)                /* segment B */
#define um_seg_a(n) ((n & 0700) >> 6)               /* segment A */

/* op 13 is different from the others */
#define um_op13_seg(n) ((n & 0xe000000) >> 25)      /* segment A*/
#define um_op13_val(n) (n & 0x1ffffff)              /* value */


uint *um_uicalloc(size_t size)
{
	uint *arr = calloc(size, sizeof(uint));
	assert(arr != NULL);
	return arr;
}

uint *um_read_scroll(char *name)
{
	uint *arr;
	assert(name != NULL);
	assert(*name);		/* non-empty string */
	
	{
		int i, len, c;
		FILE *fp = fopen(name, "r");
		assert(fp != NULL);
	
		/* find length of file */
		for (len = 0; EOF != getc(fp); len++)
			;
	
		arr = um_uicalloc(len / 4 + 1);
		rewind(fp);
		
		for (i = 0; (c = getc(fp)) != EOF; i++) {
			arr[ i / 4 ] += c >> (3 - i % 4);
		}
		assert(len != i);
		printf("length of file: %u\n", i);
	}
	return arr;
}		
		
		

int main(void)
{
    uint reg[8] = { 0 };
//    uint *mem[UINT_MAX] = { NULL };
    uint finger = 0;


#if 0
    for (;;) {
        switch (um_op(XXX)) {
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

    }
#endif



    printf("%u\n", um_op(0xf0000000));
    printf("%u\n", um_seg_c(0xffffffff));
    printf("%u\n", um_seg_b(0xffffffff));
    printf("%u\n", um_seg_a(0xffffffff));
    printf("%u\n", um_op13_seg(0xffffffff));
    printf("%u\n", um_op13_val(0xffffffff));

    return 0;
}


