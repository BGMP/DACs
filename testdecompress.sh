#!/bin/bash

NUM_RUNS=10
TOTAL_TIME=0

echo "Running decompress test $NUM_RUNS times..."

./testdecompress dblp.xml.100MB.dacs > /dev/null

for i in $(seq 1 $NUM_RUNS); do
    echo "Run $i:"
    TIME=$(./testdecompress dblp.xml.100MB.dacs)
    echo "Time: $TIME"
    TOTAL_TIME=$(echo "$TOTAL_TIME + $TIME" | bc -l)
    echo ""
done

OVERALL_AVG=$(echo "scale=6; $TOTAL_TIME / $NUM_RUNS" | bc -l)

echo "Overall average across $NUM_RUNS runs: $(printf "%.6f" $OVERALL_AVG) seconds"
echo "OK."
