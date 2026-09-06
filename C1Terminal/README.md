# C1Terminal

Standalone graphical terminal for the C1-Slim / MP-D261 e-paper device.

It starts the device's own `/bin/bash` in a PTY, parses VT100/xterm output,
renders 49 columns by 18 rows on `/dev/epaper_lcd`, exclusively reads the
physical keyboard from `/dev/input/event0` and `/dev/input/event1`, and exits
immediately to its caller when HOME is pressed.

The executable contains no C1ancher desktop, settings, Wi-Fi UI, package manager,
service supervisor, or application launcher. See `NOTICE.md` for attribution and
third-party licenses.

## Build

Use Go 1.25 or newer. No C compiler is needed.

```powershell
$env:GOOS = 'linux'
$env:GOARCH = 'mipsle'
$env:GOMIPS = 'hardfloat'
$env:CGO_ENABLED = '0'
go build -trimpath -ldflags '-s -w -buildid=' -o build/c1term ./cmd/c1term
```

On the device, `/usr/data/t` is the stock-launcher bridge: it temporarily
suspends the existing `mpenMain`, runs `/usr/data/c1term/c1term`, then resumes
that same launcher process after HOME or shell exit. It does not restart
`app_daemon`, so returning to the desktop does not show the initialization page.
