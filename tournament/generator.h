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


#ifndef GENERATOR_H
#define GENERATOR_H

#include <functional>
#include <ostream>
#include <stdexcept>
#include <string>

#include <swisssystems/common.h>
#include <utility/random.h>
#include <utility/uinttypes.h>

#include "tournament.h"

namespace tournament
{
  namespace generator
  {
    struct BadConfigurationException : public std::runtime_error
    {
      BadConfigurationException(const std::string &explanation)
        : std::runtime_error(explanation) { }
    };

    struct Configuration;

    /**
     * A class representing configuration options affecting the matches in the
     * tournament (once the players have already been chosen).
     */
    class MatchesConfiguration
    {
    public:
      tournament::Tournament tournament;

      /**
       * Indicates that an acceleration defined by the Swiss system used should
       * be applied when generating the tournament.
       */
      bool automaticAcceleration{ };

      tournament::round_index roundsNumber;

      float forfeitRate;
      float retiredRate;
      float halfPointByeRate;
      utility::uinttypes::uint_least_for_value<100> drawPercentage;

      MatchesConfiguration() = default;

      template <class RandomEngine>
      MatchesConfiguration(
        Configuration &&,
        RandomEngine &);
      MatchesConfiguration(tournament::Tournament &&);

    protected:
      template <class RandomEngine>
      explicit MatchesConfiguration(RandomEngine &randomEngine)
        : roundsNumber(utility::random::uniformUint(randomEngine, 5u, 15u)),
          forfeitRate(utility::random::uniformUint(randomEngine, 6u, 30u)),
          retiredRate(
            utility::random::uniformUint(randomEngine, 15u, 3225u)),
          halfPointByeRate(
            utility::random::uniformUint(randomEngine, 15u, 3225u)),
          drawPercentage(
            utility::random::uniformUint(randomEngine, 10u, 50u))
      {
        static_assert(15u <= tournament::maxRounds, "Overflow");
        tournament.expectedRounds = roundsNumber;
      }
    };

    /**
     * A class represent options to generate a random tournament, including
     * random players.
     */
    struct Configuration : public MatchesConfiguration
    {
      tournament::player_index playersNumber;

      tournament::rating highestRating;
      tournament::rating lowestRating;

      template <class RandomEngine>
      Configuration(RandomEngine &randomEngine_)
        : MatchesConfiguration(randomEngine_),
          playersNumber(
            utility::random::uniformUint(randomEngine_, 15u, 215u)),
          highestRating(
            utility::random::uniformUint(randomEngine_, 2400u, 2800u)),
          lowestRating(
            utility::random::uniformUint(randomEngine_, 1400u, 2300u))
      {
        static_assert(215u <= tournament::maxPlayers, "Overflow");
        static_assert(2800u <= ~tournament::rating{ }, "Overflow");
      }
    };

    template <class RandomEngine>
    void generateTournament(
      Tournament &,
      MatchesConfiguration &&,
      swisssystems::SwissSystem,
      RandomEngine &,
      std::ostream *);
  }
}

#endif
