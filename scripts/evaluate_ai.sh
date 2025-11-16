#!/bin/bash
# Evaluate the strength of different checkpoints

cd "$(dirname "$0")/.."

if [ $# -lt 1 ]; then
    echo "Usage: $0 <weights_file> [opponent] [games]"
    echo ""
    echo "Examples:"
    echo "  $0 ntuple_selfplay.bin.50000"
    echo "  $0 ntuple_selfplay.bin.50000 greedy 1000"
    echo "  $0 ntuple_weights_vs_greedy.bin.100000 rulebased 500"
    exit 1
fi

WEIGHTS_FILE="$1"
OPPONENT="${2:-greedy}"
GAMES="${3:-1000}"

WEIGHTS_PATH="build/apps/train_ntuple/${WEIGHTS_FILE}"

if [ ! -f "${WEIGHTS_PATH}" ]; then
    echo "Error: Weights file not found: ${WEIGHTS_PATH}"
    exit 1
fi

echo "========================================="
echo "Evaluating AI Strength"
echo "========================================="
echo "Weights: ${WEIGHTS_FILE}"
echo "Opponent: ${OPPONENT}"
echo "Games: ${GAMES}"
echo ""

cd build/apps/eval_ntuple

echo "Playing ${GAMES} games as Black..."
./eval_ntuple \
    --weights "../train_ntuple/${WEIGHTS_FILE}" \
    --opponent "${OPPONENT}" \
    --games "${GAMES}"

echo ""
echo "Playing ${GAMES} games as White (swap colors)..."
./eval_ntuple \
    --weights "../train_ntuple/${WEIGHTS_FILE}" \
    --opponent "${OPPONENT}" \
    --games "${GAMES}" \
    --swap-colors

echo ""
echo "========================================="
echo "Evaluation completed!"
echo "========================================="
