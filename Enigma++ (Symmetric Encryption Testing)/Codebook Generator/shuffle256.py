import random


def getMapping(data):
    result = [None] * len(data)
    for i in xrange(len(data)):
        result[data[(i + 1) % len(data)]] = data[i]
    return result

def write(fileName, data):
    with open(fileName, "w") as f:
        f.write("void shuffleKey(Bytes *key)\n{\n")
        f.write(" " * 4 + "byte temp[" + str(len(data)) + "];\n")
        for i in xrange(len(data)):
            if i & 1 == 1:
                f.write("temp[" + str(i) +\
                        "] = key->bytes[" + str(data[i]) + "];\n")
            else:
                f.write(" " * 4 + "temp[" + str(i) +\
                        "] = key->bytes[" + str(data[i]) + "], ")
        f.write(" " * 4 + "memcpy(&key->bytes[0], temp, " +\
                str(len(data)) + ");\n}\n")
        f.write("\nvoid unshuffleKey(Bytes *key)\n{\n")
        f.write(" " * 4 + "byte temp[" + str(len(data)) + "];\n")
        for i in xrange(len(data)):
            if i & 1 == 1:
                f.write("temp[" + str(data[i]) +\
                        "] = key->bytes[" + str(i) + "];\n")
            else:
                f.write(" " * 4 + "temp[" + str(data[i]) +\
                        "] = key->bytes[" + str(i) + "], ")
        f.write(" " * 4 + "memcpy(&key->bytes[0], temp, " +\
                str(len(data)) + ");\n}")


if __name__ == "__main__":
    data = range(32)
    for i in xrange(4):
        random.shuffle(data)
    mapping = getMapping(data)
    write("shuffle.c", mapping)
    raw_input("Finished")
