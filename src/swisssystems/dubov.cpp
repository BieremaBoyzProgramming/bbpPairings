/*
 * An implementation of the FIDE Dubov System, as described in C.04.4.1 of the
 * FIDE Handbook ("Dubov System", approved by the Council on 28/10/2025,
 * applied from 1 February 2026). Article references in the comments below
 * refer to that text unless stated otherwise. "Basic Rules" refers to C.04.1
 * (Basic rules for Swiss Systems) and "GHR" to C.04.2 (General handling rules
 * for Swiss Tournaments), both effective from 1 February 2026.
 *
 * Like the other engines in this program, every optimization step is solved
 * with the weighted matching algorithm. The criteria of Article 2 are encoded
 * as bit fields of the edge weights, in descending priority, so that a maximum
 * weight matching is a pairing that complies at best with the criteria. The
 * sequential choices required by Articles 3.2.2, 3.2.4 and 3.2.6 ("the first
 * set ... in the order given by Article 4.x") are made greedily: candidates are
 * examined in sequence-number order, each one is given a small incentive that
 * cannot outweigh any criterion, and the choice is frozen when the incentive
 * is accepted by the optimal matching.
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <tournament/tournament.h>
#include <utility/typesizes.h>
#include <utility/uintstringconversion.h>
#include <utility/uinttypes.h>

#include "common.h"
#include "dubov.h"

#ifndef OMIT_DUBOV
namespace swisssystems
{
  namespace dubov
  {
    namespace
    {
      typedef matching_computer::edge_weight edge_weight;
      typedef matching_computer::vertex_index vertex_index;
      typedef std::vector<std::uintmax_t> weight_fields;

      /**
       * The strength of a colour preference (Article 1.6).
       */
      enum Strength : utility::uinttypes::uint_least_for_value<3>
      {
        STRENGTH_MILD, STRENGTH_STRONG, STRENGTH_ABSOLUTE
      };

      /**
       * The data of a player that is specific to the Dubov System.
       */
      struct DubovPlayer
      {
        const tournament::Player *player;
        tournament::points score;
        /**
         * The colour preference (Article 1.6). It is never COLOR_NONE, since
         * players who did not play any games have a mild preference for Black
         * (Article 1.6.4).
         */
        tournament::Color preference;
        Strength strength;
        /**
         * The average rating of opponents (Article 1.7).
         */
        std::uintmax_t aro;
        /**
         * The number of times the player has been upfloated (Article 1.8).
         */
        tournament::round_index upfloats;
        bool upfloatedPreviousRound;
        bool maximumUpfloater;
      };

      unsigned int bitsFor(const std::uintmax_t value)
      {
        return utility::typesizes::bitsToRepresent<unsigned int>(value);
      }

      /**
       * Compute the Dubov-specific data of the player.
       */
      DubovPlayer makeDubovPlayer(
        const tournament::Player &player,
        const tournament::Tournament &tournament,
        const tournament::round_index maxT)
      {
        DubovPlayer result{ };
        result.player = &player;
        result.score = player.scoreWithAcceleration(tournament);

        // Article 1.6. The colour difference and the colour history used here
        // only take played games into account (GHR Article 3.4).
        if (player.colorPreference == tournament::COLOR_NONE)
        {
          // Article 1.6.4.
          result.preference = tournament::COLOR_BLACK;
          result.strength = STRENGTH_MILD;
        }
        else
        {
          result.preference = player.colorPreference;
          result.strength =
            player.absoluteColorPreference() ? STRENGTH_ABSOLUTE // 1.6.1
              : player.strongColorPreference ? STRENGTH_STRONG // 1.6.2
              : STRENGTH_MILD; // 1.6.3
        }

        std::uintmax_t ratingSum{ };
        std::uintmax_t ratingCount{ };
        for (
          tournament::round_index roundIndex{ };
          roundIndex < tournament.playedRounds
            && roundIndex < player.matches.size();
          ++roundIndex)
        {
          const tournament::Match &match = player.matches[roundIndex];
          const tournament::Player &opponent =
            tournament.players[match.opponent];

          // Article 1.7.1: only games played over-the-board are used.
          if (match.gameWasPlayed)
          {
            ratingSum += opponent.rating;
            ++ratingCount;
          }

          // Article 1.8: a player is upfloated when they play an opponent
          // from a higher scoregroup. The Dubov rules do not say how unplayed
          // games count; as in the Dutch engine of this program (and as in
          // C.04.3, where only played games make floats for the loser of a
          // forfeit), a game that was not played is not an upfloat.
          if (match.gameWasPlayed)
          {
            const auto roundsBack = tournament.playedRounds - roundIndex;
            if (
              opponent.scoreWithAcceleration(tournament, roundsBack)
                > player.scoreWithAcceleration(tournament, roundsBack))
            {
              ++result.upfloats;
              result.upfloatedPreviousRound =
                roundIndex + 1u == tournament.playedRounds;
            }
          }
        }

        // Article 1.7.1: rounded to the nearest integer, the higher if the
        // division ends for 0.5. Article 1.7.3: zero if no game was played.
        result.aro =
          ratingCount
            ? (2u * ratingSum + ratingCount) / (2u * ratingCount)
            : 0u;

        // Article 1.8.1.
        result.maximumUpfloater = result.upfloats >= maxT;

        return result;
      }

      /**
       * Return whether player0 is ranked higher than player1, that is, has more
       * points or, when points are equal, a smaller TPN (Article 5.2.1).
       */
      bool ranksHigher(const DubovPlayer &player0, const DubovPlayer &player1)
      {
        return
          player0.score > player1.score
            || (player0.score == player1.score
                  && player0.player->rankIndex < player1.player->rankIndex);
      }

      /**
       * Order players by ascending ARO, and by ascending TPN when AROs are
       * equal (Articles 3.2.5 and 4.3.2).
       */
      bool aroCompare(
        const DubovPlayer *const player0,
        const DubovPlayer *const player1)
      {
        return
          std::tie(player0->aro, player0->player->rankIndex)
            < std::tie(player1->aro, player1->player->rankIndex);
      }

      /**
       * Order players by ascending TPN (Articles 4.3.2 and 4.4.2).
       */
      bool tpnCompare(
        const DubovPlayer *const player0,
        const DubovPlayer *const player1)
      {
        return player0->player->rankIndex < player1->player->rankIndex;
      }

      /**
       * Check the absolute criteria [C1] and [C3] (Articles 2.1.1 and 2.1.3),
       * as well as the pairs forbidden by the tournament configuration.
       */
      bool legal(
        const DubovPlayer &player0,
        const DubovPlayer &player1,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs)
      {
        return
          !forbiddenPairs[player0.player->id].count(player1.player->id)
            && !(player0.strength == STRENGTH_ABSOLUTE
                  && player1.strength == STRENGTH_ABSOLUTE
                  && player0.preference == player1.preference);
      }

      /**
       * Return the colour of player0 when playing player1, according to
       * Article 5.2.
       */
      tournament::Color chooseColor(
        const DubovPlayer &player0,
        const DubovPlayer &player1,
        const tournament::Tournament &tournament)
      {
        const bool player0IsHigher = ranksHigher(player0, player1);

        // 5.2.1: both players have yet to play a game. The higher ranked player
        // receives the initial-colour if their TPN is odd. (TPNs are one more
        // than rankIndex.)
        if (!player0.player->playedGames && !player1.player->playedGames)
        {
          const DubovPlayer &higher = player0IsHigher ? player0 : player1;
          const tournament::Color higherColor =
            higher.player->rankIndex & 1u
              ? invert(tournament.initialColor)
              : tournament.initialColor;
          return player0IsHigher ? higherColor : invert(higherColor);
        }

        // 5.2.2: grant both colour preferences.
        if (player0.preference != player1.preference)
        {
          return player0.preference;
        }

        // 5.2.3: grant the stronger colour preference.
        if (player0.strength != player1.strength)
        {
          return
            player0.strength > player1.strength
              ? player0.preference
              : invert(player1.preference);
        }

        // 5.2.4: alternate the colours to the most recent time in which one
        // player had White and the other Black (GHR Article 3.4: only played
        // games count).
        tournament::Color color0;
        tournament::Color color1;
        findFirstColorDifference(
          *player0.player,
          *player1.player,
          color0,
          color1);
        if (
          color0 != tournament::COLOR_NONE
            && color1 != tournament::COLOR_NONE)
        {
          return color1;
        }

        // 5.2.5: grant the colour preference of the higher ranked player.
        return
          player0IsHigher ? player0.preference : invert(player1.preference);
      }

      /**
       * Return the indices 0, ..., size - 1, ordered by the sequence numbers
       * of Article 4.3.3: starting from the (remaining) middle of the list
       * and, when two players are in the (remaining) middle, with the one with
       * a higher position in the list first.
       */
      std::vector<std::size_t> middleOutOrder(const std::size_t size)
      {
        std::vector<std::size_t> result(size);
        for (std::size_t index{ }; index < size; ++index)
        {
          result[index] = index;
        }
        // Compare the doubled distances to the middle to stay in integers.
        std::stable_sort(
          result.begin(),
          result.end(),
          [size](const std::size_t index0, const std::size_t index1)
          {
            const std::size_t doubledMiddle = size - 1u;
            const std::size_t distance0 =
              2u * index0 > doubledMiddle
                ? 2u * index0 - doubledMiddle
                : doubledMiddle - 2u * index0;
            const std::size_t distance1 =
              2u * index1 > doubledMiddle
                ? 2u * index1 - doubledMiddle
                : doubledMiddle - 2u * index1;
            return distance0 < distance1;
          });
        return result;
      }

      /**
       * A wrapper around a matching computer whose edge weights are made of
       * bit fields of given widths (in descending priority). The last field is
       * reserved for the incentives used to make sequential choices.
       */
      class FieldMatcher
      {
      public:
        /**
         * The function returns the fields of the weight of an edge, or an
         * empty vector if the edge is not allowed. Its value in the last
         * (incentive) field is ignored.
         */
        typedef std::function<weight_fields(vertex_index, vertex_index)>
          weight_function;

        FieldMatcher(
            const vertex_index size_,
            std::vector<unsigned int> &&widths_,
            weight_function &&weightFunction_)
          : size(size_),
            widths(std::move(widths_)),
            weightFunction(std::move(weightFunction_)),
            maxEdgeWeight(computeMaxEdgeWeight(widths)),
            computer(size_, maxEdgeWeight),
            allowed(size_, std::vector<bool>(size_, true)),
            matching(),
            dirty(true)
        {
          for (vertex_index vertex{ }; vertex < size; ++vertex)
          {
            computer.addVertex();
          }
          for (vertex_index vertex0{ }; vertex0 < size; ++vertex0)
          {
            for (vertex_index vertex1{ }; vertex1 < vertex0; ++vertex1)
            {
              if (weightFunction(vertex0, vertex1).empty())
              {
                allowed[vertex0][vertex1] = false;
                allowed[vertex1][vertex0] = false;
              }
              else
              {
                reset(vertex0, vertex1);
              }
            }
          }
        }

        bool isAllowed(const vertex_index vertex0, const vertex_index vertex1)
          const
        {
          return vertex0 != vertex1 && allowed[vertex0][vertex1];
        }

        /**
         * Permanently remove the edge.
         */
        void forbid(const vertex_index vertex0, const vertex_index vertex1)
        {
          if (isAllowed(vertex0, vertex1))
          {
            allowed[vertex0][vertex1] = false;
            allowed[vertex1][vertex0] = false;
            computer.setEdgeWeight(vertex0, vertex1, maxEdgeWeight & 0u);
            dirty = true;
          }
        }

        /**
         * Set the incentive field of the edge to the given value.
         */
        void reward(
          const vertex_index vertex0,
          const vertex_index vertex1,
          const std::uintmax_t incentive)
        {
          if (isAllowed(vertex0, vertex1))
          {
            weight_fields fields = weightFunction(vertex0, vertex1);
            fields.back() = incentive;
            computer.setEdgeWeight(vertex0, vertex1, compose(fields));
            dirty = true;
          }
        }

        /**
         * Remove any incentive from the edge.
         */
        void reset(const vertex_index vertex0, const vertex_index vertex1)
        {
          reward(vertex0, vertex1, 0u);
        }

        vertex_index partner(const vertex_index vertex)
        {
          update();
          return matching[vertex];
        }

        /**
         * Check whether all vertices are matched.
         */
        bool isPerfect()
        {
          update();
          for (vertex_index vertex{ }; vertex < size; ++vertex)
          {
            if (matching[vertex] == vertex)
            {
              return false;
            }
          }
          return true;
        }

        /**
         * Decide whether the vertex can be matched to one of the candidates
         * in an optimal matching (satisfying the constraints imposed so far).
         */
        bool canBeMatchedWithin(
          const vertex_index vertex,
          const std::vector<bool> &candidates)
        {
          if (candidates[partner(vertex)] && partner(vertex) != vertex)
          {
            return true;
          }
          for (vertex_index other{ }; other < size; ++other)
          {
            if (candidates[other])
            {
              reward(vertex, other, 1u);
            }
          }
          const vertex_index result = partner(vertex);
          for (vertex_index other{ }; other < size; ++other)
          {
            if (candidates[other])
            {
              reset(vertex, other);
            }
          }
          return result != vertex && candidates[result];
        }

      private:
        static edge_weight computeMaxEdgeWeight(
          const std::vector<unsigned int> &widths)
        {
          unsigned int totalWidth{ };
          for (const unsigned int width : widths)
          {
            totalWidth += width;
          }
          // The edge weights should have room to expand by two bits for the
          // matching subroutine. Subtracting 1 sets all bits to 1.
          edge_weight result(1u);
          result.shiftGrow(totalWidth);
          result.shiftGrow(2u);
          result >>= 1u;
          result -= 1u;
          return result;
        }

        edge_weight compose(const weight_fields &fields) const
        {
          assert(fields.size() == widths.size());
          edge_weight result = maxEdgeWeight & 0u;
          for (std::size_t index{ }; index < fields.size(); ++index)
          {
            assert(bitsFor(fields[index]) <= widths[index]);
            result <<= widths[index];
            result |= fields[index];
          }
          return result;
        }

        void update()
        {
          if (dirty)
          {
            computer.computeMatching();
            matching = computer.getMatching();
            dirty = false;
          }
        }

        const vertex_index size;
        const std::vector<unsigned int> widths;
        const weight_function weightFunction;
        const edge_weight maxEdgeWeight;
        matching_computer computer;
        std::vector<std::vector<bool>> allowed;
        std::vector<vertex_index> matching;
        bool dirty;
      };

      /**
       * Choose the player receiving the pairing-allocated bye (Article 3.1),
       * and return their index in players.
       */
      std::size_t assignBye(
        const std::vector<DubovPlayer> &players,
        const tournament::Tournament &tournament,
        const std::vector<std::unordered_set<tournament::player_index>>
          &forbiddenPairs)
      {
        // Rank the players eligible for the bye ([C2], Article 3.1.1) by
        // Articles 3.1.3 (lowest score), 3.1.4 (highest number of played
        // games) and 3.1.5 (largest TPN).
        std::vector<std::size_t> candidates;
        for (std::size_t index{ }; index < players.size(); ++index)
        {
          if (eligibleForBye(*players[index].player, tournament))
          {
            candidates.push_back(index);
          }
        }
        std::sort(
          candidates.begin(),
          candidates.end(),
          [&players](const std::size_t index0, const std::size_t index1)
          {
            const DubovPlayer &player0 = players[index0];
            const DubovPlayer &player1 = players[index1];
            return
              std::make_tuple(
                  player0.score,
                  player1.player->playedGames,
                  player1.player->rankIndex)
                < std::make_tuple(
                    player1.score,
                    player0.player->playedGames,
                    player0.player->rankIndex);
          });
        std::vector<std::uintmax_t> priorities(players.size());
        for (std::size_t rank{ }; rank < candidates.size(); ++rank)
        {
          priorities[candidates[rank]] = candidates.size() - rank;
        }

        // Article 3.1.2: the bye assignee must allow a complete pairing of the
        // remaining players. The last vertex represents the bye.
        const vertex_index byeVertex = players.size();
        FieldMatcher matcher(
          players.size() + 1u,
          { bitsFor(players.size()), bitsFor(players.size()), 0u },
          [&](const vertex_index vertex0, const vertex_index vertex1)
          {
            if (vertex0 == byeVertex || vertex1 == byeVertex)
            {
              const vertex_index player = std::min(vertex0, vertex1);
              return
                priorities[player]
                  ? weight_fields{ 1u, priorities[player], 0u }
                  : weight_fields{ };
            }
            return
              legal(players[vertex0], players[vertex1], forbiddenPairs)
                ? weight_fields{ 1u, 0u, 0u }
                : weight_fields{ };
          });
        if (!matcher.isPerfect())
        {
          throw NoValidPairingException(
            "No player can receive the pairing-allocated bye while allowing a "
              "complete pairing of the remaining players.");
        }
        return matcher.partner(byeVertex);
      }

      /**
       * Pair the players of a bracket, whose score group has already been
       * completed with upfloaters, using Articles 3.2.3 to 3.2.6. All the
       * players must be legally pairable. Return the pairs as (S1, T2) player
       * pairs.
       */
      std::vector<std::pair<const DubovPlayer *, const DubovPlayer *>>
        pairBracket(
          std::vector<const DubovPlayer *> bracket,
          const std::vector<std::unordered_set<tournament::player_index>>
            &forbiddenPairs)
      {
        const vertex_index size = bracket.size();
        const vertex_index half = size / 2u;
        std::sort(bracket.begin(), bracket.end(), tpnCompare);

        // Article 3.2.3: initial composition of G1 and G2.
        bool nobodyPlayed = true;
        for (const DubovPlayer *const player : bracket)
        {
          if (player->player->playedGames)
          {
            nobodyPlayed = false;
          }
        }
        std::vector<bool> inG1(size);
        vertex_index g1Size{ };
        for (vertex_index vertex{ }; vertex < size; ++vertex)
        {
          inG1[vertex] =
            nobodyPlayed
              ? vertex < half
              : bracket[vertex]->preference == tournament::COLOR_WHITE;
          g1Size += inG1[vertex];
        }

        // Article 3.2.4: A is the smaller subgroup (G1 if their sizes are
        // equal), B the other one.
        const bool aIsG1 = g1Size <= size - g1Size;
        std::vector<bool> inA(size);
        for (vertex_index vertex{ }; vertex < size; ++vertex)
        {
          inA[vertex] = inG1[vertex] == aIsG1;
        }

        // Maximize the number of pairs between A and B, which is the same as
        // complying at best with [C7] (Article 2.3.3) when the subgroups are
        // formed by colour preference.
        FieldMatcher matcher(
          size,
          { bitsFor(half), bitsFor(half), bitsFor(size) },
          [&](const vertex_index vertex0, const vertex_index vertex1)
          {
            return
              legal(*bracket[vertex0], *bracket[vertex1], forbiddenPairs)
                ? weight_fields{
                    1u,
                    inA[vertex0] != inA[vertex1],
                    0u
                  }
                : weight_fields{ };
          });
        if (!matcher.isPerfect())
        {
          throw std::logic_error("The bracket cannot be paired.");
        }

        // Choose the shifters (Articles 3.2.4 and 4.3). A shifter is moved
        // out of its subgroup to face a player of its own subgroup, so it
        // must be paired with a non-shifter of the same subgroup.
        std::vector<bool> shifted(size);
        const auto chooseShifters =
          [&](const bool fromA)
          {
            vertex_index required{ };
            for (vertex_index vertex{ }; vertex < size; ++vertex)
            {
              const vertex_index partner = matcher.partner(vertex);
              required +=
                vertex < partner
                  && inA[vertex] == fromA
                  && inA[partner] == fromA;
            }
            if (!required)
            {
              return;
            }

            // Article 4.3.2: White seekers by ascending ARO then TPN, Black
            // seekers by ascending TPN.
            std::vector<const DubovPlayer *> pool;
            for (vertex_index vertex{ }; vertex < size; ++vertex)
            {
              if (inA[vertex] == fromA)
              {
                pool.push_back(bracket[vertex]);
              }
            }
            std::stable_sort(
              pool.begin(),
              pool.end(),
              fromA == aIsG1 ? aroCompare : tpnCompare);

            // Article 4.3.3 sequence numbers, and Article 4.1.3 ordering of
            // the sets: choose greedily the first possible shifters.
            for (const std::size_t poolIndex : middleOutOrder(pool.size()))
            {
              if (!required)
              {
                break;
              }
              const vertex_index candidate =
                std::find(bracket.begin(), bracket.end(), pool[poolIndex])
                  - bracket.begin();
              std::vector<bool> partners(size);
              for (vertex_index vertex{ }; vertex < size; ++vertex)
              {
                partners[vertex] =
                  vertex != candidate
                    && inA[vertex] == fromA
                    && !shifted[vertex];
              }
              if (matcher.canBeMatchedWithin(candidate, partners))
              {
                shifted[candidate] = true;
                --required;
                for (vertex_index vertex{ }; vertex < size; ++vertex)
                {
                  if (!partners[vertex])
                  {
                    matcher.forbid(candidate, vertex);
                  }
                }
              }
            }
            assert(!required);
          };
        // First from the smaller subgroup, then, to equalise the sizes, from
        // the larger one.
        chooseShifters(true);
        chooseShifters(false);

        // The final subgroups.
        std::vector<const DubovPlayer *> s1;
        std::vector<const DubovPlayer *> g2;
        std::vector<bool> inFinalG1(size);
        for (vertex_index vertex{ }; vertex < size; ++vertex)
        {
          inFinalG1[vertex] = inG1[vertex] != shifted[vertex];
          (inFinalG1[vertex] ? s1 : g2).push_back(bracket[vertex]);
        }
        assert(s1.size() == half && g2.size() == half);

        // Article 3.2.5: S1 is G1 sorted by ascending ARO, then TPN.
        std::stable_sort(s1.begin(), s1.end(), aroCompare);
        // Article 4.4.2: transpositions of G2 are ordered by ascending TPN.
        std::stable_sort(g2.begin(), g2.end(), tpnCompare);

        // Article 3.2.6: choose the first transposition T2 that yields a legal
        // pairing, S1[i] being paired with T2[i]. Vertices 0, ..., half - 1
        // are the players of S1, the others the players of G2.
        std::vector<const DubovPlayer *> ordered(s1);
        ordered.insert(ordered.end(), g2.begin(), g2.end());
        FieldMatcher transpositionMatcher(
          size,
          { bitsFor(half), bitsFor(half) },
          [&](const vertex_index vertex0, const vertex_index vertex1)
          {
            return
              (vertex0 < half) != (vertex1 < half)
                && legal(*ordered[vertex0], *ordered[vertex1], forbiddenPairs)
                ? weight_fields{ 1u, 0u }
                : weight_fields{ };
          });
        std::vector<std::pair<const DubovPlayer *, const DubovPlayer *>> result;
        for (vertex_index s1Vertex{ }; s1Vertex < half; ++s1Vertex)
        {
          for (vertex_index g2Index{ }; g2Index < half; ++g2Index)
          {
            transpositionMatcher.reward(
              s1Vertex,
              half + g2Index,
              half - g2Index);
          }
          const vertex_index partner = transpositionMatcher.partner(s1Vertex);
          assert(transpositionMatcher.isPerfect());
          for (vertex_index vertex{ }; vertex < size; ++vertex)
          {
            if (vertex != partner && vertex != s1Vertex)
            {
              transpositionMatcher.forbid(s1Vertex, vertex);
              transpositionMatcher.forbid(partner, vertex);
            }
          }
          transpositionMatcher.reset(s1Vertex, partner);
          result.emplace_back(ordered[s1Vertex], ordered[partner]);
        }
        return result;
      }

      /**
       * Print the checklist file for the current round to ostream.
       */
      void printChecklist(
        const tournament::Tournament &tournament,
        const std::vector<DubovPlayer> &players,
        std::ostream &ostream,
        const std::vector<const tournament::Player *> *const matching = nullptr)
      {
        std::vector<const DubovPlayer *> playersById(tournament.players.size());
        std::vector<const tournament::Player *> sortedPlayers;
        for (const DubovPlayer &player : players)
        {
          playersById[player.player->id] = &player;
          sortedPlayers.push_back(player.player);
        }
        swisssystems::printChecklist(
          ostream,
          std::deque<std::string>{ "C2", "ARO", "Up", "UpP", "Cur" },
          [&](const tournament::Player &player)
          {
            const DubovPlayer &dubovPlayer = *playersById[player.id];
            const tournament::Player *const opponent =
              matching ? (*matching)[player.id] : nullptr;
            return std::deque<std::string>{
              eligibleForBye(player, tournament) ? "Y" : "N",
              utility::uintstringconversion::toString(dubovPlayer.aro),
              utility::uintstringconversion::toString(dubovPlayer.upfloats)
                + (dubovPlayer.maximumUpfloater ? "*" : ""),
              dubovPlayer.upfloatedPreviousRound ? "Y" : "N",
              matching
                ? opponent && opponent != &player
                    ? '('
                        + utility::uintstringconversion
                            ::toString(opponent->id + 1u)
                        + (chooseColor(
                              dubovPlayer,
                              *playersById[opponent->id],
                              tournament)
                            == tournament::COLOR_WHITE ? 'W' : 'B')
                        + ')'
                    : "(bye)"
                : ""
            };
          },
          tournament,
          sortedPlayers);
      }
    }

    /**
     * Compute the matching, and return the list of Pairings in arbitrary order.
     * If no matching is possible, throw a NoValidPairingException.
     */
    std::list<Pairing> computeMatching(
      tournament::Tournament &&tournament,
      std::ostream *const ostream)
    {
      // Filter out the absent players, and sort the remainder by score and
      // TPN. Players who already met over-the-board may not meet again ([C1],
      // Basic Rules Article 2; GHR Article 3.5).
      std::vector<const tournament::Player *> sortedPlayers;
      auto forbiddenPairs =
        tournament.resolveForbiddenPairs(tournament.playedRounds);
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

      // Article 1.8.2: MaxT = 2 + [Rnds/5].
      const tournament::round_index maxT = 2u + tournament.expectedRounds / 5u;
      // [C8] to [C10] do not apply to the last round.
      const bool lastRound =
        tournament.playedRounds + 1u >= tournament.expectedRounds;

      std::vector<DubovPlayer> players;
      for (const tournament::Player *const player : sortedPlayers)
      {
        players.push_back(makeDubovPlayer(*player, tournament, maxT));
      }

      std::list<Pairing> result;
      std::vector<const tournament::Player *>
        matchingById(tournament.players.size());

      try
      {
        // Article 1.9.2: the pairing process starts with the assignment of the
        // pairing-allocated bye.
        std::vector<const DubovPlayer *> remaining;
        {
          const std::size_t byeIndex =
            players.size() & 1u
              ? assignBye(players, tournament, forbiddenPairs)
              : players.size();
          for (std::size_t index{ }; index < players.size(); ++index)
          {
            if (index == byeIndex)
            {
              const tournament::player_index id = players[index].player->id;
              result.emplace_back(id, id);
              matchingById[id] = players[index].player;
            }
            else
            {
              remaining.push_back(&players[index]);
            }
          }
        }

        // Article 1.9.2: pair the score groups in descending order of score.
        while (!remaining.empty())
        {
          const tournament::points residentScore = remaining.front()->score;
          vertex_index residentCount{ };
          while (
            residentCount < remaining.size()
              && remaining[residentCount]->score == residentScore)
          {
            ++residentCount;
          }
          const vertex_index size = remaining.size();
          const auto isResident =
            [residentCount](const vertex_index vertex)
            {
              return vertex < residentCount;
            };

          // The distinct scores of the possible upfloaters (Article 4.2.1), in
          // descending order of score difference with the residents.
          std::vector<tournament::points> lowerScores;
          tournament::round_index maxUpfloats{ };
          for (vertex_index vertex = residentCount; vertex < size; ++vertex)
          {
            if (
              lowerScores.empty()
                || lowerScores.front() != remaining[vertex]->score)
            {
              lowerScores.insert(lowerScores.begin(), remaining[vertex]->score);
            }
            maxUpfloats =
              std::max(maxUpfloats, remaining[vertex]->upfloats);
          }

          // The fields of the edge weights, in descending priority:
          // - every pair, so that all the remaining players can be paired
          //   ([C4], Article 2.2.1);
          // - pairs of residents ([C5], Article 2.3.1);
          // - for each lower score, in descending order of score difference,
          //   upfloaters not having this score ([C6], Article 2.3.2);
          // - pairs of the bracket granting both colour preferences ([C7],
          //   Article 2.3.3);
          // - upfloaters who are not maximum upfloaters ([C8], 2.3.4);
          // - for maximum upfloaters, maxUpfloats minus their number of
          //   upfloats ([C9], 2.3.5);
          // - upfloaters who did not upfloat in the previous round ([C10],
          //   2.3.6);
          // - the incentive.
          std::vector<unsigned int> widths{
            bitsFor(size / 2u),
            bitsFor(residentCount / 2u)
          };
          for (std::size_t index{ }; index < lowerScores.size(); ++index)
          {
            widths.push_back(bitsFor(residentCount));
          }
          widths.push_back(bitsFor(residentCount));
          widths.push_back(bitsFor(residentCount));
          widths.push_back(bitsFor(residentCount * maxUpfloats));
          widths.push_back(bitsFor(residentCount));
          widths.push_back(1u);

          FieldMatcher matcher(
            size,
            std::move(widths),
            [&](const vertex_index vertex0, const vertex_index vertex1)
            {
              const DubovPlayer &player0 = *remaining[vertex0];
              const DubovPlayer &player1 = *remaining[vertex1];
              if (!legal(player0, player1, forbiddenPairs))
              {
                return weight_fields{ };
              }
              const bool resident0 = isResident(vertex0);
              const bool resident1 = isResident(vertex1);
              weight_fields fields{ 1u, resident0 && resident1 };
              const DubovPlayer *const upfloater =
                resident0 == resident1 ? nullptr
                  : resident0 ? &player1
                  : &player0;
              for (const tournament::points lowerScore : lowerScores)
              {
                fields.push_back(upfloater && upfloater->score != lowerScore);
              }
              fields.push_back(
                (resident0 || resident1)
                  && player0.preference != player1.preference);
              const bool optionalCriteria = upfloater && !lastRound;
              fields.push_back(
                optionalCriteria && !upfloater->maximumUpfloater);
              fields.push_back(
                optionalCriteria && upfloater->maximumUpfloater
                  ? maxUpfloats - upfloater->upfloats
                  : 0u);
              fields.push_back(
                optionalCriteria && !upfloater->upfloatedPreviousRound);
              fields.push_back(0u);
              return fields;
            });
          if (!matcher.isPerfect())
          {
            throw NoValidPairingException(
              "The players could not be simultaneously matched while "
                "satisfying all absolute criteria.");
          }

          // Article 3.2.1: the minimum number of upfloaters (the matching
          // already minimizes it, [C5]).
          vertex_index requiredUpfloaters{ };
          for (vertex_index vertex = residentCount; vertex < size; ++vertex)
          {
            requiredUpfloaters += isResident(matcher.partner(vertex));
          }

          // Article 3.2.2: choose the first set of upfloaters, in the order of
          // Article 4.2.2 (descending score, then ascending TPN, which is the
          // order of remaining), among those complying at best with the
          // criteria.
          std::vector<bool> residents(size);
          for (vertex_index vertex{ }; vertex < residentCount; ++vertex)
          {
            residents[vertex] = true;
          }
          std::vector<const DubovPlayer *> bracket(
            remaining.begin(),
            remaining.begin() + residentCount);
          std::vector<bool> inBracket(residents);
          for (
            vertex_index candidate = residentCount;
            candidate < size && requiredUpfloaters;
            ++candidate)
          {
            const bool upfloats =
              matcher.canBeMatchedWithin(candidate, residents);
            for (vertex_index vertex{ }; vertex < size; ++vertex)
            {
              if (residents[vertex] != upfloats)
              {
                matcher.forbid(candidate, vertex);
              }
            }
            if (upfloats)
            {
              bracket.push_back(remaining[candidate]);
              inBracket[candidate] = true;
              --requiredUpfloaters;
            }
          }

          // Articles 3.2.3 to 3.2.6.
          for (const auto &pair : pairBracket(bracket, forbiddenPairs))
          {
            matchingById[pair.first->player->id] = pair.second->player;
            matchingById[pair.second->player->id] = pair.first->player;
            result.emplace_back(
              pair.first->player->id,
              pair.second->player->id,
              chooseColor(*pair.first, *pair.second, tournament));
          }

          std::vector<const DubovPlayer *> nextRemaining;
          for (vertex_index vertex{ }; vertex < size; ++vertex)
          {
            if (!inBracket[vertex])
            {
              nextRemaining.push_back(remaining[vertex]);
            }
          }
          remaining = std::move(nextRemaining);
        }
      }
      catch (const NoValidPairingException &)
      {
        if (ostream)
        {
          printChecklist(tournament, players, *ostream);
        }
        throw;
      }

      if (ostream)
      {
        printChecklist(tournament, players, *ostream, &matchingById);
      }

      return result;
    }
  }
}
#endif
