#include "rsa.h"
#include <stdlib.h>


const uint64 MSB1 = (uint64)1 << 63;
const uint64 MSB15 = (uint64)-1 << 49;

void ASM_ZERO(uint64 *, int);
void ASM_MOV_Y_X(uint64 *, uint64 *, int);
void ASM_ADD_XY_X(uint64 *, uint64 *, int);
void ASM_ADD_XYC_X(uint64 *, uint64 *, int, int);
void ASM_SUB_XYB_X(uint64 *, uint64 *, int, int);
void ASM_MUL1_XY_Z(uint64 *, uint64, uint64 *, int);


int _eq(BigInteger x, BigInteger y)
{
    if (x.size != y.size)
        return 0;

    int i;

    for (i = 0; i != x.size; ++i)
        if (x.uints[i] != y.uints[i])
            return 0;

    return 1;
}

int _eq_0(BigInteger x)
{
    return x.size == 1 && !x.uints[0];
}

int _eq_1(BigInteger x)
{
    return x.size == 1 && x.uints[0] == 1;
}

int _ge(BigInteger x, BigInteger y)
{
    if (x.size != y.size)
        return x.size > y.size;

    int i;

    for (i = x.size - 1; i != -1; --i)
        if (x.uints[i] != y.uints[i])
            return x.uints[i] > y.uints[i];

    return 1;
}

int compare(uint64 *x, uint64 *y, int size)
{
    int i;

    for (i = size - 1; i != -1; --i)
        if (x[i] != y[i])
            return x[i] > y[i] ? 1 : -1;

    return 0;
}

int byteLength(BigInteger x)
{
    char *c = (char *)&x.uints[x.size];
    int i;

    for (i = 8; i; --i)
        if (*--c)
            break;

    return 8 * (x.size - 1) + i;
}

int multiplier(uint64 *x, BigInteger v[], int i1, int i2)
{
    int i = (i1 + i2) / 2, c = compare(x, v[i].uints, v[255].size);

    if (!c)
        return i;
    else if (i1 + 1 == i2)
        return i1;
    else if (c < 0)
        return multiplier(x, v, i1, i);
    else
        return multiplier(x, v, i, i2);
}

uint64 *malloc8(int size)
{
    uint64 *r = malloc(8 * size);

    if (!r)
        exit(0);

    return r;
}

uint64 *realloc8(void *ptr, int size)
{
    uint64 *r = realloc(ptr, 8 * size);

    if (!r)
        free(ptr), exit(0);

    return r;
}

void shiftLeft(BigInteger *x)
{
    int i = x->size++;

    for (x->uints[i] = (x->uints[i - 1] & MSB1) >> 63, --i; i; --i)
        x->uints[i] = x->uints[i] << 1 | (x->uints[i - 1] & MSB1) >> 63;

    x->uints[i] <<= 1;

    if (!x->uints[x->size - 1])
        --x->size;
}

void shiftRight(BigInteger *x)
{
    int size = x->size - 1, i;

    for (i = 0; i != size; ++i)
        x->uints[i] = x->uints[i] >> 1 | (x->uints[i + 1] & 1) << 63;

    x->uints[i] >>= 1;

    if (!x->uints[i] && i)
        --x->size;
}

void shiftLeftBy15(BigInteger *x)
{
    int i = x->size++;

    for (x->uints[i] = (x->uints[i - 1] & MSB15) >> 49, --i; i; --i)
        x->uints[i] = x->uints[i] << 15 | (x->uints[i - 1] & MSB15) >> 49;

    x->uints[i] <<= 15;

    if (!x->uints[x->size - 1])
        --x->size;
}

void shiftRightBy6(BigInteger *x)
{
    int size = x->size - 1, i;

    for (i = 0; i != size; ++i)
        x->uints[i] = x->uints[i] >> 6 | (x->uints[i + 1] & 63) << 58;

    x->uints[i] >>= 6;

    if (!x->uints[i] && i)
        --x->size;
}

void zero(BigInteger *x)
{
    x->size = 1, x->uints[0] = 0;
}

void one(BigInteger *x)
{
    x->size = 1, x->uints[0] = 1;
}

void two(BigInteger *x)
{
    x->size = 1, x->uints[0] = 2;
}

void publicExponent(BigInteger *x)
{
    x->size = 1, x->uints[0] = 65537;
}

