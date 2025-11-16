#!/bin/bash
# Self-play training script for N-tuple network
# Continues training from existing weights using self-play

cd "$(dirname "$0")/.."

# Configuration
WEIGHTS_DIR="build/apps/train_ntuple"
INITIAL_WEIGHTS="${WEIGHTS_DIR}/ntuple_weights_vs_greedy.bin.100000"
OUTPUT_PREFIX="${WEIGHTS_DIR}/ntuple_selfplay"
GAMES=50000
SAVE_INTERVAL=5000
EPSILON=0.1
LR=0.001

echo "========================================="
echo "Self-Play Training"
echo "========================================="
echo "Initial weights: ${INITIAL_WEIGHTS}"
echo "Output prefix: ${OUTPUT_PREFIX}"
echo "Games: ${GAMES}"
echo "Save interval: ${SAVE_INTERVAL}"
echo "Epsilon: ${EPSILON}"
echo "Learning rate: ${LR}"
echo ""

# Check if initial weights exist
if [ ! -f "${INITIAL_WEIGHTS}" ]; then
    echo "Error: Initial weights not found at ${INITIAL_WEIGHTS}"
    echo "Please train against greedy opponent first:"
    echo "  ./scripts/train_vs_greedy.sh"
    exit 1
fi

# Create weights directory if needed
mkdir -p "${WEIGHTS_DIR}"

cd build/apps/train_ntuple

echo "Starting self-play training..."
echo "This will take a while. Progress will be shown every 1000 games."
echo ""

./train_ntuple \
    --load "ntuple_weights_vs_greedy.bin.100000" \
    --games ${GAMES} \
    --opponent self \
    --epsilon ${EPSILON} \
    --learning-rate ${LR} \
    --save-interval ${SAVE_INTERVAL} \
    --output "ntuple_selfplay.bin"

echo ""
echo "========================================="
echo "Self-play training completed!"
echo "========================================="
echo ""
echo "Weights saved with checkpoints:"
ls -lh ntuple_selfplay.bin.* 2>/dev/null | tail -5
echo ""
echo "Latest weights: ntuple_selfplay.bin.${GAMES}"
echo ""
echo "To continue training, run:"
echo "  ./scripts/continue_selfplay.sh"
echo ""
echo "To play against the new AI:"
echo "  cd build/apps/gui_play"
echo "  ./gui_play ../train_ntuple/ntuple_selfplay.bin.${GAMES}"
