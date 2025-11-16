#include "renderer.hpp"
#include <imgui.h>
#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/random_policy.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include "contrast_ai/ntuple.hpp"
#include <set>
#include <string>
#include <memory>
#include <cmath>
#include <vector>
#include <iostream>

static contrast::GameState* g_state = nullptr;

void set_game_state(contrast::GameState* s) { g_state = s; }

// internal UI state
static int sel_x = -1, sel_y = -1;
static std::vector<contrast::Move> sel_moves; // legal moves from selected piece
static contrast::Move pending_move; // when destination chosen, pending until tile placement resolved
static bool has_pending_move = false;
static bool tile_placement_mode = false;

// AI vs AI mode
static bool ai_mode = false;
static bool ai_running = false;
static float ai_delay = 0.5f; // seconds between moves
static float ai_timer = 0.0f;
static int ai_black_type = 0; // 0=Random, 1=Greedy, 2=NTuple
static int ai_white_type = 0;
static std::unique_ptr<contrast_ai::RandomPolicy> random_black;
static std::unique_ptr<contrast_ai::GreedyPolicy> greedy_black;
static std::unique_ptr<contrast_ai::NTuplePolicy> ntuple_black;
static std::unique_ptr<contrast_ai::RandomPolicy> random_white;
static std::unique_ptr<contrast_ai::GreedyPolicy> greedy_white;
static std::unique_ptr<contrast_ai::NTuplePolicy> ntuple_white;
static char weights_path[256] = "build/weights_10k.bin";
static bool ntuple_loaded = false;

