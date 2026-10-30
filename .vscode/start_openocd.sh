#!/bin/bash

PORT=3333
PID_FILE="/tmp/ovwatch_openocd.pid"
LOG_FILE="/tmp/ovwatch_openocd.log"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CFG="$ROOT_DIR/openocd-stm32wb55.cfg"

MODE="${1:-start}"

port_listen_pids() {
    lsof -nP -iTCP:$PORT -sTCP:LISTEN 2>/dev/null | tail -n +2 | awk '{print $2}' | sort -u
}

port_is_listening() {
    [ -n "$(port_listen_pids)" ]
}

kill_pid_file() {
    if [ -f "$PID_FILE" ]; then
        OLD=$(cat "$PID_FILE" 2>/dev/null)
        if [ -n "$OLD" ]; then
            kill -9 "$OLD" 2>/dev/null
        fi
        rm -f "$PID_FILE"
    fi
}

if [ "$MODE" = "stop" ]; then
    kill_pid_file
    PIDS=$(port_listen_pids)
    if [ -z "$PIDS" ]; then
        echo "No OpenOCD running on port $PORT"
    else
        echo "Stopping OpenOCD: $PIDS"
        kill $PIDS 2>/dev/null
        for _ in $(seq 1 20); do
            sleep 0.2
            port_is_listening || break
        done
        if port_is_listening; then
            echo "Force killing OpenOCD"
            kill -9 $PIDS 2>/dev/null
        fi
        echo "Port $PORT released"
    fi
    exit 0
fi

kill_pid_file
STALE=$(port_listen_pids)
if [ -n "$STALE" ]; then
    echo "Stale OpenOCD on port $PORT (PID: $STALE), terminating it"
    kill $STALE 2>/dev/null
    for _ in $(seq 1 20); do
        sleep 0.2
        port_is_listening || break
    done
    if port_is_listening; then
        echo "Force killing stale OpenOCD"
        kill -9 $STALE 2>/dev/null
        sleep 1
    fi
fi

CONFLICT=$(pgrep -f "STM32CubeProgrammer" | tr '\n' ' ')
if [ -n "$CONFLICT" ]; then
    echo "ERROR: STM32CubeProgrammer is running (PID: $CONFLICT)"
    echo "It holds the ST-Link and causes SWD bus contention."
    echo "Close STM32CubeProgrammer before debugging."
    exit 1
fi

if [ ! -f "$CFG" ]; then
    echo "Config not found: $CFG"
    exit 1
fi

OPENOCD="$(command -v openocd 2>/dev/null)"
if [ -z "$OPENOCD" ]; then
    ESP_OCD="$(ls -d "$HOME"/.espressif/tools/openocd-esp32/*/openocd-esp32/bin/openocd 2>/dev/null | tail -1)"
    if [ -n "$ESP_OCD" ]; then
        OPENOCD="$ESP_OCD"
    else
        echo "openocd not found, install it or set PATH"
        exit 1
    fi
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found, cannot daemonize OpenOCD"
    exit 1
fi

python3 - "$OPENOCD" "$CFG" "$PID_FILE" "$LOG_FILE" <<'PY'
import os
import sys
import time

openocd, cfg, pid_file, log_file = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]

try:
    with open(pid_file) as f:
        old = int(f.read().strip())
    os.kill(old, 9)
    time.sleep(0.3)
except Exception:
    pass
try:
    os.remove(pid_file)
except Exception:
    pass

if os.fork() > 0:
    os._exit(0)
os.setsid()
if os.fork() > 0:
    os._exit(0)

with open(log_file, 'w') as f:
    os.dup2(f.fileno(), 1)
    os.dup2(f.fileno(), 2)

with open(pid_file, 'w') as f:
    f.write(str(os.getpid()))

os.execv(openocd, [openocd, '-f', cfg])
PY

OCD_PID=$(cat "$PID_FILE" 2>/dev/null)
if [ -z "$OCD_PID" ]; then
    echo "ERROR: failed to start OpenOCD daemon"
    exit 1
fi

READY=0
for _ in $(seq 1 60); do
    if port_is_listening; then
        READY=1
        break
    fi
    if ! kill -0 "$OCD_PID" 2>/dev/null; then
        break
    fi
    sleep 0.2
done

if [ "$READY" != "1" ]; then
    echo "ERROR: OpenOCD failed to listen on port $PORT"
    echo "--- OpenOCD log ---"
    tail -n 30 "$LOG_FILE" 2>/dev/null
    kill -9 "$OCD_PID" 2>/dev/null
    rm -f "$PID_FILE"
    exit 1
fi

echo "OpenOCD ready on port $PORT (PID: $OCD_PID)"
exit 0