#include "game_session.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>

namespace contrast_server {

GameSession::GameSession(const std::string& id, AIType white, AIType black)
    : session_id(id), white_ai(white), black_ai(black) {
    state.reset();
    
    // Initialize AI players
    if (white_ai == AIType::GREEDY) {
        greedy_white = std::make_unique<contrast_ai::GreedyPolicy>();
    } else if (white_ai == AIType::RULEBASED) {
        rulebased_white = std::make_unique<contrast_ai::RuleBasedPolicy>();
    }
    
    if (black_ai == AIType::GREEDY) {
        greedy_black = std::make_unique<contrast_ai::GreedyPolicy>();
    } else if (black_ai == AIType::RULEBASED) {
        rulebased_black = std::make_unique<contrast_ai::RuleBasedPolicy>();
    }
}

bool GameSession::apply_move(const contrast::Move& move) {
    // Validate move is legal
    contrast::MoveList legal_moves;
    contrast::Rules::legal_moves(state, legal_moves);
    
    bool is_legal = false;
    for (const auto& legal_move : legal_moves) {
        if (legal_move.sx == move.sx && legal_move.sy == move.sy &&
            legal_move.dx == move.dx && legal_move.dy == move.dy &&
            legal_move.tile == move.tile &&
            legal_move.tile_x == move.tile_x && legal_move.tile_y == move.tile_y) {
            is_legal = true;
            break;
        }
    }
    
    if (!is_legal) {
        return false;
    }
    
    state.apply_move(move);
    move_history.push_back(move);
    return true;
}

contrast::Move GameSession::get_ai_move() {
    contrast::Player current = state.current_player();
    AIType ai_type = (current == contrast::Player::White) ? white_ai : black_ai;
    
    if (ai_type == AIType::GREEDY) {
        auto& policy = (current == contrast::Player::White) ? greedy_white : greedy_black;
        return policy->pick(state);
    } else if (ai_type == AIType::RULEBASED) {
        auto& policy = (current == contrast::Player::White) ? rulebased_white : rulebased_black;
        return policy->pick(state);
    } else if (ai_type == AIType::NTUPLE) {
        auto& policy = (current == contrast::Player::White) ? ntuple_white : ntuple_black;
        return policy->pick(state);
    }
    
    // Return invalid move if no AI
    return contrast::Move{};
}

bool GameSession::is_current_player_ai() const {
    contrast::Player current = state.current_player();
    AIType ai_type = (current == contrast::Player::White) ? white_ai : black_ai;
    return ai_type != AIType::NONE;
}

void GameSession::reset() {
    state.reset();
    move_history.clear();
}

std::string GameSession::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"session_id\": \"" << session_id << "\",\n";
    oss << "  \"current_player\": \"" 
        << (state.current_player() == contrast::Player::Black ? "black" : "white") << "\",\n";
    
    // Board state
    oss << "  \"board\": {\n";
    oss << "    \"pieces\": [";
    bool first_piece = true;
    for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
            auto piece = state.board().piece_at(x, y);
            if (piece != contrast::Player::None) {
                if (!first_piece) oss << ",";
                oss << "\n      {\"x\":" << x << ",\"y\":" << y 
                    << ",\"color\":\"" << (piece == contrast::Player::Black ? "black" : "white") << "\"}";
                first_piece = false;
            }
        }
    }
    oss << "\n    ],\n";
    
    // Tiles
    oss << "    \"tiles\": [";
    bool first_tile = true;
    for (int y = 0; y < 9; ++y) {
        for (int x = 0; x < 9; ++x) {
            auto tile = state.board().tile_at(x, y);
            if (tile != contrast::TileType::None) {
                if (!first_tile) oss << ",";
                oss << "\n      {\"x\":" << x << ",\"y\":" << y 
                    << ",\"type\":\"" << (tile == contrast::TileType::Black ? "black" : "gray") << "\"}";
                first_tile = false;
            }
        }
    }
    oss << "\n    ]\n";
    oss << "  },\n";
    
    // Game status
    bool black_win = contrast::Rules::is_win(state, contrast::Player::Black);
    bool white_win = contrast::Rules::is_win(state, contrast::Player::White);
    contrast::MoveList moves;
    contrast::Rules::legal_moves(state, moves);
    bool no_moves = moves.empty();
    
    oss << "  \"status\": \"";
    if (black_win) {
        oss << "black_wins";
    } else if (white_win) {
        oss << "white_wins";
    } else if (no_moves) {
        oss << (state.current_player() == contrast::Player::Black ? "white_wins" : "black_wins");
    } else {
        oss << "in_progress";
    }
    oss << "\",\n";
    
    // AI configuration
    oss << "  \"ai\": {\n";
    oss << "    \"white\": \"";
    if (white_ai == AIType::GREEDY) oss << "greedy";
    else if (white_ai == AIType::RULEBASED) oss << "rulebased";
    else if (white_ai == AIType::NTUPLE) oss << "ntuple";
    else oss << "human";
    oss << "\",\n";
    oss << "    \"black\": \"";
    if (black_ai == AIType::GREEDY) oss << "greedy";
    else if (black_ai == AIType::RULEBASED) oss << "rulebased";
    else if (black_ai == AIType::NTUPLE) oss << "ntuple";
    else oss << "human";
    oss << "\"\n";
    oss << "  }\n";
    
    oss << "}";
    return oss.str();
}

