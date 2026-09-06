#!/bin/sh
set -u

install_dir=/storage/c1rich
runtime=$install_dir/c1lavax
root=$install_dir/os
font=$install_dir/LVM.bin
program=/Rich.lav
log=$install_dir/rich.log
lock=/tmp/c1rich.lock
game_pid=
launcher_pids=
launcher_suspended=0
launcher_frame=/tmp/c1rich-launcher.frame
display_refresh_max=/sys/devices/platform/e0266a128/epaper/refresh_max
display_fast_only=/sys/devices/platform/e0266a128/epaper/fast_refresh_only
saved_refresh_max=
saved_fast_only=
refresh_max_tuned=0
fast_only_tuned=0

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
    # mpenMain is a fixed-address MIPS executable. Its 5640-byte LVGL epaper
    # buffer starts at 0x82d790 with a 16-byte area header; the remaining 5624
    # bytes are the exact panel frame which was visible before this app opened.
    dd if="/proc/$1/mem" of="$launcher_frame" bs=8 skip=1071860 count=703 2>/dev/null || return 1
    [ "$(stat -c %s "$launcher_frame" 2>/dev/null || true)" = 5624 ] || return 1
}

restore_launcher_frame() {
    if [ -r "$launcher_frame" ]; then
        dd if="$launcher_frame" of=/dev/epaper_lcd bs=5624 count=1 2>/dev/null || true
    fi
    rm -f "$launcher_frame"
}

tune_display() {
    if [ -r "$display_fast_only" ] && [ -w "$display_fast_only" ]; then
        IFS= read -r saved_fast_only < "$display_fast_only"
        if printf '%s' 1 > "$display_fast_only"; then
            fast_only_tuned=1
        fi
    fi
    if [ -r "$display_refresh_max" ] && [ -w "$display_refresh_max" ]; then
        IFS= read -r saved_refresh_max < "$display_refresh_max"
        if printf '%s' 100 > "$display_refresh_max"; then
            refresh_max_tuned=1
        fi
    fi
}

restore_display() {
    if [ "$refresh_max_tuned" -eq 1 ]; then
        printf '%s' "$saved_refresh_max" > "$display_refresh_max" 2>/dev/null || true
        refresh_max_tuned=0
    fi
    if [ "$fast_only_tuned" -eq 1 ]; then
        printf '%s' "$saved_fast_only" > "$display_fast_only" 2>/dev/null || true
        fast_only_tuned=0
    fi
}

cleanup() {
    status=$?
    trap - 0 1 2 15
    if [ -n "$game_pid" ] && kill -0 "$game_pid" 2>/dev/null; then
        kill "$game_pid" 2>/dev/null || true
        wait "$game_pid" 2>/dev/null || true
    fi
    restore_display
    restore_launcher_frame
    resume_launcher
    rmdir "$lock" 2>/dev/null || true
    exit "$status"
}

trap cleanup 0 1 2 15

if ! mkdir "$lock" 2>/dev/null; then
    exit 0
fi
if [ ! -x "$runtime" ] || [ ! -r "$font" ] || [ ! -r "$root$program" ]; then
    printf '%s\n' 'Wawa Rich installation is incomplete' >> "$log"
    exit 1
fi

if ! suspend_launcher; then
    printf '%s\n' 'Could not suspend the stock launcher' >> "$log"
    exit 1
fi
if ! capture_launcher_frame; then
    printf '%s\n' 'Could not capture the stock launcher frame' >> "$log"
fi
tune_display

printf '%s START program=%s\n' "$(date -Iseconds 2>/dev/null || date)" "$program" >> "$log"
C1LAVAX_ROOT="$root" C1LAVAX_FONT="$font" "$runtime" "$program" >> "$log" 2>&1 &
game_pid=$!
wait "$game_pid"
status=$?
game_pid=
printf '%s STOP status=%s\n' "$(date -Iseconds 2>/dev/null || date)" "$status" >> "$log"
exit 0
