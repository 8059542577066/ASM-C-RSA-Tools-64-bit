void shuffleKey(Bytes *key)
{
    byte temp[32];
    temp[0] = key->bytes[3], temp[1] = key->bytes[27];
    temp[2] = key->bytes[22], temp[3] = key->bytes[15];
    temp[4] = key->bytes[5], temp[5] = key->bytes[10];
    temp[6] = key->bytes[4], temp[7] = key->bytes[30];
    temp[8] = key->bytes[19], temp[9] = key->bytes[8];
    temp[10] = key->bytes[7], temp[11] = key->bytes[9];
    temp[12] = key->bytes[23], temp[13] = key->bytes[18];
    temp[14] = key->bytes[16], temp[15] = key->bytes[24];
    temp[16] = key->bytes[21], temp[17] = key->bytes[2];
    temp[18] = key->bytes[28], temp[19] = key->bytes[1];
    temp[20] = key->bytes[12], temp[21] = key->bytes[11];
    temp[22] = key->bytes[31], temp[23] = key->bytes[17];
    temp[24] = key->bytes[26], temp[25] = key->bytes[0];
    temp[26] = key->bytes[13], temp[27] = key->bytes[25];
    temp[28] = key->bytes[6], temp[29] = key->bytes[20];
    temp[30] = key->bytes[29], temp[31] = key->bytes[14];
    memcpy(&key->bytes[0], temp, 32);
}

void unshuffleKey(Bytes *key)
{
    byte temp[32];
    temp[3] = key->bytes[0], temp[27] = key->bytes[1];
    temp[22] = key->bytes[2], temp[15] = key->bytes[3];
    temp[5] = key->bytes[4], temp[10] = key->bytes[5];
    temp[4] = key->bytes[6], temp[30] = key->bytes[7];
    temp[19] = key->bytes[8], temp[8] = key->bytes[9];
    temp[7] = key->bytes[10], temp[9] = key->bytes[11];
    temp[23] = key->bytes[12], temp[18] = key->bytes[13];
    temp[16] = key->bytes[14], temp[24] = key->bytes[15];
    temp[21] = key->bytes[16], temp[2] = key->bytes[17];
    temp[28] = key->bytes[18], temp[1] = key->bytes[19];
    temp[12] = key->bytes[20], temp[11] = key->bytes[21];
    temp[31] = key->bytes[22], temp[17] = key->bytes[23];
    temp[26] = key->bytes[24], temp[0] = key->bytes[25];
    temp[13] = key->bytes[26], temp[25] = key->bytes[27];
    temp[6] = key->bytes[28], temp[20] = key->bytes[29];
    temp[29] = key->bytes[30], temp[14] = key->bytes[31];
    memcpy(&key->bytes[0], temp, 32);
}