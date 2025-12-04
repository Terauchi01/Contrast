#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common/protocol.hpp"
#include "contrast/game_state.hpp"
#include "contrast/move_list.hpp"
#include "contrast/rules.hpp"
#include "contrast/types.hpp"

namespace {

constexpr int kServerPort = 8765;
constexpr int kBacklog = 8;

struct ClientSession {
    int socket{-1};
    std::string role{"spectator"};
    std::string name{"anon"};
    bool active{true};
};

std::mutex g_clients_mutex;
std::vector<std::shared_ptr<ClientSession>> g_clients;
std::mutex g_game_mutex;
contrast::GameState g_state;
std::string g_last_move;
std::string g_status = "ongoing";

char player_to_symbol(contrast::Player player) {
    switch (player) {
        case contrast::Player::Black:
            return 'X';
        case contrast::Player::White:
            return 'O';
        default:
            return '?';
    }
}

contrast::Player role_to_player(const std::string& role) {
    if (role == "X") {
        return contrast::Player::Black;
    }
    if (role == "O") {
        return contrast::Player::White;
    }
    return contrast::Player::None;
}

std::pair<int, int> coord_to_xy(const std::string& coord) {
    const int x = coord[0] - 'a';
    const int rank_index = coord[1] - '1';
    const int y = contrast::BOARD_H - 1 - rank_index;
    return {x, y};
}

std::string xy_to_coord(int x, int y) {
    const char file = static_cast<char>('a' + x);
    const char rank = static_cast<char>('1' + (contrast::BOARD_H - 1 - y));
    return std::string{file, rank};
}

contrast::TileType tile_from_char(char color) {
    if (color == 'b') {
        return contrast::TileType::Black;
    }
    if (color == 'g') {
        return contrast::TileType::Gray;
    }
    return contrast::TileType::None;
}

char tile_to_char(contrast::TileType tile) {
    switch (tile) {
        case contrast::TileType::Black:
            return 'b';
        case contrast::TileType::Gray:
            return 'g';
        default:
            return '-';
    }
}

contrast::Move convert_move(const protocol::Move& move) {
    auto [sx, sy] = coord_to_xy(move.origin);
    auto [dx, dy] = coord_to_xy(move.target);
    contrast::Move core_move;
    core_move.sx = sx;
    core_move.sy = sy;
    core_move.dx = dx;
    core_move.dy = dy;
    if (!move.tile.skip) {
        auto [tx, ty] = coord_to_xy(move.tile.coord);
        core_move.place_tile = true;
        core_move.tx = tx;
        core_move.ty = ty;
        core_move.tile = tile_from_char(move.tile.color);
    }
    return core_move;
}

bool moves_equal(const contrast::Move& a, const contrast::Move& b) {
    if (a.sx != b.sx || a.sy != b.sy || a.dx != b.dx || a.dy != b.dy) {
        return false;
    }
    if (a.place_tile != b.place_tile) {
        return false;
    }
    if (!a.place_tile) {
        return true;
    }
    return a.tx == b.tx && a.ty == b.ty && a.tile == b.tile;
}

protocol::StateSnapshot build_snapshot() {
    protocol::StateSnapshot snapshot;
    const contrast::Board& board = g_state.board();
    for (int y = 0; y < board.height(); ++y) {
        for (int x = 0; x < board.width(); ++x) {
            const auto& cell = board.at(x, y);
            const std::string coord = xy_to_coord(x, y);
            if (cell.occupant != contrast::Player::None) {
                snapshot.pieces[coord] = player_to_symbol(cell.occupant);
            }
            if (cell.tile != contrast::TileType::None) {
                snapshot.tiles[coord] = tile_to_char(cell.tile);
            }
        }
    }
    snapshot.turn = player_to_symbol(g_state.current_player());
    snapshot.status = g_status;
    snapshot.last_move = g_last_move;
    const auto& inv_black = g_state.inventory(contrast::Player::Black);
    const auto& inv_white = g_state.inventory(contrast::Player::White);
    snapshot.stock_black = {{'X', inv_black.black}, {'O', inv_white.black}};
    snapshot.stock_gray = {{'X', inv_black.gray}, {'O', inv_white.gray}};
    return snapshot;
}

void update_status(contrast::Player last_player) {
    if (contrast::Rules::is_win(g_state, last_player)) {
        g_status = std::string(1, player_to_symbol(last_player)) + "_win";
        return;
    }
    const contrast::Player opponent =
        (last_player == contrast::Player::Black) ? contrast::Player::White : contrast::Player::Black;
    if (contrast::Rules::is_loss(g_state, opponent)) {
        g_status = std::string(1, player_to_symbol(last_player)) + "_win";
        return;
    }
    g_status = "ongoing";
}

void send_all(int socket, const std::string& payload) {
    const char* data = payload.data();
    ssize_t remaining = static_cast<ssize_t>(payload.size());
    while (remaining > 0) {
        const ssize_t sent = ::send(socket, data, remaining, 0);
        if (sent <= 0) {
            throw std::runtime_error("send failed: " + std::string(std::strerror(errno)));
        }
        remaining -= sent;
        data += sent;
    }
}

std::optional<std::string> recv_line(int socket, std::string& buffer) {
    for (;;) {
        const auto newline_pos = buffer.find('\n');
        if (newline_pos != std::string::npos) {
            std::string line = buffer.substr(0, newline_pos);
            buffer.erase(0, newline_pos + 1);
            return line;
        }
        char chunk[512];
        const ssize_t received = ::recv(socket, chunk, sizeof(chunk), 0);
        if (received == 0) {
            if (buffer.empty()) {
                return std::nullopt;
            }
            std::string line = buffer;
            buffer.clear();
            return line;
        }
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return std::nullopt;
        }
        buffer.append(chunk, received);
    }
}

