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


/* allows us to store a list of indices that can be reused */
typedef struct _um_freelist {
    struct _um_freelist *next;
    size_t idx;
} um_freelist;


/* we need to store lengths of collections */
struct um {
    size_t pc,          /* program counter ("finger") */
           len,         /* length of parent array */
           next;        /* next slot for platter array */
    um_uint reg[8];     /* UM registers */
    um_array **parr;    /* collection of platter arrays */
    um_freelist *free;  /* free list; holds indices into parr that can be reused */
};


/* create a node for the freelist */
static inline um_freelist *um_freelist_push(um_freelist *p, um_uint n)
{
    um_freelist *l = calloc(sizeof(um_freelist), 1);
    assert(l != NULL);
    l->idx = n;
    if (p)
        l->next = p;
    return l;
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
                for (um_freelist *l; l = um->free; free(l))
                    um->free = l->next;                
                free(um->parr);
                return 0;
                break;

            case 8: /* Allocation. */
                { 
                    um_uint idx;
                    if (um->free) {
                        um_freelist *l = um->free;
                        um->free = l->next;
                        idx = l->idx;
                        free(l);
                    }
                    else {
                        if (++um->next == um->len) {
                            um->len *= 2;
                            um->parr = realloc(um->parr, um->len * sizeof(um_uint*));
                        }
                        idx = um->next;
                    }
                    um->parr[ idx ] = um_mkarray( REGC );
                    REGB = idx;
                }
                break;

            case 9: /* Abandonment. */
                free(um->parr[ REGC ]);
                um->free = um_freelist_push( um->free, REGC );
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
    
    um.len = 128;   /* preallocate some memory */
    um.parr = calloc(sizeof(um_uint*), um.len);

    um.parr[0] = um_read_scroll(argv[1]);
    if (!um_run(&um))
        return 0;
    return EXIT_FAILURE;
}
