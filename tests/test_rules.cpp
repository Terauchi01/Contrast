
#include <gtest/gtest.h>
#include "contrast/rules.hpp"
#include "contrast/game_state.hpp"
#include "contrast/board.hpp"
#include "contrast/move_list.hpp"

TEST(RulesTest, InitialHasMoves) {
  contrast::GameState s;
  s.reset();
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  // initial pieces should have some legal moves
  EXPECT_FALSE(moves.empty());
}

TEST(RulesTest, InitialBoardSetup) {
  contrast::GameState s;
  s.reset();
  const auto& b = s.board();
  
  // Top row (y=0) should have Black pieces
  for (int x = 0; x < b.width(); ++x) {
    EXPECT_EQ(b.at(x, 0).occupant, contrast::Player::Black) << "at (" << x << ",0)";
    EXPECT_EQ(b.at(x, 0).tile, contrast::TileType::None);
  }
  
  // Bottom row (y=4) should have White pieces
  for (int x = 0; x < b.width(); ++x) {
    EXPECT_EQ(b.at(x, b.height()-1).occupant, contrast::Player::White) << "at (" << x << "," << b.height()-1 << ")";
    EXPECT_EQ(b.at(x, b.height()-1).tile, contrast::TileType::None);
  }
  
  // Middle rows should be empty
  for (int y = 1; y < b.height()-1; ++y) {
    for (int x = 0; x < b.width(); ++x) {
      EXPECT_EQ(b.at(x,y).occupant, contrast::Player::None) << "at (" << x << "," << y << ")";
    }
  }
}

TEST(RulesTest, OrthoDirectionsOnWhiteTile) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear board and place one Black piece at (2,2)
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x) {
      b.at(x,y).occupant = contrast::Player::None;
      b.at(x,y).tile = contrast::TileType::None;
    }
  b.at(2,2).occupant = contrast::Player::Black;
  s.to_move_ = contrast::Player::Black;
  
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  
  // Count base moves without tile placement
  std::set<std::pair<int,int>> dests;
  for (const auto& m : moves) {
    if (!m.place_tile && m.sx == 2 && m.sy == 2) {
      dests.emplace(m.dx, m.dy);
    }
  }
  
  // Should have 4 orthogonal destinations
  ASSERT_EQ(dests.size(), 4) << "White tile should allow 4 orthogonal directions";
  
  EXPECT_TRUE(dests.count({2,1})) << "up";
  EXPECT_TRUE(dests.count({2,3})) << "down";
  EXPECT_TRUE(dests.count({1,2})) << "left";
  EXPECT_TRUE(dests.count({3,2})) << "right";
}

TEST(RulesTest, DiagDirectionsOnBlackTile) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear board, place Black piece at (2,2) on black tile
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x) {
      b.at(x,y).occupant = contrast::Player::None;
      b.at(x,y).tile = contrast::TileType::None;
    }
  b.at(2,2).occupant = contrast::Player::Black;
  b.at(2,2).tile = contrast::TileType::Black;
  s.to_move_ = contrast::Player::Black;
  
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  
  // Should have 4 diagonal moves
  std::set<std::pair<int,int>> dests;
  for (const auto& m : moves) {
    if (!m.place_tile && m.sx == 2 && m.sy == 2) dests.emplace(m.dx, m.dy);
  }
  EXPECT_TRUE(dests.count({1,1})) << "up-left";
  EXPECT_TRUE(dests.count({3,1})) << "up-right";
  EXPECT_TRUE(dests.count({1,3})) << "down-left";
  EXPECT_TRUE(dests.count({3,3})) << "down-right";
}

TEST(RulesTest, EightDirectionsOnGrayTile) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear board, place Black piece at (2,2) on gray tile
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x) {
      b.at(x,y).occupant = contrast::Player::None;
      b.at(x,y).tile = contrast::TileType::None;
    }
  b.at(2,2).occupant = contrast::Player::Black;
  b.at(2,2).tile = contrast::TileType::Gray;
  s.to_move_ = contrast::Player::Black;
  
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  
  // Should have 8 directions
  std::set<std::pair<int,int>> dests;
  for (const auto& m : moves) {
    if (!m.place_tile && m.sx == 2 && m.sy == 2) dests.emplace(m.dx, m.dy);
  }
  EXPECT_EQ(dests.size(), 8) << "Gray tile should allow 8 directions";
}

