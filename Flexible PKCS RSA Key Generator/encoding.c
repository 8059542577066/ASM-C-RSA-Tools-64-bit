#include "rsa.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>


typedef unsigned char byte;

typedef struct
{
    int size;
    byte *bytes;
} Bytes;

const byte B7 = 1 << 7;
const char *BASE64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const unsigned long long HEADER_1 = 0x86F70D0101010500;
const unsigned long long HEADER_2 = 0x00300D06092A8648;
const unsigned short HEADER_3 = 0x0201;


byte *malloc1(int size)
{
    byte *r = malloc(size);

    if (!r)
        exit(0);

    return r;
}

Bytes PUBLIC_HEADER()
{
    Bytes r;
    r.size = 15, r.bytes = (byte *)malloc1(r.size);
    memcpy(r.bytes, &HEADER_1, 8), memcpy(&r.bytes[8], &HEADER_2, 7);
    int i;

    for (i = 0; i != 7; ++i)
        r.bytes[i] ^= r.bytes[14 - i], r.bytes[14 - i] ^= r.bytes[i],
            r.bytes[i] ^= r.bytes[14 - i];

    return r;
}

Bytes PRIVATE_HEADER()
{
    Bytes r;
    r.size = 18, r.bytes = (byte *)malloc1(r.size);
    memcpy(r.bytes, &HEADER_1, 8), memcpy(&r.bytes[8], &HEADER_2, 8);
    memcpy(&r.bytes[16], &HEADER_3, 2);
    int i;

    for (i = 0; i != 9; ++i)
        r.bytes[i] ^= r.bytes[17 - i], r.bytes[17 - i] ^= r.bytes[i],
            r.bytes[i] ^= r.bytes[17 - i];

    return r;
}

Bytes getSizePrefix(int size)
{
    Bytes r;

    if (size <= 0x7F)
        r.size = 1, r.bytes = (byte *)malloc1(r.size),
        r.bytes[0] = size;
    else if (size <= 0xFF)
        r.size = 2, r.bytes = (byte *)malloc1(r.size),
        r.bytes[0] = 0x81, r.bytes[1] = size;
    else if (size <= 0xFFFF)
        r.size = 3, r.bytes = (byte *)malloc1(r.size),
        r.bytes[0] = 0x82, r.bytes[1] = size >> 8, r.bytes[2] = size;

    return r;
}

Bytes bytesToASN1Int(Bytes b)
{
    Bytes ts, tc, r;
    int i = 0, j;

    if (b.bytes[b.size - 1] & B7)
        tc.size = b.size + 1,
        tc.bytes = (byte *)malloc1(tc.size), tc.bytes[i++] = 0;
    else
        tc.size = b.size,
        tc.bytes = (byte *)malloc1(tc.size);

    for (ts = getSizePrefix(tc.size), j = b.size - 1; j != -1; --j)
        tc.bytes[i++] = b.bytes[j];

    r.size = ts.size + tc.size + 1, r.bytes = (byte *)malloc1(r.size);
    r.bytes[0] = 0x02, memcpy(&r.bytes[1], ts.bytes, ts.size);
    memcpy(&r.bytes[ts.size + 1], tc.bytes, tc.size);
    free(ts.bytes), free(tc.bytes);

    return r;
}

Bytes buildPublicPKCS1(Bytes n, Bytes e)
{
    Bytes ts, r;
    int size = n.size + e.size, i;
    ts = getSizePrefix(size);
    r.size = ts.size + size + 1, r.bytes = (byte *)malloc1(r.size);
    r.bytes[0] = 0x30, memcpy(&r.bytes[1], ts.bytes, ts.size);
    i = ts.size + 1, free(ts.bytes);
    memcpy(&r.bytes[i], n.bytes, n.size), i += n.size;
    memcpy(&r.bytes[i], e.bytes, e.size);

    return r;
}

Bytes buildPublicPKCS8(Bytes b)
{
    Bytes th, tbs, ts, r;
    int i;
    th = PUBLIC_HEADER();
    tbs = getSizePrefix(b.size + 1), i = th.size + b.size + tbs.size + 2;
    ts = getSizePrefix(i), i += ts.size + 1;
    r.size = i, r.bytes = (byte *)malloc1(r.size), i = 0;
    r.bytes[i++] = 0x30;
    memcpy(&r.bytes[i], ts.bytes, ts.size), i += ts.size;
    memcpy(&r.bytes[i], th.bytes, th.size), i += th.size;
    r.bytes[i++] = 0x03;
    memcpy(&r.bytes[i], tbs.bytes, tbs.size), i += tbs.size;
    r.bytes[i++] = 0x00;
    memcpy(&r.bytes[i], b.bytes, b.size);
    free(th.bytes), free(tbs.bytes), free(ts.bytes);

    return r;
}

