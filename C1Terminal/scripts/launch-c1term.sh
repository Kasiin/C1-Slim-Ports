#!/bin/sh
set -u

terminal=/usr/data/c1term/c1term
log=/usr/data/c1term/c1term.log
lock=/tmp/c1term.lock
terminal_pid=
launcher_pids=
launcher_suspended=0
launcher_frame=/tmp/c1term-launcher.frame

suspend_launcher() {
    launcher_pids=$(pidof mpenMain 2>/dev/null || true)
    [ -n "$launcher_pids" ] || return 1
    if ! kill -STOP $launcher_pids 2>/dev/null; then
        launcher_pids=
        return 1
    fi
    launcher_suspended=1

    count=0
    while [ "$count" -lt 20 ]; do
        all_stopped=1
        for pid in $launcher_pids; do
            state=$(sed -n 's/^State:[[:space:]]*\([^[:space:]]*\).*/\1/p' "/proc/$pid/status" 2>/dev/null || true)
            [ "$state" = T ] || all_stopped=0
        done
        [ "$all_stopped" -eq 0 ] || return 0
        sleep 0.05
        count=$((count + 1))
    done
    return 0
}

resume_launcher() {
    [ "$launcher_suspended" -eq 1 ] || return 0
    if [ -n "$launcher_pids" ]; then
        kill -CONT $launcher_pids 2>/dev/null || true
    fi
    launcher_suspended=0
    launcher_pids=
}

capture_launcher_frame() {
    rm -f "$launcher_frame"
    set -- $launcher_pids
    [ "$#" -gt 0 ] || return 1
    dd if="/proc/$1/mem" of="$launcher_frame" bs=8 skip=1071860 count=703 2>/dev/null || return 1
    [ "$(stat -c %s "$launcher_frame" 2>/dev/null || true)" = 5624 ] || return 1
}

restore_launcher_frame() {
    if [ -r "$launcher_frame" ]; then
        dd if="$launcher_frame" of=/dev/epaper_lcd bs=5624 count=1 2>/dev/null || true
    fi
    rm -f "$launcher_frame"
}

cleanup() {
    status=$?
    trap - 0 1 2 15
    if [ -n "$terminal_pid" ] && kill -0 "$terminal_pid" 2>/dev/null; then
        kill "$terminal_pid" 2>/dev/null || true
        wait "$terminal_pid" 2>/dev/null || true
    fi
    restore_launcher_frame
    resume_launcher
    rmdir "$lock" 2>/dev/null || true
    exit "$status"
}

trap cleanup 0 1 2 15

if ! mkdir "$lock" 2>/dev/null; then
    exit 0
fi
if [ ! -x "$terminal" ]; then
    printf '%s\n' 'standalone C1Terminal executable is missing' >> "$log"
    exit 1
fi

if ! suspend_launcher; then
    printf '%s\n' 'Could not suspend the stock launcher' >> "$log"
    exit 1
fi
if ! capture_launcher_frame; then
    printf '%s\n' 'Could not capture the stock launcher frame' >> "$log"
fi

printf '%s START standalone=true\n' "$(date -Iseconds 2>/dev/null || date)" >> "$log"
"$terminal" >> "$log" 2>&1 &
terminal_pid=$!
wait "$terminal_pid" || true
terminal_pid=
printf '%s STOP\n' "$(date -Iseconds 2>/dev/null || date)" >> "$log"
exit 0
