#ifndef HANDY_MP_LAYOUT_H
#define HANDY_MP_LAYOUT_H
#pragma once

#include <cassert>
#include <memory>

struct XYPair
{
    unsigned x = {};
    unsigned y = {};
};

inline constexpr XYPair DistributeProportions(int players)
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

/**
 * When there are multiple lynxes, they need to be lay out on a screen.
 * This can change based on the number of them and the rotation.
 */
struct Layout
{
    int players = 0;

    XYPair single_system_pixels = {};
    XYPair total_pixels = {};
    XYPair n_systems = {};

    Layout(int players, unsigned single_width, unsigned single_height) 
    : players(players)
    , single_system_pixels{single_width, single_height}
    {
        assert(players > 0);
        assert(players <= 16); // the max is 18, but there are no games for >16 players

        n_systems = DistributeProportions(players);
        total_pixels = { n_systems.x * single_system_pixels.x, n_systems.y * single_system_pixels.y };
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
};

#endif // HANDY_MP_LAYOUT_H
