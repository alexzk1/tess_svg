#!/bin/bash
# $1 - path to binary
# $2 - input SVG
# $3 - expected JSON

TEMP_JSON=$(mktemp)

$1 -i $2 -P > $TEMP_JSON

if diff -w $3 $TEMP_JSON > /dev/null; then
    echo "Test Passed!"
    rm $TEMP_JSON
    exit 0
else
    echo "Test Failed! Output differs from expected.json"
    diff -u $3 $TEMP_JSON
    rm $TEMP_JSON
    exit 1
fi