static void clear_selection() {
	sel_x = sel_y = -1;
	sel_moves.clear();
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

// Initialize NTuple weights on first use
static void ensure_ntuple_loaded() {
	if (!ntuple_loaded) {
		ntuple_black = std::make_unique<contrast_ai::NTuplePolicy>();
		ntuple_white = std::make_unique<contrast_ai::NTuplePolicy>();
		if (ntuple_black->load(weights_path) && ntuple_white->load(weights_path)) {
			ntuple_loaded = true;
			std::cout << "Auto-loaded N-tuple weights from: " << weights_path << "\n";
		} else {
			std::cout << "N-tuple weights not found at: " << weights_path << "\n";
			std::cout << "You can load them manually using the GUI.\n";
			ntuple_black.reset();
			ntuple_white.reset();
		}
	}
}

void render_frame() {
	if (!g_state) return;

	// Make window fullscreen-like
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | 
	                                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
	                                ImGuiWindowFlags_NoTitleBar;

	ImGui::Begin("Contrast Board", nullptr, window_flags);

	const auto& b = g_state->board();
	int bw = b.width();
	int bh = b.height();

	// Use manual layout: board on left, controls on right
	float window_width = ImGui::GetWindowWidth();
	float window_height = ImGui::GetWindowHeight();
	
	// Fixed width for right panel
	float side_panel_w = 280.0f;
	float board_width = window_width - side_panel_w - 30.0f; // leave padding
	
	// Compute cell size based on available board area
	float max_cell = std::min(board_width / bw, (window_height - 50) / bh);
	if (max_cell < 40.0f) max_cell = 40.0f;
	if (max_cell > 120.0f) max_cell = 120.0f;
	ImVec2 cell = ImVec2(max_cell, max_cell);
	
	// Left side: board
	ImGui::BeginChild("board_child", ImVec2(board_width, 0), true);

	// gather set of legal dest cells for highlighting
	std::set<std::pair<int,int>> dests;
	for (const auto &m : sel_moves) dests.emplace(m.dx, m.dy);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 board_start = ImGui::GetCursorScreenPos();

	for (int y = 0; y < bh; ++y) {
		for (int x = 0; x < bw; ++x) {
			const auto& c = b.at(x,y);

			ImGui::PushID(x + y*bw);

			// Calculate cell position
			ImVec2 cell_min = ImVec2(board_start.x + x * cell.x, board_start.y + y * cell.y);
			ImVec2 cell_max = ImVec2(cell_min.x + cell.x, cell_min.y + cell.y);
			ImVec2 cell_center = ImVec2((cell_min.x + cell_max.x) * 0.5f, (cell_min.y + cell_max.y) * 0.5f);

			// Determine tile background color
			ImU32 base_col = IM_COL32(255,255,255,255);
			if (c.tile == contrast::TileType::Black) {
				base_col = IM_COL32(50,50,50,255);
			} else if (c.tile == contrast::TileType::Gray) {
				base_col = IM_COL32(160,160,160,255);
			}

			// Highlight if selected or destination
			if (x == sel_x && y == sel_y) {
				base_col = IM_COL32(180,200,255,255);
			} else if (dests.count({x,y})) {
				base_col = IM_COL32(200,255,200,255);
			}

			// Draw cell background
			draw_list->AddRectFilled(cell_min, cell_max, base_col);
			draw_list->AddRect(cell_min, cell_max, IM_COL32(100,100,100,255), 0.0f, 0, 1.0f);

			// Draw piece if present
			if (c.occupant != contrast::Player::None) {
				float piece_radius = cell.x * 0.3f;
				ImU32 piece_color = (c.occupant == contrast::Player::Black) 
					? IM_COL32(229, 62, 62, 255)   // Red
					: IM_COL32(49, 130, 206, 255);  // Blue
				
				bool point_down = (c.occupant == contrast::Player::Black);
				draw_pentagon(draw_list, cell_center, piece_radius, piece_color, point_down);
				
				// Draw directional arrows
				draw_arrows(draw_list, cell_center, cell.x, c.tile);
			}

			// Draw tile indicator if no piece
			if (c.occupant == contrast::Player::None && c.tile != contrast::TileType::None) {
				const char* tile_text = (c.tile == contrast::TileType::Black) ? "B" : "G";
				ImVec2 text_size = ImGui::CalcTextSize(tile_text);
				ImVec2 text_pos = ImVec2(cell_center.x - text_size.x * 0.5f, cell_center.y - text_size.y * 0.5f);
				ImU32 text_color = (c.tile == contrast::TileType::Black) ? IM_COL32(255,255,255,255) : IM_COL32(0,0,0,255);
				draw_list->AddText(text_pos, text_color, tile_text);
			}

			// Invisible button for click detection
			ImGui::SetCursorScreenPos(cell_min);
			if (ImGui::InvisibleButton("##cell", cell)) {
				// click handling
				if (tile_placement_mode) {
					// in tile placement mode: clicking an empty cell selects tile location
					if (c.occupant == contrast::Player::None && c.tile == contrast::TileType::None) {
						// show immediate small modal by setting pending_move.tx/ty later via popup state
						pending_move.tx = x; pending_move.ty = y;
						// open a small ImGui dialog to pick Black/Gray (depending on inventory)
						ImGui::OpenPopup("Place Tile");
					}
				} else if (sel_x == x && sel_y == y) {
					// deselect
					clear_selection();
				} else if (c.occupant == g_state->to_move_) {
					// select piece
					sel_x = x; sel_y = y;
					sel_moves.clear();
					contrast::MoveList all;
					contrast::Rules::legal_moves(*g_state, all);
					for (size_t i = 0; i < all.size; ++i) {
						if (all[i].sx == x && all[i].sy == y) sel_moves.push_back(all[i]);
					}
				} else {
					// clicked a destination or other cell
					if (!sel_moves.empty()) {
						// check if clicked an allowed dest
						bool is_dest = false;
						for (const auto &m : sel_moves) if (m.dx == x && m.dy == y) { is_dest = true; break; }
						if (is_dest) {
							// prepare pending move
							pending_move = contrast::Move();
							pending_move.sx = sel_x; pending_move.sy = sel_y;
							pending_move.dx = x; pending_move.dy = y;
							pending_move.place_tile = false;
							has_pending_move = true;
							// open popup to ask tile placement
							ImGui::OpenPopup("AfterMove");
						}
					}
				}
			}

			ImGui::SameLine();
			ImGui::PopID();
		}
		ImGui::NewLine();
	}

	// Popup: decide to place tile
	if (ImGui::BeginPopupModal("AfterMove", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Apply move to (%d,%d)?", pending_move.dx, pending_move.dy);
		ImGui::Separator();
		if (ImGui::Button("Apply without tile")) {
			g_state->apply_move(pending_move);
			has_pending_move = false; clear_selection();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Place tile")) {
			// enter tile placement mode: next click selects tile location
			tile_placement_mode = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			has_pending_move = false; clear_selection(); ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Popup: choose tile type after selecting tx/ty
	if (ImGui::BeginPopupModal("Place Tile", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Place tile at (%d,%d)", pending_move.tx, pending_move.ty);
		ImGui::Separator();
		auto &inv = g_state->inventory(g_state->to_move_);
		if (ImGui::Button("Black")) {
			if (inv.black > 0) {
				pending_move.place_tile = true; pending_move.tile = contrast::TileType::Black;
				g_state->apply_move(pending_move);
				tile_placement_mode = false; has_pending_move = false; clear_selection();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Gray")) {
			if (inv.gray > 0) {
				pending_move.place_tile = true; pending_move.tile = contrast::TileType::Gray;
				g_state->apply_move(pending_move);
				tile_placement_mode = false; has_pending_move = false; clear_selection();
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) { tile_placement_mode = false; has_pending_move = false; clear_selection(); ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	ImGui::EndChild();

	// Right side: controls
	ImGui::SameLine();
	ImGui::BeginChild("side_child", ImVec2(0, 0), true);
	ImGui::Text("Turn: %s", (g_state->to_move_==contrast::Player::Black)?"Black":"White");
	ImGui::Separator();

	// Show Next move (first legal move) and list of a few legal moves
	contrast::MoveList all_moves;
	contrast::Rules::legal_moves(*g_state, all_moves);
	if (!all_moves.empty()) {
		ImGui::Text("Next move (auto):");
		const auto &nm = all_moves[0];
		ImGui::Text("from (%d,%d) -> (%d,%d)", nm.sx, nm.sy, nm.dx, nm.dy);
		if (ImGui::Button("Apply Next Move")) {
			g_state->apply_move(nm);
			clear_selection();
		}
		ImGui::Separator();
		ImGui::Text("Legal moves (sample):");
		ImGui::BeginChild("moves_list", ImVec2(0,150), true);
		size_t shown = 0;
		for (size_t i = 0; i < all_moves.size && shown < 50; ++i, ++shown) {
			const auto& m = all_moves[i];
			char buf[64]; snprintf(buf, sizeof(buf), "(%d,%d)->(%d,%d)%s", m.sx,m.sy,m.dx,m.dy, m.place_tile?" +tile":"");
			ImGui::TextUnformatted(buf);
		}
		ImGui::EndChild();
	} else {
		ImGui::Text("No legal moves");
	}

	ImGui::Separator();
	ImGui::Text("Inventory (Black/Gray):");
	auto inv = g_state->inventory(g_state->to_move_);
	ImGui::Text("%d / %d", inv.black, inv.gray);

	ImGui::Separator();
	ImGui::Text("AI vs AI Mode");
	
	// N-tuple weights loading
	ImGui::Text("N-tuple Weights:");
	ImGui::InputText("##weights", weights_path, sizeof(weights_path));
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		ntuple_black = std::make_unique<contrast_ai::NTuplePolicy>();
		ntuple_white = std::make_unique<contrast_ai::NTuplePolicy>();
		if (ntuple_black->load(weights_path) && ntuple_white->load(weights_path)) {
			ntuple_loaded = true;
			std::cout << "Loaded N-tuple weights from: " << weights_path << "\n";
		} else {
			std::cerr << "Failed to load N-tuple weights from: " << weights_path << "\n";
			ntuple_loaded = false;
			ntuple_black.reset();
			ntuple_white.reset();
		}
	}
	if (ntuple_loaded) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0,1,0,1), "Loaded");
	}
	
	ImGui::Separator();
	
	// AI type selection
	const char* ai_types[] = { "Random", "Greedy", "NTuple" };
	ImGui::Text("Black:");
	ImGui::SameLine();
	ImGui::PushItemWidth(100);
	ImGui::Combo("##BlackAI", &ai_black_type, ai_types, 3);
	ImGui::PopItemWidth();
	
	ImGui::Text("White:");
	ImGui::SameLine();
	ImGui::PushItemWidth(100);
	ImGui::Combo("##WhiteAI", &ai_white_type, ai_types, 3);
	ImGui::PopItemWidth();
	
	ImGui::SliderFloat("Delay (s)", &ai_delay, 0.1f, 2.0f);
	
	if (!ai_running) {
		if (ImGui::Button("Start AI vs AI")) {
			// Auto-load NTuple weights if needed
			if ((ai_black_type == 2 || ai_white_type == 2) && !ntuple_loaded) {
				ensure_ntuple_loaded();
			}
			
			// Check if NTuple is selected but still not loaded
			if ((ai_black_type == 2 || ai_white_type == 2) && !ntuple_loaded) {
				std::cerr << "Error: NTuple selected but weights not loaded!\n";
				std::cerr << "Please load weights using the Load button above.\n";
			} else {
				ai_running = true;
				ai_timer = 0.0f;
				// Initialize AI policies
				if (ai_black_type == 0) random_black = std::make_unique<contrast_ai::RandomPolicy>();
				else if (ai_black_type == 1) greedy_black = std::make_unique<contrast_ai::GreedyPolicy>();
				// ntuple_black already loaded
				if (ai_white_type == 0) random_white = std::make_unique<contrast_ai::RandomPolicy>();
				else if (ai_white_type == 1) greedy_white = std::make_unique<contrast_ai::GreedyPolicy>();
				// ntuple_white already loaded
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset Game")) {
			*g_state = contrast::GameState();
			clear_selection();
		}
	} else {
		if (ImGui::Button("Stop AI vs AI")) {
			ai_running = false;
		}
		
		// Update AI timer and make moves
		ai_timer += ImGui::GetIO().DeltaTime;
		if (ai_timer >= ai_delay) {
			ai_timer = 0.0f;
			
			// Check if game is over
			bool game_over = false;
			if (contrast::Rules::is_loss(*g_state, g_state->current_player())) {
				ImGui::Text("Game Over: %s wins!", 
					g_state->current_player() == contrast::Player::Black ? "White" : "Black");
				game_over = true;
				ai_running = false;
			} else if (contrast::Rules::is_win(*g_state, g_state->current_player())) {
				ImGui::Text("Game Over: %s wins!", 
					g_state->current_player() == contrast::Player::Black ? "Black" : "White");
				game_over = true;
				ai_running = false;
			}
			
			if (!game_over && ai_running) {
				// Get move from appropriate AI
				contrast::Move move;
				if (g_state->current_player() == contrast::Player::Black) {
					if (ai_black_type == 0 && random_black) {
						move = random_black->pick(*g_state);
					} else if (ai_black_type == 1 && greedy_black) {
						move = greedy_black->pick(*g_state);
					} else if (ai_black_type == 2 && ntuple_black) {
						move = ntuple_black->pick(*g_state);
					}
				} else {
					if (ai_white_type == 0 && random_white) {
						move = random_white->pick(*g_state);
					} else if (ai_white_type == 1 && greedy_white) {
						move = greedy_white->pick(*g_state);
					} else if (ai_white_type == 2 && ntuple_white) {
						move = ntuple_white->pick(*g_state);
					}
				}
				
				// Apply move
				g_state->apply_move(move);
				clear_selection();
			}
		}
		
		ImGui::Text("AI is running...");
	}

	ImGui::EndChild();
	ImGui::End();
}

