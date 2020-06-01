/*
 * This file is part of BBP Pairings, a Swiss-system chess tournament engine
 * Copyright (C) 2016  Jeremy Bierema
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3.0, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef FAST_H
#define FAST_H

#include <cstdint>
#include <list>
#include <ostream>
#include <utility>

#include <matching/computer.h>
#include <tournament/tournament.h>

#include "common.h"

#ifndef OMIT_FAST
namespace swisssystems
{
  namespace fast
  {
    namespace detail
    {
      constexpr std::uintmax_t preferenceSize =
        tournament::maxPlayers - (tournament::maxPlayers & 1u);
      constexpr std::uintmax_t colorCountSize =
        tournament::maxPlayers / 2u + 1u;
      constexpr std::uintmax_t sameScoreGroupSize =
        tournament::maxPlayers / 2u + 1u;

      constexpr std::uintmax_t sameScoreGroupMultiplierSize =
        preferenceSize * colorCountSize;
      constexpr std::uintmax_t compatibleMultiplierSize =
        sameScoreGroupMultiplierSize * sameScoreGroupSize;
      static_assert(
        compatibleMultiplierSize / colorCountSize / sameScoreGroupSize
          >= preferenceSize,
        "Overflow");
    }

    constexpr std::uintmax_t maxEdgeWeight =
      detail::compatibleMultiplierSize
        + detail::sameScoreGroupMultiplierSize
        + detail::preferenceSize
        + detail::preferenceSize
        - 1u;
    static_assert(
      maxEdgeWeight >= detail::compatibleMultiplierSize,
      "Overflow");

    typedef
      matching::computer_supporting_value<maxEdgeWeight>::type
      matching_computer;

    namespace detail
    {
      constexpr matching_computer::edge_weight colorMultiplier =
        preferenceSize;
      constexpr matching_computer::edge_weight sameScoreGroupMultiplier =
        sameScoreGroupMultiplierSize;
      constexpr matching_computer::edge_weight compatibleMultiplier =
        compatibleMultiplierSize;
    }

    std::list<Pairing> computeMatching(
      tournament::Tournament &&,
      std::ostream *const = nullptr);

    struct FastInfo final : public Info
    {
      std::list<Pairing> computeMatching(
        tournament::Tournament &&tournament,
        std::ostream *const ostream
      ) const override
      {
        return fast::computeMatching(std::move(tournament), ostream);
      }
      void updateAccelerations(tournament::Tournament &, tournament::round_index
      ) const override;
    };
  }
}
#endif

#endif
