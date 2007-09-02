#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* I need a fast, unsigned, 32-bit integer type. C99's got just the thing. */
#include <stdint.h>
typedef uint_fast32_t um_uint;

/* we need to know the length of a platter array to duplicate it */
typedef struct {
    size_t len;
    um_uint content[];
} um_array;

/* we need to store lengths of collections */
struct um {
    size_t pc,          /* program counter ("finger") */
           len,         /* length of parent array */
           next;        /* next slot for platter array */
    um_uint reg[8];     /* UM registers */
    um_array **parr;    /* collection of platter arrays */
};

/* allocates room for more platter arrays */
static void um_ppuirealloc(struct um *p, size_t size)
{
    p->parr = realloc(p->parr, size * sizeof(um_uint));
    assert(p != NULL);
    
    /* init new pointers to NULL */
    memset(p->parr + p->len, 0, size - p->len);

//    fprintf(stderr, "allocating parent array of size: %e\n", (float)size);
    /* update the count of elements */
    p->len = size;
}

/* creates a platter array of the given size */
static inline um_array *um_mkarray(size_t size)
{
    um_array *p = calloc(size * sizeof(um_uint) + sizeof(um_array), 1);
    p->len = size;
    return p;
}

/* read in the program from a file */
static um_array *um_read_scroll(char *name)
{
    um_array *arr;
    assert(name != NULL);
    assert(*name);      /* non-empty string */
    
    {
        int i, len, c;
        FILE *fp = fopen(name, "r");
        assert(fp != NULL);
    
        /* find length of file */
        for (len = 0; EOF != getc(fp); len++)
            ;
    
        /* allocate array 'big enough' */
        arr = um_mkarray(len / 4 + 1);
        
        rewind(fp);
        
        /* read bytestream and write into array at correct location */
        for (i = 0; (c = getc(fp)) != EOF; i++) {
            assert(c <= 255);
            assert(c >= 0);
            arr->content[ i / 4 ] <<= 8;
            arr->content[ i / 4 ] += c;
        }

        /* make sure the last is shifted correctly */
        arr->content[ i / 4] <<= (i % 4) * 8;
        
        assert(len == i);
    }
    return arr;
}

/* operators are found in the high nibble */
#define OP(n) (n >> 28)

/* extract segment values */
#define SEGC(n) (n & 07)            /* segment C */
#define SEGB(n) ((n >> 3) & 07)     /* segment B */
#define SEGA(n) ((n >> 6) & 07)     /* segment A */

#define REGA um->reg[SEGA(p)]
#define REGB um->reg[SEGB(p)]
#define REGC um->reg[SEGC(p)]

/* op 13 is different from the others */
#define OP13SEG(n) ((n & 0xe000000) >> 25)      /* segment A */
#define OP13VAL(n) (n & 0x1ffffff)              /* value */

/* run loop */
static int um_run(struct um *um)
{
    for (;;) {
        um_uint p = um->parr[0]->content[um->pc++];
        switch (OP(p)) {
        
            case 0: /* Conditional Move. */
                if (REGC) {
                    REGA = REGB;
                }
                break;

            case 1: /* Array Index. */
                REGA = um->parr[ REGB ]->content[ REGC ];
                break;

            case 2: /* Array Amendment. */
                um->parr[ REGA ]->content[ REGB ] = REGC;
                break;

            case 3: /* Addition. */
                REGA = REGB + REGC;
                break;

            case 4: /* Multiplication. */
                REGA = REGB * REGC;
                break;

            case 5: /* Division. */
                assert(REGC);
                REGA = REGB / REGC;
                break;

            case 6: /* Not-And. */
                REGA = ~(REGB & REGC);
                break;

            case 7: /* Halt. */
                for (size_t i = 0; i < um->len; i++)
                    if (um->parr[i])
                        free(um->parr[i]);
                free(um->parr);
                return 0;
                break;

            case 8: /* Allocation. */
                if (++um->next == um->len)
                    um_ppuirealloc(um, um->len * 2);
                assert(um->parr[um->next] == NULL);
                um->parr[um->next] = um_mkarray( REGC );
                REGB = um->next;
                break;

            case 9: /* Abandonment. */
                assert(um->parr[ REGC ] != NULL);
                free(um->parr[ REGC ]);
                um->parr[ REGC ] = NULL;
                break;

            case 10: /* Output. */
                assert(REGC <= 255);
                putchar(REGC);
                break;

            case 11: /* Input. */
                {
                    int c = getchar();
                    REGC = c == EOF ? ~0 : c;
                }
                break;

            case 12: /* Load Program. */
                if (REGB) {
                    free(um->parr[0]);
                    um->parr[0] = um_mkarray( um->parr[ REGB ]->len );
                    memmove(um->parr[ 0 ],
                            um->parr[ REGB ],
                            um->parr[ REGB ]->len * sizeof(um_uint) + sizeof(um_array));
                }
                um->pc = REGC;
                break;

            case 13: /* Orthography. (SET) */
                um->reg[ OP13SEG(p) ] = OP13VAL(p);
                break;

            default:
                fprintf(stderr, "Illegal operator encounted: '%u'\n", OP(p));
                return -1;
                break;
        }
    }
}


int main(int argc, char **argv)
{
    struct um um = { 0 };
    
    um_ppuirealloc(&um, 1);
    um.parr[0] = um_read_scroll(argv[1]);
    if (!um_run(&um))
        return 0;
    return EXIT_FAILURE;
}
