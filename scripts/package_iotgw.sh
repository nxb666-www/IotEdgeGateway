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

copy_dir "$ROOT_DIR/config" "$PACKAGE_DIR/config"
copy_dir "$ROOT_DIR/www" "$PACKAGE_DIR/www"

cat > "$PACKAGE_DIR/start.sh" <<'EOF'
#!/bin/sh
set -eu

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
MQTT_PORT="${IOTGW_MQTT_PORT:-1884}"
CONFIG_FILE="${IOTGW_CONFIG_FILE:-$APP_DIR/config/environments/production.yaml}"
LOG_DIR="${IOTGW_LOG_DIR:-$APP_DIR/data/logs}"
WIFI_SSID="${IOTGW_WIFI_SSID:-nie}"
WIFI_PASS="${IOTGW_WIFI_PASS:-12345678}"

mkdir -p "$LOG_DIR" "$APP_DIR/data"

ensure_wifi() {
    if iwconfig wlan0 2>/dev/null | grep -q "ESSID:.*off/any"; then
        echo "[start.sh] WiFi not connected, connecting to $WIFI_SSID ..."
        if command -v nmcli >/dev/null 2>&1; then
            nmcli device wifi connect "$WIFI_SSID" password "$WIFI_PASS" 2>/dev/null || true
        else
            wpa_passphrase "$WIFI_SSID" "$WIFI_PASS" > /tmp/iotgw_wpa.conf
            wpa_supplicant -B -i wlan0 -c /tmp/iotgw_wpa.conf 2>/dev/null || true
            sleep 2
            udhcpc -i wlan0 2>/dev/null || true
        fi
        sleep 2
        WLAN_IP=$(ifconfig wlan0 2>/dev/null | grep "inet addr" | awk -F: '{print $2}' | awk '{print $1}')
        echo "[start.sh] WiFi connected, IP: ${WLAN_IP:-unknown}"
    fi
}

stop_services() {
    killall iotgw_gateway 2>/dev/null || true
    killall iotgw_mqtt_broker 2>/dev/null || true
}

case "${1:-start}" in
    start)
        ensure_wifi
        stop_services
        if [ -x "$APP_DIR/bin/iotgw_mqtt_broker" ]; then
            "$APP_DIR/bin/iotgw_mqtt_broker" "$MQTT_PORT" >"$LOG_DIR/broker.log" 2>&1 &
            sleep 1
        fi
        "$APP_DIR/bin/iotgw_gateway" \
            --yaml-config "$CONFIG_FILE" \
            --log-file "$LOG_DIR/iotgw.log" \
            >"$LOG_DIR/gateway.stdout.log" 2>&1 &
        ;;
    stop)
        stop_services
        ;;
    restart)
        "$0" stop
        sleep 1
        "$0" start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac
EOF

chmod +x "$PACKAGE_DIR/start.sh"

echo "Package created: $PACKAGE_DIR"
echo "Run on board:"
echo "  cd $PACKAGE_DIR"
echo "  sh start.sh start"
