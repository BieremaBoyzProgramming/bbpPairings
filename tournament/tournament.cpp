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


#include <deque>

#include <utility/uintstringconversion.h>

#include "tournament.h"

namespace tournament
{
  /**
   * Update players' rankIndex and isValid members. Check that the maximum
   * number of players has not been exceeded.
   */
  void Tournament::updateRanks() &
  {
    player_index effectivePairingNumber{ };
    for (const player_index playerIndex : playersByRank)
    {
      Player &player = players[playerIndex];

      // Update isValid.
      player.isValid = player.matches.size() <= playedRounds;
      for (const Match &match : player.matches)
      {
        if (match.participatedInPairing)
        {
          player.isValid = true;
        }
      }

      if (player.isValid)
      {
        // Update rankIndex.
        player.rankIndex = effectivePairingNumber++;
      }
    }
  }

  /**
   * Update players' acceleration and color preference data members, while also
   * verifying that overflow is not caused by exceeding build limits.
   */
  void Tournament::computePlayerData() &
  {
    for (Player &player : players)
    {
      if (player.isValid)
      {
        // Update color preferences.
        round_index gamesAsWhite{ };
        round_index gamesAsBlack{ };
        player_index consecutiveCount{ };
        for (const Match &match : player.matches)
        {
          if (match.gameWasPlayed)
          {
            ++(match.color == COLOR_WHITE ? gamesAsWhite : gamesAsBlack);
            if (!consecutiveCount || match.color != player.repeatedColor)
            {
              consecutiveCount = 1;
            }
            else
            {
              ++consecutiveCount;
            }
            player.repeatedColor = match.color;
          }
        }
        const Color lowerColor =
          gamesAsWhite > gamesAsBlack
            ? tournament::COLOR_BLACK
            : tournament::COLOR_WHITE;
        player.colorImbalance =
          lowerColor == COLOR_BLACK
            ? gamesAsWhite - gamesAsBlack
            : gamesAsBlack - gamesAsWhite;
        player.colorPreference =
          player.colorImbalance > 1 ? lowerColor
            : consecutiveCount > 1 ? invert(player.repeatedColor)
            : player.colorImbalance > 0 ? lowerColor
            : consecutiveCount ? invert(player.repeatedColor)
            : COLOR_NONE;
        if (consecutiveCount <= 1u)
        {
          player.repeatedColor = COLOR_NONE;
        }
        player.strongColorPreference =
          !player.absoluteColorPreference() && player.colorImbalance;
      }
    }
  }

  /**
    * Exclude any players in forbidden from playing each other.
    */
   void Tournament::forbidPairs(const std::deque<player_index> &forbidden) &
   {
     for (const player_index playerId : forbidden)
     {
       if (playerId >= players.size())
       {
         players.insert(
           players.end(),
           playerId - players.size() + 1,
           Player());
       }
       for (const player_index teammate : forbidden)
       {
         players[playerId].forbiddenPairs.insert(teammate);
       }
     }
   }
}
