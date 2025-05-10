#!/bin/bash

# SCRIPT_DIR=$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)
# echo "Папка скрипта:" $SCRIPT_DIR
# cd "$SCRIPT_DIR"

# g++ "$SCRIPT_DIR/test_server.cpp" "$SCRIPT_DIR/../common/message_data.cpp" -o test_server
# echo "$SCRIPT_DIR/test_server > $SCRIPT_DIR/test_server.log &"
# "$SCRIPT_DIR/test_server" > "$SCRIPT_DIR/test_server.log" &

./TestServer > test_server.log &
./ClientApp