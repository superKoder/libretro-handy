// MIT License
//
// Copyright (c) 2024 superKoder (github.com/superKoder/)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The ABOVE COPYRIGHT notice and this permission notice SHALL BE INCLUDED in all
// copies or substantial portions of the Software.
//
// The software is provided "as is", without warranty of any kind, express or
// implied, including but not limited to the warranties of merchantability,
// fitness for a particular purpose and noninfringement. In no event shall the
// authors or copyright holders be liable for any claim, damages or other
// liability, whether in an action of contract, tort or otherwise, arising from,
// out of or in connection with the software or the use or other dealings in the
// software.

#ifndef HANDY_MP_LAYOUT_H
#define HANDY_MP_LAYOUT_H
#pragma once

#include <cassert>
#include <memory>

#include "mikie.h"

struct XYPair
{
    unsigned x = {};
    unsigned y = {};
};

inline constexpr XYPair DistributeProportionsHorizontally(int players)
{
    switch (players)
    {
    case 1:
        return {1, 1};

    case 2:
        return {2, 1};

    case 3:
    case 4:
        return {2, 2};

    case 5:
    case 6:
        return {3, 2};

    case 7:
    case 8:
        return {4, 2};

    case 9:
        return {3, 3};

    case 10:
    case 11:
    case 12:
        return {4, 3};

    case 13:
    case 14:
    case 15:
        return {5, 3};

    case 16:
        return {4, 4};
    }
    return {1, 1};
}

inline constexpr XYPair DistributeProportionsVertically(int players)
{
    switch (players)
    {
    case 1:
        return {1, 1};

    case 2:
        return {1, 2};

    case 3:
    case 4:
        return {2, 2};

    case 5:
    case 6:
        return {2, 3};

    case 7:
    case 8:
        return {2, 4};

    case 9:
        return {3, 3};

    case 10:
    case 11:
    case 12:
        return {3, 4};

    case 13:
    case 14:
    case 15:
        return {3, 5};

    case 16:
        return {4, 4};
    }
    return {1, 1};
}

inline constexpr XYPair DistributeProportions(int players, bool vertical) {
    return (vertical 
           ? DistributeProportionsVertically(players) 
           : DistributeProportionsHorizontally(players));
}

/**
 * When there are multiple lynxes, they need to be lay out on a screen.
 * This can change based on the number of them and the rotation.
 */
struct Layout
{
    enum class Orientation : UBYTE
    {
        // Values must match
        Invalid = MIKIE_BAD_MODE,

        None = MIKIE_NO_ROTATE,
        Left = MIKIE_ROTATE_L,
        Flip = MIKIE_ROTATE_B,
        Right = MIKIE_ROTATE_R,

        Auto = 255,
    };

    static constexpr bool IsVertical(Orientation orientation) {
        switch(orientation) {
            case Orientation::None: 
            case Orientation::Flip:
                return false;
            case Orientation::Left: 
            case Orientation::Right:
                return true;
            default:
                return false;  // unknown
        }
    }

    int players = {};
    bool is_vertical = {};
    XYPair single_system_pixels = {};
    XYPair n_systems = {};
    XYPair total_pixels = {};

    Layout(int players, unsigned single_width, unsigned single_height, Orientation orientation = Orientation::None) 
        : players {players}
        , is_vertical {IsVertical(orientation)}
        , single_system_pixels{single_width, single_height}
        , n_systems {DistributeProportions(players, is_vertical)}
        , total_pixels { n_systems.x * single_system_pixels.x, n_systems.y * single_system_pixels.y }
    {
        assert(players > 0);
        assert(players <= 16); // the max is 18, but there are no games for >16 players
    }

    constexpr XYPair PositionOfPlayer(int player) const {
        assert(players > 0);
        assert(players <= 16); // the max is 18, but there are no games for >16 players

        return { (player / n_systems.y), (player % n_systems.y) };
    }

    constexpr uint32_t FramebufferPitchForPos(XYPair pos, unsigned single_system_pitch) const
    {
        assert(players > 0);
        assert(players <= 16); // the max is 18, but there are no games for >16 players

        return n_systems.x * single_system_pitch;
    }

    uint32_t FramebufferPitchForPlayer(int player, unsigned single_system_pitch) const
    {
        return FramebufferPitchForPos(PositionOfPlayer(player), single_system_pitch);
    }

    uint32_t FramebufferOffsetForRow(unsigned y, unsigned single_system_pitch) const
    {
        assert(players > 0);
        assert(players <= 16); // the max is 18, but there are no games for >16 players

        return (single_system_pitch * single_system_pixels.y) * y;
    }

    uint32_t FramebufferOffsetForCol(unsigned x, unsigned single_system_pitch) const
    {
        assert(players > 0);
        assert(players <= 16); // the max is 18, but there are no games for >16 players

        return x * single_system_pitch;
    }

    uint32_t FramebufferOffsetForPlayer(int player, unsigned single_system_pitch) const
    {
        assert(players > 0);
        assert(players <= 16); // the max is 18, but there are no games for >16 players

        auto const pos = PositionOfPlayer(player);
        return (single_system_pixels.y * single_system_pitch * pos.y * n_systems.x) + (single_system_pitch * pos.x);
    }

    Layout ForOrientation(Orientation orientation) {
        return Layout(players, single_system_pixels.x, single_system_pixels.y, orientation);
    }
};

#endif // HANDY_MP_LAYOUT_H
