#!/bin/sh
set -u
app=/storage/c1bible/c1bible
log=/storage/c1bible/bible.log
lock=/tmp/c1bible.lock
frame=$lock/launcher.frame
launcher_pids=
child=
suspended=0
fast=/sys/devices/platform/e0266a128/epaper/fast_refresh_only
refresh_max=/sys/devices/platform/e0266a128/epaper/refresh_max
saved_fast=
saved_max=

mkdir "$lock" 2>/dev/null || exit 0
cleanup() {
    status=$?
    trap - 0 1 2 15
    if [ -n "$child" ] && kill -0 "$child" 2>/dev/null; then
        kill "$child" 2>/dev/null || true
        wait "$child" 2>/dev/null || true
    fi
    if [ -f "$frame" ] && [ "$(stat -c %s "$frame")" = 5624 ]; then
        dd if="$frame" of=/dev/epaper_lcd bs=5624 count=1 2>/dev/null || true
    fi
    [ -z "$saved_max" ] || printf '%s' "$saved_max" > "$refresh_max"
    [ -z "$saved_fast" ] || printf '%s' "$saved_fast" > "$fast"
    if [ "$suspended" -eq 1 ]; then kill -CONT $launcher_pids 2>/dev/null || true; fi
    rm -f "$frame"
    rmdir "$lock" 2>/dev/null || true
    exit "$status"
}
trap cleanup 0 1 2 15
[ -x "$app" ] || exit 1
launcher_pids=$(pidof mpenMain 2>/dev/null || true)
[ -n "$launcher_pids" ] || exit 1
suspended=1
kill -STOP $launcher_pids || exit 1
count=0
while [ "$count" -lt 20 ]; do
    ready=1
    for pid in $launcher_pids; do
        state=$(sed -n 's/^State:[[:space:]]*\([^[:space:]]*\).*/\1/p' "/proc/$pid/status")
        [ "$state" = T ] || ready=0
    done
    [ "$ready" -eq 0 ] || break
    sleep 0.05
    count=$((count+1))
done
[ "$ready" -eq 1 ] || exit 1
set -- $launcher_pids
dd if="/proc/$1/mem" of="$frame" bs=8 skip=1071860 count=703 2>/dev/null || exit 1
[ "$(stat -c %s "$frame")" = 5624 ] || exit 1
IFS= read -r saved_fast < "$fast"
IFS= read -r saved_max < "$refresh_max"
printf 1 > "$fast"
printf 100 > "$refresh_max"
printf '%s START\n' "$(date -Iseconds)" >> "$log"
"$app" >> "$log" 2>&1 &
child=$!
wait "$child"
status=$?
child=
printf '%s STOP status=%s\n' "$(date -Iseconds)" "$status" >> "$log"
exit "$status"
