#ifndef RSA_H
#define RSA_H


typedef unsigned long long uint64;

typedef struct
{
    int size;
    uint64 *uints;
} BigInteger;


uint64 *malloc8(int);
BigInteger randomBigInteger(int);


#endif