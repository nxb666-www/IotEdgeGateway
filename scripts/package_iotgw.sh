#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="${IOTGW_BUILD_DIR:-}"
PACKAGE_DIR="${IOTGW_PACKAGE_DIR:-$ROOT_DIR/iotgw_package}"

find_build_dir() {
    if [ -n "$BUILD_DIR" ]; then
        printf "%s\n" "$BUILD_DIR"
        return
    fi

    for dir in \
        "$ROOT_DIR/build/aarch64/release" \
        "$ROOT_DIR/build/aarch64/debug" \
        "$ROOT_DIR/build/native/release" \
        "$ROOT_DIR/build/native/debug" \
        "$ROOT_DIR/build"
    do
        if [ -x "$dir/iotgw_gateway" ]; then
            printf "%s\n" "$dir"
            return
        fi
    done

    echo "ERROR: no built iotgw_gateway found. Run ./build.sh -a first." >&2
    exit 1
}

copy_dir() {
    src="$1"
    dst="$2"
    rm -rf "$dst"
    mkdir -p "$(dirname "$dst")"
    cp -r "$src" "$dst"
}

BUILD_DIR="$(find_build_dir)"

rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR/bin" "$PACKAGE_DIR/data/logs"

cp "$BUILD_DIR/iotgw_gateway" "$PACKAGE_DIR/bin/"

if [ -x "$BUILD_DIR/iotgw_mqtt_broker" ]; then
    cp "$BUILD_DIR/iotgw_mqtt_broker" "$PACKAGE_DIR/bin/"
fi

QT_CLIENT="$ROOT_DIR/qt_client/iotgw_qt_client"
if [ -x "$QT_CLIENT" ]; then
    cp "$QT_CLIENT" "$PACKAGE_DIR/bin/"
fi

copy_dir "$ROOT_DIR/config" "$PACKAGE_DIR/config"
copy_dir "$ROOT_DIR/www" "$PACKAGE_DIR/www"

cat > "$PACKAGE_DIR/start.sh" <<'EOF'
#!/bin/sh
set -eu

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$APP_DIR"
MQTT_PORT="${IOTGW_MQTT_PORT:-1883}"
HTTP_PORT="${IOTGW_HTTP_PORT:-8081}"
CONFIG_FILE="${IOTGW_CONFIG_FILE:-$APP_DIR/config/environments/production.yaml}"
LOG_DIR="${IOTGW_LOG_DIR:-$APP_DIR/data/logs}"
WIFI_SSID="${IOTGW_WIFI_SSID:-nie}"
WIFI_PASS="${IOTGW_WIFI_PASS:-12345678}"
START_QT="${IOTGW_START_QT:-0}"
START_QT_DELAY="${IOTGW_START_QT_DELAY:-20}"
QT_ROTATION="${IOTGW_QT_ROTATION:-90}"
VIDEO_ROTATION="${IOTGW_VIDEO_ROTATION:-270}"
VIDEO_WIDTH="${IOTGW_VIDEO_WIDTH:-640}"
VIDEO_FPS="${IOTGW_VIDEO_FPS:-12}"
VIDEO_QUALITY="${IOTGW_VIDEO_QUALITY:-6}"
QT_PLATFORM="${IOTGW_QT_PLATFORM:-wayland}"

mkdir -p "$LOG_DIR" "$APP_DIR/data" "$APP_DIR/data/media/photos" "$APP_DIR/data/media/videos"

ensure_wifi() {
    wait_count=0
    while [ $wait_count -lt 10 ] && ! ifconfig wlan0 >/dev/null 2>&1; do
        sleep 1
        wait_count=$((wait_count + 1))
    done

    WLAN_IP=$(ifconfig wlan0 2>/dev/null | grep "inet addr" | awk -F: '{print $2}' | awk '{print $1}')
    if [ -z "${WLAN_IP:-}" ] || iwconfig wlan0 2>/dev/null | grep -qE "ESSID:.*off/any|Not-Associated|unassociated"; then
        echo "[start.sh] WiFi not connected, connecting to $WIFI_SSID ..."
        if command -v nmcli >/dev/null 2>&1; then
            nmcli device wifi connect "$WIFI_SSID" password "$WIFI_PASS" 2>/dev/null || true
        else
            wpa_passphrase "$WIFI_SSID" "$WIFI_PASS" > /tmp/iotgw_wpa.conf
            wpa_supplicant -B -i wlan0 -c /tmp/iotgw_wpa.conf 2>/dev/null || true
            sleep 2
        fi
    fi

    WLAN_IP=$(ifconfig wlan0 2>/dev/null | grep "inet addr" | awk -F: '{print $2}' | awk '{print $1}')
    if [ -z "${WLAN_IP:-}" ]; then
        udhcpc -n -q -t 5 -T 3 -i wlan0 2>/dev/null || true
    fi

    WLAN_IP=$(ifconfig wlan0 2>/dev/null | grep "inet addr" | awk -F: '{print $2}' | awk '{print $1}')
    echo "[start.sh] WiFi IP: ${WLAN_IP:-unknown}"
}

