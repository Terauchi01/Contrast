#!/bin/bash
# Continue self-play training from the latest checkpoint

cd "$(dirname "$0")/.."

WEIGHTS_DIR="build/apps/train_ntuple"
GAMES=50000
SAVE_INTERVAL=5000
EPSILON=0.1
LR=0.001

# Find the latest selfplay weights
LATEST_WEIGHTS=$(ls -t ${WEIGHTS_DIR}/ntuple_selfplay.bin.* 2>/dev/null | head -1)

if [ -z "${LATEST_WEIGHTS}" ]; then
    echo "Error: No self-play weights found."
    echo "Please run initial self-play training first:"
    echo "  ./scripts/train_selfplay.sh"
    exit 1
fi

# Extract the game number from filename
LATEST_NUM=$(basename "${LATEST_WEIGHTS}" | sed 's/ntuple_selfplay.bin.//')
NEW_NUM=$((LATEST_NUM + GAMES))

echo "========================================="
echo "Continue Self-Play Training"
echo "========================================="
echo "Loading from: ${LATEST_WEIGHTS}"
echo "Current games: ${LATEST_NUM}"
echo "Will train: ${GAMES} more games"
echo "Final count: ${NEW_NUM}"
echo "Epsilon: ${EPSILON}"
echo "Learning rate: ${LR}"
echo ""

cd build/apps/train_ntuple

./train_ntuple \
    --load "$(basename ${LATEST_WEIGHTS})" \
    --games ${GAMES} \
    --opponent self \
    --epsilon ${EPSILON} \
    --learning-rate ${LR} \
    --save-interval ${SAVE_INTERVAL} \
    --output "ntuple_selfplay_cont.bin"

# Rename to continue the numbering
for file in ntuple_selfplay_cont.bin.*; do
    if [ -f "$file" ]; then
        num=$(echo $file | sed 's/ntuple_selfplay_cont.bin.//')
        new_num=$((LATEST_NUM + num))
        mv "$file" "ntuple_selfplay.bin.${new_num}"
        echo "Renamed: $file -> ntuple_selfplay.bin.${new_num}"
    fi
done

echo ""
echo "========================================="
echo "Training completed!"
echo "========================================="
echo "Latest weights: ntuple_selfplay.bin.${NEW_NUM}"
echo ""
echo "To continue training further, run this script again."
echo ""
echo "To evaluate against greedy:"
echo "  cd build/apps/eval_ntuple"
echo "  ./eval_ntuple --weights ../train_ntuple/ntuple_selfplay.bin.${NEW_NUM} --opponent greedy --games 1000"
