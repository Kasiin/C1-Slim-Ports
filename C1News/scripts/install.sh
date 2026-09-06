#!/bin/sh
set -eu
stage=/usr/data/c1news-stage
appdir=/storage/c1news
recovery=/storage/c1/recovery/news
launcher=/usr/bin/d261/mpenMain
icon=/usr/bin/d261/assets/images/ic_desktop_dcsg.png
. "$stage/manifest.env"
hash(){ sha256sum "$1" | cut -d' ' -f1; }
check(){ [ "$(hash "$1")" = "$2" ]; }
check "$stage/mpenMain.news" "$LAUNCHER"
check "$stage/ic_desktop_dcsg.png" "$ICON"
check "$stage/c1news" "$APP"
check "$stage/cacert.pem" "$CA"
check "$stage/launch-news.sh" "$WRAPPER"
current=$(hash "$launcher")
[ "$current" = "$BASE" ] || [ "$current" = "$PREVIOUS" ] || [ "$current" = "$LAUNCHER" ] || { echo 'Unsupported launcher; refusing overwrite'; exit 1; }
current_icon=$(hash "$icon")
[ "$current_icon" = "$BASE_ICON" ] || [ "$current_icon" = "$ICON" ] || exit 1
for proc in c1lavax c1term c1sudoku c1news; do
    if pidof "$proc" >/dev/null; then echo 'Exit custom apps before installation'; exit 1; fi
done
if [ -e /usr/data/d ]; then
    [ -f "$appdir/manifest.env" ] && check /usr/data/d "$WRAPPER" || { echo 'Unrelated /usr/data/d exists'; exit 1; }
fi
mkdir -p "$recovery" "$appdir"
if [ ! -f "$recovery/mpenMain.pre-news" ]; then
    [ "$current" = "$BASE" ]
    cp "$launcher" "$recovery/mpenMain.pre-news"
    cp "$icon" "$recovery/ic_desktop_dcsg.pre-news.png"
    [ ! -f "$appdir/c1news" ] || cp "$appdir/c1news" "$recovery/c1news.preview"
fi
check "$recovery/mpenMain.pre-news" "$BASE"
check "$recovery/ic_desktop_dcsg.pre-news.png" "$BASE_ICON"
# Per-attempt snapshots also support a safe retry of this exact integration.
cp "$launcher" "$recovery/launcher.before-install"
cp "$icon" "$recovery/icon.before-install.png"
changed=0
daemon_stopped=0
cleanup(){
    rc=$?
    trap - 0 1 2 15
    if [ "$rc" -ne 0 ] && [ "$changed" -eq 1 ]; then
        mount -o remount,rw / || true
        cp "$recovery/launcher.before-install" "$launcher" || true
        cp "$recovery/icon.before-install.png" "$icon" || true
        chmod 755 "$launcher" || true
    fi
    sync
    mount -o remount,ro / || true
    if [ "$daemon_stopped" -eq 1 ]; then /bin/busybox start-stop-daemon -S -b -x /etc/app_daemon; fi
    exit "$rc"
}
trap cleanup 0 1 2 15
cp "$stage/c1news" "$appdir/c1news.new"
chmod 755 "$appdir/c1news.new"
check "$appdir/c1news.new" "$APP"
mv "$appdir/c1news.new" "$appdir/c1news"
cp "$stage/cacert.pem" "$appdir/cacert.pem.new"
check "$appdir/cacert.pem.new" "$CA"
mv "$appdir/cacert.pem.new" "$appdir/cacert.pem"
cp "$stage/launch-news.sh" /usr/data/d.new
chmod 755 /usr/data/d.new
check /usr/data/d.new "$WRAPPER"
mv /usr/data/d.new /usr/data/d
cp /usr/data/d "$appdir/launch-news.sh"
daemon_stopped=1
/etc/init.d/S80app stop >/dev/null 2>&1 || true
i=0
while pidof mpenMain >/dev/null; do
    [ "$i" -lt 10 ] || exit 1
    sleep 1;i=$((i+1))
done
mount -o remount,rw /
changed=1
cp "$stage/mpenMain.news" "$launcher"
cp "$stage/ic_desktop_dcsg.png" "$icon"
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
echo 'Installed News in slot 4 / D; slot 6 remapped to A. Existing custom apps preserved.'
