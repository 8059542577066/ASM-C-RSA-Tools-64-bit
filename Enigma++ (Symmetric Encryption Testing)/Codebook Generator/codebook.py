import random


def rotateRight(data, n):
    return data[-n:] + data[:-n]

def getUniqueRotations(data):
    result = []
    for i in xrange(len(data)):
        result.append(rotateRight(data, i))
    return result

def swapCol(data, combi):
    x, y = combi
    for row in data:
        row[x], row[y] = row[y], row[x]

def swapRow(data, combi):
    x, y = combi
    data[x], data[y] = data[y], data[x]

def randomize(data, times):
    for i in xrange(times / 16):
        for j in xrange(16):
            if i & 1 == 1:
                swapCol(data, random.sample(xrange(len(data)), 2))
            else:
                swapRow(data, random.sample(xrange(len(data)), 2))
    for j in xrange(times % 16):
        if j & 1 == 1:
            swapCol(data, random.sample(xrange(len(data)), 2))
        else:
            swapRow(data, random.sample(xrange(len(data)), 2))

def getInversedBox(data):
    result = []
    for row in data:
        result.append([row.index(i) for i in xrange(len(data))])
    return result

def merge(data):
    result = []
    for row in data:
        result += row
    return result

def write(fileName, arrayName, size, data, cols):
    with open(fileName, "w") as f:
        f.write("const byte " + arrayName +\
                "[" + str(size) + "][" + str(size) + "] =\n")
        f.write(" " * 4 + "{\n")
        if len(data) % cols == 0:
            tail = cols
        else:
            tail = len(data) % cols
        for i, byte in enumerate(data[:len(data) - tail]):
            if i % cols == 0:
                f.write(" " * 8)
            f.write("0x%02X" % byte)
            if i % cols == cols - 1:
                f.write(",\n")
            else:
                f.write(", ")
        for i, byte in enumerate(data[len(data) - tail:]):
            if i % cols == 0:
                f.write(" " * 8)
            f.write("0x%02X" % byte)
            if i == tail - 1:
                f.write("};")
            else:
                f.write(", ")


if __name__ == "__main__":
    size = 256
    times = int(raw_input("Times to Shuffle: "))
    suffix = raw_input("Array Suffix: ")
    col = int(raw_input("Number of Columns: "))
    initialVector = range(size)
    random.shuffle(initialVector)
    data = getUniqueRotations(initialVector)
    randomize(data, times)
    dataInv = getInversedBox(data)
    data = merge(data)
    dataInv = merge(dataInv)
    write("ENC" + suffix + ".c", "ENC" + suffix, size, data, col)
    write("DEC" + suffix + ".c", "DEC" + suffix, size, dataInv, col)
    raw_input("Finished")
