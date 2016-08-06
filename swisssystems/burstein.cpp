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


#include <algorithm>
#include <deque>
#include <iterator>
#include <limits>
#include <list>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include <matching/computer.h>
#include <tournament/tournament.h>
#include <utility/typesizes.h>
#include <utility/uintstringconversion.h>
#include <utility/uinttypes.h>

#include "burstein.h"
#include "common.h"

namespace swisssystems
{
  namespace burstein
  {
    void BursteinInfo::updateAccelerations(tournament::Tournament &tournament)
      const
    {
      if (tournament.playedRounds < 2)
      {
        tournament::player_index rankBound{ };
        for (const tournament::Player &player : tournament.players)
        {
          if (player.isValid)
          {
            ++rankBound;
          }
        }

        for (tournament::Player &player : tournament.players)
        {
          player.acceleration =
            player.isValid && player.rankIndex < rankBound >> 1
              ? tournament.pointsForWin
              : 0;
          player.accelerations.push_back(player.acceleration);
        }
      }
    }

    namespace
    {
      using namespace detail;

      constexpr unsigned int adjustedScoreSize =
        utility::typesizes
            ::bitsToRepresent<unsigned int>(tournament::maxRounds - 1u)
          + utility::typesizes::bitsToRepresent<unsigned int>(
              tournament::maxPoints);
      static_assert(
        adjustedScoreSize >=
          utility::typesizes
            ::bitsToRepresent<unsigned int>(tournament::maxPoints),
        "Overflow");

      constexpr unsigned int pointsProductSize = adjustedScoreSize * 2u;
      static_assert(pointsProductSize / 2u >= adjustedScoreSize, "Overflow");

      /**
       * A type big enough to hold a player's adjusted score, where unplayed
       * games are replaced with draws.
       */
      typedef utility::uinttypes::uint_least<adjustedScoreSize> adjusted_score;
      /**
       * A type big enough to hold the product of a player's score and the sum
       * of his opponents' adjusted scores.
       */
      typedef utility::uinttypes::uint_least<pointsProductSize> points_product;

      /**
       * Get the adjusted score for a game: an unplayed game counts as a draw.
       */
      tournament::points getAdjustedPoints(
        const tournament::Match &match,
        const tournament::Tournament &tournament)
      {
        return
          match.gameWasPlayed
            ? tournament.getPoints(match.matchScore)
            : tournament.pointsForDraw;
      }

      points_product calculateSonnebornBerger(
        const tournament::Player &player,
        const tournament::Tournament &tournament,
        const std::vector<adjusted_score> &adjustedScores)
      {
        if (!player.isValid)
        {
          return 0;
        }
        tournament::points scoreSoFar = player.acceleration;
        points_product result{ };
        adjusted_score futureVirtualPoints =
          adjusted_score{ tournament.playedRounds } * tournament.pointsForDraw;
        for (const tournament::Match &match : player.matches)
        {
          futureVirtualPoints -= tournament.pointsForDraw;
          if (match.gameWasPlayed)
          {
            result +=
              points_product{ adjustedScores[match.opponent] }
                * tournament.getPoints(match.matchScore);
          }
          else
          {
            result +=
              tournament.getPoints(match.matchScore)
                * (points_product{ scoreSoFar }
                    + tournament.getPoints(tournament::invert(match.matchScore))
                    + futureVirtualPoints);
          }
          scoreSoFar += tournament.getPoints(match.matchScore);
        }
        return result;
      }

      points_product calculateBuchholzTiebreak(
        const tournament::Player &player,
        const tournament::Tournament &tournament,
        const std::vector<adjusted_score> &adjustedScores,
        const bool median = false)
      {
        if (!player.isValid || (median && tournament.playedRounds <= 2))
        {
          return 0;
        }
        tournament::points scoreSoFar = player.acceleration;
        points_product result{ };
        adjusted_score futureVirtualPoints =
          adjusted_score{ tournament.playedRounds } * tournament.pointsForDraw;
        adjusted_score min =
          std::numeric_limits<adjusted_score>::max();
        adjusted_score max{ };
        for (const tournament::Match &match : player.matches)
        {
          futureVirtualPoints -= tournament.pointsForDraw;
          adjusted_score adjustment;
          if (match.gameWasPlayed)
          {
            adjustment = adjustedScores[match.opponent];
          }
          else
          {
            adjustment = scoreSoFar;
            adjustment +=
              tournament.getPoints(tournament::invert(match.matchScore));
            adjustment += futureVirtualPoints;
          }
          result += adjustment;
          min = std::min(min, adjustment);
          max = std::max(max, adjustment);
          scoreSoFar += tournament.getPoints(match.matchScore);
        }
        if (median)
        {
          result -= min;
          result -= max;
        }
        return result;
      }

