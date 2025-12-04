#include "contrast/rules.hpp"
#include "contrast/game_state.hpp"
#include "contrast/precomputed_move_table.hpp"

namespace contrast {

void Rules::legal_moves(const GameState& s, MoveList& out) {
  out.clear();
  
  const Board& b = s.board();
  Player p = s.current_player();
  
  // Temporary storage for base moves (without tile placement)
  MoveList base_moves;

  for (int y = 0; y < b.height(); ++y) {
    for (int x = 0; x < b.width(); ++x) {
      if (b.at(x,y).occupant != p) continue;

      const auto tile = b.at(x, y).tile;
      const auto tile_index = static_cast<int>(tile);
      const int origin_index = y * b.width() + x;
      const auto& table_entry = kMoveTable[tile_index][origin_index];

      for (uint8_t dir_idx = 0; dir_idx < table_entry.dir_count; ++dir_idx) {
        const auto& dir = table_entry.dirs[dir_idx];
        if (dir.step_count == 0) continue;

        bool encountered_friend = false;

        for (uint8_t step = 0; step < dir.step_count; ++step) {
          int target_index = origin_index + dir.rel_index[step];
          int ty = target_index / b.width();
          int tx = target_index % b.width();
          const auto& cell = b.at(tx, ty);

          if (cell.occupant == Player::None) {
            if (!encountered_friend) {
              if (step == 0) {
                Move m;
                m.sx = x; m.sy = y; m.dx = tx; m.dy = ty; m.place_tile = false;
                base_moves.push_back(m);
              }
            } else {
              Move m;
              m.sx = x; m.sy = y; m.dx = tx; m.dy = ty; m.place_tile = false;
              base_moves.push_back(m);
            }
            break;
          }

          if (cell.occupant == p) {
            encountered_friend = true;
            continue;
          }

          // Opponent blocks further progression along this ray
          break;
        }
      }
    }
  }

  // For each base move, add optional tile placement variants
  const TileInventory& inv = s.inventory(p);
  
  for (size_t i = 0; i < base_moves.size; ++i) {
    const Move& base = base_moves[i];
    
    // No tile placement
    out.push_back(base);
    
    // With black tile placement
    if (inv.black > 0) {
      for (int y = 0; y < b.height(); ++y) {
        for (int x = 0; x < b.width(); ++x) {
          if (b.at(x,y).occupant == Player::None && b.at(x,y).tile == TileType::None) {
            Move m = base; 
            m.place_tile = true; 
            m.tx = x; 
            m.ty = y; 
            m.tile = TileType::Black;
            out.push_back(m);
          }
        }
      }
    }
    
    // With gray tile placement
    if (inv.gray > 0) {
      for (int y = 0; y < b.height(); ++y) {
        for (int x = 0; x < b.width(); ++x) {
          if (b.at(x,y).occupant == Player::None && b.at(x,y).tile == TileType::None) {
            Move m = base;
            m.place_tile = true;
            m.tx = x;
            m.ty = y;
            m.tile = TileType::Gray;
            out.push_back(m);
          }
        }
      }
    }
  }
}

bool Rules::is_win(const GameState& s, Player p) {
  const Board& b = s.board();
  // player wins if any of their pieces on opponent's back row
  int target_row = (p == Player::Black) ? (b.height()-1) : 0;
  for (int x = 0; x < b.width(); ++x) if (b.at(x,target_row).occupant == p) return true;
  return false;
}

bool Rules::is_loss(const GameState& s, Player p) {
  // loss if no legal moves
  MoveList moves;
  legal_moves(s, moves);
  return moves.empty();
}

} // namespace contrast
