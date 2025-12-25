#!/usr/bin/env python3
"""AlphaZero-like client that plays over the contrast_arena protocol."""
from __future__ import annotations

import argparse
import logging
import re
import socket
import sys
from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import torch

# Ensure kaneko modules are importable
CLIENT_DIR = Path(__file__).resolve().parent
KANEKO_DIR = CLIENT_DIR / "kaneko"
if str(KANEKO_DIR) not in sys.path:
    sys.path.append(str(KANEKO_DIR))

from contrast_game import (  # type: ignore  # pylint: disable=import-error
    OPPONENT,
    P1,
    P2,
    TILE_BLACK,
    TILE_GRAY,
    TILE_WHITE,
    ContrastGame,
    decode_action,
)
from logger import setup_logger  # type: ignore  # pylint: disable=import-error
from mcts import MCTS  # type: ignore  # pylint: disable=import-error
from model import ContrastDualPolicyNet  # type: ignore  # pylint: disable=import-error

BOARD_W = 5
BOARD_H = 5
LOGGER = logging.getLogger("alphazero_bot")


@dataclass
class StateSnapshot:
    turn: str = "X"
    status: str = "ongoing"
    last_move: str = ""
    pieces: Dict[str, str] = None
    tiles: Dict[str, str] = None
    stock_black: Dict[str, int] = None
    stock_gray: Dict[str, int] = None

    def __post_init__(self) -> None:
        self.pieces = self.pieces or {}
        self.tiles = self.tiles or {}
        self.stock_black = self.stock_black or {}
        self.stock_gray = self.stock_gray or {}


# === State parsing helpers (borrowed from python_random_bot) ===

def coord_to_xy(coord: str) -> Tuple[int, int]:
    x = ord(coord[0]) - ord("a")
    rank = ord(coord[1]) - ord("1")
    y = BOARD_H - 1 - rank
    return x, y


def xy_to_coord(x: int, y: int) -> str:
    file_char = chr(ord("a") + x)
    rank_char = chr(ord("1") + (BOARD_H - 1 - y))
    return f"{file_char}{rank_char}"


def parse_entries(text: str) -> Dict[str, str]:
    result: Dict[str, str] = {}
    if not text:
        return result
    for item in text.split(","):
        if not item or ":" not in item:
            continue
        coord, value = item.split(":", 1)
        result[coord.strip()] = value.strip()
    return result


def parse_counts(text: str) -> Dict[str, int]:
    result: Dict[str, int] = {}
    if not text:
        return result
    for item in text.split(","):
        if not item or ":" not in item:
            continue
        key, value = item.split(":", 1)
        try:
            result[key.strip().upper()] = int(value.strip())
        except ValueError:
            continue
    return result


def parse_state_block(lines: List[str]) -> StateSnapshot:
    data: Dict[str, str] = {}
    for line in lines:
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        data[key.strip()] = value.strip()
    snapshot = StateSnapshot()
    if "turn" in data and data["turn"]:
        snapshot.turn = data["turn"][0].upper()
    if "status" in data:
        snapshot.status = data["status"]
    if "last" in data:
        snapshot.last_move = data["last"]
    if "pieces" in data:
        snapshot.pieces = {k: v.upper() for k, v in parse_entries(data["pieces"]).items()}
    if "tiles" in data:
        snapshot.tiles = {k: v.lower() for k, v in parse_entries(data["tiles"]).items()}
    if "stock_b" in data:
        snapshot.stock_black = parse_counts(data["stock_b"])
    if "stock_g" in data:
        snapshot.stock_gray = parse_counts(data["stock_g"])
    return snapshot


# === Conversion helpers ===

