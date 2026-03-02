#!/bin/bash
# run_tests.sh - Run all C parser tests
# This script should be executed from the project root directory.

TEST_DIR="examples/c_parser/tests"
PARSER_BIN="./build/examples/c_parser/c_parser_example"

if [ ! -f "$PARSER_BIN" ]; then
    echo "Error: Parser binary not found at $PARSER_BIN"
    echo "Please build the project first."
    exit 1
fi

echo "Running C Parser tests..."
echo "--------------------------"

SUCCESS_COUNT=0
TOTAL_COUNT=0

for f in "$TEST_DIR"/*.c; do
    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    printf "Parsing %-50s " "$(basename "$f")..."
    
    $PARSER_BIN "$f" > /dev/null 2>&1
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
