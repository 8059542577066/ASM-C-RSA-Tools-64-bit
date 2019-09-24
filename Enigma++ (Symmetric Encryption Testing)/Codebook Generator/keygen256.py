import random


if __name__ == "__main__":
    data = random.SystemRandom().sample(xrange(256), 32)
    with open("random.key", "wb") as f:
        f.write("".join(map(lambda n: chr(n), data)))
    raw_input("Finished")