TEST(RulesTest, JumpOverOwnPiece) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear board
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x) {
      b.at(x,y).occupant = contrast::Player::None;
      b.at(x,y).tile = contrast::TileType::None;
    }
  
  // Place Black pieces at (2,2) and (2,3)
  b.at(2,2).occupant = contrast::Player::Black;
  b.at(2,3).occupant = contrast::Player::Black;
  s.to_move_ = contrast::Player::Black;
  
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  
  // From (2,2), should be able to jump over (2,3) to (2,4)
  bool found_jump = false;
  for (const auto& m : moves) {
    if (m.sx == 2 && m.sy == 2 && m.dx == 2 && m.dy == 4 && !m.place_tile) {
      found_jump = true;
      break;
    }
  }
  EXPECT_TRUE(found_jump) << "Should be able to jump over own piece";
}

TEST(RulesTest, CannotJumpOverOpponentPiece) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear board
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x) {
      b.at(x,y).occupant = contrast::Player::None;
      b.at(x,y).tile = contrast::TileType::None;
    }
  
  // Place Black at (2,2) and White at (2,3)
  b.at(2,2).occupant = contrast::Player::Black;
  b.at(2,3).occupant = contrast::Player::White;
  s.to_move_ = contrast::Player::Black;
  
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  
  // From (2,2) down direction should be blocked by White piece
  bool has_down_move = false;
  for (const auto& m : moves) {
    if (m.sx == 2 && m.sy == 2 && m.dx == 2 && m.dy == 3 && !m.place_tile) {
      has_down_move = true;
    }
  }
  EXPECT_FALSE(has_down_move) << "Cannot move to cell occupied by opponent";
}

TEST(RulesTest, WinConditionBlackReachesBottomRow) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear and place Black piece at bottom row
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x)
      b.at(x,y).occupant = contrast::Player::None;
  
  b.at(2, b.height()-1).occupant = contrast::Player::Black;
  
  EXPECT_TRUE(contrast::Rules::is_win(s, contrast::Player::Black)) 
    << "Black should win when reaching bottom row";
  EXPECT_FALSE(contrast::Rules::is_win(s, contrast::Player::White));
}

TEST(RulesTest, WinConditionWhiteReachesTopRow) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear and place White piece at top row
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x)
      b.at(x,y).occupant = contrast::Player::None;
  
  b.at(2, 0).occupant = contrast::Player::White;
  
  EXPECT_TRUE(contrast::Rules::is_win(s, contrast::Player::White))
    << "White should win when reaching top row";
  EXPECT_FALSE(contrast::Rules::is_win(s, contrast::Player::Black));
}

TEST(RulesTest, TilePlacementDecreasesInventory) {
  contrast::GameState s;
  s.reset();
  auto& b = s.board();
  
  // Clear board and place one piece
  for (int y = 0; y < b.height(); ++y)
    for (int x = 0; x < b.width(); ++x) {
      b.at(x,y).occupant = contrast::Player::None;
      b.at(x,y).tile = contrast::TileType::None;
    }
  b.at(2,2).occupant = contrast::Player::Black;
  s.to_move_ = contrast::Player::Black;
  
  int initial_black = s.inventory(contrast::Player::Black).black;
  EXPECT_EQ(initial_black, 3) << "Initial black tile inventory should be 3";
  
  // Apply move with black tile placement
  contrast::Move m;
  m.sx = 2; m.sy = 2;
  m.dx = 2; m.dy = 3;
  m.place_tile = true;
  m.tx = 1; m.ty = 1;
  m.tile = contrast::TileType::Black;
  
  s.apply_move(m);
  
  int after_black = s.inventory(contrast::Player::Black).black;
  EXPECT_EQ(after_black, 2) << "Black tile inventory should decrease after placement";
  EXPECT_EQ(b.at(1,1).tile, contrast::TileType::Black) << "Tile should be placed at target location";
}
