#!/bin/bash
# run_tests.sh - Run all C parser tests

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR"

# Default parser binary path relative to the project root (assuming run from project root)
# But we'll override it if an argument is provided.
DEFAULT_PARSER_BIN="./build/examples/c_parser/c_parser_example"
PARSER_BIN="${1:-$DEFAULT_PARSER_BIN}"

if [ ! -f "$PARSER_BIN" ]; then
    echo "Error: Parser binary not found at $PARSER_BIN"
    echo "Usage: $0 [path_to_c_parser_example]"
    exit 1
fi

echo "Running C Parser tests..."
echo "Parser: $PARSER_BIN"
echo "Test Directory: $TEST_DIR"
echo "--------------------------"

SUCCESS_COUNT=0
TOTAL_COUNT=0

for f in "$TEST_DIR"/*.c; do
    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    printf "Parsing %-50s " "$(basename "$f")..."
    
    "$PARSER_BIN" "$f" > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo "SUCCESS"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        echo "FAILED"
    fi
done

echo "--------------------------"
echo "Tests completed: $SUCCESS_COUNT / $TOTAL_COUNT passed."

if [ "$SUCCESS_COUNT" -eq "$TOTAL_COUNT" ]; then
    exit 0
else
    exit 1
fi
