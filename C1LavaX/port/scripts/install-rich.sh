#!/bin/sh
set -eu

stage=/usr/data/wawa-rich-stage
install_dir=/storage/c1rich
recovery=/storage/c1/recovery/wawa-rich
launcher=/usr/bin/d261/mpenMain
icon=/usr/bin/d261/assets/images/ic_desktop_ccyj.png
wrapper=/usr/data/w
root_writable=0
root_files_started=0
original_stopped=0

. "$stage/manifest.env"

hash_file() {
    sha256sum "$1" | while IFS=' ' read -r value remainder; do
        printf '%s\n' "$value"
        break
    done
}

require_hash() {
    path=$1
    expected=$2
    [ -f "$path" ] || {
        echo "missing file: $path" >&2
        return 1
    }
    actual=$(hash_file "$path")
    [ "$actual" = "$expected" ] || {
        echo "hash mismatch: $path expected=$expected actual=$actual" >&2
        return 1
    }
}

root_is_read_only() {
    while IFS=' ' read -r device mountpoint filesystem options remainder; do
        [ "$mountpoint" = / ] || continue
        case ",$options," in
            *,ro,*) return 0 ;;
            *) return 1 ;;
        esac
    done < /proc/mounts
    return 1
}

remount_root_ro() {
    sync
    /bin/mount -o remount,ro / 2>/dev/null || true
    root_is_read_only
    root_writable=0
}

start_original() {
    /bin/busybox start-stop-daemon -S -b -x /etc/app_daemon
    original_stopped=0
}

cleanup() {
    status=$?
    trap - 0 1 2 15
    if [ "$status" -ne 0 ] && [ "$root_files_started" -eq 1 ]; then
        if root_is_read_only; then
            /bin/mount -o remount,rw / 2>/dev/null || true
        fi
        if ! root_is_read_only; then
            cp "$recovery/mpenMain.pre-rich" "$launcher" || true
            cp "$recovery/ic_desktop_ccyj.pre-rich.png" "$icon" || true
            chmod 755 "$launcher" || true
            chmod 644 "$icon" || true
            sync || true
        fi
    fi
    if ! root_is_read_only; then
        root_writable=1
    fi
    [ "$root_writable" -eq 0 ] || remount_root_ro || true
    [ "$original_stopped" -eq 0 ] || start_original || true
    exit "$status"
}

trap cleanup 0 1 2 15

require_hash "$stage/c1lavax" "$RUNTIME_SHA256"
require_hash "$stage/evsend" "$EVENT_SENDER_SHA256"
require_hash "$stage/LVM.bin" "$FONT_SHA256"
require_hash "$stage/os/Rich.lav" "$PROGRAM_SHA256"
require_hash "$stage/os/LavaData/RichMap.dat" "$MAP_SHA256"
require_hash "$stage/os/LavaData/RichPic.dat" "$PICTURE_SHA256"
require_hash "$stage/launch-rich.sh" "$WRAPPER_SHA256"
require_hash "$stage/mpenMain.rich" "$PATCHED_LAUNCHER_SHA256"
require_hash "$stage/ic_desktop_ccyj.png" "$PATCHED_ICON_SHA256"

current_launcher=$(hash_file "$launcher")
[ "$current_launcher" = "$BASE_LAUNCHER_SHA256" ] || \
    [ "$current_launcher" = "$PATCHED_LAUNCHER_SHA256" ] || {
        echo "unsupported launcher hash: $current_launcher" >&2
        exit 1
    }
current_icon=$(hash_file "$icon")
[ "$current_icon" = "$BASE_ICON_SHA256" ] || \
    [ "$current_icon" = "$PATCHED_ICON_SHA256" ] || {
        echo "unsupported second-icon hash: $current_icon" >&2
        exit 1
    }

mkdir -p "$install_dir/os/LavaData" "$recovery"
if [ ! -e "$recovery/mpenMain.pre-rich" ]; then
    [ "$current_launcher" = "$BASE_LAUNCHER_SHA256" ]
    cp "$launcher" "$recovery/mpenMain.pre-rich"
fi
if [ ! -e "$recovery/ic_desktop_ccyj.pre-rich.png" ]; then
    [ "$current_icon" = "$BASE_ICON_SHA256" ]
    cp "$icon" "$recovery/ic_desktop_ccyj.pre-rich.png"
fi
require_hash "$recovery/mpenMain.pre-rich" "$BASE_LAUNCHER_SHA256"
require_hash "$recovery/ic_desktop_ccyj.pre-rich.png" "$BASE_ICON_SHA256"

install_one() {
    source=$1
    target=$2
    mode=$3
    expected=$4
    cp "$source" "$target.new.$$"
    chmod "$mode" "$target.new.$$"
    require_hash "$target.new.$$" "$expected"
    mv "$target.new.$$" "$target"
}

install_one "$stage/c1lavax" "$install_dir/c1lavax" 755 "$RUNTIME_SHA256"
install_one "$stage/evsend" "$install_dir/evsend" 755 "$EVENT_SENDER_SHA256"
install_one "$stage/LVM.bin" "$install_dir/LVM.bin" 644 "$FONT_SHA256"
install_one "$stage/os/Rich.lav" "$install_dir/os/Rich.lav" 644 "$PROGRAM_SHA256"
install_one "$stage/os/LavaData/RichMap.dat" "$install_dir/os/LavaData/RichMap.dat" 644 "$MAP_SHA256"
install_one "$stage/os/LavaData/RichPic.dat" "$install_dir/os/LavaData/RichPic.dat" 644 "$PICTURE_SHA256"
install_one "$stage/launch-rich.sh" "$wrapper" 755 "$WRAPPER_SHA256"

original_stopped=1
/etc/init.d/S80app stop >/dev/null 2>&1 || true
count=0
while [ "$count" -lt 10 ] && { pidof mpenMain >/dev/null 2>&1 || pidof app_daemon >/dev/null 2>&1; }; do
    sleep 1
    count=$((count + 1))
done
! pidof mpenMain >/dev/null 2>&1
! pidof app_daemon >/dev/null 2>&1

/bin/mount -o remount,rw /
root_writable=1
root_files_started=1
install_one "$stage/mpenMain.rich" "$launcher" 755 "$PATCHED_LAUNCHER_SHA256"
install_one "$stage/ic_desktop_ccyj.png" "$icon" 644 "$PATCHED_ICON_SHA256"
sync
remount_root_ro

start_original
count=0
while [ "$count" -lt 10 ] && ! pidof mpenMain >/dev/null 2>&1; do
    sleep 1
    count=$((count + 1))
done
pidof mpenMain >/dev/null 2>&1

require_hash "$launcher" "$PATCHED_LAUNCHER_SHA256"
require_hash "$icon" "$PATCHED_ICON_SHA256"
require_hash "$install_dir/c1lavax" "$RUNTIME_SHA256"
require_hash "$install_dir/evsend" "$EVENT_SENDER_SHA256"
require_hash "$wrapper" "$WRAPPER_SHA256"
root_is_read_only

printf '%s\n' \
    "launcher_sha256=$PATCHED_LAUNCHER_SHA256" \
    "icon_sha256=$PATCHED_ICON_SHA256" \
    "runtime_sha256=$RUNTIME_SHA256" \
    "program_sha256=$PROGRAM_SHA256" \
    "shortcut=W" \
    "label=蛙蛙富翁" \
    > "$install_dir/install-state.txt"
sync

root_files_started=0
echo 'Wawa Rich installed in launcher slot 2 (W)'
exit 0
