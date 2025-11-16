#include "httplib.h"
#include "game_session.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>

using namespace contrast_server;

// Global session storage
std::map<std::string, std::shared_ptr<GameSession>> g_sessions;
std::mutex g_sessions_mutex;

// Generate random session ID
std::string generate_session_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int i = 0; i < 16; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// Parse AI type from string
AIType parse_ai_type(const std::string& str) {
    if (str == "greedy") return AIType::GREEDY;
    if (str == "rulebased") return AIType::RULEBASED;
    if (str == "ntuple") return AIType::NTUPLE;
    return AIType::NONE;
}

// Get legal moves as JSON
std::string legal_moves_to_json(const contrast::GameState& state) {
    contrast::MoveList moves;
    contrast::Rules::legal_moves(state, moves);
    
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < moves.size; ++i) {
        if (i > 0) oss << ",";
        const auto& m = moves[i];
        oss << "\n  {\"sx\":" << m.sx << ",\"sy\":" << m.sy
            << ",\"dx\":" << m.dx << ",\"dy\":" << m.dy
            << ",\"tile\":\"";
        if (m.tile == contrast::TileType::Black) oss << "black";
        else if (m.tile == contrast::TileType::Gray) oss << "gray";
        else oss << "none";
        oss << "\",\"tile_x\":" << m.tile_x << ",\"tile_y\":" << m.tile_y << "}";
    }
    oss << "\n]";
    return oss.str();
}

