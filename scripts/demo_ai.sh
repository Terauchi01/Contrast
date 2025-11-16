#!/bin/bash

# AIポリシーのデモ実行スクリプト

cd "$(dirname "$0")/.." || exit 1

echo "===================================="
echo "Contrast AI Demo"
echo "===================================="
echo ""
echo "1. AI Policy Tests (Random & Greedy)"
echo "   - 各AIが正しく手を選択できるかテスト"
echo "   - Greedyが前進を優先するかテスト"
echo "   - AI同士の対戦がゲーム終了まで進むかテスト"
echo "   - Greedy vs Random 50戦の統計"
echo ""
echo "Running tests..."
echo ""

./build/tests/test_ai

echo ""
echo "===================================="
echo "2. GUI with AI vs AI mode"
echo "   - GUIを起動してAI対戦を視覚化"
echo "   - 右サイドバーでAIタイプを選択"
echo "   - 'Start AI vs AI'ボタンでAI対戦開始"
echo "===================================="
echo ""
echo "To run GUI with AI vs AI:"
echo "  ./build/apps/gui_debug/gui_debug"
echo ""
echo "Or run with auto-close after 10 seconds:"
echo "  GUI_DEBUG_HOLD_SECONDS=10 ./build/apps/gui_debug/gui_debug"
