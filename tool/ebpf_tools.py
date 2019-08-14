#!/usr/bin/env python3

from collections import namedtuple
import binascii
import struct


instr = namedtuple('instr', 'op dst src off imm')


def packi(i: instr):
    tmp = (i.dst & 0xF) + ((i.src << 4) & 0xF0)
    return binascii.hexlify(struct.pack('<IHBB', i.imm, i.off, tmp, i.op))
