#pragma once
#include "contrast/types.hpp"
#include <cstdint>

namespace contrast {

constexpr int kMaxRayLength = (BOARD_W > BOARD_H ? BOARD_W : BOARD_H) - 1;
constexpr int kMaxDirections = 8;
constexpr int kTileTypeCount = 3;
constexpr int kBoardSize = BOARD_W * BOARD_H;

struct PrecomputedDirection {
    uint8_t step_count;
    int8_t rel_index[kMaxRayLength];
};

struct MoveTableEntry {
    uint8_t dir_count;
    PrecomputedDirection dirs[kMaxDirections];
};

inline constexpr MoveTableEntry kMoveTable[kTileTypeCount][kBoardSize] = {
// TileType::None
{
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
},
// TileType::Black
{
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(4),
                { +6, +12, +18, +24 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +6, +12, +18, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +4, +8, +12, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +4, +8, +12, +16 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +6, +12, +18, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(3),
                { +6, +12, +18, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +4, +8, +12, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +4, +8, +12, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -4, -8, -12, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -4, -8, -12, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -6, -12, -18, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -6, -12, -18, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -4, -8, -12, -16 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -4, -8, -12, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -6, -12, -18, +0 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
    {
        static_cast<uint8_t>(4),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -6, -12, -18, -24 }
            },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
            { 0, { +0, +0, +0, +0 } },
        }
    },
},
// TileType::Gray
{
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +6, +12, +18, +24 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +6, +12, +18, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +4, +8, +12, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(4),
                { +5, +10, +15, +20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { +4, +8, +12, +16 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +6, +12, +18, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +6, +12, +18, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +4, +8, +12, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(3),
                { +5, +10, +15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { +4, +8, +12, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +6, +12, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(2),
                { +5, +10, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -5, -10, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { +4, +8, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -4, -8, -12, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -4, -8, -12, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +6, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -6, -12, -18, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(1),
                { +5, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -5, -10, -15, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { +4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -6, -12, -18, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(4),
                { +1, +2, +3, +4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -4, -8, -12, -16 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(3),
                { +1, +2, +3, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -4, -8, -12, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -6, +0, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(2),
                { +1, +2, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -1, -2, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -4, -8, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(2),
                { -6, -12, +0, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(1),
                { +1, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -1, -2, -3, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(1),
                { -4, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(3),
                { -6, -12, -18, +0 }
            },
        }
    },
    {
        static_cast<uint8_t>(8),
        {
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -1, -2, -3, -4 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -5, -10, -15, -20 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(0),
                { +0, +0, +0, +0 }
            },
            {
                static_cast<uint8_t>(4),
                { -6, -12, -18, -24 }
            },
        }
    },
},
};
}