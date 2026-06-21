#!/bin/sh
set -eu

PORT="${1:-1884}"
CONF="/tmp/iotgw-mosquitto.conf"
LOG="/tmp/iotgw-mosquitto.log"

echo "== IotEdgeGateway MQTT setup =="
echo "port: $PORT"

if ! command -v mosquitto >/dev/null 2>&1; then
    echo "mosquitto not found, trying package manager..."
    if command -v apt-get >/dev/null 2>&1; then
        apt-get update
        apt-get install -y mosquitto mosquitto-clients
    elif command -v opkg >/dev/null 2>&1; then
        opkg update
        opkg install mosquitto mosquitto-client
    elif command -v apk >/dev/null 2>&1; then
        apk add mosquitto mosquitto-clients
    elif command -v yum >/dev/null 2>&1; then
        yum install -y mosquitto mosquitto-clients
    else
        echo "ERROR: no package manager found. Need copy static mosquitto for this rootfs."
        exit 1
    fi
fi

cat > "$CONF" <<EOF
listener $PORT 0.0.0.0
allow_anonymous true
persistence false
EOF

killall mosquitto 2>/dev/null || true
nohup mosquitto -c "$CONF" -v > "$LOG" 2>&1 &
sleep 1

echo "== listening =="
netstat -tln 2>/dev/null | grep ":$PORT" || true

echo "== quick pub/sub test =="
mosquitto_sub -h 127.0.0.1 -p "$PORT" -t 'iotgw/test' -C 1 -v > /tmp/iotgw-mqtt-test.out 2>&1 &
SUB_PID=$!
sleep 1
mosquitto_pub -h 127.0.0.1 -p "$PORT" -t 'iotgw/test' -m 'ok'
wait "$SUB_PID" || true
cat /tmp/iotgw-mqtt-test.out

echo "== done =="
echo "broker: 127.0.0.1:$PORT on RK3568"
echo "log: $LOG"