void random(BigInteger *x)
{
    x->size = 1, x->uints[0] = rand() << 15 | rand();
}

void copy(BigInteger *x, BigInteger y, int size)
{
    x->size = y.size, ASM_MOV_Y_X(x->uints, y.uints, size);
}

void reduceSize(BigInteger *x)
{
    int i = x->size - 1;

    while (!x->uints[i] && i)
        --i;

    x->size = ++i;
}

void add(BigInteger *x, BigInteger y)
{
    if (x->size == y.size)
        x->uints[x->size] = 0, ASM_ADD_XY_X(x->uints, y.uints, x->size);
    else if (x->size > y.size)
        x->uints[x->size] = 0,
        ASM_ADD_XYC_X(x->uints, y.uints, y.size, x->size - y.size);
    else
        x->uints[y.size] = 0,
        ASM_MOV_Y_X(&x->uints[x->size], &y.uints[x->size], y.size - x->size),
        ASM_ADD_XYC_X(x->uints, y.uints, x->size, y.size - x->size),
        x->size = y.size;

    if (x->uints[x->size])
        ++x->size;
}

void subtract(BigInteger *x, BigInteger y)
{
    if (x->size == y.size)
        ASM_SUB_XY_X(x->uints, y.uints, x->size);
    else
        ASM_SUB_XYB_X(x->uints, y.uints, y.size, x->size - y.size);

    reduceSize(x);
}

void multiplyDigit(BigInteger x, uint64 y, BigInteger *r)
{
    if (!y)
        return zero(r);

    r->size = x.size, ASM_MUL1_XY_Z(x.uints, y, r->uints, r->size);

    if (r->uints[r->size])
        ++r->size;
}

void multiply(BigInteger x, BigInteger y, BigInteger t, BigInteger *r)
{
    if (_eq_0(x) || _eq_0(y))
        return zero(r);

    r->size = x.size + y.size, ASM_ZERO(r->uints, r->size);
    int i;

    for (i = 0; i != y.size; ASM_ADD_XY_X(&r->uints[i++], t.uints, t.size))
        multiplyDigit(x, y.uints[i], &t);

    if (!r->uints[r->size - 1])
        --r->size;
}

void modFast(BigInteger x, BigInteger v[], BigInteger *r)
{
    x.uints[x.size] = 0;
    char *c = (char *)x.uints + byteLength(x) - byteLength(v[1]);
    int i;

    while (_ge(x, v[1]))
        i = multiplier((uint64 *)c, v, 0, 256),
        ASM_SUB_XYB_X((uint64 *)c--, v[i].uints, v[i].size, 1), reduceSize(&x);

    copy(r, x, x.size);
}

void modPow(BigInteger b, BigInteger e, BigInteger m, BigInteger *r)
{
    int atSize = m.size + 1,
        mtSize = (b.size > m.size ? b.size : m.size) + 1,
        mrSize = 2 * (b.size > m.size ? b.size : m.size) + 1, i, j;
    uint64 *heap = malloc8(e.size + 257 * atSize +
                           mtSize + mrSize + 64 * m.size);
    BigInteger newE, at, mt, mr, vMod[256], vModPow[64];
    newE.uints = heap, copy(&newE, e, e.size), j = e.size;
    at.uints = &heap[j], at.size = 1, ASM_ZERO(at.uints, atSize), j += atSize;
    mt.uints = &heap[j], j += mtSize, mr.uints = &heap[j], j += mrSize;
    char c[64 * e.size / 6 + 1];

    for (i = 0; i != 256; j += atSize, add(&at, m))
        vMod[i].uints = &heap[j], copy(&vMod[i++], at, atSize);

    for (vModPow[0].uints = &heap[j], one(&vModPow[0]), i = 1, j += m.size;
         i != 64; j += m.size, modFast(mr, vMod, &vModPow[i++]))
        vModPow[i].uints = &heap[j], multiply(vModPow[i - 1], b, mt, &mr);

    for (i = 0; !_eq_0(newE); shiftRightBy6(&newE))
        c[i++] = newE.uints[0] & 63;

    for (one(r); i;
         multiply(*r, vModPow[c[--i]], mt, &mr), modFast(mr, vMod, r))
        for (j = 6; j; --j)
            multiply(*r, *r, mt, &mr), modFast(mr, vMod, r);

    free(heap);
}

