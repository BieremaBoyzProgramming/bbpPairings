/*
 * An implementation of the FIDE Lim System, as described in C.04.4.3 of the
 * FIDE Handbook ("Lim System", approved by the Council on 28/10/2025, applied
 * from 1 February 2026). Article references in the comments below refer to
 * that text unless stated otherwise. "Basic Rules" refers to C.04.1 (Basic
 * rules for Swiss Systems) and "GHR" to C.04.2 (General handling rules for
 * Swiss Tournaments), both effective from 1 February 2026.
 *
 * Unlike the Dutch, Burstein and Dubov systems, the Lim System is written as a
 * procedure: score groups are paired one after the other, from the extremes
 * towards the Median Scoregroup (Article 2.2), floaters are chosen with the
 * rules of Article 3 and the pairs are found by scrutinising the proposed
 * pairings in a fixed order of exchanges (Article 4). This implementation
 * follows that procedure. The weighted matching algorithm is used, as in the
 * other engines, to answer the questions the procedure asks ("can the rest of
 * the score group still be paired?"): each choice keeps the largest possible
 * number of pairs in the score group (Articles 3.10.2 and 4.4), and among
 * those, the first one in the order given by the rules.
 *
 * Interpretations, also listed in README.txt:
 * - The "highest numbered player" is player #1 of the score group (Article
 *   4.2: "Player #1, the highest numbered player in the scoregroup"), that is,
 *   the player with the smallest pairing number.
 * - Only played games count for colours and for previous meetings (GHR
 *   Articles 3.4 and 3.5). A player floated in the previous round when they
 *   played an opponent with a different score.
 * - Compatibility (Article 2.1) requires that the players did not meet and
 *   that a colour allocation exists that gives neither player the same colour
 *   three times in a row nor three more games with one colour than with the
 *   other (Article 5.1). In the last round, players with the same score are
 *   compatible regardless of colours (Article 6). If no complete pairing
 *   satisfies Article 2.1, only the absolute criterion of the Basic Rules
 *   (players do not meet twice) is kept.
 * - The pairing-allocated bye (Article 1.1) goes to the lowest ranked player of
 *   the lowest score group who is eligible (Basic Rules Article 3) and whose
 *   removal leaves a complete pairing possible.
 * - The second scrutiny of Article 5.2 is merged with the first one: each
 *   player, in scrutiny order, receives the first opponent in the order of
 *   exchanges who grants both players their due colours, if one keeps the most
 *   pairs in the score group, and the first such opponent otherwise.
 * - The Maxi-tournament restrictions (Articles 3.2.3, 3.8 and 5.7) are not
 *   applied.
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
#include "lim.h"

#ifndef OMIT_LIM
namespace swisssystems
{
  namespace lim
  {
    namespace
    {
      typedef matching_computer::edge_weight edge_weight;
      typedef matching_computer::vertex_index vertex_index;
      typedef std::vector<std::uintmax_t> weight_fields;
      typedef std::size_t index;
      typedef std::vector<index> index_list;

      constexpr index NO_INDEX = static_cast<index>(-1);

      unsigned int bitsFor(const std::uintmax_t value)
      {
        return utility::typesizes::bitsToRepresent<unsigned int>(value);
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
       * The direction in which a score group is paired (Article 2.2).
       */
      enum Direction { DOWNWARD, UPWARD };

      /**
       * The data of a player that is specific to the Lim System.
       */
      struct LimPlayer
      {
        const tournament::Player *player;
        tournament::points score;
        /**
         * The colours of the played games, in round order (GHR Article 3.4).
         */
        std::vector<tournament::Color> colors;
        int colorDifference;
        bool floatedPreviousRound;
        bool byeEligible;
        std::unordered_set<tournament::player_index> forbidden;

        /**
         * Articles 5.1.1 and 5.1.2: the colour does not give the player the
         * same colour three times in a row, nor three more games with one
         * colour than with the other.
         */
        bool colorAllowed(const tournament::Color color) const
        {
          if (
            colors.size() >= 2u
              && colors[colors.size() - 1u] == color
              && colors[colors.size() - 2u] == color)
          {
            return false;
          }
          const int difference =
            colorDifference + (color == tournament::COLOR_WHITE ? 1 : -1);
          return difference <= 2 && difference >= -2;
        }

        /**
         * The due colour: the equalising colour, otherwise the alternate colour
         * (Articles 5.1, 5.5 and 5.6). COLOR_NONE if no game was played.
         */
        tournament::Color due() const
        {
          if (colors.empty())
          {
            return tournament::COLOR_NONE;
          }
          if (colorDifference > 0)
          {
            return tournament::COLOR_BLACK;
          }
          if (colorDifference < 0)
          {
            return tournament::COLOR_WHITE;
          }
          return invert(colors.back());
        }

        /**
         * The colour the player must receive (Articles 5.3 and 5.1.2), or
         * COLOR_NONE.
         */
        tournament::Color imposed() const
        {
          if (
            colors.size() >= 2u
              && colors[colors.size() - 1u] == colors[colors.size() - 2u])
          {
            return invert(colors.back());
          }
          if (colorDifference >= 2)
          {
            return tournament::COLOR_BLACK;
          }
          if (colorDifference <= -2)
          {
            return tournament::COLOR_WHITE;
          }
          return tournament::COLOR_NONE;
        }
      };

      /**
       * A player transferred to another score group (Article 3.1), with the
       * score of the group they come from.
       */
      struct Floater
      {
        index player;
        tournament::points source;
      };

      /**
       * A pair, made in the score group with the given score. The first player
       * is the one who looked for an opponent.
       */
      struct Pair
      {
        index player0;
        index player1;
        tournament::points group;
      };

      struct NoLimPairing { };

      class LimPairer
      {
      public:
        LimPairer(
            const tournament::Tournament &tournament_,
            std::vector<LimPlayer> &&players_)
          : tournament(tournament_),
            players(std::move(players_)),
            doubledMedian(
              static_cast<std::uintmax_t>(tournament_.playedRounds)
                * tournament_.pointsForWin),
            lastRound(
              tournament_.expectedRounds
                && tournament_.playedRounds + 1u
                     >= tournament_.expectedRounds),
            relaxed(false)
        { }

        /**
         * Pair the round. Return the pairs, and set bye to the player
         * receiving the pairing-allocated bye (NO_INDEX if none).
         */
        std::vector<Pair> pair(index &bye)
        {
          index_list everybody;
          for (index player{ }; player < players.size(); ++player)
          {
            everybody.push_back(player);
          }
          bye = assignBye(everybody);
          index_list remaining;
          for (const index player : everybody)
          {
            if (player != bye)
            {
              remaining.push_back(player);
            }
          }
          relaxed = false;
          if (2u * maxPairs(remaining) != remaining.size())
          {
            relaxed = true;
            if (2u * maxPairs(remaining) != remaining.size())
            {
              throw NoValidPairingException(
                "The players could not be simultaneously matched without "
                  "meeting an opponent again.");
            }
          }
          try
          {
            return pairRemaining(remaining);
          }
          catch (const NoLimPairing &)
          {
            if (relaxed)
            {
              throw;
            }
            relaxed = true;
            return pairRemaining(remaining);
          }
        }

        /**
         * Return the colour of player0 when playing player1 in a pair made in
         * the given score group (Articles 5, 6 and 7.2).
         */
        tournament::Color chooseColor(
          const index player0,
          const index player1,
          const tournament::points group) const
        {
          const LimPlayer &limPlayer0 = players[player0];
          const LimPlayer &limPlayer1 = players[player1];
          const tournament::Color imposed0 = limPlayer0.imposed();
          const tournament::Color imposed1 = limPlayer1.imposed();
          if (
            imposed0 != tournament::COLOR_NONE
              || imposed1 != tournament::COLOR_NONE)
          {
            // Article 6: both players must have the same colour, which only
            // happens in the last round; decide as in Article 5.4.
            if (imposed0 == imposed1)
            {
              return breakTie(player0, player1, imposed0, group);
            }
            // Articles 5.3 and 5.1.2.
            return
              imposed0 != tournament::COLOR_NONE
                ? imposed0
                : invert(imposed1);
          }
          const tournament::Color due0 = limPlayer0.due();
          const tournament::Color due1 = limPlayer1.due();
          if (
            due0 == tournament::COLOR_NONE
              && due1 == tournament::COLOR_NONE)
          {
            // Article 7.2: the colour of player #1 is drawn by lot, the
            // odd-numbered players of the upper half have the same colour and
            // the even-numbered ones the other colour. Pairing numbers are
            // one more than rankIndex.
            const bool player0IsHigher = ranksHigher(player0, player1);
            const index higher = player0IsHigher ? player0 : player1;
            const tournament::Color higherColor =
              players[higher].player->rankIndex & 1u
                ? invert(tournament.initialColor)
                : tournament.initialColor;
            return player0IsHigher ? higherColor : invert(higherColor);
          }
          // Articles 5.5 and 5.6: each player receives their due colour.
          if (due0 == tournament::COLOR_NONE)
          {
            return invert(due1);
          }
          if (due1 == tournament::COLOR_NONE || due0 != due1)
          {
            return due0;
          }
          return breakTie(player0, player1, due0, group);
        }

      private:
        const tournament::Tournament &tournament;
        const std::vector<LimPlayer> players;
        /**
         * Twice the score of the Median Scoregroup, which is half the number
         * of rounds played (Article 2.2).
         */
        const std::uintmax_t doubledMedian;
        const bool lastRound;
        bool relaxed;

        bool isAbove(const tournament::points score) const
        {
          return 2u * static_cast<std::uintmax_t>(score) > doubledMedian;
        }

        bool isMedian(const tournament::points score) const
        {
          return 2u * static_cast<std::uintmax_t>(score) == doubledMedian;
        }

        tournament::player_index rank(const index player) const
        {
          return players[player].player->rankIndex;
        }

        bool ranksHigher(const index player0, const index player1) const
        {
          return
            players[player0].score > players[player1].score
              || (players[player0].score == players[player1].score
                    && rank(player0) < rank(player1));
        }

        /**
         * Article 5.4: both players are due the same colour. The colours they
         * had in earlier rounds, going back in sequence, decide; with identical
         * histories, the higher ranked player receives the due colour in the
         * Median Scoregroup or above, the lower ranked one below it.
         */
        tournament::Color breakTie(
          const index player0,
          const index player1,
          const tournament::Color dueColor,
          const tournament::points group) const
        {
          const std::vector<tournament::Color> &colors0 =
            players[player0].colors;
          const std::vector<tournament::Color> &colors1 =
            players[player1].colors;
          for (
            std::size_t back = 1u;
            back <= colors0.size() && back <= colors1.size();
            ++back)
          {
            const tournament::Color color0 = colors0[colors0.size() - back];
            const tournament::Color color1 = colors1[colors1.size() - back];
            if (color0 != color1)
            {
              // The player who then had the other colour receives it now.
              return color0 != dueColor ? dueColor : invert(dueColor);
            }
          }
          const bool player0IsHigher = ranksHigher(player0, player1);
          const bool upper = isAbove(group) || isMedian(group);
          return
            player0IsHigher == upper ? dueColor : invert(dueColor);
        }

        /**
         * Article 2.1, with Article 6 for the last round.
         */
        bool compatible(const index player0, const index player1) const
        {
          if (player0 == player1)
          {
            return false;
          }
          const LimPlayer &limPlayer0 = players[player0];
          const LimPlayer &limPlayer1 = players[player1];
          if (
            limPlayer0.forbidden.count(limPlayer1.player->id)
              || limPlayer1.forbidden.count(limPlayer0.player->id))
          {
            return false;
          }
          if (relaxed || (lastRound && limPlayer0.score == limPlayer1.score))
          {
            return true;
          }
          return
            (limPlayer0.colorAllowed(tournament::COLOR_WHITE)
                && limPlayer1.colorAllowed(tournament::COLOR_BLACK))
              || (limPlayer0.colorAllowed(tournament::COLOR_BLACK)
                    && limPlayer1.colorAllowed(tournament::COLOR_WHITE));
        }

        bool colorClash(const index player0, const index player1) const
        {
          const tournament::Color due0 = players[player0].due();
          return due0 != tournament::COLOR_NONE && due0 == players[player1].due();
        }

        /**
         * The maximum number of pairs that can be made in the set.
         */
        vertex_index maxPairs(const index_list &set) const
        {
          if (set.size() < 2u)
          {
            return 0u;
          }
          FieldMatcher matcher(
            set.size(),
            { bitsFor(set.size()), 0u },
            [&](const vertex_index vertex0, const vertex_index vertex1)
            {
              return
                compatible(set[vertex0], set[vertex1])
                  ? weight_fields{ 1u, 0u }
                  : weight_fields{ };
            });
          vertex_index result{ };
          for (vertex_index vertex{ }; vertex < set.size(); ++vertex)
          {
            result += matcher.partner(vertex) > vertex;
          }
          return result;
        }

        /**
         * Return the first player of the list (in order) who can be removed
         * from the set while keeping the maximum number of pairs of the rest
         * as large as possible. The list must be included in the set.
         */
        index bestToRemove(
          const index_list &set,
          const index_list &ordered,
          const std::function<bool(index)> &eligible) const
        {
          // The last vertex stands for the removal.
          const vertex_index removal = set.size();
          std::vector<std::uintmax_t> incentives(set.size());
          for (std::size_t position{ }; position < ordered.size(); ++position)
          {
            if (eligible(ordered[position]))
            {
              incentives[
                  std::find(set.begin(), set.end(), ordered[position])
                    - set.begin()
                ] = ordered.size() - position;
            }
          }
          FieldMatcher matcher(
            set.size() + 1u,
            { bitsFor(set.size() + 1u), bitsFor(ordered.size()), 0u },
            [&](const vertex_index vertex0, const vertex_index vertex1)
            {
              if (vertex0 == removal || vertex1 == removal)
              {
                const vertex_index other = std::min(vertex0, vertex1);
                return
                  incentives[other]
                    ? weight_fields{ 1u, incentives[other], 0u }
                    : weight_fields{ };
              }
              return
                compatible(set[vertex0], set[vertex1])
                  ? weight_fields{ 1u, 0u, 0u }
                  : weight_fields{ };
            });
          const vertex_index partner = matcher.partner(removal);
          return partner == removal ? NO_INDEX : set[partner];
        }

        /**
         * Article 1.1: the lowest ranked player of the lowest score group,
         * among the players eligible for the bye (Basic Rules Article 3)
         * whose removal leaves a complete pairing possible.
         */
        index assignBye(const index_list &everybody)
        {
          relaxed = false;
          if (!(everybody.size() & 1u))
          {
            return NO_INDEX;
          }
          index_list candidates;
          for (const index player : everybody)
          {
            if (players[player].byeEligible)
            {
              candidates.push_back(player);
            }
          }
          std::sort(
            candidates.begin(),
            candidates.end(),
            [this](const index player0, const index player1)
            {
              return
                std::make_tuple(players[player0].score, rank(player1))
                  < std::make_tuple(players[player1].score, rank(player0));
            });
          for (const bool relax : { false, true })
          {
            relaxed = relax;
            const index bye =
              bestToRemove(
                everybody,
                candidates,
                [](index) { return true; });
            if (bye != NO_INDEX)
            {
              index_list rest;
              for (const index player : everybody)
              {
                if (player != bye)
                {
                  rest.push_back(player);
                }
              }
              if (2u * maxPairs(rest) == rest.size())
              {
                return bye;
              }
            }
          }
          throw NoValidPairingException(
            "No player can receive the pairing-allocated bye while allowing a "
              "complete pairing of the remaining players.");
        }

        /**
         * The players in the order of scrutiny: from player #1 down when
         * pairing downward, from the last player up when pairing upward
         * (Articles 4.1.1 and 4.1.2).
         */
        index_list byNumber(index_list list, const Direction direction) const
        {
          std::sort(
            list.begin(),
            list.end(),
            [this, direction](const index player0, const index player1)
            {
              return
                direction == DOWNWARD
                  ? rank(player0) < rank(player1)
                  : rank(player0) > rank(player1);
            });
          return list;
        }

        /**
         * Pair the floaters coming from another score group first (Articles
         * 3.6 and 3.7), each with the opponent of Article 3.8 among those
         * keeping the largest number of pairs in the rest of the group. A
         * floater without a compatible opponent is returned (Article 3.5).
         * The players of later lists are part of the rest of the group.
         */
        std::vector<Floater> pairFloaters(
          std::vector<Floater> floaters,
          index_list &pool,
          const Direction direction,
          const std::vector<Floater> &later,
          const bool unpairedStay,
          const tournament::points group,
          std::vector<Pair> &pairs) const
        {
          std::sort(
            floaters.begin(),
            floaters.end(),
            [this, direction](const Floater &floater0, const Floater &floater1)
            {
              return
                direction == DOWNWARD
                  ? std::make_tuple(floater1.source, rank(floater0.player))
                      < std::make_tuple(floater0.source, rank(floater1.player))
                  : std::make_tuple(floater0.source, rank(floater1.player))
                      < std::make_tuple(floater1.source, rank(floater0.player));
            });
          std::vector<Floater> unpaired;
          for (std::size_t position{ }; position < floaters.size(); ++position)
          {
            const index floater = floaters[position].player;
            index_list set{ floater };
            const std::size_t poolSize = pool.size();
            set.insert(set.end(), pool.begin(), pool.end());
            for (std::size_t other = position + 1u; other < floaters.size(); ++other)
            {
              set.push_back(floaters[other].player);
            }
            for (const Floater &other : later)
            {
              set.push_back(other.player);
            }
            if (unpairedStay)
            {
              for (const Floater &other : unpaired)
              {
                set.push_back(other.player);
              }
            }

            // Article 3.8: the first player available, in the order of
            // numbers of the direction, who is due the alternate colour; then
            // the others in the same order.
            const index_list ordered = byNumber(pool, direction);
            std::vector<std::uintmax_t> incentives(poolSize);
            std::uintmax_t incentive = 2u * ordered.size();
            for (const bool alternate : { true, false })
            {
              for (const index candidate : ordered)
              {
                const bool isAlternate =
                  players[floater].due() == tournament::COLOR_NONE
                    || players[candidate].due() == tournament::COLOR_NONE
                    || players[candidate].due() != players[floater].due();
                if (isAlternate == alternate)
                {
                  incentives[
                    std::find(pool.begin(), pool.end(), candidate)
                      - pool.begin()
                  ] = incentive--;
                }
              }
            }
            // Outside the Median Scoregroup, the floater is paired in
            // priority (Article 3.9.2: avoid the floaters of types a and b),
            // then the most pairs are kept. In the Median Scoregroup, which
            // must be completely paired, the most pairs come first, and a
            // floater may stay in the group to be paired with another one.
            FieldMatcher matcher(
              set.size(),
              {
                unpairedStay ? bitsFor(set.size()) : 1u,
                unpairedStay ? 1u : bitsFor(set.size()),
                bitsFor(2u * ordered.size()),
                0u
              },
              [&](const vertex_index vertex0, const vertex_index vertex1)
              {
                const vertex_index low = std::min(vertex0, vertex1);
                const vertex_index high = std::max(vertex0, vertex1);
                if (!compatible(set[low], set[high]))
                {
                  return weight_fields{ };
                }
                if (low == 0u && high <= poolSize)
                {
                  return weight_fields{ 1u, 1u, incentives[high - 1u], 0u };
                }
                if (low == 0u && !unpairedStay)
                {
                  return weight_fields{ };
                }
                return
                  unpairedStay
                    ? weight_fields{ 1u, 0u, 0u, 0u }
                    : weight_fields{ 0u, 1u, 0u, 0u };
              });
            const vertex_index partner = matcher.partner(0u);
            if (partner == 0u || partner > poolSize)
            {
              unpaired.push_back(floaters[position]);
              continue;
            }
            const index opponent = set[partner];
            pool.erase(std::find(pool.begin(), pool.end(), opponent));
            pairs.push_back(Pair{ floater, opponent, group });
          }
          return unpaired;
        }

        /**
         * Articles 2.3.1 to 2.3.3: the players without a compatible opponent
         * in the score group are transferred.
         */
        index_list transfer(index_list &pool) const
        {
          index_list result;
          index_list kept;
          for (const index player : pool)
          {
            bool hasOpponent = false;
            for (const index other : pool)
            {
              if (compatible(player, other))
              {
                hasOpponent = true;
                break;
              }
            }
            (hasOpponent ? kept : result).push_back(player);
          }
          pool = std::move(kept);
          return result;
        }

        /**
         * Article 2.3.4: choose the floater that makes the score group even.
         * In priority: keep the most pairs in the group (3.10.2), a player who
         * has a compatible opponent in the adjacent score group (3.9), who did
         * not float in the previous round (3.10), who tends to equalise the
         * due colours (3.2.2), then the lowest numbered player when pairing
         * downward, the highest numbered when pairing upward (3.2.4, 3.3).
         */
        index evennessFloater(
          const index_list &pool,
          const index_list &adjacent,
          const Direction direction) const
        {
          std::size_t dueWhite{ };
          std::size_t dueBlack{ };
          for (const index player : pool)
          {
            dueWhite += players[player].due() == tournament::COLOR_WHITE;
            dueBlack += players[player].due() == tournament::COLOR_BLACK;
          }
          index_list ordered = byNumber(pool, direction);
          std::reverse(ordered.begin(), ordered.end());
          typedef std::tuple<bool, bool, bool, std::size_t> key;
          std::vector<std::pair<key, index>> keyed;
          for (std::size_t position{ }; position < ordered.size(); ++position)
          {
            const index player = ordered[position];
            const tournament::Color due = players[player].due();
            const bool equalises =
              dueWhite == dueBlack
                || (dueWhite > dueBlack && due == tournament::COLOR_WHITE)
                || (dueBlack > dueWhite && due == tournament::COLOR_BLACK);
            bool hasAdjacent = false;
            for (const index other : adjacent)
            {
              if (compatible(player, other))
              {
                hasAdjacent = true;
                break;
              }
            }
            keyed.emplace_back(
              key(
                !hasAdjacent,
                players[player].floatedPreviousRound,
                !equalises,
                position),
              player);
          }
          std::sort(keyed.begin(), keyed.end());
          index_list priority;
          for (const auto &entry : keyed)
          {
            priority.push_back(entry.second);
          }
          return bestToRemove(pool, priority, [](index) { return true; });
        }

        /**
         * Articles 2.4, 2.5 and 4: scrutinise the proposed pairings, the
         * upper half against the lower half, exchanging opponents in the order
         * of Articles 4.2 and 4.3. A player who cannot be paired while keeping
         * the most pairs floats (Article 4.4). evenness is the floater of
         * Article 2.3.4, set to NO_INDEX if they come back (Article 4.4.1).
         */
        std::vector<std::pair<index, index>> scrutinise(
          const index_list &pool,
          const Direction direction,
          index &evenness,
          index_list &floated) const
        {
          const index_list list = byNumber(pool, direction);
          const std::size_t half = list.size() / 2u;
          std::vector<bool> upper(players.size());
          for (std::size_t position{ }; position < half; ++position)
          {
            upper[list[position]] = true;
          }
          index_list remaining = list;
          std::vector<std::pair<index, index>> result;
          while (!remaining.empty())
          {
            const index player = remaining.front();
            // The exchange order: the players of the lower half in order, then
            // the players of the upper half in reverse order.
            index_list candidates;
            for (const index other : remaining)
            {
              if (other != player && !upper[other])
              {
                candidates.push_back(other);
              }
            }
            for (auto other = remaining.rbegin(); other != remaining.rend(); ++other)
            {
              if (*other != player && upper[*other])
              {
                candidates.push_back(*other);
              }
            }
            // Article 5.2: the second scrutiny exchanges opponents, in the same
            // order, to give both players their due colours where possible.
            std::vector<std::uintmax_t> incentives(players.size());
            std::uintmax_t incentive = candidates.size();
            for (const bool grantsColors : { true, false })
            {
              for (const index candidate : candidates)
              {
                if (!colorClash(player, candidate) == grantsColors)
                {
                  incentives[candidate] = incentive--;
                }
              }
            }
            FieldMatcher matcher(
              remaining.size(),
              { bitsFor(remaining.size()), bitsFor(candidates.size()), 0u },
              [&](const vertex_index vertex0, const vertex_index vertex1)
              {
                if (!compatible(remaining[vertex0], remaining[vertex1]))
                {
                  return weight_fields{ };
                }
                const std::uintmax_t bonus =
                  vertex0 == 0u ? incentives[remaining[vertex1]]
                    : vertex1 == 0u ? incentives[remaining[vertex0]]
                    : 0u;
                return weight_fields{ 1u, bonus, 0u };
              });
            const vertex_index partner = matcher.partner(0u);
            if (partner != 0u)
            {
              const index opponent = remaining[partner];
              result.emplace_back(player, opponent);
              remaining.erase(
                std::find(remaining.begin(), remaining.end(), opponent));
              remaining.erase(remaining.begin());
              continue;
            }

            // Article 4.4: the player floats.
            remaining.erase(remaining.begin());
            floated.push_back(player);
            if (evenness != NO_INDEX)
            {
              // Article 4.4.1: exchanged with the floater of Article 2.3.4.
              remaining.push_back(evenness);
              remaining = byNumber(remaining, direction);
              evenness = NO_INDEX;
              continue;
            }
            // Article 4.4.2: the lowest numbered player remaining floats too.
            const index_list reversed(remaining.rbegin(), remaining.rend());
            const index companion =
              bestToRemove(remaining, reversed, [](index) { return true; });
            assert(companion != NO_INDEX);
            remaining.erase(
              std::find(remaining.begin(), remaining.end(), companion));
            floated.push_back(companion);
          }
          return result;
        }

        /**
         * Pair a score group other than the Median Scoregroup. Return the
         * floaters leaving it.
         */
        std::vector<Floater> pairGroup(
          const tournament::points score,
          const index_list &residents,
          const std::vector<Floater> &incoming,
          const Direction direction,
          const index_list &adjacent,
          std::vector<Pair> &pairs) const
        {
          index_list pool = residents;
          std::vector<Floater> outgoing =
            pairFloaters(incoming, pool, direction, { }, false, score, pairs);
          for (const index player : transfer(pool))
          {
            outgoing.push_back(Floater{ player, score });
          }
          index evenness = NO_INDEX;
          if (pool.size() & 1u)
          {
            evenness = evennessFloater(pool, adjacent, direction);
            pool.erase(std::find(pool.begin(), pool.end(), evenness));
          }
          index_list floated;
          auto made = scrutinise(pool, direction, evenness, floated);
          if (evenness != NO_INDEX)
          {
            outgoing.push_back(Floater{ evenness, score });
          }
          for (const index player : floated)
          {
            outgoing.push_back(Floater{ player, score });
          }
          for (const auto &pair : made)
          {
            pairs.push_back(Pair{ pair.first, pair.second, score });
          }
          return outgoing;
        }

        /**
         * Pair the Median Scoregroup, last and downward (Article 2.2): the
         * downfloaters first, then the upfloaters, then the remaining players
         * (Article 3.6.3). Return false if it cannot be paired.
         */
        bool pairMedian(
          const tournament::points score,
          const index_list &residents,
          const std::vector<Floater> &downfloaters,
          const std::vector<Floater> &upfloaters,
          std::vector<Pair> &pairs) const
        {
          index_list pool = residents;
          const std::vector<Floater> unpairedDown =
            pairFloaters(
              downfloaters, pool, DOWNWARD, upfloaters, true, score, pairs);
          const std::vector<Floater> unpairedUp =
            pairFloaters(
              upfloaters, pool, UPWARD, unpairedDown, true, score, pairs);
          for (const Floater &floater : unpairedDown)
          {
            pool.push_back(floater.player);
          }
          for (const Floater &floater : unpairedUp)
          {
            pool.push_back(floater.player);
          }
          if (pool.size() & 1u)
          {
            return false;
          }
          index evenness = NO_INDEX;
          index_list floated;
          auto made = scrutinise(pool, DOWNWARD, evenness, floated);
          if (!floated.empty())
          {
            return false;
          }
          for (const auto &pair : made)
          {
            pairs.push_back(Pair{ pair.first, pair.second, score });
          }
          return true;
        }

        std::vector<Pair> pairRemaining(const index_list &remaining) const
        {
          // The score groups, in descending order of score.
          std::vector<tournament::points> scores;
          for (const index player : remaining)
          {
            scores.push_back(players[player].score);
          }
          std::sort(scores.begin(), scores.end());
          scores.erase(std::unique(scores.begin(), scores.end()), scores.end());
          std::reverse(scores.begin(), scores.end());
          const auto residentsOf =
            [&](const tournament::points score)
            {
              index_list result;
              for (const index player : remaining)
              {
                if (players[player].score == score)
                {
                  result.push_back(player);
                }
              }
              return byNumber(result, DOWNWARD);
            };
          const auto adjacentTo =
            [&](const std::size_t position, const Direction direction)
            {
              if (direction == DOWNWARD)
              {
                return
                  position + 1u < scores.size()
                    ? residentsOf(scores[position + 1u])
                    : index_list{ };
              }
              return
                position ? residentsOf(scores[position - 1u]) : index_list{ };
            };

          // Article 2.2: from the highest score group down to just before the
          // Median Scoregroup, then from the lowest one up, the Median
          // Scoregroup last.
          std::vector<std::vector<Pair>> upperPairs;
          std::vector<Floater> downfloaters;
          std::size_t position{ };
          for (; position < scores.size() && isAbove(scores[position]); ++position)
          {
            upperPairs.emplace_back();
            downfloaters =
              pairGroup(
                scores[position],
                residentsOf(scores[position]),
                downfloaters,
                DOWNWARD,
                adjacentTo(position, DOWNWARD),
                upperPairs.back());
          }
          std::vector<std::vector<Pair>> lowerPairs;
          std::vector<Floater> upfloaters;
          for (
            std::size_t lower = scores.size();
            lower > position && !isMedian(scores[lower - 1u]);
            --lower)
          {
            lowerPairs.emplace_back();
            upfloaters =
              pairGroup(
                scores[lower - 1u],
                residentsOf(scores[lower - 1u]),
                upfloaters,
                UPWARD,
                adjacentTo(lower - 1u, UPWARD),
                lowerPairs.back());
          }

          // The Median Scoregroup (possibly empty, then made of the floaters).
          const tournament::points medianScore =
            static_cast<tournament::points>(doubledMedian / 2u);
          const index_list medianResidents =
            position < scores.size() && isMedian(scores[position])
              ? residentsOf(scores[position])
              : index_list{ };
          std::vector<Pair> medianPairs;
          while (
            !pairMedian(
              medianScore,
              medianResidents,
              downfloaters,
              upfloaters,
              medianPairs))
          {
            medianPairs.clear();
            // Article 2.6: crack the next pairing of the lower score group
            // if more floaters come from above (2.6.1), of the higher one
            // otherwise (2.6.2).
            const auto crack =
              [](std::vector<std::vector<Pair>> &groups, Pair &cracked)
              {
                for (auto group = groups.rbegin(); group != groups.rend(); ++group)
                {
                  if (!group->empty())
                  {
                    cracked = group->back();
                    group->pop_back();
                    return true;
                  }
                }
                return false;
              };
            Pair cracked{ };
            bool fromLower = downfloaters.size() > upfloaters.size();
            if (!crack(fromLower ? lowerPairs : upperPairs, cracked))
            {
              fromLower = !fromLower;
              if (!crack(fromLower ? lowerPairs : upperPairs, cracked))
              {
                throw NoLimPairing();
              }
            }
            std::vector<Floater> &target =
              fromLower ? upfloaters : downfloaters;
            target.push_back(
              Floater{ cracked.player0, players[cracked.player0].score });
            target.push_back(
              Floater{ cracked.player1, players[cracked.player1].score });
          }

          std::vector<Pair> result;
          for (const auto &group : upperPairs)
          {
            result.insert(result.end(), group.begin(), group.end());
          }
          for (const auto &group : lowerPairs)
          {
            result.insert(result.end(), group.begin(), group.end());
          }
          result.insert(result.end(), medianPairs.begin(), medianPairs.end());
          return result;
        }
      };

      /**
       * Compute the Lim-specific data of the player.
       */
      LimPlayer makeLimPlayer(
        const tournament::Player &player,
        const tournament::Tournament &tournament,
        const std::unordered_set<tournament::player_index> &forbidden)
      {
        LimPlayer result{ };
        result.player = &player;
        result.score = player.scoreWithAcceleration(tournament);
        result.forbidden = forbidden;
        for (
          tournament::round_index roundIndex{ };
          roundIndex < tournament.playedRounds
            && roundIndex < player.matches.size();
          ++roundIndex)
        {
          const tournament::Match &match = player.matches[roundIndex];
          if (match.gameWasPlayed)
          {
            result.colors.push_back(match.color);
            result.colorDifference +=
              match.color == tournament::COLOR_WHITE ? 1 : -1;
            result.forbidden.insert(match.opponent);
          }
        }
        if (
          tournament.playedRounds
            && tournament.playedRounds <= player.matches.size())
        {
          const tournament::Match &match =
            player.matches[tournament.playedRounds - 1u];
          if (match.gameWasPlayed)
          {
            const tournament::Player &opponent =
              tournament.players[match.opponent];
            result.floatedPreviousRound =
              opponent.scoreWithAcceleration(tournament, 1u)
                != player.scoreWithAcceleration(tournament, 1u);
          }
        }
        result.byeEligible = eligibleForBye(player, tournament);
        return result;
      }

      /**
       * Print the checklist file for the current round to ostream.
       */
      void printChecklist(
        const tournament::Tournament &tournament,
        const std::vector<LimPlayer> &players,
        std::ostream &ostream,
        const std::vector<const tournament::Player *> *const matching = nullptr,
        const std::vector<tournament::Color> *const colors = nullptr)
      {
        std::vector<const LimPlayer *> playersById(tournament.players.size());
        std::vector<const tournament::Player *> sortedPlayers;
        for (const LimPlayer &player : players)
        {
          playersById[player.player->id] = &player;
          sortedPlayers.push_back(player.player);
        }
        const auto colorString =
          [](const tournament::Color color)
          {
            return
              color == tournament::COLOR_WHITE ? std::string("W")
                : color == tournament::COLOR_BLACK ? std::string("B")
                : std::string();
          };
        swisssystems::printChecklist(
          ostream,
          std::deque<std::string>{ "C2", "Due", "Must", "FlP", "Cur" },
          [&](const tournament::Player &player)
          {
            const LimPlayer &limPlayer = *playersById[player.id];
            const tournament::Player *const opponent =
              matching ? (*matching)[player.id] : nullptr;
            return std::deque<std::string>{
              limPlayer.byeEligible ? "Y" : "N",
              colorString(limPlayer.due()),
              colorString(limPlayer.imposed()),
              limPlayer.floatedPreviousRound ? "Y" : "N",
              matching
                ? opponent && opponent != &player
                    ? '('
                        + utility::uintstringconversion
                            ::toString(opponent->id + 1u)
                        + colorString((*colors)[player.id])
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
      // Filter out the absent players, and sort the remainder by pairing
      // number. Players who already met over-the-board may not meet again
      // (Basic Rules Article 2; GHR Article 3.5).
      std::vector<const tournament::Player *> sortedPlayers;
      const auto forbiddenPairs =
        tournament.resolveForbiddenPairs(tournament.playedRounds);
      for (tournament::Player &player : tournament.players)
      {
        if (player.isValid && player.matches.size() <= tournament.playedRounds)
        {
          sortedPlayers.push_back(&player);
        }
      }
      std::sort(
        sortedPlayers.begin(),
        sortedPlayers.end(),
        [](
          const tournament::Player *const player0,
          const tournament::Player *const player1)
        {
          return player0->rankIndex < player1->rankIndex;
        });

      std::vector<LimPlayer> players;
      for (const tournament::Player *const player : sortedPlayers)
      {
        players.push_back(
          makeLimPlayer(*player, tournament, forbiddenPairs[player->id]));
      }
      const std::vector<LimPlayer> checklistPlayers = players;

      std::list<Pairing> result;
      std::vector<const tournament::Player *>
        matchingById(tournament.players.size());
      std::vector<tournament::Color> colorsById(tournament.players.size());
      try
      {
        LimPairer pairer(tournament, std::move(players));
        index bye;
        std::vector<Pair> pairs;
        try
        {
          pairs = pairer.pair(bye);
        }
        catch (const NoLimPairing &)
        {
          throw NoValidPairingException(
            "The Median Scoregroup could not be paired, even after cracking "
              "every pairing of the other score groups.");
        }
        if (bye != NO_INDEX)
        {
          const tournament::player_index id = sortedPlayers[bye]->id;
          result.emplace_back(id, id);
          matchingById[id] = sortedPlayers[bye];
        }
        for (const Pair &pair : pairs)
        {
          const tournament::Player &player0 = *sortedPlayers[pair.player0];
          const tournament::Player &player1 = *sortedPlayers[pair.player1];
          const tournament::Color color0 =
            pairer.chooseColor(pair.player0, pair.player1, pair.group);
          matchingById[player0.id] = &player1;
          matchingById[player1.id] = &player0;
          colorsById[player0.id] = color0;
          colorsById[player1.id] = invert(color0);
          result.emplace_back(player0.id, player1.id, color0);
        }
      }
      catch (const NoValidPairingException &)
      {
        if (ostream)
        {
          printChecklist(tournament, checklistPlayers, *ostream);
        }
        throw;
      }

      if (ostream)
      {
        printChecklist(
          tournament,
          checklistPlayers,
          *ostream,
          &matchingById,
          &colorsById);
      }

      return result;
    }
  }
}
#endif
