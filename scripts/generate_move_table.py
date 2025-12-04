#!/usr/bin/env python3
from pathlib import Path

BOARD_W = 5
BOARD_H = 5
TILE_TYPES = ["None", "Black", "Gray"]
MAX_RAY = max(BOARD_W, BOARD_H) - 1
MAX_DIRS = 8

dirs_map = {
    0: [(1, 0), (-1, 0), (0, 1), (0, -1)],
    1: [(1, 1), (1, -1), (-1, 1), (-1, -1)],
    2: [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)],
}

board_size = BOARD_W * BOARD_H

def compute_table():
    table = []
    for tile_idx in range(len(TILE_TYPES)):
        tile_entries = []
        for idx in range(board_size):
            x = idx % BOARD_W
            y = idx // BOARD_W
            dirs = dirs_map[tile_idx]
            dir_entries = []
            for dx, dy in dirs:
                rel_indices = []
                cx, cy = x, y
                for _ in range(MAX_RAY):
                    cx += dx
                    cy += dy
                    if not (0 <= cx < BOARD_W and 0 <= cy < BOARD_H):
                        break
                    rel_idx = (cy * BOARD_W + cx) - (y * BOARD_W + x)
                    rel_indices.append(rel_idx)
                dir_entries.append(rel_indices)
            tile_entries.append(dir_entries)
        table.append(tile_entries)
    return table

def format_array(rel_values):
    padded = rel_values + [0] * (MAX_RAY - len(rel_values))
    return ", ".join(f"{v:+d}" for v in padded)

def emit_header(table: list, path: Path):
    header = []
    header.append("#pragma once")
    header.append("#include \"contrast/types.hpp\"")
    header.append("#include <cstdint>\n")
    header.append("namespace contrast {\n")
    header.append("constexpr int kMaxRayLength = (BOARD_W > BOARD_H ? BOARD_W : BOARD_H) - 1;")
    header.append("constexpr int kMaxDirections = 8;")
    header.append("constexpr int kTileTypeCount = 3;")
    header.append("constexpr int kBoardSize = BOARD_W * BOARD_H;\n")
    header.append("struct PrecomputedDirection {\n    uint8_t step_count;\n    int8_t rel_index[kMaxRayLength];\n};\n")
    header.append("struct MoveTableEntry {\n    uint8_t dir_count;\n    PrecomputedDirection dirs[kMaxDirections];\n};\n")
    header.append("inline constexpr MoveTableEntry kMoveTable[kTileTypeCount][kBoardSize] = {")

    for tile_idx, tile_entries in enumerate(table):
        header.append("// TileType::{}".format(TILE_TYPES[tile_idx]))
        header.append("{")
        for entry in tile_entries:
            header.append("    {")
            header.append(f"        static_cast<uint8_t>({len(entry)}),")
            header.append("        {")
            for dir_idx in range(MAX_DIRS):
                if dir_idx < len(entry):
                    rel_indices = entry[dir_idx]
                    step_count = len(rel_indices)
                    header.append("            {" )
                    header.append(f"                static_cast<uint8_t>({step_count}),")
                    header.append("                { " + format_array(rel_indices) + " }")
                    header.append("            },")
                else:
                    header.append("            { 0, { " + format_array([]) + " } },")
            header.append("        }")
            header.append("    },")
        header.append("},")
    header.append("};\n}")
    path.write_text("\n".join(header))

if __name__ == "__main__":
    output_path = Path(__file__).resolve().parents[1] / "core" / "include" / "contrast" / "precomputed_move_table.hpp"
    table = compute_table()
    emit_header(table, output_path)
    print(f"Wrote move table to {output_path}")