kill_by_name() {
    name="$1"
    pids=$(ps | awk -v n="$name" '$0 ~ n && $0 !~ /awk/ {print $1}')
    if [ -n "$pids" ]; then
        kill $pids 2>/dev/null || true
        sleep 1
        pids=$(ps | awk -v n="$name" '$0 ~ n && $0 !~ /awk/ {print $1}')
        if [ -n "$pids" ]; then
            kill -9 $pids 2>/dev/null || true
        fi
    fi
}

stop_services() {
    kill_by_name iotgw_gateway
    kill_by_name iotgw_qt_client
    kill_by_name ffmpeg
    kill_by_name gst-launch-1.0
}

stop_all_services() {
    stop_services
    kill_by_name iotgw_mqtt_broker
    kill_by_name mosquitto
}

is_port_listening() {
    netstat -tln 2>/dev/null | grep -q ":$1 "
}

is_mqtt_open_for_lan() {
    netstat -tln 2>/dev/null | grep -Eq "0\.0\.0\.0:$MQTT_PORT |:::$MQTT_PORT "
}

kill_tcp_listener() {
    port_hex=$(printf "%04X" "$1")
    inodes=$(awk -v p=":$port_hex" '$2 ~ p && $4 == "0A" {print $10}' /proc/net/tcp /proc/net/tcp6 2>/dev/null || true)
    for inode in $inodes; do
        for fd in /proc/[0-9]*/fd/*; do
            target=$(readlink "$fd" 2>/dev/null || true)
            if [ "$target" = "socket:[$inode]" ]; then
                pid=${fd#/proc/}
                pid=${pid%%/*}
                kill "$pid" 2>/dev/null || true
            fi
        done
    done
    sleep 1
    for inode in $inodes; do
        for fd in /proc/[0-9]*/fd/*; do
            target=$(readlink "$fd" 2>/dev/null || true)
            if [ "$target" = "socket:[$inode]" ]; then
                pid=${fd#/proc/}
                pid=${pid%%/*}
                kill -9 "$pid" 2>/dev/null || true
            fi
        done
    done
}

start_broker() {
    if is_mqtt_open_for_lan; then
        return
    fi

    if is_port_listening "$MQTT_PORT"; then
        kill_by_name mosquitto
        kill_by_name iotgw_mqtt_broker
        kill_tcp_listener "$MQTT_PORT"
        sleep 1
    fi

    if command -v mosquitto >/dev/null 2>&1; then
        cat > "$APP_DIR/data/mosquitto.conf" <<MOSQEOF
listener $MQTT_PORT 0.0.0.0
allow_anonymous true
persistence false
log_dest file $LOG_DIR/mosquitto.log
log_type all
MOSQEOF
        mosquitto -c "$APP_DIR/data/mosquitto.conf" -d
        sleep 1
    elif [ -x "$APP_DIR/bin/iotgw_mqtt_broker" ]; then
        "$APP_DIR/bin/iotgw_mqtt_broker" "$MQTT_PORT" >"$LOG_DIR/broker.log" 2>&1 &
        sleep 1
    else
        echo "[start.sh] no MQTT broker found, please install mosquitto"
    fi
}

start_gateway() {
    if is_port_listening "$HTTP_PORT"; then
        kill_by_name iotgw_gateway
        kill_tcp_listener "$HTTP_PORT"
        sleep 1
    fi

    export IOTGW_VIDEO_ROTATION="$VIDEO_ROTATION"
    export IOTGW_VIDEO_WIDTH="$VIDEO_WIDTH"
    export IOTGW_VIDEO_FPS="$VIDEO_FPS"
    export IOTGW_VIDEO_QUALITY="$VIDEO_QUALITY"
    export IOTGW_APP_DIR="$APP_DIR"
    export IOTGW_VIDEO_HOST="${IOTGW_VIDEO_HOST:-${WLAN_IP:-192.168.233.107}}"

    "$APP_DIR/bin/iotgw_gateway" \
        --yaml-config "$CONFIG_FILE" \
        --log-file "$LOG_DIR/iotgw.log" \
        >"$LOG_DIR/gateway.stdout.log" 2>&1 &

    for i in 1 2 3 4 5; do
        if is_port_listening "$HTTP_PORT"; then
            return 0
        fi
        sleep 1
    done

    echo "[start.sh] gateway failed to listen on $HTTP_PORT"
    tail -80 "$LOG_DIR/gateway.stdout.log" 2>/dev/null || true
    return 1
}

start_qt() {
    if [ "$START_QT" != "1" ] || [ ! -x "$APP_DIR/bin/iotgw_qt_client" ]; then
        return
    fi

    sleep "$START_QT_DELAY"
    export IOTGW_QT_ROTATION="$QT_ROTATION"
    export IOTGW_VIDEO_ROTATION="$VIDEO_ROTATION"
    if [ -z "${DISPLAY:-}" ]; then
        export QT_QPA_PLATFORM="$QT_PLATFORM"
    fi
    if [ -d /usr/lib/fonts ]; then
        export QT_QPA_FONTDIR="${QT_QPA_FONTDIR:-/usr/lib/fonts}"
    fi
    "$APP_DIR/bin/iotgw_qt_client" >"$LOG_DIR/qt_client.log" 2>&1 &
}

case "${1:-start}" in
    start)
        ensure_wifi
        stop_services
        start_broker
        start_gateway
        start_qt
        ;;
    stop)
        stop_all_services
        ;;
    restart)
        stop_services
        sleep 1
        ensure_wifi
        start_broker
        start_gateway
        start_qt
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac
EOF

chmod +x "$PACKAGE_DIR/start.sh"

cat > "$PACKAGE_DIR/qt_start.sh" <<'EOF'
#!/bin/sh
set -eu

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
LOG_DIR="${IOTGW_LOG_DIR:-$APP_DIR/data/logs}"
QT_PLATFORM="${IOTGW_QT_PLATFORM:-wayland}"
QT_ROTATION="${IOTGW_QT_ROTATION:-90}"
VIDEO_ROTATION="${IOTGW_VIDEO_ROTATION:-270}"

mkdir -p "$LOG_DIR"
kill -9 $(ps | grep '[i]otgw_qt_client' | awk '{print $1}') 2>/dev/null || true

export QT_QPA_PLATFORM="$QT_PLATFORM"
export IOTGW_QT_ROTATION="$QT_ROTATION"
export IOTGW_VIDEO_ROTATION="$VIDEO_ROTATION"
"$APP_DIR/bin/iotgw_qt_client" >"$LOG_DIR/qt_client.log" 2>&1 &

echo "Qt started: platform=$QT_PLATFORM rotation=$QT_ROTATION video_rotation=$VIDEO_ROTATION"
echo "Log: $LOG_DIR/qt_client.log"
EOF

cat > "$PACKAGE_DIR/qt_stop.sh" <<'EOF'
#!/bin/sh
kill -9 $(ps | grep '[i]otgw_qt_client' | awk '{print $1}') 2>/dev/null || true
EOF

cat > "$PACKAGE_DIR/board_net_mount.sh" <<'EOF'
#!/bin/sh
set -eu

WIFI_SSID="${IOTGW_WIFI_SSID:-nie}"
WIFI_PASS="${IOTGW_WIFI_PASS:-12345678}"
BOARD_ETH_IP="${IOTGW_BOARD_ETH_IP:-192.168.137.170}"
NFS_HOST="${IOTGW_NFS_HOST:-192.168.137.118}"
NFS_EXPORT="${IOTGW_NFS_EXPORT:-/home/nfs}"
NFS_MOUNT="${IOTGW_NFS_MOUNT:-/mnt}"

echo "[net] WiFi: $WIFI_SSID"
ifconfig wlan0 up 2>/dev/null || true
if ! ifconfig wlan0 2>/dev/null | grep -q "inet addr"; then
    if command -v wpa_passphrase >/dev/null 2>&1; then
        wpa_passphrase "$WIFI_SSID" "$WIFI_PASS" > /tmp/iotgw_wpa.conf
        killall wpa_supplicant 2>/dev/null || true
        wpa_supplicant -B -i wlan0 -c /tmp/iotgw_wpa.conf 2>/dev/null || true
        sleep 3
    fi
    echo "[net] dhcp wlan0"
    udhcpc -n -q -t 5 -T 3 -i wlan0 2>/dev/null || true
fi
ifconfig wlan0 2>/dev/null | grep "inet addr" || true

echo "[net] eth0: $BOARD_ETH_IP/24"
ip addr add "$BOARD_ETH_IP/24" dev eth0 2>/dev/null || true
ip link set eth0 up 2>/dev/null || true

echo "[net] ping NFS host: $NFS_HOST"
if ! ping "$NFS_HOST" -c 3; then
    echo "[net] ERROR: cannot reach $NFS_HOST"
    exit 1
fi

mkdir -p "$NFS_MOUNT"
if grep -q " $NFS_MOUNT " /proc/mounts; then
    echo "[net] already mounted: $NFS_MOUNT"
else
    echo "[net] mount: $NFS_HOST:$NFS_EXPORT -> $NFS_MOUNT"
    mount -t nfs -o nfsvers=3,nolock,tcp,soft,intr,timeo=5,retrans=2 "$NFS_HOST:$NFS_EXPORT" "$NFS_MOUNT"
fi

echo "[net] done"
ls "$NFS_MOUNT"
EOF

cat > "$PACKAGE_DIR/S90iotgw" <<'EOF'
#!/bin/sh

APP_DIR="${IOTGW_APP_DIR:-/aiot/iotgw_package}"

case "$1" in
    start)
        sleep "${IOTGW_BOOT_DELAY:-8}"
        cd "$APP_DIR" && sh "$APP_DIR/start.sh" start
        ;;
    stop)
        cd "$APP_DIR" && sh "$APP_DIR/start.sh" stop
        ;;
    restart|reload)
        cd "$APP_DIR" && sh "$APP_DIR/start.sh" restart
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac
EOF

cat > "$PACKAGE_DIR/install_autostart.sh" <<'EOF'
#!/bin/sh
set -eu

SRC_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
INSTALL_DIR="${IOTGW_INSTALL_DIR:-/aiot/iotgw_package}"
TMP_DIR="${INSTALL_DIR}.new.$$"
OLD_DIR="${INSTALL_DIR}.old"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"
cp -a "$SRC_DIR"/. "$TMP_DIR"/

if [ -d "$INSTALL_DIR" ]; then
    rm -rf "$OLD_DIR"
    mv "$INSTALL_DIR" "$OLD_DIR"
fi

mv "$TMP_DIR" "$INSTALL_DIR"
chmod +x "$INSTALL_DIR/start.sh" "$INSTALL_DIR/S90iotgw"
cp "$INSTALL_DIR/S90iotgw" /etc/init.d/S90iotgw
chmod +x /etc/init.d/S90iotgw

if [ -f /etc/init.d/rcS ] && ! grep -q "S90iotgw start" /etc/init.d/rcS; then
    cat >> /etc/init.d/rcS <<'RCS_EOF'

# IotEdgeGateway autostart
/etc/init.d/S90iotgw start &
RCS_EOF
fi

echo "Installed to $INSTALL_DIR"
echo "Autostart: /etc/init.d/S90iotgw"
echo "Start now: /etc/init.d/S90iotgw start"
EOF

chmod +x "$PACKAGE_DIR/S90iotgw" "$PACKAGE_DIR/install_autostart.sh" "$PACKAGE_DIR/board_net_mount.sh" "$PACKAGE_DIR/qt_start.sh" "$PACKAGE_DIR/qt_stop.sh"

echo "Package created: $PACKAGE_DIR"
echo "Run on board:"
echo "  cd $PACKAGE_DIR"
echo "  sh start.sh start"
echo "Install autostart on board:"
echo "  sh install_autostart.sh"
