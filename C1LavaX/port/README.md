# C1LavaX for C1-Slim

This port runs LavaX programs directly on the C1-Slim's 296×152 one-bit
e-paper display. Rendering is aspect-preserving, nearest-neighbour, and strictly
black/white; panel writes are capped at 12.5 frames per second. While the game
is active, the controller is put into fast-refresh-only mode and the runtime
submits only final target frames: it never requests a slow full refresh or
inserts a whole-screen cleaning frame. The display's original
`fast_refresh_only` and `refresh_max` values are restored on exit.

For programs with typewriter effects or rapid animation, setting
`C1LAVAX_STABLE_FRAMES=1` coalesces intermediate drawing and publishes a frame
only after it has remained unchanged for 140 ms or the program reaches a
blocking key read. This mode is used by the Magic Tower integration so map
movement and dialogue transitions do not flash every temporary frame.

`C1LAVAX_MOTA_KEY_FIX=1` preserves palette index zero only inside Magic
Tower's fixed third-key status cell. The coordinate-scoped conversion keeps
that key visible without turning other coloured status artwork into black
blocks.

The VM also preserves the original LavaX absolute-pointer semantics for
`strchr` and `strstr`. This is required by resource-pack lookups whose match
starts at the first byte, including the Rich game's startup logos.

## 蛙蛙富翁 integration

`scripts/deploy-rich.ps1` cross-builds the static MIPS runtime, creates a
hash-locked launcher patch, uploads the game and its two data files, and installs
the game into launcher slot 2 with shortcut `W`.

```powershell
.\scripts\deploy-rich.ps1
```

The on-device files live under `/storage/c1rich`; `/usr/data/w` is the launcher
bridge. The stock launcher stays resident but is suspended while the game owns
the display and input devices, then resumes without showing its boot screen.
Exit through the game's own menu; Home is intentionally not a global C1LavaX
exit key. The pre-install launcher and icon are retained in
`/storage/c1/recovery/wawa-rich/`.

Controls use the physical arrow and Enter keys. Back maps to LavaX Escape,
Wakeup maps to Help, and the volume keys map to Page Up/Page Down.