      /**
       * A class holding a player's accelerated score, rank index, and tiebreak
       * scores, used to order the players within a scoregroup (including a
       * floater from a higher scoregroup).
       */
      struct MetricScores
      {
        const tournament::points playerScore;
        const points_product sonnebornBerger;
        const points_product buchholzTiebreak;
        const points_product medianTiebreak;
        const tournament::player_index rankIndex;

        MetricScores(
            const tournament::Player &player,
            const tournament::Tournament &tournament,
            const std::vector<adjusted_score> &adjustedScores)
          : playerScore(player.scoreWithAcceleration()),
            sonnebornBerger(
              calculateSonnebornBerger(player, tournament, adjustedScores)),
            buchholzTiebreak(
              calculateBuchholzTiebreak(player, tournament, adjustedScores)),
            medianTiebreak(
              calculateBuchholzTiebreak(player, tournament, adjustedScores, true
              )
            ),
            rankIndex(player.rankIndex)
          { }

        /**
         * Compare two players in the same scoregroup (including a floater from
         * a higher scoregroup).
         */
        bool operator<(const MetricScores &that) const
        {
          return
            playerScore == that.playerScore
              ? std::tie(
                  sonnebornBerger,
                  buchholzTiebreak,
                  medianTiebreak,
                  that.rankIndex
                ) < std::tie(
                      that.sonnebornBerger,
                      that.buchholzTiebreak,
                      that.medianTiebreak,
                      rankIndex)
              : std::make_tuple(
                  sonnebornBerger,
                  buchholzScore(),
                  medianScore(),
                  that.rankIndex
                ) < std::make_tuple(
                      that.sonnebornBerger,
                      that.buchholzScore(),
                      that.medianScore(),
                      rankIndex);
        }

        points_product buchholzScore() const
        {
          return buchholzTiebreak * playerScore;
        }
        points_product medianScore() const
        {
          return medianTiebreak * playerScore;
        }
      };

      /**
       * Compute the weight of the edge to be passed to the matching algorithm.
       * useDueColor should be false for players with different scores.
       * The function assumes the two players are eligible to be paired (that
       * is, they are in the same scoregroup, or one could be floated into the
       * scoregroup of the other). The ranking of the player's opponents is not
       * incorporated.
       *
       * The edge weight is of the form 1 001 001 037 where the first 1
       * indicates two players are compatible (have not met and do not violate
       * absolute color preferences), the second one indicates the players are
       * from the same scoregroup (after merging), the third one indicates that
       * the players have different due colors, and the last section is reserved
       * for use in ranking the player's opponents.
       */
      matching_computer::edge_weight computeEdgeWeight(
        const tournament::Player &player0,
        const tournament::Player &player1,
        bool sameScoreGroup,
        bool useDueColor)
      {
        return
          player0.forbiddenPairs.count(player1.id)
              || (player0.absoluteColorPreference
                    && player1.absoluteColorPreference
                    && player0.colorPreference == player1.colorPreference)
            ? 0
            : compatibleMultiplier
                + sameScoreGroup * sameScoreGroupMultiplier
                + (
                    sameScoreGroup
                      && useDueColor
                      && colorPreferencesAreCompatible(
                          player0.colorPreference,
                          player1.colorPreference)
                  ) * colorMultiplier;
      }

