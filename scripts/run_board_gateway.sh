#!/bin/sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BROKER_PORT="${1:-1884}"
BROKER_LOG="${IOTGW_BROKER_LOG:-/tmp/iotgw_broker.log}"

cd "$ROOT_DIR"

if [ ! -x ./build/iotgw_mqtt_broker ]; then
    echo "ERROR: ./build/iotgw_mqtt_broker not found or not executable"
    exit 1
fi

if [ ! -x ./build/iotgw_gateway ]; then
    echo "ERROR: ./build/iotgw_gateway not found or not executable"
    exit 1
fi

if ps | grep '[i]otgw_mqtt_broker' >/dev/null 2>&1; then
    echo "MQTT broker already running"
else
    echo "Starting MQTT broker on port ${BROKER_PORT}, log: ${BROKER_LOG}"
    ./build/iotgw_mqtt_broker "$BROKER_PORT" >"$BROKER_LOG" 2>&1 &
    sleep 1
fi

echo "Starting gateway"
exec ./build/iotgw_gateway