// Helper: map file letter 'a'.. to x (0-based). Returns -1 on error
static int file_to_x(char f) {
    if (f >= 'a' && f <= 'z') f = static_cast<char>(f - 'a' + 'a');
    if (f >= 'A' && f <= 'Z') f = static_cast<char>(f - 'A' + 'a');
    if (f < 'a' || f > 'z') return -1;
    int x = f - 'a';
    return x;
}

// Helper: parse rank digit '1'.. -> y index (board origin y=0 is top)
static int rank_to_y(char r) {
    if (r < '1' || r > '9') return -1;
    int rank = r - '0';
    // board height is 5 in this project
    const int H = contrast::BOARD_H;
    if (rank < 1 || rank > H) return -1;
    // user ranks: 1 is bottom, H is top -> y = H - rank
    return H - rank;
}

std::string GameSession::board_text() const {
    std::ostringstream oss;
    const int W = state.board().width();
    const int H = state.board().height();

    // We'll render each cell as 3 chars wide for alignment.
    // Conventions:
    //  - Empty cell (no tile): "   " (3 spaces)
    //  - Black piece: " x "
    //  - White piece: " o "
    //  - Black tile (no piece): "[ ]"
    //  - Gray tile (no piece):  "( )"
    //  - If piece is on a tile, piece takes precedence in display

    for (int y = 0; y < H; ++y) {
        int rank = H - y; // display rank
        // left margin space
        oss << "    " ;
        // row label
        oss << std::setw(2) << rank << "| ";
        for (int x = 0; x < W; ++x) {
            const auto& cell = state.board().at(x, y);
            if (cell.occupant != contrast::Player::None) {
                // piece
                if (cell.occupant == contrast::Player::Black) oss << " x ";
                else oss << " o ";
            } else if (cell.tile != contrast::TileType::None) {
                if (cell.tile == contrast::TileType::Black) oss << "[ ]";
                else oss << "( )"; // Gray tile
            } else {
                oss << "   "; // empty cell (white tile = nothing)
            }
            if (x + 1 < W) oss << ' ';
        }
        oss << " |\n";
    }

    // files
    oss << "       ";
    for (int x = 0; x < W; ++x) {
        char file = static_cast<char>('a' + x);
        oss << " " << file << "  ";
    }
    oss << "\n";
    return oss.str();
}

bool GameSession::apply_move_text(const std::string& move_str, std::string& err_msg) {
    // Trim
    auto s = move_str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch){ return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch){ return !std::isspace(ch); }).base(), s.end());
    if (s.empty()) { err_msg = "empty move"; return false; }

    // Split by whitespace: first token is from,to ; optional second is tile
    std::istringstream iss(s);
    std::string first, second;
    if (!(iss >> first)) { err_msg = "invalid move format"; return false; }
    iss >> second; // may be empty

    // parse first: expect "<from>,<to>" where each coord like a1
    auto comma_pos = first.find(',');
    if (comma_pos == std::string::npos) { err_msg = "expected from,to"; return false; }
    std::string from = first.substr(0, comma_pos);
    std::string to = first.substr(comma_pos+1);
    if (from.size() < 2 || to.size() < 2) { err_msg = "invalid coordinates"; return false; }

    int sx = file_to_x(from[0]);
    int sy = rank_to_y(from[1]);
    int dx = file_to_x(to[0]);
    int dy = rank_to_y(to[1]);
    if (sx < 0 || sy < 0 || dx < 0 || dy < 0) { err_msg = "coordinate out of range"; return false; }

    contrast::Move m;
    m.sx = sx; m.sy = sy; m.dx = dx; m.dy = dy;

    if (!second.empty()) {
        // parse tile like b3g -> pos b3 and color g/b
        if (second.size() < 3) { err_msg = "invalid tile token"; return false; }
        char fx = second[0];
        char ry = second[1];
        char color = second[2];
        int tx = file_to_x(fx);
        int ty = rank_to_y(ry);
        if (tx < 0 || ty < 0) { err_msg = "tile coordinate out of range"; return false; }
        m.place_tile = true;
        m.tx = tx; m.ty = ty;
        if (color == 'g' || color == 'G') m.tile = contrast::TileType::Gray;
        else if (color == 'b' || color == 'B') m.tile = contrast::TileType::Black;
        else { err_msg = "unknown tile color"; return false; }
    }

    // Validate and apply via existing apply_move
    if (!apply_move(m)) {
        err_msg = "illegal move";
        return false;
    }
    return true;
}