void mod(BigInteger x, BigInteger y, BigInteger t, BigInteger *r)
{
    int i = x.size - 1, j = y.size - 1;
    copy(r, x, x.size);

    if (x.size < y.size)
        return;

    for (t.size = x.size; j != -1; --i, --j)
        t.uints[i] = y.uints[j];

    while (i != -1)
        t.uints[i--] = 0;

    while (_ge(x, t))
        shiftLeft(&t);

    for (shiftRight(&t); _ge(*r, y); shiftRight(&t))
        if (_ge(*r, t))
            subtract(r, t);
}

void divide(BigInteger x, BigInteger y, BigInteger newX,
            BigInteger t, BigInteger p, BigInteger *r)
{
    int i = x.size - 1, j = y.size - 1;
    copy(&newX, x, x.size), zero(r);

    if (x.size < y.size)
        return;

    for (t.size = x.size; j != -1; --i, --j)
        t.uints[i] = y.uints[j];

    for (p.size = i + 2, p.uints[i + 1] = 1; i != -1; --i)
        t.uints[i] = 0, p.uints[i] = 0;

    while (_ge(x, t))
        shiftLeft(&t), shiftLeft(&p);

    for (shiftRight(&t), shiftRight(&p); _ge(newX, y);
         shiftRight(&t), shiftRight(&p))
        if (_ge(newX, t))
            subtract(&newX, t), add(r, p);
}

void modInv(BigInteger x, BigInteger y, BigInteger *r)
{
    int murSize = y.size + 1, tSize = murSize + 1, i;
    uint64 *heap = malloc8(3 * murSize + 2 * tSize), k;
    BigInteger mur, t, p, mor, newMur;
    mur.uints = heap, i = murSize, t.uints = &heap[i], i += tSize;
    p.uints = &heap[i], i += tSize, mor.uints = &heap[i], i += murSize;

    for (newMur.uints = &heap[i], k = 1; !_eq_0(mor); ++k)
        multiplyDigit(y, k, &mur), ++mur.uints[0], mod(mur, x, t, &mor);

    copy(&newMur, mur, mur.size), divide(mur, x, newMur, t, p, r), free(heap);
}

int isProbablePrime(BigInteger n)
{
    uint64 *heap = malloc8(2 * n.size + 1);
    BigInteger b, e, r;
    b.uints = heap, e.uints = &heap[1], r.uints = &heap[n.size + 1];
    random(&b), copy(&e, n, n.size), --e.uints[0], modPow(b, e, n, &r);
    int i = _eq_1(r);
    free(heap);

    return i;
}

void nextProbablePrime(BigInteger *n)
{
    n->uints = realloc8(n->uints, n->size + 2), n->uints[0] |= 1;
    BigInteger t;
    t.uints = malloc8(1), two(&t);

    while (!isProbablePrime(*n))
        add(n, t);

    free(t.uints);
}

void setPrivateExponent(BigInteger p, BigInteger q, BigInteger e,
                        BigInteger *r)
{
    --p.uints[0], --q.uints[0];
    uint64 *heap = malloc8(2 * p.size + q.size + 1);
    BigInteger t, m;
    t.uints = heap, m.uints = &heap[p.size + 1], multiply(p, q, t, &m);
    modInv(e, m, r), ++p.uints[0], ++q.uints[0], free(heap);
}

void lockSize(BigInteger *n, int d)
{
    int size;
    uint64 t;

    if (d % 64)
        size = d / 64 + 1, t = (uint64)1 << d % 64 - 1;
    else
        size = d / 64, t = (uint64)1 << 63;

    if (n->size < size)
        n->size = size, n->uints = realloc8(n->uints, size),
        n->uints[size - 1] = 0;

    n->uints[size - 1] |= t;
}

BigInteger randomBigInteger(int d)
{
    int i = d / 15 + 1, size = 15 * (i + 1) / 64 + 2;
    uint64 *heap = malloc8(2 * size);
    BigInteger t, p, r;
    t.uints = heap, p.uints = &heap[size], r.uints = malloc8(size);
    time_t s;
    time(&s), srand(s);

    for (one(&p), zero(&r); i; --i)
        multiplyDigit(p, rand(), &t), add(&r, t), shiftLeftBy15(&p);

    if (d % 64)
        r.size = d / 64 + 1, r.uints[r.size - 1] &= (1 << d % 64) - 1;
    else
        r.size = d / 64;

    reduceSize(&r), free(heap);

    return r;
}