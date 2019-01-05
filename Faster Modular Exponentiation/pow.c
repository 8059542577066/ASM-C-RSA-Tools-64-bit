#include <string.h>
#include <stdio.h>


typedef unsigned long long __uint64_t;

typedef struct
{
    int size;
    __uint64_t uints[130];
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

const BigInteger TEN()
{
    BigInteger r;
    r.size = 1, r.uints[0] = 10;

    return r;
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
    char c[684];
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

int isValid(const char *s)
{
    int i;

    for (i = 0; s[i] != '\0'; ++i)
        if (s[i] < 48 || s[i] > 57)
            return 0;

    return 1;
}

BigInteger toBigInteger(const char *s)
{
    int size = strlen(s), i;
    BigInteger t, p = ONE(), r = ZERO();

    for (i = 0; i != size; p = multiplyDigit(&p, 10))
        t = multiplyDigit(&p, s[size - 1 - i++] - 48), add(&r, &t);

    return r;
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


int main(int argc, char **argv)
{
    if (argc != 4)
        printf("ERROR - 3 arguments required: <Base> <Exponent> <Modulus>");
    else if (!isValid(argv[1]))
        printf("ERROR - 1st argument(Base) is not a valid number.");
    else if (!isValid(argv[2]))
        printf("ERROR - 2nd argument(Exponent) is not a valid number.");
    else if (!isValid(argv[3]))
        printf("ERROR - 3rd argument(Modulus) is not a valid number.");
    else if (strlen(argv[1]) > MAX_LENGTH)
        printf("ERROR - 1st argument(Base) is too long.");
    else if (strlen(argv[2]) > MAX_LENGTH)
        printf("ERROR - 2nd argument(Exponent) is too long.");
    else if (strlen(argv[3]) > MAX_LENGTH)
        printf("ERROR - 3rd argument(Modulus) is too long.");
    else
    {
        BigInteger m = toBigInteger(argv[3]);

        if (_eq_0(&m))
            printf("ERROR - 3rd argument(Modulus) cannot be 0.");
        else if (_eq_1(&m))
            printf("0");
        else
        {
            BigInteger b = toBigInteger(argv[1]), e = toBigInteger(argv[2]);
            char r[MAX_LENGTH + 1];
            setString(modPow(&b, e, &m), r), printf(r);
        }
    }

    return 0;
}