Bytes buildPrivatePKCS1(Bytes n, Bytes e, Bytes d, Bytes p, Bytes q,
                        Bytes dP, Bytes dQ, Bytes qInv)
{
    Bytes ts, r;
    int size = 3 + n.size + e.size + d.size +
               p.size + q.size + dP.size + dQ.size + qInv.size,
        i;
    ts = getSizePrefix(size);
    r.size = ts.size + size + 1, r.bytes = (byte *)malloc1(r.size);
    r.bytes[0] = 0x30, memcpy(&r.bytes[1], ts.bytes, ts.size);
    i = ts.size + 1, free(ts.bytes);
    r.bytes[i++] = 0x02, r.bytes[i++] = 0x01, r.bytes[i++] = 0;
    memcpy(&r.bytes[i], n.bytes, n.size), i += n.size;
    memcpy(&r.bytes[i], e.bytes, e.size), i += e.size;
    memcpy(&r.bytes[i], d.bytes, d.size), i += d.size;
    memcpy(&r.bytes[i], p.bytes, p.size), i += p.size;
    memcpy(&r.bytes[i], q.bytes, q.size), i += q.size;
    memcpy(&r.bytes[i], dP.bytes, dP.size), i += dP.size;
    memcpy(&r.bytes[i], dQ.bytes, dQ.size), i += dQ.size;
    memcpy(&r.bytes[i], qInv.bytes, qInv.size);

    return r;
}

Bytes buildPrivatePKCS8(Bytes b)
{
    Bytes th, tbs, ts, r;
    int i;
    th = PRIVATE_HEADER();
    tbs = getSizePrefix(b.size), i = th.size + b.size + tbs.size + 1;
    ts = getSizePrefix(i), i += ts.size + 1;
    r.size = i, r.bytes = (byte *)malloc1(r.size), i = 0;
    r.bytes[i++] = 0x30;
    memcpy(&r.bytes[i], ts.bytes, ts.size), i += ts.size;
    memcpy(&r.bytes[i], th.bytes, th.size), i += th.size;
    r.bytes[i++] = 0x04;
    memcpy(&r.bytes[i], tbs.bytes, tbs.size), i += tbs.size;
    memcpy(&r.bytes[i], b.bytes, b.size);
    free(th.bytes), free(tbs.bytes), free(ts.bytes);

    return r;
}

int base64Length(Bytes b)
{
    return b.size % 3 ? b.size / 3 * 4 + 4 : b.size / 3 * 4;
}

void setBase64(Bytes b, char *r)
{
    int t = 0, i, j;

    for (i = 0, j = 0; i < b.size - 3; t = 0, i += 3, j += 4)
        t = (int)b.bytes[i] << 16 |
            (int)b.bytes[i + 1] << 8 | (int)b.bytes[i + 2],
        r[j + 3] = BASE64[t % 64], t >>= 6, r[j + 2] = BASE64[t % 64], t >>= 6,
        r[j + 1] = BASE64[t % 64], t >>= 6, r[j] = BASE64[t];

    if (b.size % 3 == 1)
        t = (int)b.bytes[i] << 16,
        r[j + 3] = '=', r[j + 2] = '=', t >>= 12,
        r[j + 1] = BASE64[t % 64], t >>= 6, r[j] = BASE64[t];
    else if (b.size % 3 == 2)
        t = (int)b.bytes[i] << 16 | (int)b.bytes[i + 1] << 8,
        r[j + 3] = '=', t >>= 6, r[j + 2] = BASE64[t % 64], t >>= 6,
        r[j + 1] = BASE64[t % 64], t >>= 6, r[j] = BASE64[t];
    else
        t = (int)b.bytes[i] << 16 |
            (int)b.bytes[i + 1] << 8 | (int)b.bytes[i + 2],
        r[j + 3] = BASE64[t % 64], t >>= 6, r[j + 2] = BASE64[t % 64], t >>= 6,
        r[j + 1] = BASE64[t % 64], t >>= 6, r[j] = BASE64[t];

    r[j + 4] = '\0';
}

Bytes bigIntegerToBytes(BigInteger b)
{
    Bytes r;
    r.size = 8 * b.size, r.bytes = (byte *)malloc1(r.size);
    memcpy(r.bytes, b.uints, r.size);
    int i = r.size - 1;

    while (!r.bytes[i] && i)
        --i;

    r.size = ++i;

    return r;
}

void writeKey(const char *b, FILE *file, const char *mode)
{
    fprintf(file, "-----BEGIN %s KEY-----", mode);
    int i;

    for (i = 0; b[i] != '\0'; ++i)
        if (i % 64)
            fprintf(file, "%c", b[i]);
        else
            fprintf(file, "\n%c", b[i]);

    fprintf(file, "\n-----END %s KEY-----", mode);
}