void send_state_to(int socket, const protocol::StateSnapshot& snapshot) {
    const std::string message = protocol::build_state_message(snapshot);
    send_all(socket, message);
}

void broadcast_state(const protocol::StateSnapshot& snapshot) {
    const std::string message = protocol::build_state_message(snapshot);
    std::lock_guard<std::mutex> lock(g_clients_mutex);
    for (auto it = g_clients.begin(); it != g_clients.end();) {
        auto session = *it;
        if (!session->active) {
            it = g_clients.erase(it);
            continue;
        }
        try {
            send_all(session->socket, message);
            ++it;
        } catch (...) {
            session->active = false;
            ::close(session->socket);
            it = g_clients.erase(it);
        }
    }
}

void send_info(int socket, const std::string& text) {
    send_all(socket, "INFO " + text + "\n");
}

void send_error(int socket, const std::string& text) {
    send_all(socket, "ERROR " + text + "\n");
}

std::string assign_role_locked() {
    bool has_O = false;
    bool has_X = false;
    for (const auto& client : g_clients) {
        if (!client->active) {
            continue;
        }
        if (client->role == "O") {
            has_O = true;
        } else if (client->role == "X") {
            has_X = true;
        }
    }
    if (!has_X) {
        return "X";
    }
    if (!has_O) {
        return "O";
    }
    return "spectator";
}

bool role_in_use_locked(const std::string& role, const std::shared_ptr<ClientSession>& requester) {
    if (role != "X" && role != "O") {
        return false;
    }
    for (const auto& client : g_clients) {
        if (!client->active || client == requester) {
            continue;
        }
        if (client->role == role) {
            return true;
        }
    }
    return false;
}

void remove_client(const std::shared_ptr<ClientSession>& session) {
    std::lock_guard<std::mutex> lock(g_clients_mutex);
    for (auto it = g_clients.begin(); it != g_clients.end(); ++it) {
        if (*it == session) {
            g_clients.erase(it);
            break;
        }
    }
}

void handle_move(const std::shared_ptr<ClientSession>& session, const std::string& payload) {
    if (session->role != "O" && session->role != "X") {
        send_error(session->socket, "Spectators cannot submit moves");
        return;
    }
    const contrast::Player player = role_to_player(session->role);
    if (player == contrast::Player::None) {
        send_error(session->socket, "Unknown player role");
        return;
    }
    protocol::Move move;
    try {
        move = protocol::parse_move(payload);
    } catch (const std::exception& ex) {
        send_error(session->socket, ex.what());
        return;
    }
    protocol::StateSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(g_game_mutex);
        if (player != g_state.current_player()) {
            send_error(session->socket, std::string("It is ") + player_to_symbol(g_state.current_player()) + "'s turn");
            return;
        }
        contrast::Move desired = convert_move(move);
        contrast::MoveList legal;
        contrast::Rules::legal_moves(g_state, legal);
        auto it = std::find_if(legal.begin(), legal.end(),
                               [&](const contrast::Move& candidate) { return moves_equal(candidate, desired); });
        if (it == legal.end()) {
            send_error(session->socket, "Illegal move according to core rules");
            return;
        }
        g_state.apply_move(*it);
        g_last_move = protocol::format_move(move);
        update_status(player);
        snapshot = build_snapshot();
    }
    std::cout << "\n" << protocol::render_board(snapshot.pieces, snapshot.tiles) << "\n";
    broadcast_state(snapshot);
}

