#pragma once
#include "contrast/game_state.hpp"
#include "contrast/move.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include "contrast_ai/ntuple.hpp"
#include <string>
#include <memory>
#include <vector>

namespace contrast_server {

enum class AIType {
    NONE,        // Human vs Human
    GREEDY,      // Greedy AI
    RULEBASED,   // Rule-based AI
    NTUPLE       // N-tuple AI
};

struct GameSession {
    std::string session_id;
    contrast::GameState state;
    AIType white_ai;  // AI type for white player
    AIType black_ai;  // AI type for black player
    
    std::unique_ptr<contrast_ai::GreedyPolicy> greedy_white;
    std::unique_ptr<contrast_ai::GreedyPolicy> greedy_black;
    std::unique_ptr<contrast_ai::RuleBasedPolicy> rulebased_white;
    std::unique_ptr<contrast_ai::RuleBasedPolicy> rulebased_black;
    std::unique_ptr<contrast_ai::NTuplePolicy> ntuple_white;
    std::unique_ptr<contrast_ai::NTuplePolicy> ntuple_black;
    
    std::vector<contrast::Move> move_history;
    
    GameSession(const std::string& id, AIType white = AIType::NONE, AIType black = AIType::NONE);
    
    // Apply a move and return true if successful
    bool apply_move(const contrast::Move& move);
    
    // Get AI move for current player
    contrast::Move get_ai_move();
    
    // Check if current player is AI
    bool is_current_player_ai() const;
    
    // Reset game
    void reset();
    
    // Produce a human-readable ASCII board representation in the requested format
    // See server-side format expected by client. Returns a multi-line string.
    std::string board_text() const;

    // Parse a move in the textual format described by the client and apply it.
    // Format: "<from>,<to> [<tilePos><tileColor>]" e.g. "b1,b2 b3g" or "c5,c4"
    // tileColor: 'g' = Gray, 'b' = Black
    // Returns true on success, false on parse/validation error (err_msg filled).
    bool apply_move_text(const std::string& move_str, std::string& err_msg);
    
    // Encode full game state to 1D array (29 elements)
    // [0-24]: Board cells (row-major order), each 0-8 encoding (occupant * 3 + tile)
    // [25]: Black player's remaining black tiles (0-3)
    // [26]: Black player's remaining gray tiles (0-1)
    // [27]: White player's remaining black tiles (0-3)
    // [28]: White player's remaining gray tiles (0-1)
    std::vector<int> board_to_array() const;
    
    // Decode 1D array (29 elements) to game state
    // Returns true on success, false if array is invalid
    bool array_to_board(const std::vector<int>& array, std::string& err_msg);
    
    // Helper: encode single cell to 0-8
    static int encode_cell(const contrast::Cell& cell);
    
    // Helper: decode 0-8 value to cell state
    static bool decode_cell(int value, contrast::Cell& cell);
    
    // Serialize game state to JSON string
    std::string to_json() const;
};

} // namespace contrast_server
