#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iterator>
#include <list>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tournament/tournament.h>
#include <utility/typesizes.h>
#include <utility/uintstringconversion.h>
#include <utility/uinttypes.h>

#include "common.h"
#include "dutch.h"

#ifndef OMIT_DUTCH
namespace swisssystems
{
  namespace dutch
  {
    namespace
    {
      /**
       * Determine whether the two players could meet without violating absolute
       * criteria.
       */
      bool compatible(
        const tournament::Player &player0,
        const tournament::Player &player1,
        const tournament::Tournament &tournament,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs)
      {
        constexpr unsigned int maxPointsSize =
          utility::typesizes
              ::bitsToRepresent<unsigned int>(tournament::maxRounds)
            + utility::typesizes
                ::bitsToRepresent<unsigned int>(tournament::maxPoints);
        static_assert(
          maxPointsSize
            >= utility::typesizes
                 ::bitsToRepresent<unsigned int>(tournament::maxRounds),
          "Overflow");

        const auto topScoreThreshold =
          utility::uinttypes
              ::uint_least<maxPointsSize>{ tournament.playedRounds }
            * std::max(tournament.pointsForWin, tournament.pointsForDraw)
            >> 1;
        return
          !forbiddenPairs[player0.id].count(player1.id)
            && (!player0.absoluteColorPreference()
                  || !player1.absoluteColorPreference()
                  || player0.colorPreference != player1.colorPreference
                  || (tournament.playedRounds >= tournament.expectedRounds - 1u
                        && (player0.scoreWithAcceleration(tournament)
                                > topScoreThreshold
                              || player1.scoreWithAcceleration(tournament)
                                  > topScoreThreshold)
                      )
                );
      }

      /**
       * Check whether the matching is an eligible round pairing (where at least
       * all players but one are matched and no absolute criteria are violated).
       */
      bool matchingIsComplete(
        const std::vector<tournament::player_index> &matching,
        const tournament::Tournament &tournament,
        const std::vector<const tournament::Player *> &sortedPlayers)
      {
        bool encounteredUnmatchedPlayer{ };
        tournament::player_index vertexIndex{ };
        for (const tournament::player_index matchedIndex : matching)
        {
          if (matchedIndex == vertexIndex++)
          {
            if (
              encounteredUnmatchedPlayer
                || !eligibleForBye(*sortedPlayers[matchedIndex], tournament))
            {
              return false;
            }
            encounteredUnmatchedPlayer = true;
          }
        }
        return true;
      }

      /**
       * The different types of floaters.
       */
      enum Float : utility::uinttypes::uint_least_for_value<3>
      {
        FLOAT_DOWN, FLOAT_UP, FLOAT_NONE
      };

      /**
       * Determine the float direction of the specified player on the round that
       * is roundsBack before the current round.
       */
      Float getFloat(
        const tournament::Player &player,
        const tournament::round_index roundsBack,
        const tournament::Tournament &tournament)
      {
        const tournament::Match &match =
          player.matches[tournament.playedRounds - roundsBack];
        if (!match.gameWasPlayed)
        {
          return
            tournament.getPoints(player, match) > tournament.pointsForLoss
              ? FLOAT_DOWN
              : FLOAT_NONE;
        }
        const tournament::points playerScore =
          player.scoreWithAcceleration(tournament, roundsBack);
        const tournament::points opponentScore =
          tournament.players[match.opponent]
            .scoreWithAcceleration(tournament, roundsBack);
        return
          playerScore > opponentScore ? FLOAT_DOWN
            : playerScore < opponentScore ? FLOAT_UP
            : FLOAT_NONE;
      }

      /**
       * Left-shift the edgeWeight by the specified amount. If max is true,
       * also expand the number of pieces in edgeWeight so the shifted value
       * will fit.
       */
      template <bool max, typename Shift>
      void shiftEdgeWeight(
        matching_computer::edge_weight &edgeWeight,
        const Shift shift)
      {
        if (max)
        {
          edgeWeight.shiftGrow(shift);
        }
        else
        {
          edgeWeight <<= shift;
        }
      }

      /**
       * Given edgeWeight containing the high-order bits of the edge weight,
       * shift it over to make room for the bits reserved for color preferences,
       * and set the appropriate bits to true.
       * If max is true, just shift without setting bits to true.
       */
      template <bool max>
      void insertColorBits(
        matching_computer::edge_weight &edgeWeight,
        const tournament::Player &player,
        const tournament::Player &opponent,
        const bool inCurrentScoreGroup,
        const tournament::player_index playerCountBits)
      {
        const bool mask = !max && inCurrentScoreGroup;

        shiftEdgeWeight<max>(edgeWeight, playerCountBits);
        edgeWeight |=
          mask
            && (!player.absoluteColorImbalance()
                  || !opponent.absoluteColorImbalance()
                  || player.colorPreference != opponent.colorPreference);

        shiftEdgeWeight<max>(edgeWeight, playerCountBits);
        edgeWeight |=
          mask
            && (!player.absoluteColorPreference()
                  || !opponent.absoluteColorPreference()
                  || player.colorPreference != opponent.colorPreference
                  || (player.colorImbalance == opponent.colorImbalance
                        ? player.repeatedColor == tournament::COLOR_NONE
                            || player.repeatedColor != opponent.repeatedColor
                        : (player.colorImbalance > opponent.colorImbalance
                            ? opponent
                            : player
                          ).repeatedColor
                              != tournament::invert(player.colorPreference)));

        shiftEdgeWeight<max>(edgeWeight, playerCountBits);
        edgeWeight |=
          mask
            && colorPreferencesAreCompatible(
                player.colorPreference,
                opponent.colorPreference);

        shiftEdgeWeight<max>(edgeWeight, playerCountBits);
        edgeWeight |=
          mask
            && ((!player.strongColorPreference
                    && !player.absoluteColorPreference()
                  ) || (!opponent.strongColorPreference
                          && !opponent.absoluteColorPreference())
                    || (player.absoluteColorPreference()
                          && opponent.absoluteColorPreference())
                    || player.colorPreference != opponent.colorPreference
                );
      }

      bool isByeCandidate(
        const tournament::Player &player,
        const tournament::Tournament &tournament,
        const tournament::points byeAssigneeScore)
      {
        return
          eligibleForBye(player, tournament)
            && player.scoreWithAcceleration(tournament) <= byeAssigneeScore;
      }

      typedef tournament::player_index score_group_shift;