      /**
       * Check, for all scoregroups in the tentative matching, that all players
       * are matched (except one in the lowest matched scoregroup) and that
       * there is at most one floater from each scoregroup.
       */
      bool checkMatchingIsValid(
        const matching_computer &matchingComputer,
        const std::deque<tournament::player_index> &scoreGroups)
      {
        const std::vector<matching_computer::vertex_index> currentMatching =
          matchingComputer.getMatching();
        std::deque<tournament::player_index>::const_iterator
            scoreGroupIterator =
          scoreGroups.begin();

        assert(0 == *scoreGroupIterator);

        utility::uinttypes::uint_least_for_value<3> unmatchedPlayerCount{ };
        tournament::player_index scoreGroupBegin;
        tournament::player_index vertexIndex{ };
        for (const tournament::player_index matchedIndex : currentMatching)
        {
          if (vertexIndex >= scoreGroups.back())
          {
            return true;
          }
          if (vertexIndex >= *scoreGroupIterator)
          {
            unmatchedPlayerCount = 0;
            scoreGroupBegin = vertexIndex;
            do
            {
              ++scoreGroupIterator;
            } while (vertexIndex >= *scoreGroupIterator);
          }
          if (
            vertexIndex == matchedIndex && vertexIndex < *++scoreGroups.rbegin()
          )
          {
            return false;
          }
          if (
            vertexIndex == matchedIndex
              || matchedIndex < scoreGroupBegin
              || matchedIndex >= *scoreGroupIterator)
          {
            if (++unmatchedPlayerCount > (*scoreGroupIterator & 1 ? 2 : 1))
            {
              return false;
            }
          }
          ++vertexIndex;
        }
        return true;
      }

      /**
       * A function to compute the color given to the first argument, whose
       * opponent is the second argument.
       */
      tournament::Color choosePlayerColor(
        const tournament::Player &player,
        const tournament::Player &opponent,
        const tournament::Tournament &tournament,
        const std::vector<MetricScores> &metricScores)
      {
        if (
          colorPreferencesAreCompatible(
            player.colorPreference,
            opponent.colorPreference))
        {
          if (player.colorPreference != tournament::COLOR_NONE)
          {
            return player.colorPreference;
          }
          else if (opponent.colorPreference != tournament::COLOR_NONE)
          {
            return invert(opponent.colorPreference);
          }
          else
          {
            return
              player.rankIndex < opponent.rankIndex
                ? player.rankIndex & 1u
                    ? invert(tournament.initialColor)
                    : tournament.initialColor
                : opponent.rankIndex & 1u
                    ? tournament.initialColor
                    : invert(tournament.initialColor);
          }
        }
        else if (player.absoluteColorPreference)
        {
          return player.colorPreference;
        }
        else if (opponent.absoluteColorPreference)
        {
          return invert(opponent.colorPreference);
        }
        else if (
          player.strongColorPreference && !opponent.strongColorPreference)
        {
          return player.colorPreference;
        }
        else if (
          opponent.strongColorPreference && !player.strongColorPreference)
        {
          return invert(opponent.colorPreference);
        }
        else
        {
          tournament::Color playerColor;
          tournament::Color opponentColor;
          findFirstColorDifference(
            player,
            opponent,
            playerColor,
            opponentColor);
          if (
            playerColor != tournament::COLOR_NONE
              && opponentColor != tournament::COLOR_NONE)
          {
            return opponentColor;
          }
          else
          {
            return
              metricScores[player.id] < metricScores[opponent.id]
                ? invert(opponent.colorPreference)
                : player.colorPreference;
          }
        }
      }

      void printChecklist(
        const tournament::Tournament &tournament,
        const std::list<const tournament::Player *> &sortedPlayers,
        std::ostream &ostream,
        std::vector<MetricScores> &metricScores,
        const std::vector<const tournament::Player *> *const vertexLabels =
          nullptr,
        const tournament::Player *const bye = nullptr,
        const std::vector<tournament::player_index> *const matching = nullptr)
      {
        std::vector<const tournament::Player *>
          matchingById(tournament.players.size());
        tournament::player_index vertexIndex{ };
        for (const tournament::Player *player : sortedPlayers)
        {
          if (matching && vertexLabels && player != bye)
          {
            matchingById[player->id] =
              (*vertexLabels)[(*matching)[vertexIndex++]];
          }
        }

        swisssystems::printChecklist(
          ostream,
          std::deque<std::string>{
            "Sonneborn-Berger",
            "Buchholz score",
            "Buchholz tiebreak",
            "Median score",
            "Median tiebreak",
            "Cur"
          },
          [&metricScores, &matchingById, bye, &tournament]
            (const tournament::Player &player)
          {
            const MetricScores &metricScore = metricScores[player.id];
            const tournament::Player *opponent = matchingById[player.id];
            return std::deque<std::string>{
              utility::uintstringconversion
                ::toString(metricScore.sonnebornBerger, 2),
              utility::uintstringconversion
                ::toString(metricScore.buchholzScore(), 2),
              utility::uintstringconversion
                ::toString(metricScore.buchholzTiebreak, 1),
              utility::uintstringconversion
                ::toString(metricScore.medianScore(), 2),
              utility::uintstringconversion
                ::toString(metricScore.medianTiebreak, 1),
              bye == &player ? "(bye)"
                : opponent
                  ? '('
                      + utility::uintstringconversion
                          ::toString(opponent->id + 1u)
                      + (choosePlayerColor(
                            player,
                            *opponent,
                            tournament,
                            metricScores
                          ) == tournament::COLOR_WHITE ? 'W' : 'B'
                        )
                      + ')'
                : ""
            };
          },
          tournament,
          sortedPlayers
        );
      }
    }

