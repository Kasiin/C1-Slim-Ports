#!/usr/bin/env python3
"""Small, read-only ELF/MIPS disassembly helper for the stock C1 launcher."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs
from elftools.elf.elffile import ELFFile


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("elf", type=Path)
    parser.add_argument("pattern")
    args = parser.parse_args()

    wanted = re.compile(args.pattern)
    with args.elf.open("rb") as stream:
        elf = ELFFile(stream)
        symtab = elf.get_section_by_name(".symtab")
        if symtab is None:
            raise SystemExit("ELF has no .symtab")

        symbols = [
            symbol
            for symbol in symtab.iter_symbols()
            if symbol["st_info"]["type"] == "STT_FUNC"
            and symbol["st_size"]
            and wanted.search(symbol.name)
        ]
        symbols.sort(key=lambda symbol: int(symbol["st_value"]))

        disassembler = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
        disassembler.detail = False
        for symbol in symbols:
            address = int(symbol["st_value"])
            size = int(symbol["st_size"])
            section = elf.get_section(symbol["st_shndx"])
            offset = address - int(section["sh_addr"])
            code = section.data()[offset : offset + size]
            print(f"\n### {symbol.name} @ 0x{address:08x} size=0x{size:x}")
            for instruction in disassembler.disasm(code, address):
                print(
                    f"0x{instruction.address:08x}: "
                    f"{instruction.mnemonic:<9} {instruction.op_str}"
                )
    return 0


if __name__ == "__main__":
    sys.exit(main())