void createRSAKeys(int size, const char *pubName1, const char *pubName8,
                   const char *priName1, const char *priName8)
{
    time_t start = clock();
    BigInteger p = randomBigInteger(size / 2), n = randomBigInteger(size),
               e, d, newN, dt, dp, q, m, c, m_;
    lockSize(&n, size), q.uints = malloc8(n.size);
    int dtSize = n.size + 1, i;
    uint64 *heap1 = malloc8(4 * n.size + 2 * dtSize + 2);
    e.uints = heap1, i = 1, d.uints = &heap1[i], i += n.size;
    newN.uints = &heap1[i], i += n.size, dt.uints = &heap1[i], i += dtSize;
    dp.uints = &heap1[i], i += dtSize, m.uints = &heap1[i++], random(&m);
    c.uints = &heap1[i], i += n.size, m_.uints = &heap1[i];
    copy(&newN, n, n.size), divide(n, p, newN, dt, dp, &q);
    nextProbablePrime(&p), nextProbablePrime(&q), multiply(p, q, dt, &n);
    publicExponent(&e), setPrivateExponent(p, q, e, &d);
    modPow(m, e, n, &c), modPow(c, d, n, &m_);

    if (_eq(m, m_))
    {
        --p.uints[0], --q.uints[0];
        uint64 *heap2 = malloc8(3 * p.size + q.size + d.size + 1);
        BigInteger t, newP, dP, dQ, qInv;
        t.uints = heap2, i = d.size + 1, newP.uints = &heap2[i], i += p.size;
        dP.uints = &heap2[i], i += p.size, dQ.uints = &heap2[i], i += q.size;
        qInv.uints = &heap2[i], mod(d, p, t, &dP), mod(d, q, t, &dQ);
        copy(&newP, p, p.size), one(&t), subtract(&newP, t);
        ++p.uints[0], ++q.uints[0], modPow(q, newP, p, &qInv);
        Bytes bn = bigIntegerToBytes(n), an = bytesToASN1Int(bn),
              be = bigIntegerToBytes(e), ae = bytesToASN1Int(be),
              bd = bigIntegerToBytes(d), ad = bytesToASN1Int(bd),
              bp = bigIntegerToBytes(p), ap = bytesToASN1Int(bp),
              bq = bigIntegerToBytes(q), aq = bytesToASN1Int(bq),
              bdP = bigIntegerToBytes(dP), adP = bytesToASN1Int(bdP),
              bdQ = bigIntegerToBytes(dQ), adQ = bytesToASN1Int(bdQ),
              bqInv = bigIntegerToBytes(qInv), aqInv = bytesToASN1Int(bqInv),
              pubSeq1 = buildPublicPKCS1(an, ae),
              pubSeq8 = buildPublicPKCS8(pubSeq1),
              priSeq1 = buildPrivatePKCS1(an, ae, ad, ap, aq, adP, adQ, aqInv),
              priSeq8 = buildPrivatePKCS8(priSeq1);
        free(p.uints), free(q.uints), free(n.uints), free(heap1), free(heap2);
        free(bn.bytes), free(an.bytes), free(be.bytes), free(ae.bytes);
        free(bd.bytes), free(ad.bytes), free(bp.bytes), free(ap.bytes);
        free(bq.bytes), free(aq.bytes), free(bdP.bytes), free(adP.bytes);
        free(bdQ.bytes), free(adQ.bytes), free(bqInv.bytes), free(aqInv.bytes);
        char pubChars1[base64Length(pubSeq1) + 1],
            pubChars8[base64Length(pubSeq8) + 1],
            priChars1[base64Length(priSeq1) + 1],
            priChars8[base64Length(priSeq8) + 1];
        setBase64(pubSeq1, pubChars1), setBase64(pubSeq8, pubChars8);
        setBase64(priSeq1, priChars1), setBase64(priSeq8, priChars8);
        free(pubSeq1.bytes), free(pubSeq8.bytes);
        free(priSeq1.bytes), free(priSeq8.bytes);
        FILE *pub1 = fopen(pubName1, "w"), *pub8 = fopen(pubName8, "w"),
             *pri1 = fopen(priName1, "w"), *pri8 = fopen(priName8, "w");
        writeKey(pubChars1, pub1, "RSA PUBLIC");
        writeKey(pubChars8, pub8, "PUBLIC");
        writeKey(priChars1, pri1, "RSA PRIVATE");
        writeKey(priChars8, pri8, "PRIVATE");
        fclose(pub1), fclose(pub8), fclose(pri1), fclose(pri8);
        time_t finish = clock();
        printf("Time Taken: %.3f Seconds\n",
               (double)(finish - start) / CLOCKS_PER_SEC);
    }
    else
        free(p.uints), free(q.uints), free(n.uints), free(heap1),
            printf("KEYS NOT VALID - Regenerating Key Pair\n"),
            createRSAKeys(size, pubName1, pubName8, priName1, priName8);
}