#!/usr/bin/env python3

import argparse
import binascii
import re
import struct


class IllegalInstruction(Exception):
    pass


class InvalidRegister(Exception):
    pass


class InvalidImmediate(Exception):
    pass


class ImmediateTooLarge(Exception):
    pass


def validate_register(v):
    match = reg_re.match(v)
    if not match:
        return None
    v = int(match.group(1))
    if v > 15:
        raise InvalidRegister("There are only 16 registers available")
    return v


def clean(v):
    return v.replace(",", "").strip()


def parse_immediate(v):
    v = clean(v)
    base = 10
    if v.startswith("#"):
        base = 16
        v = v[1:]
    return int(v, base)


class Instruction:
    def __init__(self, opcode, dst, src=0, off=0, imm=0):
        self.opcode = opcode
        self.dst = dst
        self.src = src
        self.off = off
        self.imm = imm

    def pack(self):
        reg = self.dst & 0xF | self.src << 4
        return struct.pack("<IhBB", self.imm, self.off, reg, self.opcode)

    def hex(self):
        return binascii.hexlify(self.pack()).decode("utf8")


class Program:
    def __init__(self):
        self.instructions = []
        self.labels = {}

    def __str__(self):
        return "\n".join([instr.hex() for instr in self.instructions])

    def add(self, ins):
        self.instructions.append(ins)

    def label(self, name, offset):
        self.labels[name] = offset

    def lookup(self, name):
        return self.labels.get(name, None)

    def offset(self):
        return (len(self.instructions) + 1) * 8

    def parse_numeric(self, v):
        offset = self.lookup(v)
        if offset:
            return offset

        return parse_immediate(v)

    def parse_jump(self, parts):
        (token, offset) = parts
        offset = self.parse_numeric(offset)
        offset -= self.offset()
        return Instruction(branch_instructions[token], 0, off=offset)

    def parse_branch(self, parts):
        (token, dst, src_or_imm, off) = parts
        assert token in branch_instructions
        opcode = branch_instructions[token]

        dst = validate_register(dst)
        if not dst:
            raise InvalidRegister(
                "branch instructions require a register as the first argument"
            )

        off = self.parse_numeric(off)
        assert off
        off -= self.offset()

        src = validate_register(src_or_imm)
        if src:
            opcode &= 0x08
            return Instruction(opcode, dst, src, off)

        imm = parse_immediate(src_or_imm)
        return Instruction(opcode, dst, 0, off, imm)

    def pack(self, outs):
        for instruction in self.instructions:
            outs.write(instruction.pack())


alu_instructions = {
    "ADD": 0x07,
    "SUB": 0x17,
    "MUL": 0x27,
    "DIV": 0x37,
    "OR": 0x47,
    "AND": 0x57,
    "LSH": 0x67,
    "RSH": 0x77,
    "NEG": 0x87,
    "MOD": 0x97,
    "XOR": 0xA7,
    "MOV": 0xB7,
    "ARSH": 0xC7,
    "ADD32": 0x04,
    "SUB32": 0x14,
    "MUL32": 0x24,
    "DIV32": 0x34,
    "OR32": 0x44,
    "AND32": 0x54,
    "LSH32": 0x64,
    "RSH32": 0x74,
    "NEG32": 0x84,
    "MOD32": 0x94,
    "XOR32": 0xA4,
    "MOV32": 0xB4,
    "ARSH32": 0xC4,
    "LE16": 0xD4,
    "LE32": 0xD4,
    "LE64": 0xD4,
    "BE16": 0xDC,
    "BE32": 0xDC,
    "BE64": 0xDC,
}

branch_instructions = {
    "JA": 0x05,
    "JEQ": 0x15,
    "JGT": 0x25,
    "JGE": 0x35,
    "JLT": 0xA5,
    "JLE": 0xB5,
    "JSET": 0x45,
    "JNE": 0x55,
}

reg_re = re.compile(r"r(\d{1,2})")
lbl_re = re.compile(r"^([a-zA-Z_]+):\s*$")


def parse_byteswap(token, dst):
    assert token in alu_instructions
    big_endian = False
    if token.startswith("BE"):
        big_endian = True
    elif not token.startswith("LE"):
        raise IllegalInstruction(token)

    opcode = alu_instructions[token]
    dst = validate_register(dst)
    if not dst:
        raise IllegalInstruction("Byteswap requires a destination register.")

    token = token[2:]
    base = int(token)
    return Instruction(opcode, dst, imm=base)


def parse_alu(parts):
    (token, dst, src_or_imm) = parts
    assert token in alu_instructions

    dst = validate_register(dst)
    if not dst:
        raise InvalidRegister(
            "ALU instructions require a register as the first argument"
        )

    instruction = Instruction(alu_instructions[token], dst)

    src = validate_register(src_or_imm)
    if src:
        instruction.src = src
        return instruction
    instruction.imm = parse_immediate(src_or_imm)
    if instruction.imm > 0xFFFFFFFF:
        raise ImmediateTooLarge()

    return instruction


def pass1(source):
    pgm = Program()
    lines = []

    for line in source:
        # clean line and remove comments
        com_start = line.find(";")
        if com_start != -1:
            line = line[:com_start]

        line = line.strip()
        if line == "":
            continue

        match = lbl_re.match(line)
        if match:
            pgm.label(match.group(1), len(lines) * 8)
            continue

        lines.append(line)

    return (lines, pgm)


def pass2(source, program):
    for line in source:
        parts = [part.strip() for part in line.split()]
        token = parts[0]

        if token in alu_instructions:
            if token.startswith("BE") or token.startswith("LE"):
                if len(parts) != 2:
                    print(len(parts))
                    raise IllegalInstruction(line)
                program.add(parse_byteswap(token, parts[1]))

            elif len(parts) != 3:
                raise IllegalInstruction(line)

            else:
                program.add(parse_alu(parts))

        elif token in branch_instructions:
            if token == "JA":
                program.add(program.parse_jump(parts))
            else:
                program.add(program.parse_branch(parts))

        else:
            raise IllegalInstruction(line)

    return program


def parse_file(source):
    lines, program = pass1(source)
    return pass2(lines, program)


def parse(source_path, dest=None):
    with open(source_path, "rt") as source:
        program = parse_file(source)
    if not dest:
        print(program)
    else:
        with open(dest, 'wb') as objfile:
            program.pack(objfile)        


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="ebpf_as", description="EBPF assembler")
    parser.add_argument("source", help="source file to assemble")
    parser.add_argument("-o", "--output", help="choose the file to write to")
    parser.add_argument(
        "-t",
        "--stdout",
        action="store_true",
        help="dump to hex version to standard output",
    )

    args = parser.parse_args()
    outfile = args.source + ".o"
    if args.output and args.stdout:
        print("Only one of -o and -t may be specified.")
        exit(1)

    if args.output:
        outfile = args.output
    if args.stdout:
        outfile = None
    print(f"Compiling {args.source} -> {outfile}")
    parse(args.source, outfile)
