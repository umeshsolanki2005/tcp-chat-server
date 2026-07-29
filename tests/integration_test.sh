#!/usr/bin/env bash
set -euo pipefail

make >/dev/null
./server 8081 > /tmp/server.log 2>&1 &
SERVER_PID=$!
sleep 1
printf '/register alice secret1\n' | ./client 127.0.0.1 8081 > /tmp/client1.log 2>&1 &
CLIENT1_PID=$!
sleep 1
kill $SERVER_PID
wait $CLIENT1_PID || true
wait $SERVER_PID || true
