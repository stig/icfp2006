#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* I need a fast, unsigned, 32-bit integer type. 
   C99's got just the thing. */
#include <stdint.h>
typedef uint_fast32_t um_uint;
typedef struct {
    size_t len;
    um_uint content[];
} um_array;

/* we need to store lengths of collections */
struct um {
    size_t pc, len;
    um_uint reg[8];
    um_array **parr;
};

/* operators are found in the high nibble */
#define um_op(n) (n >> 28)

/* extract segment values */
#define SEGC(n) (n & 07)            /* segment C */
#define SEGB(n) ((n >> 3) & 07)     /* segment B */
#define SEGA(n) ((n >> 6) & 07)     /* segment A */

#define REGA um.reg[SEGA(p)]
#define REGB um.reg[SEGB(p)]
#define REGC um.reg[SEGC(p)]

/* op 13 is different from the others */
#define OP13SEG(n) ((n & 0xe000000) >> 25)      /* segment A */
#define OP13VAL(n) (n & 0x1ffffff)              /* value */



/* allocate room for more platter arrays */
void um_ppuirealloc(struct um *p, size_t size)
{
    p->parr = realloc(p->parr, size * sizeof(um_uint));
    assert(p != NULL);
    
    /* init new pointers to NULL */
    memset(p->parr + p->len, 0, size - p->len);

    /* update the count of elements */
    p->len = size;
}

um_array *um_mkarray(size_t size)
{
    um_array *p = calloc(size * sizeof(um_uint) + sizeof(um_array), 1);
    p->len = size;
    return p;
}

void um_out(um_uint c)
{
    assert(c <= 255);
    putchar(c);
}

um_array *um_read_scroll(char *name)
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
    
        /* allocate array of ints big enough */
        arr = um_mkarray(len / 4 + 1);
        
        rewind(fp);
        
        /* read bytestream and write into array of ints */
        for (i = 0; (c = getc(fp)) != EOF; i++) {
            assert(c <= 255);
            assert(c >= 0);
            arr->content[ i / 4 ] <<= 8;
            arr->content[ i / 4 ] += c;
        }

        /* make sure the last int is shifted correctly */
        arr->content[ i / 4] <<= (i % 4) * 8;
//      printf("i: %u, len: %u\n", i, len);
        assert(len == i);
    }
    return arr;
}

#if 0

#define dbg(X) SEG##X(p), r[SEG##X(p)]

void debug( um_uint p, um_uint *r, um_uint finger)
{
    char *ops[] = { 
        "COND", "INDEX", "AMEND", "ADD", "MUL", "DIV", "NAND", "HALT",
        "ALLOC", "FREE", "PRINT", "READ", "LOAD", "SET",
    };

    int i, n;
    um_uint op = um_op(p);
    fprintf(stderr, "%x: %s %n", finger, ops[op], &n);
    if (op == 13)
        fprintf(stderr, "reg: %u, val: %x%n", OP13SEG(p), OP13VAL(p), &i);
    else if (op == 10)
        fprintf(stderr, "'%c' %n", REGC != '\n' ? REGC : '\\', &i);
    else
        fprintf(stderr, "a(%u): %x, b(%u): %x, c(%u): %x%n", dbg(A), dbg(B), dbg(C), &i);

    /* make output line up */
    for (i = i + n; i < 45; i++)
        fputc(' ', stderr);

    /* dump registers */
    for (i = 0; i < 8; i++)
        fprintf(stderr, "%u:%08x, ", i, r[i]);
    fputc('\n', stderr);
}

#endif

int main(int argc, char **argv)
{
    struct um um = { 0 };
    
    um_ppuirealloc(&um, 1);
    um.parr[0] = um_read_scroll(argv[1]);

    for (;;) {
        um_uint p = um.parr[0]->content[um.pc++];
 //     debug(p, r, finger - 1);
        switch (um_op(p)) {
        
            case 0: /* Conditional Move. */
                if (REGC) {
                    REGA = REGB;
                }
                break;

            case 1: /* Array Index. */
                REGA = um.parr[ REGB ]->content[ REGC ];
                break;

            case 2: /* Array Amendment. */
                um.parr[ REGA ]->content[ REGB ] = REGC;
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
                exit(0);
                break;

            case 8: /* Allocation. */
                {
                    um_uint i;
                    for (i = 0; i < um.len; i++)
                        if (!um.parr[i])
                            break;
                    if (i == um.len)
                        um_ppuirealloc(&um, um.len * 2);
                    assert(um.parr[i] == NULL);
                    um.parr[i] = um_mkarray( REGC );
                    REGB = i;
                }
                break;

            case 9: /* Abandonment. */
                assert(um.parr[ REGC ] != NULL);
                free(um.parr[ REGC ]);
                um.parr[ REGC ] = NULL;
                break;

            case 10: /* Output. */
                um_out(REGC);
                break;

            case 11: /* Input. */
                {
                    int c = getchar();
                    if (c == EOF) {
                        REGC = ~0;
                    }
                    else {
                        assert(c >= 0);
                        assert(c <= 255);
                        REGC = c;
                    }
                }
                break;

            case 12: /* Load Program. */
                if (REGB) {
                    free(um.parr[0]);
                    um.parr[0] = um_mkarray( um.parr[ REGB ]->len );
                    memmove(um.parr[0]->content, um.parr[ REGB ]->content, um.parr[ REGB ]->len * sizeof(um_uint));
                }
                um.pc = REGC;
                break;

            case 13: /* Orthography. (SET) */
                um.reg[ OP13SEG(p) ] = OP13VAL(p);
                break;

            default:
                fprintf(stderr, "Illegal operator encounted: '%u'\n", um_op(p));
                exit(EXIT_FAILURE);
                break;
        }
    }

    return 0;
}
