#include <algorithm>
#include <deque>
#include <iterator>
#include <limits>
#include <list>
#include <ostream>
#include <stdexcept>
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

#ifndef OMIT_BURSTEIN
namespace swisssystems
{
  namespace burstein
  {
    void BursteinInfo::updateAccelerations(
      tournament::Tournament &tournament,
      const tournament::round_index round_index
    ) const
    {
      if (round_index < 2)
      {
        tournament::player_index rankBound{ };
        for (
          const tournament::player_index player_index : tournament.playersByRank
        )
        {
          const tournament::Player &player = tournament.players[player_index];
          if (!player.matches.size() || player.matches[0].participatedInPairing)
          {
            ++rankBound;
          }
        }

        if (rankBound > 1u)
        {
          for (
            const tournament::player_index player_index
              : tournament.playersByRank)
          {
            tournament::Player &player = tournament.players[player_index];
            player.accelerations.push_back(tournament.pointsForWin);
            if (
              !player.matches.size() || player.matches[0].participatedInPairing)
            {
              rankBound -= 2;
              if (rankBound <= 1u)
              {
                break;
              }
            }
          }
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
        const tournament::Player &player,
        const tournament::Match &match,
        const tournament::Tournament &tournament)
      {
        return
          match.gameWasPlayed
            ? tournament.getPoints(player, match)
            : tournament.pointsForDraw;
      }

      /**
       * Get the virtual score of the opponent in a non-played game.
       */
      tournament::points getVirtualOpponentScore(
        const tournament::Player &player,
        const tournament::Match &match,
        const tournament::Tournament &tournament)
      {
        return
          match.matchScore == tournament::MATCH_SCORE_LOSS
                ? tournament.pointsForWin
            : match.matchScore == tournament::MATCH_SCORE_DRAW
                ? tournament.pointsForDraw
            : match.opponent == player.id
                  && match.participatedInPairing
                  && tournament.pointsForPairingAllocatedBye
                      < tournament.pointsForWin
                ? tournament.pointsForPairingAllocatedBye
                      < tournament.pointsForDraw
                    ? tournament.pointsForWin
                    : tournament.pointsForDraw
            : tournament.pointsForForfeitLoss;
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
        points_product result{ };
        const adjusted_score futureVirtualPoints =
          adjusted_score(tournament.playedRounds - 1u)
            * tournament.pointsForDraw;
        adjusted_score virtualPoints =
          futureVirtualPoints + player.acceleration(tournament);
        if (
          virtualPoints < futureVirtualPoints
            || (tournament.playedRounds > 1u
                  && futureVirtualPoints / (tournament.playedRounds - 1u)
                      < tournament.pointsForDraw))
        {
          assert(
            tournament.playedRounds > tournament::maxRounds
              || tournament.pointsForDraw > tournament::maxPoints);
          throw tournament::BuildLimitExceededException(
            "This build supports at most "
              + (tournament.playedRounds > tournament::maxRounds
                  ? utility::uintstringconversion
                        ::toString(tournament::maxRounds)
                      + " rounds."
                  : utility::uintstringconversion
                        ::toString(tournament::maxPoints, 1)
                      + " points per draw."));
        }

        tournament::round_index roundIndex{ };
        for (const tournament::Match &match : player.matches)
        {
          if (roundIndex >= tournament.playedRounds)
          {
            break;
          }
          if (match.gameWasPlayed)
          {
            const points_product addend =
              points_product{ adjustedScores[match.opponent] }
                * tournament.getPoints(player, match);
            result += addend;
            if (
              result < addend
                || (adjustedScores[match.opponent]
                      && addend / adjustedScores[match.opponent]
                          < tournament.getPoints(player, match)))
            {
              assert(
                tournament.playedRounds > tournament::maxRounds
                  || tournament.pointsForWin > tournament::maxPoints
                  || tournament.pointsForDraw > tournament::maxPoints
                  || tournament.pointsForLoss > tournament::maxPoints
                  || tournament.pointsForForfeitLoss > tournament::maxPoints
                  || tournament.pointsForZeroPointBye > tournament::maxPoints
                  || tournament.pointsForPairingAllocatedBye
                      > tournament::maxPoints);
              throw tournament::BuildLimitExceededException(
                "This build supports at most "
                  + (tournament.playedRounds > tournament::maxRounds
                      ? utility::uintstringconversion
                            ::toString(tournament::maxRounds)
                          + " rounds."
                      : utility::uintstringconversion
                            ::toString(tournament::maxPoints, 1)
                          + "points per match."));
            }
          }
          else
          {
            const adjusted_score virtualScore =
              virtualPoints
                + getVirtualOpponentScore(player, match, tournament);
            const points_product scaledScore =
              points_product{ tournament.getPoints(player, match) }
                * virtualScore;
            result += scaledScore;
            if (
              result < scaledScore
                || virtualScore < virtualPoints
                || (virtualScore
                      && scaledScore / virtualScore
                          < tournament.getPoints(player, match)))
            {
              assert(
                tournament.playedRounds > tournament::maxRounds
                  || tournament.pointsForWin > tournament::maxPoints
                  || tournament.pointsForDraw > tournament::maxPoints
                  || tournament.pointsForLoss > tournament::maxPoints
                  || tournament.pointsForForfeitLoss > tournament::maxPoints
                  || tournament.pointsForZeroPointBye > tournament::maxPoints
                  || tournament.pointsForPairingAllocatedBye
                      > tournament::maxPoints);
              throw tournament::BuildLimitExceededException(
                "This build supports at most "
                  + (tournament.playedRounds > tournament::maxRounds
                      ? utility::uintstringconversion
                            ::toString(tournament::maxRounds)
                          + " rounds."
                      : utility::uintstringconversion
                            ::toString(tournament::maxPoints, 1)
                          + " points per match."));
            }
          }
          virtualPoints += tournament.getPoints(player, match);
          if (
            virtualPoints < tournament.getPoints(player, match)
              && virtualPoints >= tournament.pointsForDraw)
          {
            assert(
              tournament.playedRounds > tournament::maxRounds
                || tournament.pointsForWin > tournament::maxPoints
                || tournament.pointsForDraw > tournament::maxPoints
                || tournament.pointsForLoss > tournament::maxPoints
                || tournament.pointsForForfeitLoss > tournament::maxPoints
                || tournament.pointsForZeroPointBye > tournament::maxPoints
                || tournament.pointsForPairingAllocatedBye
                    > tournament::maxPoints);
            throw tournament::BuildLimitExceededException(
              "This build supports at most "
                + (tournament.playedRounds > tournament::maxRounds
                    ? utility::uintstringconversion
                          ::toString(tournament::maxRounds)
                        + " rounds."
                    : utility::uintstringconversion
                          ::toString(tournament::maxPoints, 1)
                        + "points per match."));
          }
          virtualPoints -= tournament.pointsForDraw;
          ++roundIndex;
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
        points_product result{ };
        const adjusted_score futureVirtualPoints =
          adjusted_score(tournament.playedRounds - 1u)
            * tournament.pointsForDraw;
        adjusted_score virtualPoints =
          futureVirtualPoints + player.acceleration(tournament);
        if (
          virtualPoints < futureVirtualPoints
            || (tournament.playedRounds > 1u
                  && futureVirtualPoints / (tournament.playedRounds - 1u)
                      < tournament.pointsForDraw))
        {
          assert(
            tournament.playedRounds > tournament::maxRounds
              || tournament.pointsForDraw > tournament::maxPoints);
          throw tournament::BuildLimitExceededException(
            "This build supports at most "
              + (tournament.playedRounds > tournament::maxRounds
                  ? utility::uintstringconversion
                        ::toString(tournament::maxRounds)
                      + " rounds."
                  : utility::uintstringconversion
                        ::toString(tournament::maxPoints, 1)
                      + " points per draw."));
        }

        adjusted_score min =
          std::numeric_limits<adjusted_score>::max();
        adjusted_score max{ };

        tournament::round_index roundIndex{ };
        for (const tournament::Match &match : player.matches)
        {
          if (roundIndex >= tournament.playedRounds)
          {
            break;
          }
          adjusted_score adjustment;
          if (match.gameWasPlayed)
          {
            adjustment = adjustedScores[match.opponent];
          }
          else
          {
            adjustment =
              virtualPoints
                + getVirtualOpponentScore(player, match, tournament);
            if (adjustment < virtualPoints)
            {
              assert(
                tournament.playedRounds > tournament::maxRounds
                  || tournament.pointsForWin > tournament::maxPoints
                  || tournament.pointsForDraw > tournament::maxPoints
                  || tournament.pointsForLoss > tournament::maxPoints
                  || tournament.pointsForForfeitLoss > tournament::maxPoints
                  || tournament.pointsForZeroPointBye > tournament::maxPoints
                  || tournament.pointsForPairingAllocatedBye
                      > tournament::maxPoints);
              throw tournament::BuildLimitExceededException(
                "This build supports at most "
                  + (tournament.playedRounds > tournament::maxRounds
                      ? utility::uintstringconversion
                            ::toString(tournament::maxRounds)
                          + " rounds."
                      : utility::uintstringconversion
                            ::toString(tournament::maxPoints, 1)
                          + " points per match."));
            }
          }
          result += adjustment;
          if (result < adjustment)
          {
            assert(
              tournament.playedRounds > tournament::maxRounds
                || tournament.pointsForWin > tournament::maxPoints
                || tournament.pointsForDraw > tournament::maxPoints
                || tournament.pointsForLoss > tournament::maxPoints
                || tournament.pointsForForfeitLoss > tournament::maxPoints
                || tournament.pointsForZeroPointBye > tournament::maxPoints
                || tournament.pointsForPairingAllocatedBye
                    > tournament::maxPoints);
            throw tournament::BuildLimitExceededException(
              "This build supports at most "
                + (tournament.playedRounds > tournament::maxRounds
                    ? utility::uintstringconversion
                          ::toString(tournament::maxRounds)
                        + " rounds."
                    : utility::uintstringconversion
                          ::toString(tournament::maxPoints, 1)
                        + " points per match."));
          }
          min = std::min(min, adjustment);
          max = std::max(max, adjustment);

          virtualPoints += tournament.getPoints(player, match);
          if (
            virtualPoints < tournament.getPoints(player, match)
              && virtualPoints >= tournament.pointsForDraw)
          {
            assert(
              tournament.playedRounds > tournament::maxRounds
                || tournament.pointsForWin > tournament::maxPoints
                || tournament.pointsForDraw > tournament::maxPoints
                || tournament.pointsForLoss > tournament::maxPoints
                || tournament.pointsForForfeitLoss > tournament::maxPoints
                || tournament.pointsForZeroPointBye > tournament::maxPoints
                || tournament.pointsForPairingAllocatedBye
                    > tournament::maxPoints);
            throw tournament::BuildLimitExceededException(
              "This build supports at most "
                + (tournament.playedRounds > tournament::maxRounds
                    ? utility::uintstringconversion
                          ::toString(tournament::maxRounds)
                        + " rounds."
                    : utility::uintstringconversion
                          ::toString(tournament::maxPoints, 1)
                        + "points per match."));
          }
          virtualPoints -= tournament.pointsForDraw;
          ++roundIndex;
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
          : playerScore(player.scoreWithAcceleration(tournament)),
            sonnebornBerger(
              calculateSonnebornBerger(player, tournament, adjustedScores)),
            buchholzTiebreak(
              calculateBuchholzTiebreak(player, tournament, adjustedScores)),
            medianTiebreak(
              calculateBuchholzTiebreak(player, tournament, adjustedScores, true
              )
            ),
            rankIndex(player.rankIndex)
        {
          if (
            playerScore
              && (buchholzScore() / playerScore < buchholzTiebreak
                    || medianScore() / playerScore < medianTiebreak))
          {
            assert(
              tournament.playedRounds > tournament::maxRounds
                || tournament.pointsForWin > tournament::maxPoints
                || tournament.pointsForDraw > tournament::maxPoints
                || tournament.pointsForLoss > tournament::maxPoints
                || tournament.pointsForForfeitLoss > tournament::maxPoints
                || tournament.pointsForZeroPointBye > tournament::maxPoints
                || tournament.pointsForPairingAllocatedBye
                    > tournament::maxPoints
                || playerScore > tournament::maxPoints);
            throw tournament::BuildLimitExceededException(
              playerScore > tournament::maxPoints
                ? "This build does not support scores above "
                    + utility::uintstringconversion
                        ::toString(tournament::maxPoints, 1)
                    + '.'
                : "This build supports at most "
                    + (tournament.playedRounds > tournament::maxRounds
                        ? utility::uintstringconversion
                              ::toString(tournament::maxRounds)
                            + " rounds."
                        : utility::uintstringconversion
                              ::toString(tournament::maxPoints, 1)
                            + " points per match."));
          }
        }

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
              || (player0.absoluteColorPreference()
                    && player1.absoluteColorPreference()
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
        const tournament::Color result =
          choosePlayerNeutralColor(player, opponent);
        return
          result == tournament::COLOR_NONE
            ? player.colorPreference == tournament::COLOR_NONE
                ? player.rankIndex < opponent.rankIndex
                    ? player.rankIndex & 1u
                        ? invert(tournament.initialColor)
                        : tournament.initialColor
                    : opponent.rankIndex & 1u
                        ? tournament.initialColor
                        : invert(tournament.initialColor)
                : metricScores[player.id] < metricScores[opponent.id]
                    ? invert(opponent.colorPreference)
                    : player.colorPreference
            : result;
      }

      void printChecklist(
        const tournament::Tournament &tournament,
        const std::list<const tournament::Player *> &sortedPlayers,
        std::ostream &ostream,
        std::vector<MetricScores> &metricScores,
        const tournament::Player *const bye = nullptr,
        const std::vector<const tournament::Player *> *const matching = nullptr)
      {
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
          [&metricScores, &matching, bye, &tournament]
            (const tournament::Player &player)
          {
            const MetricScores &metricScore = metricScores[player.id];
            const tournament::Player *opponent =
              matching ? (*matching)[player.id] : nullptr;
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
     * Return a list of the pairings (in arbitrary order) produced by running
     * the Burstein algorithm. This runs in theoretical time O(n^3 + nr) for n
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
          adjustedScore = player.acceleration(tournament);
          tournament::round_index matchIndex{ };
          for (const tournament::Match &match : player.matches)
          {
            if (matchIndex++ < tournament.playedRounds)
            {
              adjustedScore += getAdjustedPoints(player, match, tournament);
            }
            if (match.gameWasPlayed)
            {
              player.forbiddenPairs.insert(match.opponent);
            }
          }
        }
        adjustedScores.push_back(adjustedScore);
      }

      if (
        sortedPlayers.size() - (sortedPlayers.size() & 1u)
          > tournament::maxPlayers)
      {
        throw tournament::BuildLimitExceededException(
          "This build supports at most "
            + utility::uintstringconversion::toString(tournament::maxPlayers)
            + " players.");
      }

      std::vector<MetricScores> metricScores;
      for (const tournament::Player &player : tournament.players)
      {
        metricScores.emplace_back(player, tournament, adjustedScores);
      }
      sortedPlayers.sort(
        [&metricScores,&tournament](
          const tournament::Player *const player0,
          const tournament::Player *const player1)
        {
          return
            std::make_tuple(
              player1->scoreWithAcceleration(tournament),
              metricScores[player1->id]
            ) < std::make_tuple(
                  player0->scoreWithAcceleration(tournament),
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
        bool eligibleForBye;
        do
        {
          --playerIterator;
          eligibleForBye =
            swisssystems::eligibleForBye(**playerIterator, tournament);
        } while (playerIterator != sortedPlayers.begin() && !eligibleForBye);
        if (!eligibleForBye)
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

      matching_computer matchingComputer(vertexLabels.size(), maxEdgeWeight);
      if (vertexLabels.size() > ~matching_computer::size_type{ })
      {
        throw std::length_error("");
      }

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
              && vertexLabels[scoreGroupBegin]
                    ->scoreWithAcceleration(tournament)
                  == vertexLabels[scoreGroups.back()]
                      ->scoreWithAcceleration(tournament)
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
            bye);
        }
        throw NoValidPairingException(
          "The non-bye players cannot be simultaneously paired without "
          "violating the absolute criteria.");
      }

      // Optimize the matching so that players at the top of their scoregroup
      // play those at the bottom.

      std::vector<const tournament::Player *>
        matchingById(tournament.players.size());
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
          if (!matchingById[vertexLabels[*vertexIterator]->id])
          {
            matching_computer::edge_weight neighborPriority = 1;
            for (
              decltype(fullScoreGroup)::const_iterator neighborIterator =
                std::next(vertexIterator, 1);
              neighborIterator != fullScoreGroup.end();
              ++neighborIterator)
            {
              if (!matchingById[vertexLabels[*neighborIterator]->id])
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
              matchingById[vertexLabels[*vertexIterator]->id] =
                vertexLabels[match];
              matchingById[vertexLabels[match]->id] =
                vertexLabels[*vertexIterator];
              result.emplace_back(
                vertexLabels[*vertexIterator]->id,
                vertexLabels[match]->id,
                choosePlayerColor(
                  *vertexLabels[*vertexIterator],
                  *vertexLabels[match],
                  tournament,
                  metricScores));

              finalizePair(*vertexIterator, match, matchingComputer);
            }
          }
        }

        scoreGroupBegin = *scoreGroupIterator;
      }

      if (ostream)
      {
        printChecklist(
          tournament,
          sortedPlayers,
          *ostream,
          metricScores,
          bye,
          &matchingById);
      }
      return result;
    }
  }
}
#endif