    /**
     * Return a list of the pairings (in standard order) produced by running the
     * Burstein algorithm. This runs in theoretical time O(n^3 + nr) for n
     * players and r previous rounds. If ostream is nonnull, output a checklist
     * file.
     *
     * @throws NoValidPairingExists if no valid pairing exists.
     */
    std::list<Pairing> computeMatching(
      tournament::Tournament &&tournament,
      std::ostream *const ostream)
    {
      // Compute tiebreak scores for each player, and sort them into scoregroups
      // and within scoregroups.
      std::list<const tournament::Player *> sortedPlayers;
      std::vector<adjusted_score> adjustedScores;
      for (tournament::Player &player : tournament.players)
      {
        adjusted_score adjustedScore{ };
        if (player.isValid)
        {
          if (player.matches.size() <= tournament.playedRounds)
          {
            sortedPlayers.push_back(&player);
          }
          adjustedScore = player.acceleration;
          tournament::round_index matchIndex{ };
          for (const tournament::Match &match : player.matches)
          {
            if (matchIndex++ < tournament.playedRounds)
            {
              adjustedScore += getAdjustedPoints(match, tournament);
            }
            if (match.gameWasPlayed)
            {
              player.forbiddenPairs.insert(match.opponent);
            }
          }
        }
        adjustedScores.push_back(adjustedScore);
      }
      std::vector<MetricScores> metricScores;
      for (const tournament::Player &player : tournament.players)
      {
        metricScores.emplace_back(player, tournament, adjustedScores);
      }
      sortedPlayers.sort(
        [&metricScores](
          const tournament::Player *const player0,
          const tournament::Player *const player1)
        {
          return
            std::make_tuple(
              player1->scoreWithAcceleration(),
              metricScores[player1->id]
            ) < std::make_tuple(
                  player0->scoreWithAcceleration(),
                  metricScores[player0->id]);
        }
      );

      std::list<Pairing> result;

      // Choose the player to receive the bye, and add the bye to result. Do not
      // include the bye player in the vector of vertices.
      std::list<const tournament::Player *>::iterator byeIterator;
      const tournament::Player *bye{ };
      if (sortedPlayers.size() & 1u)
      {
        std::list<const tournament::Player *>::iterator playerIterator =
          sortedPlayers.end();
        bool unplayedPoints;
        do
        {
          --playerIterator;
          unplayedPoints = false;
          for (const tournament::Match &match : (*playerIterator)->matches)
          {
            if (!match.gameWasPlayed && tournament.getPoints(match.matchScore))
            {
              unplayedPoints = true;
            }
          }
        } while (playerIterator != sortedPlayers.begin() && unplayedPoints);
        if (unplayedPoints)
        {
          if (ostream)
          {
            printChecklist(
              tournament,
              sortedPlayers,
              *ostream,
              metricScores);
          }
          throw NoValidPairingException(
            "No player is eligible for the pairing-allocated bye.");
        }
        result.emplace_back((*playerIterator)->id, (*playerIterator)->id);
        bye = *playerIterator;
        byeIterator = playerIterator;
        ++byeIterator;
        sortedPlayers.erase(playerIterator);
      }
      /**
       * The vector of players to be paired (not including the player receiving
       * the bye).
       */
      std::vector<const tournament::Player *> vertexLabels(
        sortedPlayers.begin(),
        sortedPlayers.end());

      if (bye)
      {
        sortedPlayers.insert(byeIterator, bye);
        --byeIterator;
      }

      matching_computer matchingComputer;

      // Add the vertices to the matching computer.
      for (
        tournament::player_index playerIndex{ };
        playerIndex < vertexLabels.size();
        ++playerIndex)
      {
        matchingComputer.addVertex();
      }

      /**
       * The scoreGroups deque contains the vertex indices of the start of each
       * scoregroup in order. If a scoregroup can have no floaters to the next
       * scoregroup, the start index is included twice.
       */
      std::deque<tournament::player_index> scoreGroups{ 0, 0 };

      // Determine the scoregroups to be used for pairing, merging a scoregroup
      // and the one below it if the scoregroup cannot be paired.
      bool matchingIsValid = true;
      while (scoreGroups.back() < vertexLabels.size())
      {
        // Assign players to one scoregroup.
        scoreGroups.push_back(scoreGroups.back());
        do
        {
          // Keep merging with lower scoregroups as needed.
          const tournament::player_index scoreGroupBegin = scoreGroups.back();
          do
          {
            // Add all the players with one score to a scoregroup.
            for (
              tournament::player_index vertexIndex =
                *++++scoreGroups.rbegin();
              vertexIndex < scoreGroups.back();
              ++vertexIndex)
            {
              matchingComputer.setEdgeWeight(
                scoreGroups.back(),
                vertexIndex,
                computeEdgeWeight(
                  *vertexLabels[vertexIndex],
                  *vertexLabels[scoreGroups.back()],
                  vertexIndex >= *++scoreGroups.rbegin(),
                  false));
            }
            ++scoreGroups.back();
          } while (
            scoreGroups.back() < vertexLabels.size()
              && vertexLabels[scoreGroupBegin]->scoreWithAcceleration()
                  == vertexLabels[scoreGroups.back()]->scoreWithAcceleration()
          );
          // Create a virtual opponent for players in the current scoregroup
          // to ensure that no player in higher scoregroup remains unpaired.
          if (scoreGroups.back() & 1u)
          {
            for (
              tournament::player_index vertexIndex =
                *std::next(scoreGroups.rbegin(), 1);
              vertexIndex < scoreGroups.back();
              ++vertexIndex)
            {
              matchingComputer.setEdgeWeight(
                scoreGroups.back(),
                vertexIndex,
                compatibleMultiplier);
            }
          }
          matchingComputer.computeMatching();
          matchingIsValid = checkMatchingIsValid(matchingComputer, scoreGroups);
        } while (scoreGroups.back() < vertexLabels.size() && !matchingIsValid);
        if (
          scoreGroups.back() < vertexLabels.size() && !(scoreGroups.back() & 1u)
        )
        {
          scoreGroups.push_back(scoreGroups.back());
        }
      }

      // If the last scoregroup cannot be paired and there is another scoregroup
      // above it, merge the two scoregroups. Repeat.
      while (scoreGroups.size() > 3u && !matchingIsValid)
      {
        scoreGroups.pop_back();
        const tournament::player_index boundaryVertex = scoreGroups.back();
        scoreGroups.pop_back();
        const tournament::player_index scoreGroupBegin = scoreGroups.back();
        scoreGroups.pop_back();
        for (
          tournament::player_index outerIndex = scoreGroups.back();
          outerIndex < boundaryVertex;
          ++outerIndex)
        {
          for (
            tournament::player_index innerIndex = boundaryVertex;
            innerIndex < vertexLabels.size();
            ++innerIndex)
          {
            matchingComputer.setEdgeWeight(
              outerIndex,
              innerIndex,
              computeEdgeWeight(
                *vertexLabels[outerIndex],
                *vertexLabels[innerIndex],
                outerIndex >= scoreGroupBegin,
                false));
          }
        }
        matchingComputer.computeMatching();
        scoreGroups.push_back(scoreGroupBegin);
        scoreGroups.push_back(vertexLabels.size());
        matchingIsValid = checkMatchingIsValid(matchingComputer, scoreGroups);
      }

      if (!matchingIsValid)
      {
        if (ostream)
        {
          printChecklist(
            tournament,
            sortedPlayers,
            *ostream,
            metricScores,
            &vertexLabels,
            bye);
        }
        throw NoValidPairingException(
          "The non-bye players cannot be simultaneously paired without "
          "violating the absolute criteria.");
      }

      // Optimize the matching so that players at the top of their scoregroup
      // play those at the bottom.

      std::vector<bool> matchedVertices(vertexLabels.size());
      std::deque<tournament::player_index>::iterator scoreGroupIterator =
        scoreGroups.begin();
      tournament::player_index scoreGroupBegin = *scoreGroupIterator;
      tournament::player_index floaterIndex;
      bool isFloater{ };
      // Iterate over the scoregroups.
      while (++scoreGroupIterator != scoreGroups.end())
      {
        // Some scoregroup boundaries are repeated.
        if (scoreGroupBegin == *scoreGroupIterator)
        {
          continue;
        }
        // Collect the players to be paired in the scoregroup, including a
        // player floated from a higher scoregroup, ordered based on SB, etc.
        std::list<tournament::player_index> fullScoreGroup;
        for (
          tournament::player_index playerIndex = scoreGroupBegin;
          playerIndex < *scoreGroupIterator;
          ++playerIndex)
        {
          fullScoreGroup.push_back(playerIndex);
        }
        if (isFloater)
        {
          fullScoreGroup.push_back(floaterIndex);
          isFloater = false;
        }
        fullScoreGroup.sort(
          [&metricScores, &vertexLabels](const tournament::player_index index0,
             const tournament::player_index index1)
          {
            return
              metricScores[vertexLabels[index1]->id]
                < metricScores[vertexLabels[index0]->id];
          }
        );

        // Update edge weights to include the floater as part of the scoregroup,
        // as well as accounting for due color.
        for (const tournament::player_index vertexIndex : fullScoreGroup)
        {
          for (
            decltype(fullScoreGroup)::const_reverse_iterator neighborIterator =
              fullScoreGroup.rbegin();
            *neighborIterator != vertexIndex;
            ++neighborIterator)
          {
            matchingComputer.setEdgeWeight(
              vertexIndex,
              *neighborIterator,
              computeEdgeWeight(
                *vertexLabels[vertexIndex],
                *vertexLabels[*neighborIterator],
                true,
                true));
          }
        }

        // Starting with the highest player, find the lowest player that
        // preserves the matching.
        for (
          decltype(fullScoreGroup)::const_iterator vertexIterator
            = fullScoreGroup.begin();
          vertexIterator != fullScoreGroup.end();
          ++vertexIterator)
        {
          if (!matchedVertices[*vertexIterator])
          {
            matching_computer::edge_weight neighborPriority = 1;
            for (
              decltype(fullScoreGroup)::const_iterator neighborIterator =
                std::next(vertexIterator, 1);
              neighborIterator != fullScoreGroup.end();
              ++neighborIterator)
            {
              if (!matchedVertices[*neighborIterator])
              {
                matching_computer::edge_weight edgeWeight =
                  computeEdgeWeight(
                    *vertexLabels[*vertexIterator],
                    *vertexLabels[*neighborIterator],
                    true,
                    true);
                if (edgeWeight)
                {
                  matchingComputer.setEdgeWeight(
                    *vertexIterator,
                    *neighborIterator,
                    edgeWeight + neighborPriority++);
                }
              }
            }
            matchingComputer.computeMatching();
            matching_computer::vertex_index match =
              matchingComputer.getMatching()[*vertexIterator];
            if (match >= *scoreGroupIterator)
            {
              floaterIndex = *vertexIterator;
              isFloater = true;
            }
            else
            {
              // Finalize the match so the two players will not be reassigned.
              matchedVertices[*vertexIterator] = true;
              matchedVertices[match] = true;
              for (
                tournament::player_index playerIndex{ };
                playerIndex < vertexLabels.size();
                ++playerIndex)
              {
                if (*vertexIterator != playerIndex)
                {
                  matchingComputer.setEdgeWeight(
                    *vertexIterator,
                    playerIndex,
                    playerIndex == match);
                }
                if (match != playerIndex)
                {
                  matchingComputer.setEdgeWeight(
                    match,
                    playerIndex,
                    playerIndex == *vertexIterator);
                }
              }
            }
          }
        }

        scoreGroupBegin = *scoreGroupIterator;
      }

      matchingComputer.computeMatching();
      std::vector<matching_computer::vertex_index> matching =
        matchingComputer.getMatching();

      if (ostream)
      {
        printChecklist(
          tournament,
          sortedPlayers,
          *ostream,
          metricScores,
          &vertexLabels,
          bye,
          &matching);
      }

      // Generate the return list.
      tournament::player_index vertexIndex{ };
      for (const tournament::Player *const player : vertexLabels)
      {
        if (matching[vertexIndex] > vertexIndex)
        {
          const tournament::Player &opponent =
            *vertexLabels[matching[vertexIndex]];

          result.emplace_back(
            player->id,
            vertexLabels[matching[vertexIndex]]->id,
            choosePlayerColor(*player, opponent, tournament, metricScores));
        }
        ++vertexIndex;
      }

      sortResults(result, tournament);
      return result;
    }
  }
}