      /**
       * Compute the basic edge weight between the two players. If max is true,
       * compute an upper bound on the edge weight for this pairing bracket
       * instead.
       */
      template <bool max = false>
      matching_computer::edge_weight
        computeEdgeWeight(
          const tournament::Player &higherPlayer,
          const tournament::Player &lowerPlayer,
          const bool lowerPlayerInCurrentBracket,
          const bool lowerPlayerInNextBracket,
          const tournament::points byeAssigneeScore,
          const tournament::Tournament &tournament,
          const std::vector<std::unordered_set<tournament::player_index>>
            &forbiddenPairs,
          const unsigned int scoreGroupSizeBits,
          const score_group_shift scoreGroupsShift,
          const std::unordered_map<tournament::points, score_group_shift>
            &scoreGroupShifts,
          const bool isSingleDownloaterTheByeAssignee,
          const
            std::unordered_map<
              tournament::round_index,
              tournament::player_index>
            &unplayedGameRanks,
          matching_computer::edge_weight &maxEdgeWeight)
      {
        typename
            std::conditional<
              max,
              decltype(maxEdgeWeight),
              matching_computer::edge_weight
            >::type
          result{ maxEdgeWeight };

        result &= 0u;

        // Check compatibility.
        if (
          !max
            && !compatible(
                  higherPlayer,
                  lowerPlayer,
                  tournament,
                  forbiddenPairs))
        {
          return result;
        }

        // Enforce completion requirement and bye eligibility.
        result |=
          max
            ? 2u
            : 1u
                + !isByeCandidate(higherPlayer, tournament, byeAssigneeScore)
                + !isByeCandidate(lowerPlayer, tournament, byeAssigneeScore);

        // Maximize the number of pairs in the current pairing bracket.
        assert(scoreGroupSizeBits);
        shiftEdgeWeight<max>(result, scoreGroupSizeBits);
        result |= max ? 0u : lowerPlayerInCurrentBracket;

        // Maximize the scores paired in the current bracket.
        shiftEdgeWeight<max>(result, scoreGroupsShift);
        if (!max && lowerPlayerInCurrentBracket)
        {
          result |=
            ((result & 0u) | 1u)
              << scoreGroupShifts.find(
                    higherPlayer.scoreWithAcceleration(tournament)
                  )->second;
        }

        // Maximize the number of pairs in the next bracket.
        shiftEdgeWeight<max>(result, scoreGroupSizeBits);
        result |= max ? 0u : lowerPlayerInNextBracket;

        // Maximize the scores paired in the next bracket.
        shiftEdgeWeight<max>(result, scoreGroupsShift);
        if (!max && lowerPlayerInNextBracket)
        {
          result |=
            ((result & 0u) | 1u)
              << scoreGroupShifts.find(
                    higherPlayer.scoreWithAcceleration(tournament)
                  )->second;
        }

        // Minimize number of unplayed games of bye assignee
        shiftEdgeWeight<max>(result, scoreGroupSizeBits);
        shiftEdgeWeight<max>(result, scoreGroupSizeBits);
        if (!max && isSingleDownloaterTheByeAssignee)
        {
          if (
            higherPlayer.scoreWithAcceleration(tournament) == byeAssigneeScore)
          {
            result |= unplayedGameRanks.find(higherPlayer.playedGames)->second;
          }
          if (lowerPlayer.scoreWithAcceleration(tournament) == byeAssigneeScore)
          {
            result += unplayedGameRanks.find(lowerPlayer.playedGames)->second;
          }
        }

        // Maximize color preference satisfaction.
        insertColorBits<max>(
          result,
          lowerPlayer,
          higherPlayer,
          lowerPlayerInCurrentBracket,
          scoreGroupSizeBits);

        if (tournament.playedRounds)
        {
          // Minimize downfloaters repeated from the previous round.
          shiftEdgeWeight<max>(result, scoreGroupSizeBits);
          if (!max && lowerPlayerInCurrentBracket)
          {
            result |= getFloat(lowerPlayer, 1, tournament) == FLOAT_DOWN;
            result +=
              higherPlayer.scoreWithAcceleration(tournament)
                  <= lowerPlayer.scoreWithAcceleration(tournament)
                && getFloat(higherPlayer, 1, tournament) == FLOAT_DOWN;
          }

          // Minimize upfloaters repeated from the previous round.
          shiftEdgeWeight<max>(result, scoreGroupSizeBits);
          if (!max && lowerPlayerInCurrentBracket)
          {
            result |=
              !(higherPlayer.scoreWithAcceleration(tournament)
                    > lowerPlayer.scoreWithAcceleration(tournament)
                  && getFloat(lowerPlayer, 1, tournament) == FLOAT_UP);
          }
        }
        if (tournament.playedRounds > 1u)
        {
          // Minimize downfloaters repeated from two rounds before.
          shiftEdgeWeight<max>(result, scoreGroupSizeBits);
          if (!max && lowerPlayerInCurrentBracket)
          {
            result |= getFloat(lowerPlayer, 2, tournament) == FLOAT_DOWN;
            result +=
              higherPlayer.scoreWithAcceleration(tournament)
                  <= lowerPlayer.scoreWithAcceleration(tournament)
                && getFloat(higherPlayer, 2, tournament) == FLOAT_DOWN;
          }

          // Minimize upfloaters repeated from two rounds before.
          shiftEdgeWeight<max>(result, scoreGroupSizeBits);
          if (!max && lowerPlayerInCurrentBracket)
          {
            result |=
              !(higherPlayer.scoreWithAcceleration(tournament)
                    > lowerPlayer.scoreWithAcceleration(tournament)
                  && getFloat(lowerPlayer, 2, tournament) == FLOAT_UP);
          }
        }

        if (tournament.playedRounds)
        {
          // Minimize the scores of downfloaters repeated from the previous
          // round.
          shiftEdgeWeight<max>(result, scoreGroupsShift);
          if (!max && lowerPlayerInCurrentBracket)
          {
            result +=
              ((result & 0u)
                | (getFloat(lowerPlayer, 1, tournament) == FLOAT_DOWN)
              ) << scoreGroupShifts.find(
                      lowerPlayer.scoreWithAcceleration(tournament)
                    )->second;
            result +=
              ((result & 0u)
                | (getFloat(higherPlayer, 1, tournament) == FLOAT_DOWN)
              ) << scoreGroupShifts.find(
                      higherPlayer.scoreWithAcceleration(tournament)
                    )->second;
          }

          // Minimize the scores of the opponents of upfloaters repeated from
          // the previous round.
          shiftEdgeWeight<max>(result, scoreGroupsShift);
          if (
            !max
              && lowerPlayerInCurrentBracket
              && !(getFloat(lowerPlayer, 1, tournament) == FLOAT_UP
                    && higherPlayer.scoreWithAcceleration(tournament)
                        > lowerPlayer.scoreWithAcceleration(tournament)))
          {
            result |=
              ((result & 0u) | 1u)
                << scoreGroupShifts.find(
                      higherPlayer.scoreWithAcceleration(tournament)
                    )->second;
          }
        }
        if (tournament.playedRounds > 1u)
        {
          // Minimize the scores of downfloaters repeated from two rounds
          // before.
          shiftEdgeWeight<max>(result, scoreGroupsShift);
          if (!max && lowerPlayerInCurrentBracket)
          {
            result +=
              ((result & 0u)
                | (getFloat(lowerPlayer, 2, tournament) == FLOAT_DOWN)
              ) << scoreGroupShifts.find(
                      lowerPlayer.scoreWithAcceleration(tournament)
                    )->second;
            result +=
              ((result & 0u)
                | (getFloat(higherPlayer, 2, tournament) == FLOAT_DOWN)
              ) << scoreGroupShifts.find(
                      higherPlayer.scoreWithAcceleration(tournament)
                    )->second;
          }

          // Minimize the scores of opponents of upfloaters repeated from two
          // rounds before.
          shiftEdgeWeight<max>(result, scoreGroupsShift);
          if (
            !max
              && lowerPlayerInCurrentBracket
              && !(getFloat(lowerPlayer, 2, tournament) == FLOAT_UP
                    && higherPlayer.scoreWithAcceleration(tournament)
                        > lowerPlayer.scoreWithAcceleration(tournament)))
          {
            result |=
              ((result & 0u) | 1u)
                << scoreGroupShifts.find(
                      higherPlayer.scoreWithAcceleration(tournament)
                    )->second;
          }
        }

        // Leave room for enforcing the ordering requirements for pairing
        // heterogeneous and homogeneous brackets.
        shiftEdgeWeight<max>(result, scoreGroupSizeBits);

        shiftEdgeWeight<max>(result, scoreGroupSizeBits);
        shiftEdgeWeight<max>(result, scoreGroupSizeBits);

        shiftEdgeWeight<max>(result, 1u);

        if (max)
        {
          // The edge weight should have room to expand by two bits for the
          // matching subroutine.
          // Subtracting 1 sets all bits to 1.
          result.shiftGrow(2u);
          result >>= 1u;
          result -= 1u;
        }

        return result;
      }