int main() {
    httplib::Server svr;
    
    // Enable CORS for all routes
    svr.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });
    
    // Handle OPTIONS requests for CORS preflight
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });
    
    // Serve static files
    svr.set_mount_point("/", "./web");
    
    // API: Create new game session
    // POST /api/game/new
    // Body: { "white_ai": "greedy|rulebased|ntuple|human", "black_ai": "..." }
    svr.Post("/api/game/new", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = generate_session_id();
        
        // Parse AI types from request body
        AIType white_ai = AIType::NONE;
        AIType black_ai = AIType::NONE;
        
        if (!req.body.empty()) {
            // Simple JSON parsing (production code should use proper JSON library)
            size_t white_pos = req.body.find("\"white_ai\"");
            size_t black_pos = req.body.find("\"black_ai\"");
            
            if (white_pos != std::string::npos) {
                size_t colon = req.body.find(":", white_pos);
                size_t quote1 = req.body.find("\"", colon);
                size_t quote2 = req.body.find("\"", quote1 + 1);
                std::string value = req.body.substr(quote1 + 1, quote2 - quote1 - 1);
                white_ai = parse_ai_type(value);
            }
            
            if (black_pos != std::string::npos) {
                size_t colon = req.body.find(":", black_pos);
                size_t quote1 = req.body.find("\"", colon);
                size_t quote2 = req.body.find("\"", quote1 + 1);
                std::string value = req.body.substr(quote1 + 1, quote2 - quote1 - 1);
                black_ai = parse_ai_type(value);
            }
        }
        
        auto session = std::make_shared<GameSession>(session_id, white_ai, black_ai);
        
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            g_sessions[session_id] = session;
        }
        
        res.set_content(session->to_json(), "application/json");
        std::cout << "Created new game session: " << session_id << "\n";
    });
    
    // API: Get game state
    // GET /api/game/:session_id
    svr.Get(R"(/api/game/([^/]+))", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }
        
        res.set_content(session->to_json(), "application/json");
    });

    // API: Get textual board representation
    // GET /api/game/:session_id/board_text
    svr.Get(R"(/api/game/([^/]+)/board_text)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("Session not found", "text/plain");
                return;
            }
            session = it->second;
        }
        res.set_content(session->board_text(), "text/plain");
    });
    
    // API: Get legal moves
    // GET /api/game/:session_id/moves
    svr.Get(R"(/api/game/([^/]+)/moves)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }
        
        std::string moves_json = legal_moves_to_json(session->state);
        res.set_content(moves_json, "application/json");
    });
    
    // API: Make a move
    // POST /api/game/:session_id/move
    // Body: { "sx": 0, "sy": 0, "dx": 1, "dy": 1, "tile": "black|gray|none", "tile_x": 2, "tile_y": 2 }
    svr.Post(R"(/api/game/([^/]+)/move)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }
        
        // Parse move from JSON body (simple parsing)
        contrast::Move move{};
        
        // Extract values (production code should use proper JSON library)
        std::istringstream iss(req.body);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("\"sx\"") != std::string::npos) {
                size_t colon = line.find(":");
                move.sx = std::stoi(line.substr(colon + 1));
            } else if (line.find("\"sy\"") != std::string::npos) {
                size_t colon = line.find(":");
                move.sy = std::stoi(line.substr(colon + 1));
            } else if (line.find("\"dx\"") != std::string::npos) {
                size_t colon = line.find(":");
                move.dx = std::stoi(line.substr(colon + 1));
            } else if (line.find("\"dy\"") != std::string::npos) {
                size_t colon = line.find(":");
                move.dy = std::stoi(line.substr(colon + 1));
            } else if (line.find("\"tile_x\"") != std::string::npos) {
                size_t colon = line.find(":");
                move.tile_x = std::stoi(line.substr(colon + 1));
            } else if (line.find("\"tile_y\"") != std::string::npos) {
                size_t colon = line.find(":");
                move.tile_y = std::stoi(line.substr(colon + 1));
            } else if (line.find("\"tile\"") != std::string::npos) {
                if (line.find("black") != std::string::npos) {
                    move.tile = contrast::TileType::Black;
                } else if (line.find("gray") != std::string::npos) {
                    move.tile = contrast::TileType::Gray;
                } else {
                    move.tile = contrast::TileType::None;
                }
            }
        }
        
        if (!session->apply_move(move)) {
            res.status = 400;
            res.set_content("{\"error\":\"Illegal move\"}", "application/json");
            return;
        }
        
        res.set_content(session->to_json(), "application/json");
        std::cout << "Move applied in session " << session_id << "\n";
    });

    // API: Make a move using textual format
    // POST /api/game/:session_id/move_text
    // Body: plain text like "b1,b2 b3g" or "c5,c4"
    svr.Post(R"(/api/game/([^/]+)/move_text)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];

        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }

        std::string err;
        std::string body = req.body;
        // trim CR/LF
        while (!body.empty() && (body.back() == '\n' || body.back() == '\r')) body.pop_back();

        if (!session->apply_move_text(body, err)) {
            res.status = 400;
            std::ostringstream oss;
            oss << "{\"error\":\"" << err << "\"}";
            res.set_content(oss.str(), "application/json");
            return;
        }

        res.set_content(session->to_json(), "application/json");
        std::cout << "Text move applied in session " << session_id << " : " << body << "\n";
    });
    
    // API: Get AI move
    // GET /api/game/:session_id/ai_move
    svr.Get(R"(/api/game/([^/]+)/ai_move)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }
        
        if (!session->is_current_player_ai()) {
            res.status = 400;
            res.set_content("{\"error\":\"Current player is not AI\"}", "application/json");
            return;
        }
        
        contrast::Move ai_move = session->get_ai_move();
        if (!session->apply_move(ai_move)) {
            res.status = 500;
            res.set_content("{\"error\":\"AI produced illegal move\"}", "application/json");
            return;
        }
        
        res.set_content(session->to_json(), "application/json");
        std::cout << "AI move applied in session " << session_id << "\n";
    });
    
    // API: Reset game
    // POST /api/game/:session_id/reset
    svr.Post(R"(/api/game/([^/]+)/reset)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }
        
        session->reset();
        res.set_content(session->to_json(), "application/json");
        std::cout << "Reset game session: " << session_id << "\n";
    });
    
    // API: Get board as 1D array (29 elements: 25 board + 4 tile inventory)
    // GET /api/game/:session_id/board_array
    svr.Get(R"(/api/game/([^/]+)/board_array)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }
        
        std::vector<int> array = session->board_to_array();
        
        // Build JSON array with tile inventory info
        std::ostringstream oss;
        oss << "{\"board_array\":[";
        for (size_t i = 0; i < array.size(); ++i) {
            if (i > 0) oss << ",";
            oss << array[i];
        }
        oss << "],\"encoding\":\"occupant*3+tile\",\"size\":29,"
            << "\"format\":\"row-major\","
            << "\"structure\":{\"board_cells\":\"[0-24]\",\"black_player_tiles\":\"[25-26]\",\"white_player_tiles\":\"[27-28]\"},"
            << "\"tile_inventory\":{"
            << "\"black_player\":{\"black\":" << array[25] << ",\"gray\":" << array[26] << "},"
            << "\"white_player\":{\"black\":" << array[27] << ",\"gray\":" << array[28] << "}"
            << "}}";
        
        res.set_content(oss.str(), "application/json");
    });
    
    // API: Set board from 1D array
    // POST /api/game/:session_id/board_array
    // Body: { "board_array": [0,3,3,3,0, ..., 3,1,3,1] } (29 elements: 25 board + 4 tile inventory)
    svr.Post(R"(/api/game/([^/]+)/board_array)", [](const httplib::Request& req, httplib::Response& res) {
        std::string session_id = req.matches[1];
        
        std::shared_ptr<GameSession> session;
        {
            std::lock_guard<std::mutex> lock(g_sessions_mutex);
            auto it = g_sessions.find(session_id);
            if (it == g_sessions.end()) {
                res.status = 404;
                res.set_content("{\"error\":\"Session not found\"}", "application/json");
                return;
            }
            session = it->second;
        }
        
        // Parse JSON body
        std::string body = req.body;
        size_t array_start = body.find("[");
        size_t array_end = body.find("]");
        if (array_start == std::string::npos || array_end == std::string::npos) {
            res.status = 400;
            res.set_content("{\"error\":\"Missing board_array\"}", "application/json");
            return;
        }
        
        std::string array_str = body.substr(array_start + 1, array_end - array_start - 1);
        std::vector<int> array;
        std::istringstream iss(array_str);
        std::string token;
        while (std::getline(iss, token, ',')) {
            try {
                array.push_back(std::stoi(token));
            } catch (...) {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid array value\"}", "application/json");
                return;
            }
        }
        
        std::string err_msg;
        if (!session->array_to_board(array, err_msg)) {
            res.status = 400;
            res.set_content("{\"error\":\"" + err_msg + "\"}", "application/json");
            return;
        }
        
        res.set_content(session->to_json(), "application/json");
        std::cout << "Board set from array in session " << session_id << "\n";
    });
    
    std::cout << "=================================\n";
    std::cout << "Contrast Game Web Server\n";
    std::cout << "=================================\n";
    std::cout << "Server starting on http://localhost:8080\n";
    std::cout << "\nAPI Endpoints:\n";
    std::cout << "  POST   /api/game/new                  - Create new game\n";
    std::cout << "  GET    /api/game/:id                  - Get game state\n";
    std::cout << "  GET    /api/game/:id/moves            - Get legal moves\n";
    std::cout << "  POST   /api/game/:id/move             - Make a move\n";
    std::cout << "  GET    /api/game/:id/ai_move          - Get AI move\n";
    std::cout << "  POST   /api/game/:id/reset            - Reset game\n";
    std::cout << "  GET    /api/game/:id/board_text       - Get ASCII board\n";
    std::cout << "  POST   /api/game/:id/move_text        - Make move (text format)\n";
    std::cout << "  GET    /api/game/:id/board_array      - Get board as 1D array\n";
    std::cout << "  POST   /api/game/:id/board_array      - Set board from 1D array\n";
    std::cout << "\nStatic files served from: ./web/\n";
    std::cout << "=================================\n\n";
    
    svr.listen("0.0.0.0", 8080);
    
    return 0;
}
