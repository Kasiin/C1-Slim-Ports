#!/usr/bin/env python3
"""Build hash-locked, reversible C1-Slim stock-launcher Terminal artifacts."""

from __future__ import annotations

import hashlib
import json
import shutil
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "artifacts" / "stock-terminal-integration"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def patch_exact(image: bytearray, offset: int, expected: bytes, replacement: bytes) -> None:
    if len(expected) != len(replacement):
        raise ValueError("patch lengths differ")
    actual = bytes(image[offset : offset + len(expected)])
    if actual != expected:
        raise RuntimeError(
            f"unexpected bytes at 0x{offset:x}: "
            f"expected={expected.hex()} actual={actual.hex()}"
        )
    image[offset : offset + len(replacement)] = replacement


def mips_word(value: int) -> bytes:
    return struct.pack("<I", value)


def mips_branch(pc: int, target: int) -> bytes:
    displacement = target - (pc + 4)
    if displacement % 4:
        raise ValueError("MIPS branch target is not word-aligned")
    immediate = displacement // 4
    if not -(1 << 15) <= immediate < (1 << 15):
        raise ValueError("MIPS branch target is out of range")
    return mips_word(0x10000000 | (immediate & 0xFFFF))


def evdev_event(event_type: int, code: int, value: int) -> bytes:
    # MP-D261 userspace is 32-bit: timeval (2 x uint32), type/code, value.
    return struct.pack("<IIHHi", 0, 0, event_type, code, value)


def read_lf(path: Path) -> bytes:
    """Return a text asset with release-compatible Unix line endings."""
    return path.read_bytes().replace(b"\r\n", b"\n")


