#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "ENC_DATA.c"
#include "DEC_DATA.c"
#include "ENC_KEY.c"
#include "DEC_KEY.c"

typedef struct
{
    int size;
    byte *bytes;
} Bytes;

const int KEY_SIZE = 32;
const int SEGMENT_SIZE = 60 * 1024 * 1024;
const int REPEAT = 16;
const int NAME_LENGTH = 1000;
const unsigned int MB_INFO = MB_ICONINFORMATION | MB_SETFOREGROUND;
const unsigned int MB_WARNING = MB_ICONWARNING | MB_SETFOREGROUND;
const unsigned int MB_ERROR = MB_ICONERROR | MB_SETFOREGROUND;

off64_t fileSize(const char *fileName)
{
    FILE *file = fopen64(fileName, "rb");
    fseeko64(file, 0, SEEK_END);
    off64_t r = ftello64(file);
    fclose(file);

    return r;
}

void read(FILE *file, off64_t start, int size, Bytes *data)
{
    data->size = size, fseeko64(file, start, SEEK_SET);
    fread(data->bytes, data->size, 1, file);
}

void write(const Bytes *data, off64_t start, FILE *file)
{
    fseeko64(file, start, SEEK_SET);
    fwrite(data->bytes, data->size, 1, file);
}

#include "shuffle.c"

void chainKey(Bytes *data, Bytes *key, int start)
{
    shuffleKey(key);
    int i, j;

    for (i = start, j = 0; j != KEY_SIZE; ++i, ++j)
        key->bytes[j] = ENC_KEY[data->bytes[i]][key->bytes[j]];

    shuffleKey(key);
    __uint128_t *dataPtr = (__uint128_t *)data->bytes,
                *keyPtr = (__uint128_t *)key->bytes;
    *keyPtr ^= *dataPtr, *(keyPtr + 1) ^= *(dataPtr + 1);
}

void unchainKey(Bytes *data, Bytes *key, int start)
{
    __uint128_t *dataPtr = (__uint128_t *)data->bytes,
                *keyPtr = (__uint128_t *)key->bytes;
    *keyPtr ^= *dataPtr, *(keyPtr + 1) ^= *(dataPtr + 1);
    unshuffleKey(key);
    int i, j;

    for (i = start, j = 0; j != KEY_SIZE; ++i, ++j)
        key->bytes[j] = DEC_KEY[data->bytes[i]][key->bytes[j]];

    unshuffleKey(key);
}

void encryptBytes(Bytes *data, Bytes *key, int start, int size)
{
    int i, j;

    for (i = start, j = 0; j != size; ++i, ++j)
        data->bytes[i] = ENC_DATA[key->bytes[j]][data->bytes[i]];
}

void decryptBytes(Bytes *data, Bytes *key, int start, int size)
{
    int i, j;

    for (i = start, j = 0; j != size; ++i, ++j)
        data->bytes[i] = DEC_DATA[key->bytes[j]][data->bytes[i]];
}

void encryptBlock(Bytes *data, Bytes *key, int start, int size)
{
    int i;

    for (i = 0; i != REPEAT; ++i)
        encryptBytes(data, key, start, size), chainKey(data, key, start);
}

void decryptBlock(Bytes *data, Bytes *key, int start, int size)
{
    int i;

    for (i = 0; i != REPEAT; ++i)
        unchainKey(data, key, start), decryptBytes(data, key, start, size);
}

void encryptSegment(Bytes *data, Bytes *key)
{
    int tail = data->size % KEY_SIZE, size = data->size - tail, i;

    for (i = 0; i != size; i += KEY_SIZE)
        encryptBlock(data, key, i, KEY_SIZE);

    if (tail)
        encryptBlock(data, key, i, tail);
}

void decryptSegment(Bytes *data, Bytes *key)
{
    int tail = data->size % KEY_SIZE, i = data->size - tail;

    if (tail)
        decryptBlock(data, key, i, tail);

    for (i -= KEY_SIZE; i != -KEY_SIZE; i -= KEY_SIZE)
        decryptBlock(data, key, i, KEY_SIZE);
}

void encrypt(const char *input, char *keyName, const char *output)
{
    off64_t totalSize = fileSize(input),
            tail = totalSize % SEGMENT_SIZE, size = totalSize - tail, i;
    Bytes segment, key;
    segment.bytes = malloc(SEGMENT_SIZE), key.bytes = malloc(KEY_SIZE);
    FILE *fin = fopen64(input, "rb"), *fkey = fopen(keyName, "rb"),
         *fout = fopen64(output, "wb");
    read(fkey, 0, KEY_SIZE, &key), fclose(fkey);

    for (i = 0; i != size; i += SEGMENT_SIZE)
        read(fin, i, SEGMENT_SIZE, &segment), encryptSegment(&segment, &key),
            write(&segment, i, fout);

    if (tail)
        read(fin, i, tail, &segment), encryptSegment(&segment, &key),
            write(&segment, i, fout);

    strcpy(keyName, output), strcat(keyName, ".key");
    fkey = fopen(keyName, "wb"), write(&key, 0, fkey);
    fclose(fin), fclose(fkey), fclose(fout);
    free(segment.bytes), free(key.bytes);
}

