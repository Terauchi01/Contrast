#include <imgui.h>
#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/ntuple.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include "contrast_ai/rule_based_policy2.hpp"
#include <set>
#include <string>
#include <memory>
#include <cmath>
#include <vector>
#include <iostream>

static contrast::GameState* g_state = nullptr;
std::string g_weights_path = "";

void set_game_state(contrast::GameState* s) { g_state = s; }

// Internal UI state
static int sel_x = -1, sel_y = -1;  // Selected piece position
static int dest_x = -1, dest_y = -1;  // Selected destination position
static std::vector<contrast::Move> sel_moves; // legal moves from selected piece
static contrast::Move pending_move;
static bool has_pending_move = false;

// New flow states
enum class GamePhase {
	SELECT_PIECE,      // Step 1: Select piece to move
	SELECT_DESTINATION, // Step 2: Select where to move
	SELECT_TILE_TYPE,   // Step 3: Choose which tile to place (if applicable)
	SELECT_TILE_LOCATION // Step 4: Choose where to place the tile (if applicable)
};
static GamePhase current_phase = GamePhase::SELECT_PIECE;
static contrast::TileType selected_tile = contrast::TileType::None;
static std::vector<std::pair<int, int>> available_tile_locations;  // Empty cells where tile can be placed

// AI player
enum class AIType {
	GREEDY,
	RULEBASED,
	RULEBASED2,
	NTUPLE
};
static AIType current_ai_type = AIType::GREEDY;
static std::unique_ptr<contrast_ai::NTuplePolicy> ntuple_ai;
static std::unique_ptr<contrast_ai::GreedyPolicy> greedy_ai;
static std::unique_ptr<contrast_ai::RuleBasedPolicy> rulebased_ai;
static std::unique_ptr<contrast_ai::RuleBasedPolicy2> rulebased2_ai;
static bool ai_loaded = false;
static bool ai_thinking = false;
static contrast::Player human_player = contrast::Player::Black;  // Human plays as Black by default
static contrast::Player ai_player = contrast::Player::White;
static std::string game_status = "Game in progress";

static void clear_selection() {
	sel_x = sel_y = -1;
	dest_x = dest_y = -1;
	sel_moves.clear();
	current_phase = GamePhase::SELECT_PIECE;
	selected_tile = contrast::TileType::None;
	available_tile_locations.clear();
	has_pending_move = false;
}

// Draw a pentagon (piece shape) using ImGui DrawList
static void draw_pentagon(ImDrawList* draw_list, ImVec2 center, float radius, ImU32 color, bool point_down) {
	std::vector<ImVec2> points;
	float angle_offset = point_down ? (3.14159f / 2.0f) : (-3.14159f / 2.0f); // Point up or down
	
	for (int i = 0; i < 5; ++i) {
		float angle = angle_offset + (2.0f * 3.14159f * i) / 5.0f;
		points.push_back(ImVec2(
			center.x + radius * cosf(angle),
			center.y + radius * sinf(angle)
		));
	}
	
	draw_list->AddConvexPolyFilled(points.data(), 5, color);
	draw_list->AddPolyline(points.data(), 5, IM_COL32(0,0,0,100), ImDrawFlags_Closed, 2.0f);
}