      /**
       * A function to compute the piece color given to the first argument,
       * whose opponent is the second argument.
       */
      tournament::Color choosePlayerColor(
        const tournament::Player &player,
        const tournament::Player &opponent,
        const tournament::Tournament &tournament)
      {
        const tournament::Color result =
          choosePlayerNeutralColor(player, opponent);
        return
          result == tournament::COLOR_NONE
            ? player.colorPreference == tournament::COLOR_NONE
                ? tournament::acceleratedScoreRankCompare(
                      &player,
                      &opponent,
                      tournament)
                    ? opponent.rankIndex & 1u
                        ? tournament.initialColor
                        : invert(tournament.initialColor)
                    : player.rankIndex & 1u
                        ? invert(tournament.initialColor)
                        : tournament.initialColor
                : tournament::acceleratedScoreRankCompare(
                      &player,
                      &opponent,
                      tournament)
                    ? invert(opponent.colorPreference)
                    : player.colorPreference
            : result;

      }

      /**
       * Return a character used in the checklist file to represent the
       * specified float direction.
       */
      char floatToChar(const Float floatDirection)
      {
        return
          floatDirection == FLOAT_DOWN ? 'D'
            : floatDirection == FLOAT_UP ? 'U'
            : ' ';
      }

      /**
       * Print the checklist file for the current round to ostream.
       */
      void printChecklist(
        const tournament::Tournament &tournament,
        const std::vector<const tournament::Player *> &sortedPlayers,
        std::ostream &ostream,
        const std::vector<const tournament::Player *> *const matching = nullptr)
      {
        swisssystems::printChecklist(
          ostream,
          std::deque<std::string>{"C2", "C14", "C16", "Cur"},
          [&matching, &tournament]
            (const tournament::Player &player)
          {
            const tournament::Player *opponent =
              matching ? (*matching)[player.id] : nullptr;
            return std::deque<std::string>{
              eligibleForBye(player, tournament) ? "Y" : "N",
              std::string{
                floatToChar(
                  tournament.playedRounds
                    ? getFloat(player, 1, tournament)
                    : FLOAT_NONE)
              },
              std::string{
                floatToChar(
                  tournament.playedRounds > 1u
                    ? getFloat(player, 2, tournament)
                    : FLOAT_NONE)
              },
              matching
                ? opponent
                    ? '('
                        + utility::uintstringconversion
                            ::toString(opponent->id + 1u)
                        + (choosePlayerColor(player, *opponent, tournament)
                            == tournament::COLOR_WHITE ? 'W' : 'B'
                          )
                        + ')'
                    : "(bye)"
                : ""
            };
          },
          tournament,
          sortedPlayers
        );
      }

      /**
       * Compute the basic edge weights for all possible pairings in this
       * pairing bracket and the next. The resulting vector is indexed by the
       * larger player index, and the sub-vectors are indexed by the smaller
       * player index.
       */
      std::vector<std::vector<matching_computer::edge_weight>>
      computeBaseEdgeWeights(
        matching_computer::edge_weight &maxEdgeWeight,
        const std::vector<const tournament::Player *> &playersByIndex,
        const tournament::player_index scoreGroupBegin,
        const tournament::player_index nextScoreGroupBegin,
        const tournament::points byeAssigneeScore,
        const tournament::Tournament &tournament,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs,
        const unsigned int scoreGroupSizeBits,
        const score_group_shift scoreGroupsShift,
        const std::unordered_map<tournament::points, score_group_shift>
          &scoreGroupShifts,
        const bool isSingleDownfloaterTheByeAssignee,
        const
          std::unordered_map<tournament::round_index, tournament::player_index>
          &unplayedGameRanks)
      {
        std::vector<std::vector<matching_computer::edge_weight>>
          result(playersByIndex.size());

        for (
          tournament::player_index largerPlayerIndex = scoreGroupBegin;
          largerPlayerIndex < playersByIndex.size();
          ++largerPlayerIndex)
        {
          for (
            tournament::player_index smallerPlayerIndex = 0;
            smallerPlayerIndex < largerPlayerIndex;
            ++smallerPlayerIndex)
          {
            result[largerPlayerIndex].emplace_back(
              computeEdgeWeight(
                *playersByIndex[smallerPlayerIndex],
                *playersByIndex[largerPlayerIndex],
                largerPlayerIndex < nextScoreGroupBegin,
                largerPlayerIndex >= nextScoreGroupBegin,
                byeAssigneeScore,
                tournament,
                forbiddenPairs,
                scoreGroupSizeBits,
                scoreGroupsShift,
                scoreGroupShifts,
                isSingleDownfloaterTheByeAssignee,
                unplayedGameRanks,
                maxEdgeWeight));
          }
        }

        return result;
      }
    }