def snapshot_to_game(
    snapshot: StateSnapshot,
    move_count: int,
    history_cache: deque,
) -> ContrastGame:
    game = ContrastGame()
    game.pieces.fill(0)
    game.tiles.fill(0)

    for coord, symbol in snapshot.pieces.items():
        x, y = coord_to_xy(coord)
        if symbol == "X":
            game.pieces[y, x] = P1
        elif symbol == "O":
            game.pieces[y, x] = P2

    for coord, tile in snapshot.tiles.items():
        x, y = coord_to_xy(coord)
        if tile == "b":
            game.tiles[y, x] = TILE_BLACK
        elif tile == "g":
            game.tiles[y, x] = TILE_GRAY
        else:
            game.tiles[y, x] = TILE_WHITE

    # Stock counts default to current values if missing
    game.tile_counts[0, 0] = snapshot.stock_black.get("X", int(game.tile_counts[0, 0]))
    game.tile_counts[1, 0] = snapshot.stock_black.get("O", int(game.tile_counts[1, 0]))
    game.tile_counts[0, 1] = snapshot.stock_gray.get("X", int(game.tile_counts[0, 1]))
    game.tile_counts[1, 1] = snapshot.stock_gray.get("O", int(game.tile_counts[1, 1]))

    game.current_player = P1 if snapshot.turn == "X" else P2
    game.game_over = snapshot.status != "ongoing"
    if snapshot.status.lower() in {"x_win", "xwin"}:
        game.winner = P1
    elif snapshot.status.lower() in {"o_win", "owin"}:
        game.winner = P2
    elif snapshot.status.lower() == "draw":
        game.winner = 0

    game.move_count = move_count

    # rebuild history cache for encoder
    game.history.clear()
    for entry in history_cache:
        game.history.append(entry)
    game.history.appendleft(
        (
            game.pieces.copy(),
            game.tiles.copy(),
            game.tile_counts.copy(),
        )
    )

    return game


# === AlphaZero Client ===


