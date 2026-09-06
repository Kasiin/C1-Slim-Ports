#!/bin/sh
set -eu

stage=/usr/data/c1mota-stage
appdir=/storage/c1mota
recovery=/storage/c1/recovery/mota
launcher=/usr/bin/d261/mpenMain
icon=/usr/bin/d261/assets/images/ic_desktop_zjcs.png
. "$stage/manifest.env"

hash() { sha256sum "$1" | cut -d' ' -f1; }
check() { [ "$(hash "$1")" = "$2" ]; }

check "$stage/mpenMain.mota" "$LAUNCHER"
check "$stage/ic_desktop_zjcs.png" "$ICON"
check "$stage/c1lavax" "$RUNTIME"
check "$stage/LVM.bin" "$FONT"
check "$stage/Mota.lav" "$PROGRAM"
check "$stage/MOTA.dat" "$DATA"
check "$stage/launch-mota.sh" "$WRAPPER"

current=$(hash "$launcher")
[ "$current" = "$BASE" ] || [ "$current" = "$PREVIOUS" ] || [ "$current" = "$LAUNCHER" ] || {
    echo 'Unsupported launcher; refusing overwrite'
    exit 1
}
current_icon=$(hash "$icon")
[ "$current_icon" = "$BASE_ICON" ] || [ "$current_icon" = "$ICON" ] || {
    echo 'Unsupported slot-5 icon; refusing overwrite'
    exit 1
}

for proc in c1lavax c1term c1sudoku c1news; do
    if pidof "$proc" >/dev/null 2>&1; then
        echo 'Exit custom apps before installation'
        exit 1
    fi
done
if [ -e /usr/data/m ]; then
    existing_bridge=$(hash /usr/data/m)
    [ -f "$appdir/manifest.env" ] &&
        { [ "$existing_bridge" = "$PREVIOUS_WRAPPER" ] || [ "$existing_bridge" = "$WRAPPER" ]; } || {
        echo 'Unrelated /usr/data/m exists'
        exit 1
    }
fi

mkdir -p "$recovery" "$appdir/os/LavaData"
if [ ! -f "$recovery/mpenMain.pre-mota" ]; then
    [ "$current" = "$BASE" ]
    cp "$launcher" "$recovery/mpenMain.pre-mota"
    cp "$icon" "$recovery/ic_desktop_zjcs.pre-mota.png"
fi
check "$recovery/mpenMain.pre-mota" "$BASE"
check "$recovery/ic_desktop_zjcs.pre-mota.png" "$BASE_ICON"
cp "$launcher" "$recovery/launcher.before-install"
cp "$icon" "$recovery/icon.before-install.png"

changed=0
daemon_stopped=0
cleanup() {
    rc=$?
    trap - 0 1 2 15
    if [ "$rc" -ne 0 ] && [ "$changed" -eq 1 ]; then
        mount -o remount,rw / || true
        cp "$recovery/launcher.before-install" "$launcher" || true
        cp "$recovery/icon.before-install.png" "$icon" || true
        chmod 755 "$launcher" || true
        chmod 644 "$icon" || true
    fi
    sync
    mount -o remount,ro / || true
    if [ "$daemon_stopped" -eq 1 ]; then
        /bin/busybox start-stop-daemon -S -b -x /etc/app_daemon
    fi
    exit "$rc"
}
trap cleanup 0 1 2 15

cp "$stage/c1lavax" "$appdir/c1lavax.new"
chmod 755 "$appdir/c1lavax.new"
check "$appdir/c1lavax.new" "$RUNTIME"
mv "$appdir/c1lavax.new" "$appdir/c1lavax"
cp "$stage/LVM.bin" "$appdir/LVM.bin.new"
check "$appdir/LVM.bin.new" "$FONT"
mv "$appdir/LVM.bin.new" "$appdir/LVM.bin"
cp "$stage/Mota.lav" "$appdir/os/Mota.lav.new"
check "$appdir/os/Mota.lav.new" "$PROGRAM"
mv "$appdir/os/Mota.lav.new" "$appdir/os/Mota.lav"
cp "$stage/MOTA.dat" "$appdir/os/LavaData/MOTA.dat.new"
check "$appdir/os/LavaData/MOTA.dat.new" "$DATA"
mv "$appdir/os/LavaData/MOTA.dat.new" "$appdir/os/LavaData/MOTA.dat"
cp "$stage/launch-mota.sh" /usr/data/m.new
chmod 755 /usr/data/m.new
check /usr/data/m.new "$WRAPPER"
mv /usr/data/m.new /usr/data/m
cp /usr/data/m "$appdir/launch-mota.sh"

daemon_stopped=1
/etc/init.d/S80app stop >/dev/null 2>&1 || true
i=0
while pidof mpenMain >/dev/null 2>&1; do
    [ "$i" -lt 10 ] || exit 1
    sleep 1
    i=$((i + 1))
done

mount -o remount,rw /
changed=1
cp "$stage/mpenMain.mota" "$launcher"
cp "$stage/ic_desktop_zjcs.png" "$icon"
chmod 755 "$launcher"
chmod 644 "$icon"
check "$launcher" "$LAUNCHER"
check "$icon" "$ICON"
cp "$stage/manifest.env" "$appdir/manifest.env"
printf '%s\n' 'Automatic launcher update checks are disabled; manual Settings update remains.' > "$appdir/update-policy.txt"
sync
mount -o remount,ro /
/bin/busybox start-stop-daemon -S -b -x /etc/app_daemon
daemon_stopped=0
sleep 2
pidof mpenMain >/dev/null
changed=0
echo 'Installed Magic Tower in slot 5 / M; automatic update prompts disabled.'
