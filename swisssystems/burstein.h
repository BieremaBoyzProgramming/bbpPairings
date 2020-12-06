#ifndef BURSTEIN_H
#define BURSTEIN_H

#include <cstdint>
#include <list>
#include <ostream>
#include <utility>

#include <matching/computer.h>
#include <tournament/tournament.h>

#include "common.h"

#ifndef OMIT_BURSTEIN
namespace swisssystems
{
  namespace burstein
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

    struct BursteinInfo final : public Info
    {
      std::list<Pairing> computeMatching(
        tournament::Tournament &&tournament,
        std::ostream *const ostream
      ) const override
      {
        return burstein::computeMatching(std::move(tournament), ostream);
      }
      void updateAccelerations(tournament::Tournament &, tournament::round_index
      ) const override;
    };
  }
}
#endif

#endif