class AlphaZeroBot:
    def __init__(
        self,
        host: str,
        port: int,
        role: str,
        name: str,
        model_name: str,
        weights_path: Path,
        num_simulations: int,
    ) -> None:
        self.host = host
        self.port = port
        self.role_request = role.upper()
        self.nickname = name
        self.model_name = model_name or "alphazero"
        self.socket: Optional[socket.socket] = None
        self.buffer = ""
        self.collecting_state = False
        self.state_lines: List[str] = []
        self.player_role: Optional[str] = None
        self.awaiting_response = False
        self.last_status: Optional[str] = None
        self.prev_last_move: Optional[str] = None
        self.local_move_count = 0
        self.history_cache: deque = deque(maxlen=8)

        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
        LOGGER.info("Using device: %s", self.device)

        self.network = ContrastDualPolicyNet().to(self.device)
        if weights_path.exists():
            state = torch.load(weights_path, map_location=self.device)
            self.network.load_state_dict(state)
            LOGGER.info("Loaded weights from %s", weights_path)
        else:
            raise FileNotFoundError
        
            LOGGER.warning("Weights file not found: %s (running untrained)", weights_path)
        self.network.eval()

        self.mcts = MCTS(network=self.network, device=self.device)
        self.num_simulations = num_simulations

    def _network_value(self, game: ContrastGame) -> float:
        """Return raw network value from the current player's perspective."""
        state = torch.from_numpy(game.encode_state()).unsqueeze(0).to(self.device)
        self.network.eval()
        with torch.no_grad():
            _, _, value = self.network(state)
        return float(value.item())

    # === Network helpers ===
    def connect(self) -> None:
        self.socket = socket.create_connection((self.host, self.port))
        LOGGER.info("Connected to %s:%d", self.host, self.port)
        self.send_role_request()

    def send_role_request(self) -> None:
        role = self.role_request or "-"
        name = self.nickname or "-"
        model = self.model_name or "alphazero"
        self.send_line(f"ROLE {role} {name} {model}\n")

    def send_line(self, payload: str) -> None:
        if not self.socket:
            return
        try:
            self.socket.sendall(payload.encode("ascii"))
        except OSError as exc:
            LOGGER.warning("Send failed: %s", exc)

    def run(self) -> None:
        if not self.socket:
            raise RuntimeError("call connect() first")
        try:
            while True:
                chunk = self.socket.recv(4096)
                if not chunk:
                    LOGGER.info("Server closed connection")
                    break
                self.buffer += chunk.decode("utf-8", errors="ignore")
                self.process_buffer()
        except KeyboardInterrupt:
            LOGGER.info("Interrupted by user, closing")
        finally:
            if self.socket:
                self.socket.close()

    # === Protocol parsing ===
    def process_buffer(self) -> None:
        while "\n" in self.buffer:
            line, self.buffer = self.buffer.split("\n", 1)
            line = line.rstrip("\r")
            if self.collecting_state:
                if line == "END":
                    snapshot = parse_state_block(self.state_lines)
                    self.state_lines.clear()
                    self.collecting_state = False
                    self.handle_snapshot(snapshot)
                else:
                    self.state_lines.append(line)
                continue
            if line == "STATE":
                self.collecting_state = True
                self.state_lines.clear()
                continue
            self.handle_line(line)

    def handle_line(self, line: str) -> None:
        if line.startswith("INFO "):
            LOGGER.info(line)
            match = re.search(r"You are\s+([XO])", line)
            if match:
                self.player_role = match.group(1)
        elif line.startswith("ERROR "):
            LOGGER.error(line)
            self.awaiting_response = False
        elif line:
            LOGGER.info(line)

    # === Snapshot handling ===
    def handle_snapshot(self, snapshot: StateSnapshot) -> None:
        self.awaiting_response = False
        self.last_status = snapshot.status

        if snapshot.last_move and snapshot.last_move != self.prev_last_move:
            self.local_move_count += 1
            self.prev_last_move = snapshot.last_move

        if snapshot.status != "ongoing":
            LOGGER.info("Game finished: %s", snapshot.status)
            return

        if not self.player_role:
            self.player_role = snapshot.turn.upper()

        game = snapshot_to_game(snapshot, self.local_move_count, self.history_cache)

        # 常に最新盤面を履歴に追加 (相手番でも保持)
        self.history_cache.appendleft(
            (
                game.pieces.copy(),
                game.tiles.copy(),
                game.tile_counts.copy(),
            )
        )

        if snapshot.turn.upper() != (self.player_role or ""):
            return

        action = self.choose_action(game)
        if action is None:
            LOGGER.error("No action available")
            return

        self.send_move(action)
        self.awaiting_response = True

    # === MCTS driver ===
    def choose_action(self, game: ContrastGame) -> Optional[int]:
        LOGGER.info("Running MCTS (%d sims)...", self.num_simulations)
        root_value = self._network_value(game)
        policy, values = self.mcts.search(game, self.num_simulations)
        if not policy:
            return None
        action = max(policy, key=policy.get)
        move_value = values.get(action, 0.0)
        child_value = -move_value

        move_idx, tile_idx = decode_action(action)
        from_idx = move_idx // 25
        to_idx = move_idx % 25
        fx, fy = from_idx % 5, from_idx // 5
        tx, ty = to_idx % 5, to_idx // 5
        LOGGER.info(
            "AI action (%s): (%d,%d)->(%d,%d) Q(parent)=%.3f, V(child)=%.3f, net_root=%.3f",
            self.player_role or "?",
            fx,
            fy,
            tx,
            ty,
            move_value,
            child_value,
            root_value,
        )
        return action

    def send_move(self, action: int) -> None:
        move_idx, tile_idx = decode_action(action)
        from_idx = move_idx // 25
        to_idx = move_idx % 25
        fx, fy = from_idx % 5, from_idx // 5
        tx, ty = to_idx % 5, to_idx // 5
        origin = xy_to_coord(fx, fy)
        target = xy_to_coord(tx, ty)

        if tile_idx == 0:
            tile_text = "-1"
        else:
            if tile_idx <= 25:
                color = "b"
                idx = tile_idx - 1
            else:
                color = "g"
                idx = tile_idx - 26
            tile_x = idx % 5
            tile_y = idx // 5
            tile_text = f"{xy_to_coord(tile_x, tile_y)}{color}"

        move_text = f"{origin},{target} {tile_text}"
        LOGGER.info("Sending move: %s", move_text)
        self.send_line(f"MOVE {move_text}\n")


# === CLI ===


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="AlphaZero-style bot for contrast_arena")
    parser.add_argument("--host", default="127.0.0.1", help="Server host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8765, help="Server port (default: 8765)")
    parser.add_argument("--role", default="-", help="Preferred role X/O/spectator/-")
    parser.add_argument("--name", default="AlphaZero", help="Nickname reported to server")
    parser.add_argument("--model", default="alphazero", help="Model name reported to server")
    parser.add_argument(
        "--weights",
        type=Path,
        default=Path("contrast_model_final.pth"),
        help="Path to trained weights",
    )
    parser.add_argument(
        "--simulations",
        type=int,
        default=100,
        help="Number of MCTS simulations per move",
    )
    return parser.parse_args()


def main() -> None:
    setup_logger()
    args = parse_args()
    bot = AlphaZeroBot(
        host=args.host,
        port=args.port,
        role=args.role,
        name=args.name,
        model_name=args.model,
        weights_path=args.weights,
        num_simulations=args.simulations,
    )
    bot.connect()
    bot.run()


if __name__ == "__main__":
    main()
