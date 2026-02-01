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
   * Update players' acceleration, unplayed games, and color preference data
   * members, while also verifying that overflow is not caused by exceeding
   * build limits.
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
        round_index playedGames{ };
        for (const Match &match : player.matches)
        {
          if (match.gameWasPlayed)
          {
            ++playedGames;
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
        player.playedGames = playedGames;
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
  std::vector<std::unordered_set<player_index>>
    Tournament::resolveForbiddenPairs(round_index roundIndex) const &
  {
    std::vector<std::unordered_set<player_index>> result(players.size());
    for (const auto &entry : forbiddenPairs)
    {
      if (roundIndex < entry.roundStart || roundIndex >= entry.roundEnd)
      {
        continue;
      }

      for (const auto player1Index : entry.players)
      {
        result[player1Index].insert(entry.players.begin(), entry.players.end());
      }
    }

    return result;
  }
}