// Draw directional arrows based on tile type
static void draw_arrows(ImDrawList* draw_list, ImVec2 center, float size, contrast::TileType tile) {
	bool show_ortho = (tile == contrast::TileType::None || tile == contrast::TileType::Gray);
	bool show_diag = (tile == contrast::TileType::Black || tile == contrast::TileType::Gray);
	
	float arrow_offset = size * 0.45f;
	float arrow_size = 6.0f;
	
	ImU32 ortho_color = IM_COL32(0, 0, 0, 200);
	ImU32 diag_color = IM_COL32(255, 255, 255, 200);
	
	// Orthogonal arrows
	if (show_ortho) {
		// Up
		draw_list->AddTriangleFilled(
			ImVec2(center.x, center.y - arrow_offset),
			ImVec2(center.x - arrow_size, center.y - arrow_offset + arrow_size),
			ImVec2(center.x + arrow_size, center.y - arrow_offset + arrow_size),
			ortho_color
		);
		// Down
		draw_list->AddTriangleFilled(
			ImVec2(center.x, center.y + arrow_offset),
			ImVec2(center.x - arrow_size, center.y + arrow_offset - arrow_size),
			ImVec2(center.x + arrow_size, center.y + arrow_offset - arrow_size),
			ortho_color
		);
		// Left
		draw_list->AddTriangleFilled(
			ImVec2(center.x - arrow_offset, center.y),
			ImVec2(center.x - arrow_offset + arrow_size, center.y - arrow_size),
			ImVec2(center.x - arrow_offset + arrow_size, center.y + arrow_size),
			ortho_color
		);
		// Right
		draw_list->AddTriangleFilled(
			ImVec2(center.x + arrow_offset, center.y),
			ImVec2(center.x + arrow_offset - arrow_size, center.y - arrow_size),
			ImVec2(center.x + arrow_offset - arrow_size, center.y + arrow_size),
			ortho_color
		);
	}
	
	// Diagonal arrows
	if (show_diag) {
		float diag_off = arrow_offset * 0.707f; // sqrt(2)/2
		float half_arrow = arrow_size * 0.707f;
		
		// Top-left
		draw_list->AddTriangleFilled(
			ImVec2(center.x - diag_off, center.y - diag_off),
			ImVec2(center.x - diag_off + half_arrow, center.y - diag_off),
			ImVec2(center.x - diag_off, center.y - diag_off + half_arrow),
			diag_color
		);
		// Top-right
		draw_list->AddTriangleFilled(
			ImVec2(center.x + diag_off, center.y - diag_off),
			ImVec2(center.x + diag_off - half_arrow, center.y - diag_off),
			ImVec2(center.x + diag_off, center.y - diag_off + half_arrow),
			diag_color
		);
		// Bottom-left
		draw_list->AddTriangleFilled(
			ImVec2(center.x - diag_off, center.y + diag_off),
			ImVec2(center.x - diag_off + half_arrow, center.y + diag_off),
			ImVec2(center.x - diag_off, center.y + diag_off - half_arrow),
			diag_color
		);
		// Bottom-right
		draw_list->AddTriangleFilled(
			ImVec2(center.x + diag_off, center.y + diag_off),
			ImVec2(center.x + diag_off - half_arrow, center.y + diag_off),
			ImVec2(center.x + diag_off, center.y + diag_off - half_arrow),
			diag_color
		);
	}
}

static ImVec2 compute_cell_size(float avail_w, int board_w, int side_w) {
	float usable = avail_w - side_w - 30.0f; // padding
	if (usable < 64.0f) usable = avail_w - 20.0f;
	float cs = usable / board_w;
	if (cs < 40.0f) cs = 40.0f;
	if (cs > 140.0f) cs = 140.0f;
	return ImVec2(cs, cs);
}

// Initialize AI on first use
static void ensure_ai_loaded() {
	if (ai_loaded) return;
	
	if (current_ai_type == AIType::GREEDY) {
		greedy_ai = std::make_unique<contrast_ai::GreedyPolicy>();
		ai_loaded = true;
		std::cout << "Loaded Greedy AI\n";
	} else if (current_ai_type == AIType::RULEBASED) {
		rulebased_ai = std::make_unique<contrast_ai::RuleBasedPolicy>();
		ai_loaded = true;
		std::cout << "Loaded RuleBased AI\n";
	} else if (current_ai_type == AIType::RULEBASED2) {
		rulebased2_ai = std::make_unique<contrast_ai::RuleBasedPolicy2>();
		ai_loaded = true;
		std::cout << "Loaded RuleBasedPolicy2 AI\n";
	} else if (current_ai_type == AIType::NTUPLE && !g_weights_path.empty()) {
		ntuple_ai = std::make_unique<contrast_ai::NTuplePolicy>();
		if (ntuple_ai->load(g_weights_path)) {
			ai_loaded = true;
			std::cout << "Loaded N-tuple AI weights from: " << g_weights_path << "\n";
		} else {
			std::cerr << "Failed to load N-tuple AI weights from: " << g_weights_path << "\n";
			ntuple_ai.reset();
		}
	}
}

