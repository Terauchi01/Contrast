#!/usr/bin/env python3
"""Small harness for inspecting MCTS value propagation."""
from __future__ import annotations

import argparse
from pathlib import Path
from typing import Tuple

import torch

from contrast_game import ContrastGame
from mcts import MCTS
from model import ContrastDualPolicyNet

MOVE_SIZE = 25 * 25  # from ContrastDualPolicyNet
TILE_SIZE = 51


class ConstantNetwork(torch.nn.Module):
    """Tiny stub network that emits constant logits and value."""

    def __init__(self, value: float):
        super().__init__()
        self.register_buffer("constant_value", torch.tensor([[value]], dtype=torch.float32))

    def forward(self, x: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
        batch = x.size(0)
        move_logits = torch.zeros(batch, MOVE_SIZE, dtype=torch.float32, device=x.device)
        tile_logits = torch.zeros(batch, TILE_SIZE, dtype=torch.float32, device=x.device)
        value = self.constant_value.expand(batch, 1)
        return move_logits, tile_logits, value


def _load_network(weights_path: str | None, device: torch.device, const_value: float) -> torch.nn.Module:
    if weights_path:
        weights = Path(weights_path)
        if not weights.exists():
            raise FileNotFoundError(f"Weights file not found: {weights}")

        net = ContrastDualPolicyNet().to(device)
        state = torch.load(weights, map_location=device)
        if isinstance(state, dict) and "state_dict" in state:
            state = state["state_dict"]
        missing, unexpected = net.load_state_dict(state, strict=False)
        if missing:
            print(f"Warning: missing keys {missing}")
        if unexpected:
            print(f"Warning: unexpected keys {unexpected}")
        print(f"Loaded weights from {weights}")
        return net

    net = ConstantNetwork(const_value).to(device)
    print(f"Using constant stub network (value={const_value})")
    return net


def main() -> None:
    parser = argparse.ArgumentParser(description="Debug MCTS value wiring")
    parser.add_argument("--weights", type=str, default=None, help="Path to ContrastDualPolicyNet weights (.pth)")
    parser.add_argument("--value", type=float, default=0.25, help="Constant value emitted when no weights are provided")
    parser.add_argument("--sims", type=int, default=16, help="Number of MCTS simulations")
    parser.add_argument("--device", type=str, default="cpu", help="Torch device string (cpu, cuda, cuda:0, ...)")
    parser.add_argument("--eps", type=float, default=0.0, help="Dirichlet noise epsilon passed to MCTS")
    args = parser.parse_args()

    device = torch.device(args.device)
    game = ContrastGame()
    net = _load_network(args.weights, device, args.value)
    mcts = MCTS(network=net, device=device, epsilon=args.eps)

    net.eval()
    with torch.no_grad():
        input_tensor = torch.from_numpy(game.encode_state()).unsqueeze(0).to(device)
        _, _, raw_value = net(input_tensor)
        raw_value = raw_value.item()

    policy, values = mcts.search(game, args.sims)

    print(f"Network root value (player-to-move view): {raw_value}")
    print(f"Unique policy entries: {len(policy)}")
    first = next(iter(values.values()), None)
    print(f"First action Q (parent view): {first}")
    if first is not None:
        print(f"Child view of same action: {-first}")
        if args.weights is None:
            diff = abs((-first) - args.value)
            print(f"Deviation from constant stub: {diff:.6f}")


if __name__ == "__main__":
    main()
