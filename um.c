#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include <stdint.h>

typedef uint_fast32_t um_uint;

/* opcodes */
char *ops[] = { 
    "COND", "INDEX", "AMEND", "ADD", "MUL", "DIV", "NAND", "HALT",
    "ALLOC", "FREE", "PRINT", "READ", "LOAD", "SET",
};

/* we need to store lengths of collections */
typedef struct {
    size_t len;
    um_uint *a;
} um_arr;

#define mod(n) (n & 0xffffffff)

/* operators are found in the high nibble */
#define um_op(n) (n >> 28)

/* extract segment values */
#define SEGC(n) (n & 07)            /* segment REGC */
#define SEGB(n) ((n >> 3) & 07)     /* segment REGB */
#define SEGA(n) ((n >> 6) & 07)     /* segment REGA */

#define REGA r[SEGA(p)]
#define REGB r[SEGB(p)]
#define REGC r[SEGC(p)]

/* op 13 is different from the others */
#define OP13SEG(n) ((n & 0xe000000) >> 25)      /* segment REGA */
#define OP13VAL(n) (n & 0x1ffffff)              /* value */



/* REGA bit like realloc, but when _increasing_ the array, 
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

    arr->a = calloc(size, sizeof(um_uint));
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

void um_out(um_uint c)
{
    assert(c <= 255);
    putchar(c);
}

um_arr *um_read_scroll(char *name)
{
    um_arr *arr;
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
//      printf("i: %u, len: %u\n", i, len);
        assert(len == i);
    }
    return arr;
}

#define dbg(a) SEG##a(p), r[SEG##a(p)]

void debug( um_uint p, um_uint *r, um_uint finger)
{
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

int main(int argc, char **argv)
{
    um_uint r[8] = { 0 };
    size_t mlen = 0;
    um_arr **m = um_ppuirealloc(NULL, &mlen, 1);
    um_uint finger = 0;
    
    m[0] = um_read_scroll(argv[1]);

    for (;;) {
        assert(finger < m[0]->len);
        um_uint p = m[0]->a[finger++];
 //     debug(p, r, finger - 1);
        switch (um_op(p)) {
        
            case 0: /* Conditional Move. */
                if (REGC) {
                    REGA = REGB;
                }
                break;

            case 1: /* Array Index. */
                assert(REGB < mlen);
                assert(m[ REGB ] != NULL);
                assert(m[ REGB ]->len > REGC);
                REGA = m[ REGB ]->a[ REGC ];
                break;

            case 2: /* Array Amendment. */
                assert(REGA < mlen);
                assert(m[ REGA ] != NULL);
                assert(m[ REGA ]->len > REGB);
                m[ REGA ]->a[ REGB ] = REGC;
                break;

            case 3: /* Addition. */
                REGA = mod(REGB + REGC);
                break;

            case 4: /* Multiplication. */
                REGA = mod(REGB * REGC);
                break;

            case 5: /* Division. */
                assert(REGC); /* don't devide by zero */
                REGA = (um_uint)REGB / (um_uint)REGC;
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
                    for (i = 0; i < mlen; i++)
                        if (!m[i])
                            break;
                    if (i == mlen) {
                        m = um_ppuirealloc(m, &mlen, mlen * 2);
                    }
                    assert(m[i] == NULL);
                    m[i] = um_uicalloc( REGC );
                    REGB = i;
                }
                break;

            case 9: /* Abandonment. */
                um_free(m[ REGC ]);
                m[ REGC ] = NULL;
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
                    um_uint i;
                    um_free(m[0]);
                    um_arr *a = m[ REGB ];
                    m[0] = um_uicalloc(a->len);

                    for (i = 0; i < a->len; i++) {
                        m[0]->a[i] = a->a[i];
                    }
                }
                finger = REGC;
                break;

            case 13: /* Orthography. (SET) */
                r[ OP13SEG(p) ] = OP13VAL(p);
                break;

            default:
                fprintf(stderr, "Illegal operator encounted: '%u'\n", um_op(p));
                exit(EXIT_FAILURE);
                break;
        }
    }

    return 0;
}