    /**
     * Compute the matching, and return the list of Pairings in arbitrary order.
     * If no matching is possible, throw a NoValidPairingException.
     * This runs in theoretic time O(n^3 * s^2 * log n), where n is the
     * number of players and s is the number of occupied score groups.
     */
    std::list<Pairing> computeMatching(
      tournament::Tournament &&tournament,
      std::ostream *const ostream)
    {
      // Filter out the absent players, and sort the remainder by score and
      // pairing ID.
      std::vector<const tournament::Player *> sortedPlayers;
      // We add forbidden pairs due to previous matches below
      auto forbiddenPairs = tournament.resolveForbiddenPairs(tournament.playedRounds);
      for (tournament::Player &player : tournament.players)
      {
        if (player.isValid)
        {
          if (player.matches.size() <= tournament.playedRounds)
          {
            sortedPlayers.push_back(&player);
          }
          for (const tournament::Match &match : player.matches)
          {
            if (match.gameWasPlayed)
            {
              forbiddenPairs[player.id].insert(match.opponent);
            }
          }
        }
      }
      std::sort(
        sortedPlayers.begin(),
        sortedPlayers.end(),
        [&tournament](
          const tournament::Player *const player0,
          const tournament::Player *const player1)
        {
          return
            tournament
              ::acceleratedScoreRankCompare(player1, player0, tournament);
        }
      );

      // Calculate the number of bits needed to prioritize moved-down players.
      score_group_shift scoreGroupsShift{ };
      std::unordered_map<tournament::points, score_group_shift>
        scoreGroupShifts{ };
      tournament::player_index maxScoreGroupSize{ };
      tournament::player_index repeatedScores{ };
      for (
        auto nextIterator = sortedPlayers.rbegin();
        nextIterator != sortedPlayers.rend();
      )
      {
        const auto currentIterator = nextIterator++;
        ++repeatedScores;
        const tournament::points currentScore =
          (*currentIterator)->scoreWithAcceleration(tournament);
        if (
          nextIterator == sortedPlayers.rend()
            || currentScore < (*nextIterator)->scoreWithAcceleration(tournament)
        )
        {
          const unsigned int newBits =
            utility::typesizes::bitsToRepresent<unsigned int>(repeatedScores);
          scoreGroupShifts[currentScore] = scoreGroupsShift;
          maxScoreGroupSize = std::max(maxScoreGroupSize, repeatedScores);
          repeatedScores = 0;
          scoreGroupsShift += newBits;
          assert(scoreGroupsShift >= newBits);
        }
      }

      const unsigned int scoreGroupSizeBits =
        utility::typesizes::bitsToRepresent<unsigned int>(maxScoreGroupSize);

      std::unordered_map<tournament::round_index, tournament::player_index>
        unplayedGameRanks{ };

      // Compute an edge weight upper bound.
      matching_computer::edge_weight maxEdgeWeight{ 0u };
      computeEdgeWeight<true>(
        *sortedPlayers.front(),
        *sortedPlayers.front(),
        true,
        false,
        0u,
        tournament,
        forbiddenPairs,
        scoreGroupSizeBits,
        scoreGroupsShift,
        scoreGroupShifts,
        false,
        unplayedGameRanks,
        maxEdgeWeight);

      // Initialize the matching computer used to optimize the pairings
      matching_computer matchingComputer(sortedPlayers.size(), maxEdgeWeight);

      // Set edge weights to enforce completability.
      if (sortedPlayers.size() > ~matching_computer::size_type{ })
      {
        throw std::length_error("");
      }
      for (
        tournament::player_index playerIndex{ };
        playerIndex < sortedPlayers.size();
        ++playerIndex)
      {
        matchingComputer.addVertex();
      }

      {
        tournament::player_index playerIndex{ };
        for (const tournament::Player *const player : sortedPlayers)
        {
          const tournament::points playerScore =
            player->scoreWithAcceleration(tournament);
          tournament::player_index opponentIndex{ };
          for (const tournament::Player *const opponent : sortedPlayers)
          {
            if (opponentIndex == playerIndex)
            {
              break;
            }
            if (sortedPlayers.size() & 1u)
            {
              matching_computer::edge_weight edgeWeight{ maxEdgeWeight };
              edgeWeight &= 0u;
              if (compatible(*player, *opponent, tournament, forbiddenPairs))
              {
                edgeWeight |=
                  1u
                    + !eligibleForBye(*player, tournament)
                    + !eligibleForBye(*opponent, tournament);
                edgeWeight <<= scoreGroupsShift;
                edgeWeight |=
                  scoreGroupShifts.find(playerScore)->second
                    + scoreGroupShifts.find(
                          opponent->scoreWithAcceleration(tournament)
                        )->second;
                edgeWeight <<= scoreGroupSizeBits;
                edgeWeight |=
                  player->scoreWithAcceleration(tournament)
                    >= sortedPlayers.front()->scoreWithAcceleration(tournament);
              }
              matchingComputer.setEdgeWeight(
                playerIndex,
                opponentIndex,
                std::move(edgeWeight));
            }
            else
            {
              matchingComputer
                .setEdgeWeight(
                  playerIndex,
                  opponentIndex,
                  computeEdgeWeight(
                    *opponent,
                    *player,
                    false,
                    false,
                    0u,
                    tournament,
                    forbiddenPairs,
                    scoreGroupSizeBits,
                    scoreGroupsShift,
                    scoreGroupShifts,
                    false,
                    unplayedGameRanks,
                    maxEdgeWeight));
            }
            ++opponentIndex;
          }
          ++playerIndex;
        }
      }

      // Check whether a pairing is possible initially, determine score of bye
      // assignee, and check whether C9 (minimise unplayed games of bye
      // assignee) takes effect in the first bracket.
      tournament::points byeAssigneeScore{ };
      bool isSingleDownfloaterTheByeAssignee;
      {
        matchingComputer.computeMatching();
        const std::vector<tournament::player_index> matching =
          matchingComputer.getMatching();
        if (!matchingIsComplete(matching, tournament, sortedPlayers))
        {
          if (ostream)
          {
            printChecklist(tournament, sortedPlayers, *ostream);
          }
          throw NoValidPairingException(
            "The players could not be simultaneously matched while satisfying "
            "all absolute criteria.");
        }

        if (sortedPlayers.size() & 1u)
        {
          tournament::player_index playerIndex{ };
          for (const tournament::Player *const player : sortedPlayers)
          {
            if (matching[playerIndex] == playerIndex)
            {
              byeAssigneeScore = player->scoreWithAcceleration(tournament);
              break;
            }
            ++playerIndex;
          }

          auto topScore = sortedPlayers.front()->scoreWithAcceleration(tournament);
          if (byeAssigneeScore >= topScore)
          {
            isSingleDownfloaterTheByeAssignee = true;
            playerIndex = 0u;
            for (const tournament::Player *const player : sortedPlayers)
            {
              if (player->scoreWithAcceleration(tournament) < topScore)
              {
                break;
              }
              if (
                sortedPlayers[matching[playerIndex]]
                    ->scoreWithAcceleration(tournament)
                  < topScore)
              {
                isSingleDownfloaterTheByeAssignee = false;
                break;
              }
              ++playerIndex;
            }
          }
          else
          {
            isSingleDownfloaterTheByeAssignee = false;
          }

          std::vector<tournament::round_index> playedGameCounts{ };
          for (const tournament::Player *const player : sortedPlayers)
          {
            if (player->scoreWithAcceleration(tournament) == byeAssigneeScore)
            {
              playedGameCounts.emplace_back(player->playedGames);
            }
          }
          std::sort(playedGameCounts.rbegin(), playedGameCounts.rend());
          tournament::player_index rank{ };
          for (const tournament::round_index playedGames : playedGameCounts)
          {
            unplayedGameRanks[playedGames] = rank++;
          }

          playerIndex = 0u;
          for (const tournament::Player *const player : sortedPlayers)
          {
            tournament::player_index opponentIndex{ };
            for (const tournament::Player *const opponent : sortedPlayers)
            {
              if (opponentIndex == playerIndex)
              {
                break;
              }
              matchingComputer.setEdgeWeight(
                playerIndex,
                opponentIndex,
                computeEdgeWeight(
                  *opponent,
                  *player,
                  false,
                  false,
                  byeAssigneeScore,
                  tournament,
                  forbiddenPairs,
                  scoreGroupSizeBits,
                  scoreGroupsShift,
                  scoreGroupShifts,
                  isSingleDownfloaterTheByeAssignee,
                  unplayedGameRanks,
                  maxEdgeWeight));
              ++opponentIndex;
            }
            ++playerIndex;
          }
        }
        else
        {
          isSingleDownfloaterTheByeAssignee = false;
        }
      }

      /**
       * A vector indicating the match for each player, indexed by player ID.
       * Unmatched players are indicated with null pointers.
       */
      std::vector<const tournament::Player *>
        matchingById(tournament.players.size());

      /**
       * Given the index of a player among those in the current pairing bracket
       * or the next, stores the pointer to the Player.
       */
      std::vector<const tournament::Player *> playersByIndex;
      /**
       * Given the index of a player among those in the current pairing bracket
       * or the next, stores the index of the player in matchingComputer, that
       * is, the index in sortedPlayers.
       */
      std::vector<tournament::player_index> vertexIndices;
      /**
       * An iterator pointing to the beginning of the next score group.
       */
      decltype(sortedPlayers)::const_iterator nextScoreGroupIterator =
        sortedPlayers.begin();
      while (
        nextScoreGroupIterator != sortedPlayers.end()
          && (*nextScoreGroupIterator)->scoreWithAcceleration(tournament)
              >= sortedPlayers.front()->scoreWithAcceleration(tournament))
      {
        playersByIndex.push_back(*nextScoreGroupIterator);
        vertexIndices.push_back(vertexIndices.size());
        ++nextScoreGroupIterator;
      }

      /**
       * Stores whether the player will be matched. The vector is indexed by
       * index in sortedPlayers.
       */
      std::vector<bool> matched(sortedPlayers.size());

      /**
       * The number of moved down players in the current pairing bracket.
       */
      tournament::player_index scoreGroupBegin{ };
      /**
       * The index of the first player in sortedPlayers from the current
       * bracket's score group.
       */
      tournament::player_index scoreGroupBeginVertex{ };

      while (
        playersByIndex.size() > 1u
          || nextScoreGroupIterator != sortedPlayers.end())
      {
        /**
         * The number of players in the current pairing bracket.
         */
        const tournament::player_index nextScoreGroupBegin =
          playersByIndex.size();
        /**
         * The index of the first player in sortedPlayers from the next score
         * group.
         */
        const tournament::player_index nextScoreGroupBeginVertex =
          scoreGroupBeginVertex + (nextScoreGroupBegin - scoreGroupBegin);
        /**
         * Save the iterator to the beginning of the next score group.
         */
        const decltype(sortedPlayers)::const_iterator scoreGroupIterator =
          nextScoreGroupIterator;
        while (
          nextScoreGroupIterator != sortedPlayers.end()
            && ((*nextScoreGroupIterator)->scoreWithAcceleration(tournament)
                  >= (*scoreGroupIterator)->scoreWithAcceleration(tournament)))
        {
          playersByIndex.push_back(*nextScoreGroupIterator);
          vertexIndices.push_back(vertexIndices.back() + 1u);
          ++nextScoreGroupIterator;
        }

        std::vector<std::vector<matching_computer::edge_weight>>
            baseEdgeWeights =
          computeBaseEdgeWeights(
            maxEdgeWeight,
            playersByIndex,
            scoreGroupBegin,
            nextScoreGroupBegin,
            byeAssigneeScore,
            tournament,
            forbiddenPairs,
            scoreGroupSizeBits,
            scoreGroupsShift,
            scoreGroupShifts,
            isSingleDownfloaterTheByeAssignee,
            unplayedGameRanks);

        // Update the matching computer for optimizing the pairing in the
        // current pairing bracket
        {
          auto opponentIterator = vertexIndices.begin();
          for (
            const std::vector<matching_computer::edge_weight> &opponentVector
              : baseEdgeWeights)
          {
            const tournament::player_index opponentVertex = *opponentIterator;
            auto playerIterator = vertexIndices.begin();
            for (
              const matching_computer::edge_weight &edgeWeight : opponentVector)
            {
              matchingComputer.setEdgeWeight(
                opponentVertex,
                *playerIterator,
                edgeWeight);
              ++playerIterator;
            }
            ++opponentIterator;
          }
        }

        /**
         * A function used to calculate an edge weight modified for pairing
         * homogeneous brackets or remainders with some of the exchange
         * preferences.
         */
        const auto edgeWeightComputer =
          [&baseEdgeWeights, scoreGroupSizeBits](
            const tournament::player_index smallerPlayerIndex,
            const tournament::player_index largerPlayerIndex,
            const tournament::player_index smallerPlayerRemainderIndex,
            const tournament::player_index remainderPairs)
          {
            matching_computer::edge_weight result =
              baseEdgeWeights[largerPlayerIndex][smallerPlayerIndex];

            if (result)
            {
              matching_computer::edge_weight addend = result & 0u;

              // Minimize the number of exchanges.
              addend |= smallerPlayerRemainderIndex < remainderPairs;

              // Minimize the difference of the exchanged branch scoring
              // numbers.
              addend <<= scoreGroupSizeBits;
              addend <<= scoreGroupSizeBits;
              addend -= smallerPlayerRemainderIndex;

              // Leave room for optimizing based on which players are exchanged.
              addend <<= 1u;

              result += addend;
            }

            return result;
          };

        matchingComputer.computeMatching();

        auto stableMatching = matchingComputer.getMatching();

        // Choose the moved down players to pair in the current pairing bracket.

        /**
         * The score of the moved down players we are currently considering.
         */
        tournament::points movedDownScoreGroup;
        /**
         * The number of moved down players with score movedDownScoreGroup that
         * we have not considered yet.
         */
        tournament::player_index remainingMovedDownScoreGroupPlayers;
        /**
         * The number of moved down players with score movedDownScoreGroup that
         * we will be able to match among those we have not considered yet.
         */
        tournament::player_index remainingMatchedMovedDownScoreGroupPlayers;
        for (
          tournament::player_index playerIndex = 0;
          playerIndex < scoreGroupBegin;
          ++playerIndex)
        {
          if (
            !playerIndex
              || playersByIndex[playerIndex]->scoreWithAcceleration(tournament)
                  < movedDownScoreGroup)
          {
            // Count the number of moved down players with the same score as
            // playerIndex, as well as the number of these that can be matched.
            movedDownScoreGroup =
              playersByIndex[playerIndex]->scoreWithAcceleration(tournament);
            remainingMatchedMovedDownScoreGroupPlayers = 0;
            remainingMovedDownScoreGroupPlayers = 0;
            for (
              tournament::player_index movedDownPlayerIndex = playerIndex;
              playersByIndex[movedDownPlayerIndex]
                  ->scoreWithAcceleration(tournament)
                >= movedDownScoreGroup;
              ++movedDownPlayerIndex)
            {
              ++remainingMovedDownScoreGroupPlayers;
              const tournament::player_index movedDownPlayerVertex =
                vertexIndices[movedDownPlayerIndex];
              if (
                stableMatching[movedDownPlayerVertex] >= scoreGroupBeginVertex
                  && stableMatching[movedDownPlayerVertex] < nextScoreGroupBeginVertex)
              {
                ++remainingMatchedMovedDownScoreGroupPlayers;
              }
            }
          }
          if (!remainingMatchedMovedDownScoreGroupPlayers)
          {
            continue;
          }
          const tournament::player_index playerVertex = vertexIndices[playerIndex];
          if (
            remainingMovedDownScoreGroupPlayers
              <= remainingMatchedMovedDownScoreGroupPlayers)
          {
            matched[playerVertex] = true;
            continue;
          }
          --remainingMovedDownScoreGroupPlayers;
          if (
            stableMatching[playerVertex] < scoreGroupBeginVertex
              || stableMatching[playerVertex] >= nextScoreGroupBeginVertex)
          {
            // Try to match the player.
            for (
              tournament::player_index opponentIndex = scoreGroupBegin;
              opponentIndex < nextScoreGroupBegin;
              ++opponentIndex)
            {
              matching_computer::edge_weight edgeWeight =
                baseEdgeWeights[opponentIndex][playerIndex];
              if (edgeWeight)
              {
                edgeWeight |= 1u;
                matchingComputer.setEdgeWeight(
                  playerVertex,
                  vertexIndices[opponentIndex],
                  std::move(edgeWeight));
              }
            }

            matchingComputer.computeMatching();

            stableMatching = matchingComputer.getMatching();
          }
          if (
            stableMatching[playerVertex] >= scoreGroupBeginVertex
              && stableMatching[playerVertex] < nextScoreGroupBeginVertex)
          {
            // Finalize the fact that this player will be matched.
            matched[playerVertex] = true;
            --remainingMatchedMovedDownScoreGroupPlayers;
            for (
              tournament::player_index opponentIndex = scoreGroupBegin;
              opponentIndex < nextScoreGroupBegin;
              ++opponentIndex)
            {
              matching_computer::edge_weight edgeWeight =
                baseEdgeWeights[opponentIndex][playerIndex];
              if (edgeWeight)
              {
                edgeWeight |= nextScoreGroupBegin - scoreGroupBegin;
                ++edgeWeight;
                matchingComputer.setEdgeWeight(
                  playerVertex,
                  vertexIndices[opponentIndex],
                  std::move(edgeWeight));
              }
            }
          }
        }

        // Choose the opponents of the moved down players.
        for (
          tournament::player_index playerIndex = 0;
          playerIndex < scoreGroupBegin;
          ++playerIndex)
        {
          const tournament::player_index playerVertex = vertexIndices[playerIndex];
          if (!matched[playerVertex])
          {
            continue;
          }
          matching_computer::edge_weight addend =
            (maxEdgeWeight & 0u) | playersByIndex.size();
          for (
            tournament::player_index opponentIndex = nextScoreGroupBegin - 1u;
            opponentIndex >= scoreGroupBegin;
            --opponentIndex)
          {
            const tournament::player_index opponentVertex = vertexIndices[opponentIndex];
            if (matched[opponentVertex])
            {
              continue;
            }
            matching_computer::edge_weight edgeWeight =
              baseEdgeWeights[opponentIndex][playerIndex];
            if (edgeWeight)
            {
              edgeWeight += addend;
              matchingComputer.setEdgeWeight(
                playerVertex,
                opponentVertex,
                std::move(edgeWeight));
              ++addend;
            }
          }

          matchingComputer.computeMatching();
          stableMatching = matchingComputer.getMatching();

          // Finalize the pairing.
          const tournament::player_index matchVertex =
            stableMatching[playerVertex];
          matched[matchVertex] = true;
          finalizePair(
            playerVertex,
            matchVertex,
            matchingComputer,
            maxEdgeWeight);
        }

        /**
         * Collects the player indexes of the players in the remainder.
         */
        std::deque<tournament::player_index> remainder;

        /**
         * The number of pairs that can be formed in the remainder.
         */
        tournament::player_index remainderPairs{ };

        /**
         * Initialize remainder and remainderPairs.
         */
        for (
          tournament::player_index playerIndex = scoreGroupBegin;
          playerIndex < nextScoreGroupBegin;
          ++playerIndex)
        {
          const tournament::player_index playerVertex = vertexIndices[playerIndex];
          if (stableMatching[playerVertex] < scoreGroupBeginVertex)
          {
            continue;
          }
          remainder.push_back(playerIndex);
          if (stableMatching[playerVertex] < playerVertex)
          {
            ++remainderPairs;
          }
        }
        /**
         * An iterator to the first element in remainder that is in the lower
         * group of players.
         */
        decltype(remainder)::const_iterator firstGroupEnd =
          std::next(remainder.begin(), remainderPairs);

        // Update edge weights to minimize exchanged players and the differences
        // of exchanged BSNs.
        for (const tournament::player_index opponentIndex : remainder)
        {
          const tournament::player_index opponentVertex = vertexIndices[opponentIndex];
          tournament::player_index playerRemainderIndex{ };
          for (const tournament::player_index playerIndex : remainder)
          {
            if (opponentIndex <= playerIndex)
            {
              break;
            }
            matchingComputer.setEdgeWeight(
              vertexIndices[playerIndex],
              opponentVertex,
              edgeWeightComputer(
                playerIndex,
                opponentIndex,
                playerRemainderIndex,
                remainderPairs));
            ++playerRemainderIndex;
          }
        }

        matchingComputer.computeMatching();
        stableMatching = matchingComputer.getMatching();

        /**
         * The number of exchanges that must be made.
         */
        tournament::player_index exchangeCount{ };
        for (const tournament::player_index playerIndex : remainder)
        {
          if (playerIndex >= *firstGroupEnd)
          {
            break;
          }
          const tournament::player_index playerVertex = vertexIndices[playerIndex];
          exchangeCount +=
            stableMatching[playerVertex] <= playerVertex
              || stableMatching[playerVertex] >= nextScoreGroupBeginVertex;
        }

        // Select lower players from the higher group to be exchanged where
        // possible.

        tournament::player_index exchangesRemaining = exchangeCount;
        tournament::player_index playerRemainderIndex = remainderPairs;
        for (
          decltype(remainder)::const_iterator playerIterator = firstGroupEnd;
          playerIterator != remainder.begin() && exchangesRemaining;
        )
        {
          // Update edge weights to determine whether the current player can
          // be exchanged.
          decltype(remainder)::const_iterator opponentIterator = playerIterator;
          --playerRemainderIndex;
          --playerIterator;
          const tournament::player_index playerVertex = vertexIndices[*playerIterator];
          if (
            stableMatching[playerVertex] > playerVertex
              && stableMatching[playerVertex] < nextScoreGroupBeginVertex)
          {
            while (opponentIterator != remainder.end())
            {
              matching_computer::edge_weight edgeWeight =
                edgeWeightComputer(
                  *playerIterator,
                  *opponentIterator,
                  playerRemainderIndex,
                  remainderPairs);
              if (edgeWeight)
              {
                edgeWeight -= 1u;
                matchingComputer.setEdgeWeight(
                  playerVertex,
                  vertexIndices[*opponentIterator],
                  std::move(edgeWeight));
              }
              ++opponentIterator;
            }

            matchingComputer.computeMatching();

            stableMatching = matchingComputer.getMatching();
          }

          const bool exchange =
            stableMatching[playerVertex] <= playerVertex
              || stableMatching[playerVertex] >= nextScoreGroupBeginVertex;

          exchangesRemaining -= exchange;

          opponentIterator = std::next(playerIterator, 1);
          while (opponentIterator != remainder.end())
          {
            // Finalize that this player must be exchanged, or restore the
            // original edge weights.
            if (exchange)
            {
              baseEdgeWeights[*opponentIterator][*playerIterator] &= 0u;
            }
            matchingComputer.setEdgeWeight(
              playerVertex,
              vertexIndices[*opponentIterator],
              edgeWeightComputer(
                *playerIterator,
                *opponentIterator,
                playerRemainderIndex,
                remainderPairs));
            ++opponentIterator;
          }
        }

        // Select higher players from the lower group to be exchanged where
        // possible.

        exchangesRemaining = exchangeCount;
        tournament::player_index remainderIndex = remainderPairs;
        for (
          decltype(remainder)::const_iterator playerIterator = firstGroupEnd;
          playerIterator != remainder.end() && exchangesRemaining > 1u;
          ++playerIterator)
        {
          const tournament::player_index playerVertex = vertexIndices[*playerIterator];
          const bool alreadyExchanged =
            stableMatching[playerVertex] > playerVertex
              && stableMatching[playerVertex] < nextScoreGroupBeginVertex;
          if (!alreadyExchanged)
          {
            // Update edge weights to determine whether the current player
            // can be exchanged.
            for (
              decltype(remainder)::const_iterator opponentIterator =
                std::next(playerIterator, 1);
              opponentIterator != remainder.end();
              ++opponentIterator)
            {
              matching_computer::edge_weight edgeWeight =
                edgeWeightComputer(
                  *playerIterator,
                  *opponentIterator,
                  remainderIndex,
                  remainderPairs);
              if (edgeWeight)
              {
                edgeWeight += 1u;
                matchingComputer.setEdgeWeight(
                  playerVertex,
                  vertexIndices[*opponentIterator],
                  std::move(edgeWeight));
              }
            }

            matchingComputer.computeMatching();

            stableMatching = matchingComputer.getMatching();
          }

          const bool exchange =
            stableMatching[playerVertex] > playerVertex
              && stableMatching[playerVertex] < nextScoreGroupBeginVertex;

          if (exchange)
          {
            --exchangesRemaining;

            // Finalize that this player must be exchanged.

            for (
              decltype(remainder)::const_iterator opponentIterator =
                remainder.begin();
              opponentIterator != playerIterator;
              ++opponentIterator)
            {
              baseEdgeWeights[*playerIterator][*opponentIterator] &= 0u;
              matchingComputer.setEdgeWeight(
                playerVertex,
                vertexIndices[*opponentIterator],
                baseEdgeWeights[*playerIterator][*opponentIterator]);
            }

            for (
              tournament::player_index opponentIndex = nextScoreGroupBegin;
              opponentIndex < playersByIndex.size();
              ++opponentIndex)
            {
              baseEdgeWeights[opponentIndex][*playerIterator] &= 0u;
              matchingComputer.setEdgeWeight(
                playerVertex,
                vertexIndices[opponentIndex],
                baseEdgeWeights[opponentIndex][*playerIterator]);
            }
          }
          if (!alreadyExchanged)
          {
            // Restore the original edge weights.
            for (
              decltype(remainder)::const_iterator opponentIterator =
                std::next(playerIterator, 1);
              opponentIterator != remainder.end();
              ++opponentIterator)
            {
              matchingComputer.setEdgeWeight(
                playerVertex,
                vertexIndices[*opponentIterator],
                edgeWeightComputer(
                  *playerIterator,
                  *opponentIterator,
                  remainderIndex,
                  remainderPairs));
            }
          }
          ++remainderIndex;
        }

        // Finalize which players will be exchanged, and reset the bits we used
        // for determining that.
        remainderIndex = 0;
        for (
          decltype(remainder)::const_iterator playerIterator =
            remainder.begin();
          playerIterator != remainder.end();
          ++playerIterator)
        {
          const tournament::player_index playerVertex = vertexIndices[*playerIterator];
          for (
            decltype(remainder)::const_iterator opponentIterator =
              std::next(playerIterator, 1);
            opponentIterator != remainder.end();
            ++opponentIterator)
          {
            const tournament::player_index opponentVertex =
              vertexIndices[*opponentIterator];
            if (
              stableMatching[playerVertex] <= playerVertex
                || stableMatching[playerVertex] >= nextScoreGroupBeginVertex
                || (stableMatching[opponentVertex] > opponentVertex
                      && stableMatching[opponentVertex] < nextScoreGroupBeginVertex
                    )
            )
            {
              baseEdgeWeights[*opponentIterator][*playerIterator] &= 0u;
            }
            matchingComputer.setEdgeWeight(
              playerVertex,
              opponentVertex,
              baseEdgeWeights[*opponentIterator][*playerIterator]);
          }
          ++remainderIndex;
        }

        // Choose the players to be paired with each of the players in the first
        // group.
        remainderIndex = 0;
        for (const tournament::player_index playerIndex : remainder)
        {
          const tournament::player_index playerVertex = vertexIndices[playerIndex];
          if (
            stableMatching[playerVertex] > playerVertex
              && stableMatching[playerVertex] < nextScoreGroupBeginVertex)
          {
            // Set edge weights to prioritize higher players.
            tournament::player_index addend{ };
            for (
              decltype(remainder)::const_reverse_iterator opponentIterator =
                remainder.rbegin();
              opponentIterator != remainder.rend();
              ++opponentIterator)
            {
              const tournament::player_index opponentVertex =
                vertexIndices[*opponentIterator];
              if (
                *opponentIterator <= playerIndex || matched[opponentVertex])
              {
                continue;
              }
              matching_computer::edge_weight edgeWeight =
                baseEdgeWeights[*opponentIterator][playerIndex];
              if (edgeWeight)
              {
                edgeWeight += addend;
                matchingComputer.setEdgeWeight(
                  playerVertex,
                  opponentVertex,
                  std::move(edgeWeight));
              }
              ++addend;
            }

            matchingComputer.computeMatching();

            stableMatching = matchingComputer.getMatching();

            // Finalize the pairing.
            const tournament::player_index matchVertex =
              stableMatching[playerVertex];
            matched[playerVertex] = true;
            matched[matchVertex] = true;
            finalizePair(
              playerVertex,
              matchVertex,
              matchingComputer,
              maxEdgeWeight);
          }
          ++remainderIndex;
        }

        // Compute the new values for the next pairing bracket.
        std::vector<const tournament::Player *> newPlayersByIndex;
        std::vector<tournament::player_index> newVertexIndices;
        scoreGroupBegin = 0;

        // Preliminary (may be set to false in the subsequent loop)
        isSingleDownfloaterTheByeAssignee =
          sortedPlayers.size() & 1u
            && scoreGroupIterator != sortedPlayers.end()
            && byeAssigneeScore
                >= (*scoreGroupIterator)->scoreWithAcceleration(tournament);

        for (
          tournament::player_index playerIndex = 0;
          playerIndex < playersByIndex.size();
          ++playerIndex)
        {
          const tournament::player_index playerVertex = vertexIndices[playerIndex];
          if (playerIndex < nextScoreGroupBegin && matched[playerVertex])
          {
            // Save the pair in matchingById.
            matchingById[playersByIndex[playerIndex]->id] =
              sortedPlayers[stableMatching[playerVertex]];
            matchingById[sortedPlayers[stableMatching[playerVertex]]->id] =
              playersByIndex[playerIndex];
          }
          else
          {
            // Add the player to the next bracket.
            newPlayersByIndex.push_back(playersByIndex[playerIndex]);
            newVertexIndices.push_back(vertexIndices[playerIndex]);
            if (playerIndex < nextScoreGroupBegin)
            {
              ++scoreGroupBegin;
            }
            if (
              isSingleDownfloaterTheByeAssignee
                && sortedPlayers[stableMatching[playerVertex]]
                      ->scoreWithAcceleration(tournament)
                    < (*scoreGroupIterator)->scoreWithAcceleration(tournament))
            {
              isSingleDownfloaterTheByeAssignee = false;
            }
          }
        }

        playersByIndex = std::move(newPlayersByIndex);
        vertexIndices = std::move(newVertexIndices);
        scoreGroupBeginVertex = nextScoreGroupBeginVertex;
      }

      // Generate the list of Pairings.
      std::list<Pairing> result;
      for (const tournament::Player *const player : sortedPlayers)
      {
        const tournament::Player *const match = matchingById[player->id];
        if (match)
        {
          assert(player->isValid);
          assert(player->matches.size() <= tournament.playedRounds);
          assert(match->isValid);
          assert(match->matches.size() <= tournament.playedRounds);
          if (player->id < match->id)
          {
            result.emplace_back(
              player->id,
              match->id,
              choosePlayerColor(
                tournament.players[player->id],
                *match,
                tournament));
          }
        }
        else
        {
          result.emplace_back(player->id, player->id);
        }
      }

      // Print the checklist.
      if (ostream)
      {
        printChecklist(tournament, sortedPlayers, *ostream, &matchingById);
      }

      return result;
    }
  }
}
#endif
