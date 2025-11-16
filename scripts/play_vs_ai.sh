#!/bin/bash
# Play against trained N-tuple AI

cd "$(dirname "$0")/.."

# Check if weights file exists
WEIGHTS="build/apps/train_ntuple/ntuple_weights_vs_greedy.bin.100000"
if [ ! -f "$WEIGHTS" ]; then
    echo "Error: Weights file not found at $WEIGHTS"
    echo "Please train the AI first or specify a different weights file."
    exit 1
fi

echo "Starting Human vs AI game..."
echo "AI weights: $WEIGHTS"
echo ""

cd build/apps/gui_play
./gui_play "../train_ntuple/ntuple_weights_vs_greedy.bin.100000"
