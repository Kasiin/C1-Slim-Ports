#!/bin/sh
set -u

lavax=/storage/c1lavax/c1lavax
log=/storage/c1lavax/c1lavax.log
lock=/tmp/c1lavax.lock
lavax_pid=
original_stopped=0
terminal_stopped=0

app_daemon_running() {
    for cmdline in /proc/[0-9]*/cmdline; do
        [ -r "$cmdline" ] || continue
        command_line=$(tr '\000' ' ' < "$cmdline")
        case "$command_line" in
            *"/etc/app_daemon"*) return 0 ;;
        esac
    done
    return 1
}

start_original() {
    if ! app_daemon_running; then
        /bin/busybox start-stop-daemon -S -b -x /etc/app_daemon
    fi
    original_stopped=0
}

resume_terminal() {
    terminal_pids=$(pidof c1term 2>/dev/null || true)
    [ -z "$terminal_pids" ] || kill -CONT $terminal_pids 2>/dev/null || true
    terminal_stopped=0
}

cleanup() {
    status=$?
    trap - 0 1 2 15
    if [ -n "$lavax_pid" ] && kill -0 "$lavax_pid" 2>/dev/null; then
        kill "$lavax_pid" 2>/dev/null || true
        wait "$lavax_pid" 2>/dev/null || true
    fi
    [ "$terminal_stopped" -eq 0 ] || resume_terminal
    [ "$original_stopped" -eq 0 ] || start_original || true
    rmdir "$lock" 2>/dev/null || true
    exit "$status"
}

trap cleanup 0 1 2 15

if ! mkdir "$lock" 2>/dev/null; then
    exit 0
fi
if [ ! -x "$lavax" ]; then
    printf '%s\n' 'standalone C1LavaX executable is missing' >> "$log"
    exit 1
fi

terminal_pids=$(pidof c1term 2>/dev/null || true)
if [ -n "$terminal_pids" ]; then
    kill -STOP $terminal_pids 2>/dev/null || true
    terminal_stopped=1
elif app_daemon_running || pidof mpenMain >/dev/null 2>&1; then
    original_stopped=1
    /etc/init.d/S80app stop >/dev/null 2>&1 || true
    count=0
    while [ "$count" -lt 10 ] && { app_daemon_running || pidof mpenMain >/dev/null 2>&1; }; do
        sleep 1
        count=$((count + 1))
    done
fi

printf '%s START standalone=true args=%s\n' "$(date -Iseconds 2>/dev/null || date)" "$*" >> "$log"
"$lavax" "$@" >> "$log" 2>&1 &
lavax_pid=$!
wait "$lavax_pid" || true
lavax_pid=
printf '%s STOP\n' "$(date -Iseconds 2>/dev/null || date)" >> "$log"
exit 0
