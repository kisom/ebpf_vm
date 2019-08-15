#!/usr/bin/env python3


import binascii
import sys


def dump(path, name="prog"):
    with open(path, 'rb') as src:
        data = src.read().strip()
        datalen = len(data)
        data = binascii.hexlify(data).decode('utf8')

    s = f"\tuint8_t\t\t{name}[] = {{"
    position = 0
    for i in range(0, len(data), 2):
        if (i % 16) == 0:
            s += "\n\t\t"
        s += f"0x{data[i:i+2]}, "
    s += "\n\t};"
    s += f"\n\tconst size_t\t{name}len = {hex(datalen)};"

    print(s)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        for arg in sys.argv[1:]:
            dump(arg)
