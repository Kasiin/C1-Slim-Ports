#!/bin/sh
set -eu
stage=/storage/c1bible-stage
appdir=/storage/c1bible
recovery=/storage/c1/recovery/bible
launcher=/usr/bin/d261/mpenMain
icon=/usr/bin/d261/assets/images/ic_desktop_cwxz.png
. "$stage/manifest.env"
hash(){ sha256sum "$1" | cut -d' ' -f1; }
check(){ [ "$(hash "$1")" = "$2" ]; }
check "$stage/mpenMain.bible" "$LAUNCHER"
check "$stage/ic_desktop_cwxz.png" "$ICON"
check "$stage/c1bible" "$APP"
check "$stage/bible.dat" "$DATA"
check "$stage/font15.bin" "$FONT"
check "$stage/width15.bin" "$WIDTH"
check "$stage/launch-bible.sh" "$WRAPPER"
current=$(hash "$launcher")
[ "$current" = "$BASE" ] || [ "$current" = "$LAUNCHER" ] || { echo 'Unsupported launcher; refusing overwrite'; exit 1; }
current_icon=$(hash "$icon")
[ "$current_icon" = "$BASE_ICON" ] || [ "$current_icon" = "$ICON" ] || exit 1
for proc in c1lavax c1term c1sudoku c1news c1bible; do
    if pidof "$proc" >/dev/null; then echo 'Exit active custom apps before installation'; exit 1; fi
done
if [ -e /usr/data/h ]; then
    [ -f "$appdir/manifest.env" ] && check /usr/data/h "$WRAPPER" || { echo 'Existing /usr/data/h is not this integration'; exit 1; }
fi
mkdir -p "$recovery" "$appdir"
if [ ! -f "$recovery/mpenMain.pre-bible" ]; then
    [ "$current" = "$BASE" ]
    cp "$launcher" "$recovery/mpenMain.pre-bible"
    cp "$icon" "$recovery/ic_desktop_cwxz.pre-bible.png"
fi
check "$recovery/mpenMain.pre-bible" "$BASE"
check "$recovery/ic_desktop_cwxz.pre-bible.png" "$BASE_ICON"
for name in c1bible bible.dat font15.bin width15.bin; do
    cp "$stage/$name" "$appdir/$name.new"
done
chmod 755 "$appdir/c1bible.new"
check "$appdir/c1bible.new" "$APP"
check "$appdir/bible.dat.new" "$DATA"
check "$appdir/font15.bin.new" "$FONT"
check "$appdir/width15.bin.new" "$WIDTH"
for name in c1bible bible.dat font15.bin width15.bin; do mv "$appdir/$name.new" "$appdir/$name"; done
cp "$stage/launch-bible.sh" /usr/data/h.new
chmod 755 /usr/data/h.new
check /usr/data/h.new "$WRAPPER"
mv /usr/data/h.new /usr/data/h
changed=0
daemon_stopped=0
cleanup(){
    rc=$?
    trap - 0 1 2 15
    if [ "$rc" -ne 0 ] && [ "$changed" -eq 1 ]; then
        mount -o remount,rw / || true
        cp "$recovery/mpenMain.pre-bible" "$launcher" || true
        cp "$recovery/ic_desktop_cwxz.pre-bible.png" "$icon" || true
        chmod 755 "$launcher" || true
    fi
    sync
    mount -o remount,ro / || true
    if [ "$daemon_stopped" -eq 1 ]; then /bin/busybox start-stop-daemon -S -b -x /etc/app_daemon; fi
    exit "$rc"
}
trap cleanup 0 1 2 15
daemon_stopped=1
/etc/init.d/S80app stop >/dev/null 2>&1 || true
i=0
while pidof mpenMain >/dev/null; do
    [ "$i" -lt 10 ] || exit 1
    sleep 1;i=$((i+1))
done
mount -o remount,rw /
changed=1
cp "$stage/mpenMain.bible" "$launcher"
cp "$stage/ic_desktop_cwxz.png" "$icon"
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
echo 'Installed 圣经 in slot 6 / H. Original launcher and icon backed up.'
