#!/usr/bin/env python3
"""Inspect a stripped little-endian MIPS ELF without modifying it."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs
from elftools.elf.elffile import ELFFile


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("elf", type=Path)
    parser.add_argument("--start", type=lambda value: int(value, 0))
    parser.add_argument("--end", type=lambda value: int(value, 0))
    parser.add_argument("--stores", action="store_true")
    args = parser.parse_args()

    disassembler = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    with args.elf.open("rb") as stream:
        elf = ELFFile(stream)
        for section in elf.iter_sections():
            if not (int(section["sh_flags"]) & 0x4):
                continue
            section_address = int(section["sh_addr"])
            section_end = section_address + int(section["sh_size"])
            start = max(section_address, args.start or section_address)
            end = min(section_end, args.end or section_end)
            if start >= end:
                continue
            data = section.data()[start - section_address : end - section_address]
            instructions = list(disassembler.disasm(data, start))
            if not args.stores:
                for instruction in instructions:
                    print(
                        f"0x{instruction.address:08x}: "
                        f"{instruction.mnemonic:<9} {instruction.op_str}"
                    )
                continue
            for index, instruction in enumerate(instructions):
                if instruction.mnemonic != "sw":
                    continue
                operands = instruction.op_str.replace(" ", "")
                if ",4(" not in operands:
                    continue
                source_register = operands.split(",", 1)[0]
                window = instructions[max(0, index - 12) : index]
                initializes_four = any(
                    (
                        nearby.mnemonic == "li"
                        and nearby.op_str.replace(" ", "") == f"{source_register},4"
                    )
                    or (
                        nearby.mnemonic in {"addiu", "ori"}
                        and nearby.op_str.replace(" ", "")
                        == f"{source_register},$zero,4"
                    )
                    for nearby in window
                )
                if not initializes_four:
                    continue
                first = max(0, index - 10)
                last = min(len(instructions), index + 8)
                print(f"\n### candidate near 0x{instruction.address:08x}")
                for nearby in instructions[first:last]:
                    print(
                        f"0x{nearby.address:08x}: "
                        f"{nearby.mnemonic:<9} {nearby.op_str}"
                    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