def main() -> None:
    terminal_source = ROOT / "build" / "C1ancher"
    launcher_source = ROOT / "artifacts" / "stock-launcher-analysis" / "mpenMain"
    expected_terminal_hash = "1bfd1851d2a7dc314248188e5c5d9d4a420dbb21518f1608dd6ad0977fc2af6c"
    expected_launcher_hash = "62a952207cb8bdada66f3b4812cc0aa21db3fb030bfc19b7b970080d138f8ad6"

    terminal = bytearray(terminal_source.read_bytes())
    launcher = bytearray(launcher_source.read_bytes())
    if digest(terminal) != expected_terminal_hash:
        raise RuntimeError("C1ancher input hash does not match the reviewed build")
    if digest(launcher) != expected_launcher_hash:
        raise RuntimeError("mpenMain input hash does not match the connected stock launcher")

    # c1_ui_initial_state(): page=TERMINAL (3), selection remains zero from memset.
    patch_exact(
        terminal,
        0x1350,
        bytes.fromhex("04 00 02 24 25 28 80 00 20 00 a2 af"),
        bytes.fromhex("03 00 02 24 25 28 80 00 1c 00 a2 af"),
    )

    # In standalone app mode, HOME must leave Terminal instead of opening the
    # C1ancher desktop embedded in the source executable. The preceding beqz
    # preserves the original desktop transition outside app mode; this branch
    # only replaces the app-mode c1_terminal_stop() call and reaches the common
    # cleanup path with C1_STATUS_OK in s3.
    patch_exact(
        terminal,
        0xA984,
        bytes.fromhex("84 82 99 8f 5f ef 11 04"),
        mips_branch(0x0040A984, 0x00409C1C) + mips_word(0x00009825),
    )

    # Reuse the stock second-icon identifier, but show Terminal instead of
    # [AI]back-word. The desktop entries are reordered below.
    patch_exact(
        launcher,
        0x37CB10,
        b"[AI]" + "背词".encode("utf-8") + b"\0\0",
        b"Terminal\0".ljust(12, b"\0"),
    )
    # on_click_memory(event) becomes a tail call to penv2::mpensystem(). The
    # command string lives in the now-unreachable body immediately after it.
    patch_exact(
        launcher,
        0x30E784,
        bytes.fromhex("78 ff bd 27 50 00 a5 27 68 00 a4 27 74 00 b0 af"),
        b"".join(
            (
                mips_word(0x3C040071),  # lui a0, 0x71
                mips_word(0x2484E794),  # addiu a0, a0, 0xe794
                mips_word(0x08131BE8),  # j 0x004c6fa0 (penv2::mpensystem)
                mips_word(0x00000000),  # delay-slot nop
            )
        ),
    )
    patch_exact(
        launcher,
        0x30E794,
        bytes.fromhex("58 00 b0 27 84 00 bf af 80 00 b3 af"),
        b"/usr/data/t\0",
    )

    # Launch Terminal on the T key-down event. The stock accelerator acts on
    # key-up; handling key-down here removes the extra focus/Enter interaction
    # and makes the response immediate. Other keys retain the stock handler.
    patch_exact(
        launcher,
        0x30DEB4,
        bytes.fromhex("4f 00 a0 14 00 00 00 00"),
        b"".join(
            (
                mips_word(0x081C39E8),  # j 0x0070e7a0 (key-down cave)
                mips_word(0x00000000),  # delay-slot nop
            )
        ),
    )
    patch_exact(
        launcher,
        0x30E7A0,
        bytes.fromhex(
            "7c 00 b2 af 78 00 b1 af 50 00 b0 af 54 00 a0 af "
            "1e 80 1a 0c 58 00 a0 a3 1b 00 40 14 38 00 a2 27 "
            "71 00 02 3c ec e3 42 24"
        ),
        b"".join(
            (
                mips_word(0x10A00005),  # beqz a1, continue stock release path
                mips_word(0x24020054),  # delay: li v0, 'T'
                mips_word(0x10820005),  # beq a0, v0, launch Terminal
                mips_word(0x00000000),  # delay-slot nop
                mips_word(0x081C37FD),  # j 0x0070dff4 (ignore other key-down)
                mips_word(0x00000000),  # delay-slot nop
                mips_word(0x081C37AF),  # j 0x0070debc (stock key-up path)
                mips_word(0x00000000),  # delay-slot nop
                mips_word(0x081C39E1),  # j 0x0070e784 (Terminal callback)
                mips_word(0x00000000),  # delay-slot nop
            )
        ),
    )

    # Put Terminal in the first desktop slot and move the stock dictionary
    # entry to the second slot. Swap all three fields rather than copying text:
    # icon identifier, label and click callback.
    patch_exact(
        launcher,
        0x30EE80,
        mips_word(0x24E2CAE0),  # first icon: ic_desktop_ccyj
        mips_word(0x24E2CB00),  # first icon: ic_desktop_dcjy
    )
    patch_exact(
        launcher,
        0x30EEA0,
        mips_word(0x24A4CAF0),  # first label: dictionary
        mips_word(0x24A4CB10),  # first label: Terminal
    )
    patch_exact(
        launcher,
        0x30EEB0,
        mips_word(0x8CB8CAF0),  # first label word 0: dictionary
        mips_word(0x8CB8CB10),  # first label word 0: Terminal
    )
    patch_exact(
        launcher,
        0x30EEA8,
        mips_word(0x2466CB00),  # second icon: ic_desktop_dcjy
        mips_word(0x2466CAE0),  # second icon: ic_desktop_ccyj
    )
    patch_exact(
        launcher,
        0x30EEF8,
        mips_word(0x24E7E250),  # first callback: dictionary
        mips_word(0x24E7E784),  # first callback: Terminal
    )
    patch_exact(
        launcher,
        0x30EF48,
        mips_word(0x2549CB10),  # second label: Terminal
        mips_word(0x2549CAF0),  # second label: dictionary
    )
    patch_exact(
        launcher,
        0x30EF5C,
        mips_word(0x8D53CB10),  # second label word 0: Terminal
        mips_word(0x8D53CAF0),  # second label word 0: dictionary
    )
    patch_exact(
        launcher,
        0x30EFA8,
        mips_word(0x24A5E784),  # second callback: Terminal
        mips_word(0x24A5E250),  # second callback: dictionary
    )

    # The launcher stores byte lengths separately from the label pointers.
    # Match those lengths to the reordered UTF-8 strings: Terminal is 8 bytes;
    # 查词译句 is 12 bytes.
    patch_exact(
        launcher,
        0x30EEEC,
        mips_word(0x2403000C),  # first label length: 12
        mips_word(0x24030008),  # first label length: 8
    )
    patch_exact(
        launcher,
        0x30EF80,
        mips_word(0x2405000A),  # second label length: 10
        mips_word(0x2405000C),  # second label length: 12
    )
    # The original second label is only 10 bytes, so its generated copy uses
    # lhu/sh for the final two bytes and writes the terminator at +10. Expand
    # that copy to a full third word and terminate at +12 for 查词译句.
    patch_exact(
        launcher,
        0x30EF64,
        mips_word(0x95310008),  # lhu s1, 8(t1)
        mips_word(0x8D310008),  # lw  s1, 8(t1)
    )
    patch_exact(
        launcher,
        0x30EFCC,
        mips_word(0xA7B10094),  # sh s1, 0x94(sp)
        mips_word(0xAFB10094),  # sw s1, 0x94(sp)
    )
    patch_exact(
        launcher,
        0x30EFF8,
        mips_word(0xA3A00096),  # sb zero, 0x96(sp)
        mips_word(0xA3A00098),  # sb zero, 0x98(sp)
    )

    # The launcher passes these letters to both the on-screen labels and the
    # physical-key bindings. First entry is now Terminal, so Q becomes T.
    patch_exact(launcher, 0x37CBBC, b"QWEASD", b"TWEASD")

    OUTPUT.mkdir(parents=True, exist_ok=True)
    terminal_output = OUTPUT / "Terminal"
    launcher_output = OUTPUT / "mpenMain.terminal"
    wrapper_output = OUTPUT / "terminal-launch.sh"
    bootstrap_output = OUTPUT / "terminal-start-enter.evdev"
    icon_output = OUTPUT / "ic_desktop_dcjy.png"
    terminal_output.write_bytes(terminal)
    launcher_output.write_bytes(launcher)
    shutil.copy2(ROOT / "scripts" / "terminal-launch.sh", wrapper_output)
    bootstrap_output.write_bytes(
        b"".join(
            (
                evdev_event(1, 28, 1),
                evdev_event(0, 0, 0),
                evdev_event(1, 28, 0),
                evdev_event(0, 0, 0),
            )
        )
    )
    neofetch_outputs = {
        "command": OUTPUT / "neofetch",
        "upstream": OUTPUT / "neofetch.upstream",
        "config": OUTPUT / "c1-config.conf",
        "logo": OUTPUT / "c1-logo.txt",
        "license": OUTPUT / "neofetch-LICENSE.md",
    }
    neofetch_sources = {
        "command": ROOT / "third_party" / "neofetch" / "neofetch",
        "upstream": ROOT / "third_party" / "neofetch" / "neofetch.upstream",
        "config": ROOT / "third_party" / "neofetch" / "c1-config.conf",
        "logo": ROOT / "third_party" / "neofetch" / "c1-logo.txt",
        "license": ROOT / "third_party" / "neofetch" / "LICENSE.md",
    }
    for name, output in neofetch_outputs.items():
        output.write_bytes(read_lf(neofetch_sources[name]))

    manifest = {
        "device": "C1-Slim / MP-D261",
        "stock_launcher_sha256": expected_launcher_hash,
        "patched_launcher_sha256": digest(launcher),
        "terminal_source_sha256": expected_terminal_hash,
        "terminal_sha256": digest(terminal),
        "wrapper_sha256": digest(wrapper_output.read_bytes()),
        "bootstrap_event_sha256": digest(bootstrap_output.read_bytes()),
        "neofetch_sha256": {
            name: digest(output.read_bytes())
            for name, output in neofetch_outputs.items()
        },
        "icon_sha256": digest(icon_output.read_bytes()),
        "launcher_entry": "T Terminal (first slot)",
        "desktop_shortcuts": "TWEASD",
        "second_entry": "W 查词译句",
        "device_paths": {
            "terminal": "/usr/data/terminal/Terminal",
            "wrapper": "/usr/data/t",
            "bootstrap_event": "/usr/data/terminal/start-enter.evdev",
            "neofetch": "/usr/data/c1/bin/neofetch",
            "neofetch_data": "/usr/data/c1/neofetch",
            "launcher": "/usr/bin/d261/mpenMain",
            "backup": "/usr/data/terminal/recovery/mpenMain.stock",
        },
    }
    (OUTPUT / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(manifest, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