void decrypt(const char *input, char *keyName, const char *output)
{
    off64_t totalSize = fileSize(input),
            tail = totalSize % SEGMENT_SIZE, i = totalSize - tail;
    Bytes segment, key;
    segment.bytes = malloc(SEGMENT_SIZE), key.bytes = malloc(KEY_SIZE);
    FILE *fin = fopen64(input, "rb"), *fkey = fopen(keyName, "rb"),
         *fout = fopen64(output, "wb");
    read(fkey, 0, KEY_SIZE, &key), fclose(fkey);

    if (tail)
        read(fin, i, tail, &segment), decryptSegment(&segment, &key),
            write(&segment, i, fout);

    for (i -= SEGMENT_SIZE; i != -SEGMENT_SIZE; i -= SEGMENT_SIZE)
        read(fin, i, SEGMENT_SIZE, &segment), decryptSegment(&segment, &key),
            write(&segment, i, fout);

    fclose(fin), fclose(fout);
    free(segment.bytes), free(key.bytes);
}

int exists(const char *filename)
{
    return fopen64(filename, "rb") != NULL;
}

void setFileName(const char *prompt, char *filename)
{
    OPENFILENAME file = {0};
    file.lStructSize = sizeof(file);
    file.lpstrFile = filename;
    file.nMaxFile = NAME_LENGTH;
    file.lpstrTitle = prompt;
    GetOpenFileName(&file);
}

void crypt(void (*function)(const char *, char *, const char *))
{
    int inputCheck, keyCheck, outputCheck;
    char input[NAME_LENGTH + 1], keyName[NAME_LENGTH + 5],
        output[NAME_LENGTH + 1], message[100];
    input[0] = '\0', keyName[0] = '\0', output[0] = '\0';
    time_t start, finish;

    for (inputCheck = 1; *input == '\0';
         setFileName("Input File Name", input), inputCheck = 0)
        if (inputCheck)
            MessageBox(0, "Enter or Select Input File Name.",
                       "Step 1", MB_INFO);
        else
            MessageBox(0, "Invalid Input File Name.",
                       "Try Again", MB_WARNING);

    for (keyCheck = 1; *keyName == '\0' ||
                       !strcmp(input, keyName);
         setFileName("Key File Name", keyName), keyCheck = 0)
        if (keyCheck)
            MessageBox(0, "Enter or Select Key File Name.",
                       "Step 2", MB_INFO);
        else
            MessageBox(0, "Invalid Key File Name.",
                       "Try Again", MB_WARNING);

    for (outputCheck = 1; *output == '\0' ||
                          !strcmp(input, output) || !strcmp(keyName, output);
         setFileName("Output File Name", output), outputCheck = 0)
        if (outputCheck)
            MessageBox(0, "Enter or Select Output File Name.",
                       "Step 3", MB_INFO);
        else
            MessageBox(0, "Invalid Output File Name.",
                       "Try Again", MB_WARNING);

    if (!exists(input))
        MessageBox(0, "Input file does not exist.", "ERROR", MB_ERROR);
    else if (!exists(keyName))
        MessageBox(0, "Key file does not exist.", "ERROR", MB_ERROR);
    else if (fileSize(keyName) != KEY_SIZE)
        sprintf(message, "Key file must be %d-byte long.", KEY_SIZE),
            MessageBox(0, message, "ERROR", MB_ERROR);
    else
        start = clock(),
        (*function)(input, keyName, output),
        finish = clock(),
        sprintf(message, "%.3f Seconds",
                (double)(finish - start) / CLOCKS_PER_SEC),
        MessageBox(0, message, "Time Taken", MB_SETFOREGROUND);
}

void main()
{
    printf("Welcome to Enigma++!\n");
    printf("\t1: Encrypt File\n");
    printf("\t2: Decrypt File\n");
    int choice;
    printf("Enter Your Choice: ");
    scanf("%d", &choice), fseek(stdin, 0, SEEK_END);

    if (choice == 1)
        crypt(encrypt);
    else if (choice == 2)
        crypt(decrypt);
    else
        MessageBox(0, "Invalid Choice.", "ERROR", MB_ERROR);
}