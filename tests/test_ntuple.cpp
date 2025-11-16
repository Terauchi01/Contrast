#include <gtest/gtest.h>
#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/ntuple.hpp"
#include <iostream>

using namespace contrast;

TEST(NTupleTest, NetworkInitialization) {
  contrast_ai::NTupleNetwork network;
  
  EXPECT_GT(network.num_tuples(), 0);
  EXPECT_GT(network.num_weights(), 0);
  
  std::cout << "N-tuple network initialized:\n";
  std::cout << "  Num tuples: " << network.num_tuples() << "\n";
  std::cout << "  Total weights: " << network.num_weights() << "\n";
}

TEST(NTupleTest, EvaluateInitialState) {
  GameState state;
  contrast_ai::NTupleNetwork network;
  
  float value = network.evaluate(state);
  
  // Initial value should be 0 (all weights are zero)
  EXPECT_FLOAT_EQ(value, 0.0f);
}

TEST(NTupleTest, PolicyCanPickMove) {
  GameState state;
  contrast_ai::NTuplePolicy policy;
  
  Move move = policy.pick(state);
  
  // Should pick a valid move
  contrast::MoveList moves;
  Rules::legal_moves(state, moves);
  bool found = false;
  for (const auto& m : moves) {
    if (m.sx == move.sx && m.sy == move.sy && 
        m.dx == move.dx && m.dy == move.dy) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST(NTupleTest, TDUpdateChangesWeights) {
  GameState state;
  contrast_ai::NTupleNetwork network;
  
  float value_before = network.evaluate(state);
  
  // Perform TD update with positive target
  network.td_update(state, 1.0f, 0.1f);
  
  float value_after = network.evaluate(state);
  
  // Value should increase after positive update
  EXPECT_GT(value_after, value_before);
}

TEST(NTupleTest, SaveAndLoad) {
  contrast_ai::NTupleNetwork network1;
  
  // Train a bit
  GameState state;
  for (int i = 0; i < 10; ++i) {
    network1.td_update(state, 1.0f, 0.1f);
  }
  
  float value1 = network1.evaluate(state);
  
  // Save
  network1.save("/tmp/test_ntuple.bin");
  
  // Load into new network
  contrast_ai::NTupleNetwork network2;
  network2.load("/tmp/test_ntuple.bin");
  
  float value2 = network2.evaluate(state);
  
  // Should be the same
  EXPECT_FLOAT_EQ(value1, value2);
}

TEST(NTupleTest, NegamaxProperty) {
  GameState state;
  contrast_ai::NTupleNetwork network;
  
  // Train with some positive value for Black (current player)
  for (int i = 0; i < 10; ++i) {
    network.td_update(state, 1.0f, 0.1f);
  }
  
  float black_value = network.evaluate(state);
  
  // Now evaluate from White's perspective (make a move to switch)
  contrast::MoveList moves;
  Rules::legal_moves(state, moves);
  if (!moves.empty()) {
    state.apply_move(moves[0]);
    float white_value = network.evaluate(state);
    
    // From White's perspective, the value should be negated
    // (approximately, since board changed slightly)
    std::cout << "Black value: " << black_value << "\n";
    std::cout << "White value: " << white_value << "\n";
    std::cout << "Sum (should be ~0): " << (black_value + white_value) << "\n";
    
    // The values should be roughly opposite (within some tolerance due to board change)
    EXPECT_LT(std::abs(black_value + white_value), std::abs(black_value) * 0.2f);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
