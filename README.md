# contrast

Small research game project skeleton for "contrast".

## Overview

Contrast is a 5x5 board game where:
- **Black** pieces start on row 0 (top) and must reach row 4 (bottom) to win
- **White** pieces start on row 4 (bottom) and must reach row 0 (top) to win
- Movement is determined by **tile types**:
  - **None/White tiles**: 4-direction orthogonal movement
  - **Black tiles**: 4-direction diagonal movement
  - **Gray tiles**: 8-direction movement
- Players can **jump over their own pieces** but not opponent pieces
- Players can **place tiles** from their inventory to change movement patterns

## Project Structure

```
contrast/
├── core/           # Game logic (Board, Rules, GameState, Move)
├── engine/         # AI policies (Random, Greedy, MCTS stub)
├── apps/           # Applications
│   ├── cli_selfplay/   # CLI selfplay (stub)
│   ├── cli_play/       # CLI interactive play (stub)
│   └── gui_debug/      # ImGui+GLFW GUI with AI vs AI mode
├── tests/          # GoogleTest unit tests
└── scripts/        # Build and run helper scripts
```

## Build

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . -- -j$(sysctl -n hw.ncpu)
```

## Usage

### Run Tests

```bash
# Rule validation tests
./build/tests/test_rules

# AI policy tests
./build/tests/test_ai
```

### Run GUI with AI vs AI

```bash
# Launch GUI
./build/apps/gui_debug/gui_debug

# Or with auto-close after N seconds
GUI_DEBUG_HOLD_SECONDS=10 ./build/apps/gui_debug/gui_debug
```

In the GUI:
1. Select AI types for Black and White (Random or Greedy)
2. Adjust delay slider to control move speed
3. Click "Start AI vs AI" to watch AI play
4. Click "Stop AI vs AI" to pause
5. Click "Reset Game" to restart

### Quick Demo

```bash
./scripts/demo_ai.sh
```

## AI Implementations

### RandomPolicy
- Picks a random legal move uniformly
- Used as baseline for testing

### GreedyPolicy
- Always tries to move forward toward the opponent's back row
- If multiple forward moves exist, picks one randomly
- Falls back to random if no forward moves available
- Significantly stronger than random (96% win rate in tests)

### Future: MCTS
- Monte Carlo Tree Search (stub exists in `engine/include/contrast_ai/mcts.hpp`)
- Planned for stronger AI gameplay

## Test Results

From `test_ai`:
- **Greedy vs Random (50 games)**: Greedy wins 96%, Random wins 4%, Draws 0%
- All AI policies successfully complete games without hanging
- Greedy successfully prioritizes forward movement (>70% of moves go forward)

See CMakeLists.txt for build instructions.
