#!/usr/bin/env python3
import random
import sys


def load_seed():
    """
    Load or create a seed.
    """
    seed_option = int(sys.argv[1].strip())
    seed_file = sys.argv[2]

    try:
        with open(seed_file, 'rt') as file:
            config = int(file.readline().strip())
            seed = int(file.readline().strip())

        if config == seed_option:
            return seed
    except:
        pass

    if seed_option >= 0:
        seed = seed_option
    else:
        seed = random.getrandbits(64)

    with open(seed_file, 'wt') as file:
        print(seed_option, file=file)
        print(seed, file=file)

    return seed


if __name__ == '__main__':
    print(f'0x{load_seed():08x}')