// AI makes a move
static void ai_make_move() {
	if (!ai_loaded) return;
	if (g_state->current_player() != ai_player) return;
	
	ai_thinking = true;
	
	contrast::MoveList moves;
	contrast::Rules::legal_moves(*g_state, moves);
	
	if (moves.empty()) {
		game_status = (g_state->current_player() == contrast::Player::Black) 
			? "White wins! (Black has no moves)" 
			: "Black wins! (White has no moves)";
		ai_thinking = false;
		return;
	}
	
	// Check win conditions
	if (contrast::Rules::is_win(*g_state, contrast::Player::Black)) {
		game_status = "Black wins!";
		ai_thinking = false;
		return;
	}
	if (contrast::Rules::is_win(*g_state, contrast::Player::White)) {
		game_status = "White wins!";
		ai_thinking = false;
		return;
	}
	
	// Pick best move using AI
	contrast::Move best_move;
	if (current_ai_type == AIType::GREEDY && greedy_ai) {
		best_move = greedy_ai->pick(*g_state);
	} else if (current_ai_type == AIType::RULEBASED && rulebased_ai) {
		best_move = rulebased_ai->pick(*g_state);
	} else if (current_ai_type == AIType::RULEBASED2 && rulebased2_ai) {
		best_move = rulebased2_ai->pick(*g_state);
	} else if (current_ai_type == AIType::NTUPLE && ntuple_ai) {
		best_move = ntuple_ai->pick(*g_state);
	} else {
		ai_thinking = false;
		return;
	}
	
	// Apply the move
	g_state->apply_move(best_move);
	
	std::cout << "AI moved: (" << best_move.sx << "," << best_move.sy << ") -> (" 
	          << best_move.dx << "," << best_move.dy << ") placing ";
	if (best_move.tile == contrast::TileType::Black) std::cout << "Black tile\n";
	else if (best_move.tile == contrast::TileType::Gray) std::cout << "Gray tile\n";
	else std::cout << "no tile\n";
	
	ai_thinking = false;
}