void handle_role(const std::shared_ptr<ClientSession>& session, const std::string& payload) {
    std::istringstream iss(payload);
    std::string role_token;
    std::string name_token;
    std::string model_token;
    if (!(iss >> role_token)) {
        send_error(session->socket, "ROLE requires a target role");
        return;
    }
    if (!(iss >> name_token)) {
        name_token = "-";
    }
    if (iss >> model_token) {
        // モデル名は現在サーバ側で利用しないが、クライアントとの整合性のため読み捨てておく
        (void)model_token;
    }
    std::string normalized = role_token;
    for (char& ch : normalized) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    std::string requested_role;
    if (normalized == "-") {
        requested_role.clear();
    } else if (normalized == "X" || normalized == "O") {
        requested_role = normalized;
    } else if (normalized == "SPECTATOR" || normalized == "SPEC") {
        requested_role = "spectator";
    } else {
        send_error(session->socket, "Unknown role: " + role_token);
        return;
    }
    if (requested_role.empty()) {
        requested_role = session->role;
    }
    {
        std::lock_guard<std::mutex> lock(g_clients_mutex);
        if (role_in_use_locked(requested_role, session)) {
            send_error(session->socket, requested_role + " already taken");
            return;
        }
        session->role = requested_role;
        if (name_token != "-") {
            session->name = name_token;
        }
    }
    send_info(session->socket, "You are " + session->role + " (" + session->name + ")");
}

void client_thread(std::shared_ptr<ClientSession> session) {
    std::string buffer;
    try {
        send_info(session->socket, "You are " + session->role + " (" + session->name + ")");
        {
            std::lock_guard<std::mutex> lock(g_game_mutex);
            send_state_to(session->socket, build_snapshot());
        }
        while (true) {
            auto line = recv_line(session->socket, buffer);
            if (!line) {
                break;
            }
            if (line->empty()) {
                continue;
            }
            if (line->rfind("MOVE ", 0) == 0) {
                handle_move(session, line->substr(5));
            } else if (line->rfind("ROLE ", 0) == 0) {
                handle_role(session, line->substr(5));
            } else if (*line == "GET_STATE") {
                std::lock_guard<std::mutex> lock(g_game_mutex);
                send_state_to(session->socket, build_snapshot());
            } else {
                send_error(session->socket, "Unknown command: " + *line);
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Client thread error: " << ex.what() << '\n';
    }
    session->active = false;
    ::close(session->socket);
    remove_client(session);
    std::cout << "Client disconnected (" << session->role << ", " << session->name << ")" << std::endl;
}

int create_server_socket() {
    const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("socket() failed");
    }
    int opt = 1;
    ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(kServerPort);
    if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(sock);
        throw std::runtime_error("bind() failed");
    }
    if (::listen(sock, kBacklog) < 0) {
        ::close(sock);
        throw std::runtime_error("listen() failed");
    }
    return sock;
}

void accept_loop(int server_sock) {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        const int client_sock = ::accept(server_sock, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client_sock < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "accept() failed: " << std::strerror(errno) << '\n';
            continue;
        }
        auto session = std::make_shared<ClientSession>();
        session->socket = client_sock;
        {
            std::lock_guard<std::mutex> lock(g_clients_mutex);
            session->role = assign_role_locked();
            g_clients.push_back(session);
        }
        std::thread(client_thread, session).detach();
    }
}

}  // namespace

int main() {
    std::signal(SIGPIPE, SIG_IGN);
    try {
        const int server_sock = create_server_socket();
        std::cout << "Server listening on port " << kServerPort << std::endl;
        accept_loop(server_sock);
        ::close(server_sock);
    } catch (const std::exception& ex) {
        std::cerr << "Fatal server error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