// ============================================================================
// Board encoding/decoding (1D array representation)
// ============================================================================

/**
 * Encode a cell to 0-8 integer
 * Same encoding as NTuple AI: occupant * 3 + tile
 * - Empty + No tile = 0*3 + 0 = 0
 * - Black piece + No tile = 1*3 + 0 = 3
 * - Black piece + Black tile = 1*3 + 1 = 4
 * - White piece + Gray tile = 2*3 + 2 = 8
 */
int GameSession::encode_cell(const contrast::Cell& cell) {
    int occ = static_cast<int>(cell.occupant);
    int tile = static_cast<int>(cell.tile);
    return occ * 3 + tile;
}

/**
 * Decode 0-8 value to cell state
 * Returns false if value is out of range (0-8)
 */
bool GameSession::decode_cell(int value, contrast::Cell& cell) {
    if (value < 0 || value > 8) {
        return false;
    }
    int occ = value / 3;
    int tile = value % 3;
    cell.occupant = static_cast<contrast::Player>(occ);
    cell.tile = static_cast<contrast::TileType>(tile);
    return true;
}

/**
 * Convert full game state to 1D array (29 elements)
 * Format: [board cells (25) + tile inventory (4)]
 * 
 * Board cells (0-24): row-major order, y=0 (rank 5) to y=4 (rank 1)
 *   Each cell encoded as: occupant * 3 + tile (0-8)
 * 
 * Tile inventory (25-28):
 *   [25]: Black player's remaining black tiles (0-3)
 *   [26]: Black player's remaining gray tiles (0-1)
 *   [27]: White player's remaining black tiles (0-3)
 *   [28]: White player's remaining gray tiles (0-1)
 * 
 * This extended encoding provides complete game state information,
 * including strategic tile availability which is crucial for endgame play.
 */
std::vector<int> GameSession::board_to_array() const {
    std::vector<int> array;
    array.reserve(29);
    
    // Board cells (25 elements): row-major order
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            const contrast::Cell& cell = state.board().at(x, y);
            array.push_back(encode_cell(cell));
        }
    }
    
    // Tile inventory (4 elements)
    const contrast::TileInventory& black_inv = state.inventory(contrast::Player::Black);
    const contrast::TileInventory& white_inv = state.inventory(contrast::Player::White);
    
    array.push_back(black_inv.black);  // [25]
    array.push_back(black_inv.gray);   // [26]
    array.push_back(white_inv.black);  // [27]
    array.push_back(white_inv.gray);   // [28]
    
    return array;
}

/**
 * Set game state from 1D array (29 elements)
 * 
 * Array format:
 *   [0-24]: Board cells (row-major)
 *   [25-28]: Tile inventory (black_black, black_gray, white_black, white_gray)
 * 
 * Validation:
 *   - Array size must be exactly 29
 *   - Board cells must be 0-8
 *   - Black tiles must be 0-3
 *   - Gray tiles must be 0-1
 * 
 * Returns false if array is invalid
 */
bool GameSession::array_to_board(const std::vector<int>& array, std::string& err_msg) {
    if (array.size() != 29) {
        err_msg = "array size must be 29 (25 board + 4 tile inventory)";
        return false;
    }
    
    // Validate board cells (0-24)
    for (size_t i = 0; i < 25; ++i) {
        if (array[i] < 0 || array[i] > 8) {
            err_msg = "invalid cell value at index " + std::to_string(i) + ": " + std::to_string(array[i]);
            return false;
        }
    }
    
    // Validate tile inventory (25-28)
    // Black player's black tiles [25]
    if (array[25] < 0 || array[25] > 3) {
        err_msg = "invalid black player black tiles at [25]: " + std::to_string(array[25]);
        return false;
    }
    // Black player's gray tiles [26]
    if (array[26] < 0 || array[26] > 1) {
        err_msg = "invalid black player gray tiles at [26]: " + std::to_string(array[26]);
        return false;
    }
    // White player's black tiles [27]
    if (array[27] < 0 || array[27] > 3) {
        err_msg = "invalid white player black tiles at [27]: " + std::to_string(array[27]);
        return false;
    }
    // White player's gray tiles [28]
    if (array[28] < 0 || array[28] > 1) {
        err_msg = "invalid white player gray tiles at [28]: " + std::to_string(array[28]);
        return false;
    }
    
    // Apply board cells
    int idx = 0;
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            contrast::Cell cell;
            if (!decode_cell(array[idx], cell)) {
                err_msg = "failed to decode cell at index " + std::to_string(idx);
                return false;
            }
            state.board().at(x, y) = cell;
            ++idx;
        }
    }
    
    // Apply tile inventory
    contrast::TileInventory& black_inv = state.inventory(contrast::Player::Black);
    contrast::TileInventory& white_inv = state.inventory(contrast::Player::White);
    
    black_inv.black = array[25];
    black_inv.gray = array[26];
    white_inv.black = array[27];
    white_inv.gray = array[28];
    
    return true;
}

} // namespace contrast_server
