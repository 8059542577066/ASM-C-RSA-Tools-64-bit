#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>


typedef unsigned long long __uint64_t;

typedef struct
{
    int size;
    __uint64_t uints[129];
} BigInteger;

const __uint64_t B63 = (__uint64_t)1 << 63;
const int MAX_LENGTH = 1234;

const BigInteger ZERO()
{
    BigInteger r;
    r.size = 1, r.uints[0] = 0;

    return r;
}

const BigInteger ONE()
{
    BigInteger r;
    r.size = 1, r.uints[0] = 1;

    return r;
}

const BigInteger TWO()
{
    BigInteger r;
    r.size = 1, r.uints[0] = 2;

    return r;
}

const BigInteger TEN()
{
    BigInteger r;
    r.size = 1, r.uints[0] = 10;

    return r;
}

const BigInteger PUBLIC_EXPONENT()
{
    BigInteger r;
    r.size = 1, r.uints[0] = 65537;

    return r;
}


int _eq(const BigInteger *x, const BigInteger *y)
{
    if (x->size != y->size)
        return 0;

    int i;

    for (i = 0; i != x->size; ++i)
        if (x->uints[i] != y->uints[i])
            return 0;

    return 1;
}

int _eq_0(const BigInteger *x)
{
    return x->size == 1 && !x->uints[0];
}

int _eq_1(const BigInteger *x)
{
    return x->size == 1 && x->uints[0] == 1;
}

int _ge(const BigInteger *x, const BigInteger *y)
{
    if (x->size != y->size)
        return x->size > y->size;

    int i;

    for (i = x->size - 1; i != -1; --i)
        if (x->uints[i] != y->uints[i])
            return x->uints[i] > y->uints[i];

    return 1;
}

int compare(const __uint64_t *x, const __uint64_t *y, int size)
{
    int i;

    for (i = size - 1; i != -1; --i)
        if (x[i] != y[i])
            return x[i] > y[i] ? 1 : -1;

    return 0;
}

int byteLength(const BigInteger *x)
{
    char *c = (char *)&x->uints[x->size];
    int i;

    for (--c, i = 7; i != -1; --i)
        if (*c--)
            break;

    return (x->size - 1) * 8 + i + 1;
}

int multiplier(const __uint64_t *x, const BigInteger *v, int i1, int i2)
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

