#include <gtest/gtest.h>
#include "contrast/game_state.hpp"

TEST(GameStateTest, CreatesAndResets) {
  contrast::GameState s;
  s.reset();
  SUCCEED();
}