void render_frame() {
	if (!g_state) return;
	
	ensure_ai_loaded();
	
	// Check if it's AI's turn and make move automatically
	if (g_state->current_player() == ai_player && !ai_thinking) {
		// Check game over first
		contrast::MoveList moves;
		contrast::Rules::legal_moves(*g_state, moves);
		
		if (!moves.empty() && !contrast::Rules::is_win(*g_state, contrast::Player::Black) 
		    && !contrast::Rules::is_win(*g_state, contrast::Player::White)) {
			ai_make_move();
		}
	}
	
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	
	ImGuiWindowFlags wf = ImGuiWindowFlags_NoDecoration 
	                    | ImGuiWindowFlags_NoMove 
	                    | ImGuiWindowFlags_NoResize 
	                    | ImGuiWindowFlags_NoSavedSettings;
	
	ImGui::Begin("GameWindow", nullptr, wf);
	
	// Game info sidebar
	float sidebar_w = 300.0f;
	ImGui::BeginChild("Sidebar", ImVec2(sidebar_w, 0), true);
	
	ImGui::Text("Human vs AI");
	ImGui::Separator();
	
	ImGui::Text("You are: %s", human_player == contrast::Player::Black ? "Black" : "White");
	ImGui::Text("AI is: %s", ai_player == contrast::Player::Black ? "Black" : "White");
	ImGui::Separator();
	
	ImGui::Text("Current turn: %s", 
		g_state->current_player() == contrast::Player::Black ? "Black" : "White");
	
	if (g_state->current_player() == ai_player) {
		if (ai_thinking) {
			ImGui::Text("AI is thinking...");
		} else {
			ImGui::Text("AI's turn");
		}
	} else {
		ImGui::Text("Your turn!");
	}
	
	ImGui::Separator();
	ImGui::TextWrapped("Status: %s", game_status.c_str());
	ImGui::Separator();
	
	// Available tiles
	ImGui::Text("Available tiles:");
	auto& inv = g_state->inventory(g_state->current_player());
	ImGui::Text("  Black: %d", inv.black);
	ImGui::Text("  Gray: %d", inv.gray);
	
	ImGui::Separator();
	
	// Phase-based UI (when human's turn)
	if (g_state->current_player() == human_player) {
		switch (current_phase) {
			case GamePhase::SELECT_PIECE:
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Step 1: Select your piece");
				ImGui::TextWrapped("Click on one of your pieces to move");
				break;
				
			case GamePhase::SELECT_DESTINATION:
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Piece at (%d, %d)", sel_x, sel_y);
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Step 2: Select destination");
				ImGui::TextWrapped("Click on a highlighted cell to move there");
				if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
					clear_selection();
				}
				break;
				
			case GamePhase::SELECT_TILE_TYPE:
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Move: (%d,%d) -> (%d,%d)", sel_x, sel_y, dest_x, dest_y);
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Step 3: Choose tile type");
				ImGui::TextWrapped("Select which tile to place (or skip)");
				
				// Black Tile button
				if (inv.black > 0) {
					if (ImGui::Button("Black Tile (orthogonal)##tile", ImVec2(-1, 0))) {
						selected_tile = contrast::TileType::Black;
						// Find empty cells to place tile
						available_tile_locations.clear();
						int bw = g_state->board().width();
						int bh = g_state->board().height();
						for (int y = 0; y < bh; ++y) {
							for (int x = 0; x < bw; ++x) {
								const auto& cell = g_state->board().at(x, y);
								bool is_origin = (x == sel_x && y == sel_y);
								bool is_destination = (x == dest_x && y == dest_y);
								if (cell.tile == contrast::TileType::None && !is_destination) {
									bool empty_after_move = (cell.occupant == contrast::Player::None) || is_origin;
									if (empty_after_move) {
										available_tile_locations.push_back({x, y});
									}
								}
							}
						}
						current_phase = GamePhase::SELECT_TILE_LOCATION;
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(%d)", inv.black);
				} else {
					ImGui::TextDisabled("Black Tile (none left)");
				}
				
				// Gray Tile button
				if (inv.gray > 0) {
					if (ImGui::Button("Gray Tile (all directions)##tile", ImVec2(-1, 0))) {
						selected_tile = contrast::TileType::Gray;
						// Find empty cells to place tile
						available_tile_locations.clear();
						int bw = g_state->board().width();
						int bh = g_state->board().height();
						for (int y = 0; y < bh; ++y) {
							for (int x = 0; x < bw; ++x) {
								const auto& cell = g_state->board().at(x, y);
								bool is_origin = (x == sel_x && y == sel_y);
								bool is_destination = (x == dest_x && y == dest_y);
								if (cell.tile == contrast::TileType::None && !is_destination) {
									bool empty_after_move = (cell.occupant == contrast::Player::None) || is_origin;
									if (empty_after_move) {
										available_tile_locations.push_back({x, y});
									}
								}
							}
						}
						current_phase = GamePhase::SELECT_TILE_LOCATION;
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(%d)", inv.gray);
				} else {
					ImGui::TextDisabled("Gray Tile (none left)");
				}
				
				// Skip tile placement
				if (ImGui::Button("Skip (no tile)##skip", ImVec2(-1, 0))) {
					// Execute move without tile
					pending_move.sx = sel_x;
					pending_move.sy = sel_y;
					pending_move.dx = dest_x;
					pending_move.dy = dest_y;
					pending_move.place_tile = false;
					pending_move.tile = contrast::TileType::None;
					g_state->apply_move(pending_move);
					std::cout << "Human moved: (" << sel_x << "," << sel_y << ") -> (" 
					          << dest_x << "," << dest_y << ") - no tile\n";
					clear_selection();
				}
				
				if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
					clear_selection();
				}
				break;
				
			case GamePhase::SELECT_TILE_LOCATION:
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "Move: (%d,%d) -> (%d,%d)", sel_x, sel_y, dest_x, dest_y);
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Step 4: Place tile");
				if (selected_tile == contrast::TileType::Black) {
					ImGui::TextWrapped("Click empty cell to place Black tile");
				} else {
					ImGui::TextWrapped("Click empty cell to place Gray tile");
				}
				ImGui::TextWrapped("(%zu empty cells available)", available_tile_locations.size());
				
				if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
					current_phase = GamePhase::SELECT_TILE_TYPE;
					selected_tile = contrast::TileType::None;
					available_tile_locations.clear();
				}
				break;
		}
		
		ImGui::Separator();
	}
	
	// AI Type Selection
	ImGui::Text("AI Type:");
	bool ai_changed = false;
	if (ImGui::RadioButton("Greedy AI", current_ai_type == AIType::GREEDY)) {
		current_ai_type = AIType::GREEDY;
		ai_changed = true;
	}
	if (ImGui::RadioButton("RuleBased AI", current_ai_type == AIType::RULEBASED)) {
		current_ai_type = AIType::RULEBASED;
		ai_changed = true;
	}
	if (ImGui::RadioButton("RuleBased2 AI", current_ai_type == AIType::RULEBASED2)) {
		current_ai_type = AIType::RULEBASED2;
		ai_changed = true;
	}
	if (ImGui::RadioButton("N-tuple AI", current_ai_type == AIType::NTUPLE)) {
		current_ai_type = AIType::NTUPLE;
		ai_changed = true;
	}
	
	if (ai_changed) {
		ai_loaded = false;
		greedy_ai.reset();
		rulebased_ai.reset();
		rulebased2_ai.reset();
		ntuple_ai.reset();
		ensure_ai_loaded();
	}
	
	ImGui::Separator();
	
	// Game controls
	if (ImGui::Button("New Game")) {
		g_state->reset();
		clear_selection();
		has_pending_move = false;
		game_status = "Game in progress";
	}
	
	if (ImGui::Button("Switch Colors")) {
		std::swap(human_player, ai_player);
		g_state->reset();
		clear_selection();
		has_pending_move = false;
		game_status = "Game in progress";
	}
	
	ImGui::Separator();
	ImGui::TextWrapped("How to play:");
	ImGui::TextWrapped("1. Click your piece");
	ImGui::TextWrapped("2. Click destination");
	ImGui::TextWrapped("3. Choose tile type:");
	ImGui::TextWrapped("   - Black: orthogonal only");
	ImGui::TextWrapped("   - Gray: all directions");
	ImGui::TextWrapped("   - Skip: no tile");
	ImGui::TextWrapped("4. Click where to place tile");
	ImGui::Separator();
	ImGui::TextWrapped("Win condition:");
	ImGui::TextWrapped("Reduce opponent to 1 piece or block all moves");
	
	ImGui::EndChild();
	
	// Board rendering
	ImGui::SameLine();
	ImGui::BeginChild("Board", ImVec2(0,0), false);

	ImVec2 avail = ImGui::GetContentRegionAvail();
	int bw = g_state->board().width();
	int bh = g_state->board().height();
	ImVec2 cs = compute_cell_size(avail.x, bw, 0);

	ImVec2 board_start = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	std::set<std::pair<int, int>> destination_cells;
	if (current_phase == GamePhase::SELECT_DESTINATION) {
		for (const auto& move : sel_moves) {
			destination_cells.emplace(move.dx, move.dy);
		}
	}

	std::set<std::pair<int, int>> tile_target_cells;
	if (current_phase == GamePhase::SELECT_TILE_LOCATION) {
		for (const auto& loc : available_tile_locations) {
			tile_target_cells.emplace(loc.first, loc.second);
		}
	}

	for (int y = 0; y < bh; ++y) {
		for (int x = 0; x < bw; ++x) {
			ImVec2 tl(board_start.x + x * cs.x, board_start.y + y * cs.y);
			ImVec2 br(tl.x + cs.x, tl.y + cs.y);
			ImVec2 center((tl.x + br.x) * 0.5f, (tl.y + br.y) * 0.5f);

			contrast::Cell cell = g_state->board().at(x, y);
			ImU32 bg_color = IM_COL32(255, 255, 255, 255);
			if (cell.tile == contrast::TileType::Black) {
				bg_color = IM_COL32(50, 50, 50, 255);
			} else if (cell.tile == contrast::TileType::Gray) {
				bg_color = IM_COL32(160, 160, 160, 255);
			}

			const std::pair<int, int> cell_coord{x, y};
			bool is_selected = (x == sel_x && y == sel_y);
			bool is_confirmed_dest = (x == dest_x && y == dest_y);
			bool is_destination = destination_cells.count(cell_coord) > 0;
			bool is_tile_target = tile_target_cells.count(cell_coord) > 0;

			if (is_tile_target) {
				bg_color = IM_COL32(200, 255, 255, 255);
			} else if (is_selected) {
				bg_color = IM_COL32(180, 200, 255, 255);
			} else if (is_confirmed_dest) {
				bg_color = IM_COL32(255, 230, 180, 255);
			} else if (is_destination) {
				bg_color = IM_COL32(200, 255, 200, 255);
			}

			dl->AddRectFilled(tl, br, bg_color);
			dl->AddRect(tl, br, IM_COL32(100, 100, 100, 255), 0.0f, 0, 1.0f);

			if (is_tile_target) {
				dl->AddRect(tl, br, IM_COL32(0, 200, 255, 200), 0.0f, 0, 2.0f);
			}
			if (is_confirmed_dest) {
				dl->AddRect(tl, br, IM_COL32(255, 170, 0, 220), 0.0f, 0, 2.0f);
			}

			if (cell.occupant != contrast::Player::None) {
				bool is_black = (cell.occupant == contrast::Player::Black);
				ImU32 piece_color = is_black ? IM_COL32(229, 62, 62, 255) : IM_COL32(49, 130, 206, 255);
				draw_pentagon(dl, center, cs.x * 0.3f, piece_color, is_black);
				draw_arrows(dl, center, cs.x, cell.tile);
			} else if (cell.tile != contrast::TileType::None) {
				const char* tile_text = (cell.tile == contrast::TileType::Black) ? "B" : "G";
				ImVec2 text_size = ImGui::CalcTextSize(tile_text);
				ImVec2 text_pos(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f);
				ImU32 text_color = (cell.tile == contrast::TileType::Black) ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
				dl->AddText(text_pos, text_color, tile_text);
			}
		}
	}
	
	// Mouse interaction (only on human's turn)
	if (g_state->current_player() == human_player && !ai_thinking) {
		ImVec2 mouse_pos = ImGui::GetMousePos();
		if (ImGui::IsMouseClicked(0)) {
			int mx = (int)((mouse_pos.x - board_start.x) / cs.x);
			int my = (int)((mouse_pos.y - board_start.y) / cs.y);
			
			if (mx >= 0 && mx < bw && my >= 0 && my < bh) {
				switch (current_phase) {
					case GamePhase::SELECT_PIECE: {
						// Step 1: Select a piece
						contrast::Cell cell = g_state->board().at(mx, my);
						if (cell.occupant == g_state->current_player()) {
							sel_x = mx;
							sel_y = my;
							
							// Get legal moves from this position
							contrast::MoveList all_moves;
							contrast::Rules::legal_moves(*g_state, all_moves);
							sel_moves.clear();
							for (const auto& m : all_moves) {
								if (m.sx == mx && m.sy == my) {
									sel_moves.push_back(m);
								}
							}
							
							if (!sel_moves.empty()) {
								current_phase = GamePhase::SELECT_DESTINATION;
							} else {
								// No legal moves from this piece
								clear_selection();
							}
						}
						break;
					}
					
					case GamePhase::SELECT_DESTINATION: {
						// Step 2: Select destination
						bool found_move = false;
						for (const auto& m : sel_moves) {
							if (m.dx == mx && m.dy == my) {
								dest_x = mx;
								dest_y = my;
								current_phase = GamePhase::SELECT_TILE_TYPE;
								found_move = true;
								break;
							}
						}
						
						if (!found_move) {
							// Clicked on invalid destination, deselect
							clear_selection();
						}
						break;
					}
					
					case GamePhase::SELECT_TILE_TYPE:
						// This phase is handled by UI buttons in sidebar
						break;
					
					case GamePhase::SELECT_TILE_LOCATION: {
						// Step 4: Select where to place the tile
						bool valid_location = false;
						for (const auto& loc : available_tile_locations) {
							if (loc.first == mx && loc.second == my) {
								valid_location = true;
								break;
							}
						}
						
						if (valid_location) {
							// Create and execute the move
							pending_move.sx = sel_x;
							pending_move.sy = sel_y;
							pending_move.dx = dest_x;
							pending_move.dy = dest_y;
							pending_move.place_tile = true;
							pending_move.tx = mx;
							pending_move.ty = my;
							pending_move.tile = selected_tile;
							
							g_state->apply_move(pending_move);
							
							std::cout << "Human moved: (" << sel_x << "," << sel_y << ") -> (" 
							          << dest_x << "," << dest_y << ") placing ";
							if (selected_tile == contrast::TileType::Black) {
								std::cout << "Black tile at (" << mx << "," << my << ")\n";
							} else {
								std::cout << "Gray tile at (" << mx << "," << my << ")\n";
							}
							
							clear_selection();
						}
						break;
					}
				}
			}
		}
	}
	
	ImGui::EndChild();
	ImGui::End();
}
