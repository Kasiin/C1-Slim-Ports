#!/bin/sh
set -eu
stage=/usr/data/c1sudoku-stage
appdir=/storage/c1sudoku
recovery=/storage/c1/recovery/sudoku
launcher=/usr/bin/d261/mpenMain
icon=/usr/bin/d261/assets/images/ic_desktop_dctx.png
. "$stage/manifest.env"
hash(){ sha256sum "$1" | cut -d' ' -f1; }
check(){ [ "$(hash "$1")" = "$2" ]; }
check "$stage/mpenMain.sudoku" "$LAUNCHER"
check "$stage/ic_desktop_dctx.png" "$ICON"
check "$stage/c1sudoku" "$APP"
check "$stage/launch-sudoku.sh" "$WRAPPER"
current=$(hash "$launcher")
[ "$current" = "$BASE" ] || [ "$current" = "$LAUNCHER" ] || { echo 'Unsupported launcher; refusing overwrite'; exit 1; }
current_icon=$(hash "$icon")
[ "$current_icon" = "$BASE_ICON" ] || [ "$current_icon" = "$ICON" ] || exit 1
for proc in c1lavax c1term c1sudoku; do
    if pidof "$proc" >/dev/null; then echo 'Exit active custom apps before installation'; exit 1; fi
done
if [ -e /usr/data/s ]; then
    [ -f "$appdir/manifest.env" ] && check /usr/data/s "$WRAPPER" || { echo 'Existing /usr/data/s is not this integration'; exit 1; }
fi
mkdir -p "$recovery" "$appdir"
if [ ! -f "$recovery/mpenMain.pre-sudoku" ]; then
    [ "$current" = "$BASE" ]
    cp "$launcher" "$recovery/mpenMain.pre-sudoku"
    cp "$icon" "$recovery/ic_desktop_dctx.pre-sudoku.png"
fi
check "$recovery/mpenMain.pre-sudoku" "$BASE"
check "$recovery/ic_desktop_dctx.pre-sudoku.png" "$BASE_ICON"
changed=0
daemon_stopped=0
cleanup(){
    rc=$?
    trap - 0 1 2 15
    if [ "$rc" -ne 0 ] && [ "$changed" -eq 1 ]; then
        mount -o remount,rw / || true
        cp "$recovery/mpenMain.pre-sudoku" "$launcher" || true
        cp "$recovery/ic_desktop_dctx.pre-sudoku.png" "$icon" || true
        chmod 755 "$launcher" || true
    fi
    sync
    mount -o remount,ro / || true
    if [ "$daemon_stopped" -eq 1 ]; then /bin/busybox start-stop-daemon -S -b -x /etc/app_daemon; fi
    exit "$rc"
}
trap cleanup 0 1 2 15
cp "$stage/c1sudoku" "$appdir/c1sudoku.new"
chmod 755 "$appdir/c1sudoku.new"
check "$appdir/c1sudoku.new" "$APP"
mv "$appdir/c1sudoku.new" "$appdir/c1sudoku"
cp "$stage/launch-sudoku.sh" /usr/data/s.new
chmod 755 /usr/data/s.new
check /usr/data/s.new "$WRAPPER"
mv /usr/data/s.new /usr/data/s
daemon_stopped=1
/etc/init.d/S80app stop >/dev/null 2>&1 || true
i=0
while pidof mpenMain >/dev/null; do
    [ "$i" -lt 10 ] || exit 1
    sleep 1;i=$((i+1))
done
mount -o remount,rw /
changed=1
cp "$stage/mpenMain.sudoku" "$launcher"
cp "$stage/ic_desktop_dctx.png" "$icon"
chmod 755 "$launcher"
chmod 644 "$icon"
check "$launcher" "$LAUNCHER"
check "$icon" "$ICON"
cp "$stage/manifest.env" "$appdir/manifest.env"
sync
mount -o remount,ro /
/bin/busybox start-stop-daemon -S -b -x /etc/app_daemon
daemon_stopped=0
sleep 2
pidof mpenMain >/dev/null
changed=0
echo 'Installed Sudoku in slot 3 / S; DIY remapped to Z. Original launcher and icon backed up.'
