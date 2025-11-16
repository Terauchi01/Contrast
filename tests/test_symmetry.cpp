#include <gtest/gtest.h>
#include "contrast/symmetry.hpp"
#include "contrast/board.hpp"
#include "contrast/game_state.hpp"

using namespace contrast;

TEST(SymmetryTest, IdentityTransform) {
  GameState state;
  state.reset();
  
  Board original = state.board();
  Board transformed = SymmetryOps::transform_board(original, Symmetry::Identity);
  
  // Identity変換では何も変わらない
  for (int y = 0; y < BOARD_H; ++y) {
    for (int x = 0; x < BOARD_W; ++x) {
      EXPECT_EQ(original.at(x, y).occupant, transformed.at(x, y).occupant);
      EXPECT_EQ(original.at(x, y).tile, transformed.at(x, y).tile);
    }
  }
}

TEST(SymmetryTest, HorizontalFlip) {
  GameState state;
  state.reset();
  
  Board original = state.board();
  Board flipped = SymmetryOps::transform_board(original, Symmetry::FlipH);
  
  // 水平反転: (x, y) -> (4-x, y)
  for (int y = 0; y < BOARD_H; ++y) {
    for (int x = 0; x < BOARD_W; ++x) {
      EXPECT_EQ(original.at(x, y).occupant, flipped.at(BOARD_W - 1 - x, y).occupant);
      EXPECT_EQ(original.at(x, y).tile, flipped.at(BOARD_W - 1 - x, y).tile);
    }
  }
}

TEST(SymmetryTest, DoubleFlipIsIdentity) {
  GameState state;
  state.reset();
  
  Board original = state.board();
  Board flipped_once = SymmetryOps::transform_board(original, Symmetry::FlipH);
  Board flipped_twice = SymmetryOps::transform_board(flipped_once, Symmetry::FlipH);
  
  // 2回反転すると元に戻る
  for (int y = 0; y < BOARD_H; ++y) {
    for (int x = 0; x < BOARD_W; ++x) {
      EXPECT_EQ(original.at(x, y).occupant, flipped_twice.at(x, y).occupant);
      EXPECT_EQ(original.at(x, y).tile, flipped_twice.at(x, y).tile);
    }
  }
}

TEST(SymmetryTest, CanonicalSymmetryIsConsistent) {
  GameState state;
  state.reset();
  
  Board original = state.board();
  Board flipped = SymmetryOps::transform_board(original, Symmetry::FlipH);
  
  // 元のボードと反転したボードの正規化対称性を取得
  Symmetry original_sym = SymmetryOps::get_canonical_symmetry(original);
  Symmetry flipped_sym = SymmetryOps::get_canonical_symmetry(flipped);
  
  // 両方を正規化したボードは同じになるべき
  Board canonical_original = SymmetryOps::transform_board(original, original_sym);
  Board canonical_flipped = SymmetryOps::transform_board(flipped, flipped_sym);
  
  for (int y = 0; y < BOARD_H; ++y) {
    for (int x = 0; x < BOARD_W; ++x) {
      EXPECT_EQ(canonical_original.at(x, y).occupant, canonical_flipped.at(x, y).occupant);
      EXPECT_EQ(canonical_original.at(x, y).tile, canonical_flipped.at(x, y).tile);
    }
  }
}

TEST(SymmetryTest, AsymmetricBoardDetection) {
  GameState state;
  state.reset();
  
  // 非対称な盤面を作る: (1, 2)にタイルを置く
  Board board = state.board();
  board.at(1, 2).tile = TileType::Black;
  
  Board flipped = SymmetryOps::transform_board(board, Symmetry::FlipH);
  
  // (1, 2)のタイルは反転後は(3, 2)にあるべき
  EXPECT_EQ(board.at(1, 2).tile, TileType::Black);
  EXPECT_EQ(flipped.at(3, 2).tile, TileType::Black);
  EXPECT_EQ(flipped.at(1, 2).tile, TileType::None);
}

TEST(SymmetryTest, SymmetricBoardStaysCanonical) {
  GameState state;
  state.reset();
  
  // 完全に左右対称な盤面
  Board board = state.board();
  board.at(1, 2).tile = TileType::Black;
  board.at(3, 2).tile = TileType::Black;  // 対称位置
  board.at(2, 3).tile = TileType::Gray;   // 中央
  
  Symmetry sym = SymmetryOps::get_canonical_symmetry(board);
  Board canonical = SymmetryOps::transform_board(board, sym);
  
  // 対称な盤面は変換してもハッシュ値が同じか、辞書順で決まる
  // いずれにせよ、正規化後は一貫しているべき
  Symmetry sym2 = SymmetryOps::get_canonical_symmetry(canonical);
  EXPECT_EQ(sym2, Symmetry::Identity);  // 正規化済みなので恒等変換
}