void shiftLeft(BigInteger *x)
{
    int i = x->size++;

    for (x->uints[i] = (x->uints[i - 1] & B63) >> 63, --i; i; --i)
        x->uints[i] = x->uints[i] << 1 | (x->uints[i - 1] & B63) >> 63;

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

void shiftRightBy6(BigInteger *x)
{
    int size = x->size - 1, i;

    for (i = 0; i != size; ++i)
        x->uints[i] = x->uints[i] >> 6 | (x->uints[i + 1] & 63) << 58;

    x->uints[i] >>= 6;

    if (!x->uints[i] && i)
        --x->size;
}

void ASM_ZERO(__uint64_t *, int);
void ASM_MOV_Y_X(__uint64_t *, const __uint64_t *, int);
void ASM_ADD_XY_X(__uint64_t *, const __uint64_t *, int);
void ASM_ADD_XYC_X(__uint64_t *, const __uint64_t *, int, int);
void ASM_SUB_XY_X(__uint64_t *, const __uint64_t *, int);
void ASM_SUB_XYB_X(__uint64_t *, const __uint64_t *, int, int);
void ASM_MUL1_XY_Z(const __uint64_t *, __uint64_t, __uint64_t *, int);

void reduceSize(BigInteger *x)
{
    int i = x->size - 1;

    while (!x->uints[i] && i)
        --i;

    x->size = ++i;
}

void add(BigInteger *x, const BigInteger *y)
{
    if (x->size == y->size)
        x->uints[x->size] = 0, ASM_ADD_XY_X(x->uints, y->uints, x->size);
    else if (x->size > y->size)
        x->uints[x->size] = 0,
        ASM_ADD_XYC_X(x->uints, y->uints, y->size, x->size - y->size);
    else
        x->uints[y->size] = 0,
        ASM_MOV_Y_X(&x->uints[x->size], &y->uints[x->size], y->size - x->size),
        ASM_ADD_XYC_X(x->uints, y->uints, x->size, y->size - x->size),
        x->size = y->size;

    if (x->uints[x->size])
        ++x->size;
}

void subtract(BigInteger *x, const BigInteger *y)
{
    if (x->size == y->size)
        ASM_SUB_XY_X(x->uints, y->uints, x->size);
    else
        ASM_SUB_XYB_X(x->uints, y->uints, y->size, x->size - y->size);

    reduceSize(x);
}

BigInteger multiplyDigit(const BigInteger *x, __uint64_t y)
{
    if (!y)
        return ZERO();

    BigInteger r;
    r.size = x->size, ASM_MUL1_XY_Z(x->uints, y, r.uints, r.size);

    if (r.uints[r.size])
        ++r.size;

    return r;
}

BigInteger multiply(const BigInteger *x, const BigInteger *y)
{
    if (_eq_0(x) || _eq_0(y))
        return ZERO();

    BigInteger t, r;
    r.size = x->size + y->size, ASM_ZERO(r.uints, r.size);
    int i;

    for (i = 0; i != y->size; ASM_ADD_XY_X(&r.uints[i++], t.uints, t.size))
        t = multiplyDigit(x, y->uints[i]);

    if (!r.uints[r.size - 1])
        --r.size;

    return r;
}

BigInteger divide(BigInteger x, const BigInteger *y)
{
    if (x.size < y->size)
        return ZERO();

    int i = x.size - 1, j = y->size - 1;
    BigInteger t, p, r = ZERO();

    for (t.size = x.size; j != -1; --i, --j)
        t.uints[i] = y->uints[j];

    for (p.size = i + 2, p.uints[i + 1] = 1; i != -1; --i)
        t.uints[i] = 0, p.uints[i] = 0;

    while (_ge(&x, &t))
        shiftLeft(&t), shiftLeft(&p);

    for (shiftRight(&t), shiftRight(&p); _ge(&x, y);
         shiftRight(&t), shiftRight(&p))
        if (_ge(&x, &t))
            subtract(&x, &t), add(&r, &p);

    return r;
}

BigInteger mod(BigInteger x, const BigInteger *y)
{
    if (x.size < y->size)
        return x;

    int i = x.size - 1, j = y->size - 1;
    BigInteger t;

    for (t.size = x.size; j != -1; --i, --j)
        t.uints[i] = y->uints[j];

    while (i != -1)
        t.uints[i--] = 0;

    while (_ge(&x, &t))
        shiftLeft(&t);

    for (shiftRight(&t); _ge(&x, y); shiftRight(&t))
        if (_ge(&x, &t))
            subtract(&x, &t);

    return x;
}

BigInteger modFast(BigInteger x, const BigInteger *y, const BigInteger *v)
{
    x.uints[x.size] = 0;
    char *c = (char *)x.uints + byteLength(&x) - byteLength(y);
    int i;

    while (_ge(&x, y))
        i = multiplier((__uint64_t *)c, v, 0, 256),
        ASM_SUB_XYB_X((__uint64_t *)c--, v[i].uints, v[i].size, 1),
        reduceSize(&x);

    return x;
}

BigInteger modPow(const BigInteger *b, BigInteger e, const BigInteger *m)
{
    BigInteger t, vMod[256], vModPow[64], r;
    t.size = 1, ASM_ZERO(t.uints, 65), vMod[0] = t;
    char c[683];
    int i, j;

    for (i = 1; i != 256; ++i)
        add(&t, m), vMod[i] = t;

    for (vModPow[0] = ONE(), i = 1; i != 64; ++i)
        vModPow[i] = modFast(multiply(&vModPow[i - 1], b), m, vMod);

    for (i = 0; !_eq_0(&e); shiftRightBy6(&e), ++i)
        c[i] = e.uints[0] & 63;

    for (r = ONE(), --i; i != -1;
         r = modFast(multiply(&r, &vModPow[c[i--]]), m, vMod))
        for (j = 6; j; --j)
            r = modFast(multiply(&r, &r), m, vMod);

    return r;
}

int isProbablePrime(const BigInteger *n)
{
    BigInteger b, e = *n;
    b.size = 1, b.uints[0] = rand(), --e.uints[0], b = modPow(&b, e, n);

    return _eq_1(&b);
}

void nextProbablePrime(BigInteger *n)
{
    BigInteger t = TWO();

    if (!(n->uints[0] & 1))
        ++n->uints[0];

    while (!isProbablePrime(n))
        add(n, &t);
}

BigInteger mask(int d)
{
    BigInteger r;
    r.size = d / 64 + 1;
    int size = r.size - 1, i;

    for (i = 0; i != size; ++i)
        r.uints[i] = 0;

    r.uints[i] = (__uint64_t)1 << d % 64;

    return r;
}

void lockSize(BigInteger *n, int d)
{
    BigInteger m = mask(d - 1);

    if (n->size < m.size)
        n->size = m.size, n->uints[n->size - 1] = 0;

    n->uints[n->size - 1] |= m.uints[m.size - 1];
}

BigInteger randomBigInteger(int d)
{
    int size = d / 15 + 1, i;
    BigInteger t, p = ONE(), r = ZERO();
    time_t s;
    time(&s), srand(s);

    for (i = 0; i != size;
         p = multiplyDigit(&p, (__uint64_t)RAND_MAX + 1), ++i)
        t = multiplyDigit(&p, rand()), add(&r, &t);

    p = mask(d);

    return mod(r, &p);
}

BigInteger getPrivateExponent(BigInteger p, BigInteger q)
{
    BigInteger t, tc, m, e = PUBLIC_EXPONENT();
    --p.uints[0], --q.uints[0], m = multiply(&p, &q);
    __uint64_t k = 1;
    t = multiplyDigit(&m, k), ++t.uints[0], tc = mod(t, &e);

    while (!_eq_0(&tc))
        t = multiplyDigit(&m, ++k), ++t.uints[0], tc = mod(t, &e);

    return divide(t, &e);
}

void setString(BigInteger x, char *s)
{
    BigInteger t = TEN();
    char r[MAX_LENGTH + 1];
    int i, j;

    for (i = 0; !_eq_0(&x); x = divide(x, &t))
        r[i++] = mod(x, &t).uints[0] + 48;

    for (--i, j = 0; i != -1; --i, ++j)
        s[j] = r[i];

    if (j)
        s[j] = '\0';
    else
        s[j] = '0', s[j + 1] = '\0';
}

void createRSAKeys(int size, const char *pubName, const char *priName)
{
    time_t start = clock();
    BigInteger p = ZERO(), q = ZERO(), n;

    while (_eq(&p, &q))
        p = randomBigInteger(size / 2),
        n = randomBigInteger(size), lockSize(&n, size), q = divide(n, &p),
        nextProbablePrime(&p), nextProbablePrime(&q);

    n = multiply(&p, &q);
    BigInteger e = PUBLIC_EXPONENT(), d = getPrivateExponent(p, q),
               m = randomBigInteger(64), c = modPow(&m, e, &n),
               m_ = modPow(&c, d, &n);

    if (_eq(&m, &m_))
    {
        --p.uints[0], --q.uints[0];
        BigInteger dP = mod(d, &p), dQ = mod(d, &q), tp = p, t = ONE();
        ++p.uints[0], ++q.uints[0], subtract(&tp, &t);
        BigInteger qInv = modPow(&q, tp, &p);
        FILE *pub = fopen(pubName, "w"), *pri = fopen(priName, "w");
        char sp[MAX_LENGTH / 2 + 2], sq[MAX_LENGTH + 1],
            se[6], sd[MAX_LENGTH + 1], sn[MAX_LENGTH + 1],
            sdP[MAX_LENGTH / 2 + 2], sdQ[MAX_LENGTH + 1],
            sqInv[MAX_LENGTH / 2 + 2];
        setString(p, sp), setString(q, sq);
        setString(e, se), setString(d, sd), setString(n, sn);
        setString(dP, sdP), setString(dQ, sdQ), setString(qInv, sqInv);
        fprintf(pub, "n =\n%s\n\ne =\n%s", sn, se);
        fprintf(pri, "n =\n%s\n\ne =\n%s\n\n", sn, se);
        fprintf(pri, "d =\n%s\n\np =\n%s\n\nq =\n%s\n\n", sd, sp, sq);
        fprintf(pri, "dP =\n%s\n\ndQ =\n%s\n\nqInv =\n%s", sdP, sdQ, sqInv);
        fclose(pub), fclose(pri);
        time_t finish = clock();
        printf("Time Taken: %.3f Seconds\n",
               (double)(finish - start) / CLOCKS_PER_SEC);
    }
    else
        printf("KEYS NOT VALID - Regenerating Key Pair\n"),
            createRSAKeys(size, pubName, priName);
}


void main()
{
    int size;
    printf("Enter RSA Key Size (Min 1024, Max 4096): ");
    scanf("%d", &size), fseek(stdin, 0, SEEK_END);

    if (size >= 1024 && size <= 4096)
    {
        unsigned int amount, i;
        printf("Enter Number of Key Pairs to Create: ");
        scanf("%u", &amount), fseek(stdin, 0, SEEK_END);

        for (i = 0; i != amount; ++i)
        {
            char pubName[26], priName[27], n[11];
            sprintf(n, "%u", i + 1), printf("\n%s.\n", n);
            strcpy(pubName, "Public Key "), strcpy(priName, "PRIVATE KEY ");
            strcat(pubName, n), strcat(pubName, ".txt");
            strcat(priName, n), strcat(priName, ".txt");
            createRSAKeys(size, pubName, priName);
        }
    }
    else
        printf("\nERROR - Invalid RSA Key Size\n");

    system("pause");
}