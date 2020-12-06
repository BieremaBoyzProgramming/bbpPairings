#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <list>
#include <ostream>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include <swisssystems/common.h>
#include <utility/random.h>
#include <utility/typesizes.h>
#include <utility/uintfloatconversion.h>
#include <utility/uintstringconversion.h>
#include <utility/uinttypes.h>

#include "generator.h"
#include "tournament.h"

#ifndef OMIT_GENERATOR
namespace tournament
{
  namespace generator
  {
    /**
     * Generate players and ratings.
     */
    template <class RandomEngine>
    MatchesConfiguration::MatchesConfiguration(
        Configuration &&configuration,
        RandomEngine &randomEngine)
      : MatchesConfiguration(std::move(configuration))
    {
      if (configuration.highestRating < configuration.lowestRating)
      {
        throw BadConfigurationException(
          "The highest rating must be higher than the lowest rating.");
      }

      std::list<rating> ratingsList;
      for (
        player_index playerIndex = 0;
        playerIndex < configuration.playersNumber;
        ++playerIndex)
      {
        ratingsList.push_back(
          utility::random::uniformUint(
            randomEngine,
            configuration.lowestRating,
            configuration.highestRating));
      }
      ratingsList.sort();

      for (
        decltype(ratingsList)::const_reverse_iterator iterator =
          ratingsList.rbegin();
        iterator != ratingsList.rend();
        ++iterator)
      {
        tournament.players
          .emplace_back(tournament.players.size(), 0, *iterator);
        tournament.playersByRank.push_back(
          tournament.playersByRank.size());
      }
    }

    template
    MatchesConfiguration::MatchesConfiguration(
      Configuration &&,
      std::minstd_rand &);

    namespace
    {
      constexpr unsigned int gameIndexSize =
        utility::typesizes
            ::bitsToRepresent<unsigned int>(maxPlayers)
          + utility::typesizes
              ::bitsToRepresent<unsigned int>(maxRounds);
      static_assert(
        gameIndexSize
          >= utility::typesizes
              ::bitsToRepresent<unsigned int>(maxPlayers),
        "Overflow");
      constexpr unsigned int scaledGameIndexSize =
        gameIndexSize + utility::typesizes::bitsToRepresent<unsigned int>(100u);
    }
    static_assert(scaledGameIndexSize >= gameIndexSize, "Overflow");
    /**
     * A type large enough to support the total number of matches played in the
     * tournament.
     */
    typedef utility::uinttypes::uint_least<scaledGameIndexSize> game_index;

    /**
     * Compute the configuration parameters of the tournament, while reusing the
     * players and ratings from that tournament.
     */
    MatchesConfiguration::MatchesConfiguration(
        Tournament &&tournament_)
      : tournament(std::move(tournament_)),
        roundsNumber(tournament_.playedRounds)
    {
      player_index retiredPlayers{ };
      player_index halfPointByePlayers{ };
      game_index forfeitGames{ };
      game_index drawnGames{ };

      game_index scheduledGames{ };
      game_index playedGames{ };

      for (const player_index playerIndex : tournament.playersByRank)
      {
        Player &player = tournament.players[playerIndex];

        bool zeroPointBye{ };
        bool halfPointBye{ };

        round_index matchIndex{ };
        for (const Match &match : player.matches)
        {
          if (match.participatedInPairing && match.opponent != player.id)
          {
            ++scheduledGames;
            if (!scheduledGames)
            {
              assert(
                tournament.playedRounds > maxRounds
                  || tournament.players.size() > maxPlayers);
              throw BuildLimitExceededException(
                "This build supports at most "
                  + (tournament.playedRounds > maxRounds
                      ? utility::uintstringconversion::toString(maxRounds)
                          + " rounds."
                      : utility::uintstringconversion::toString(maxPlayers)
                          + " players."));
            }

            if (!match.gameWasPlayed)
            {
              ++forfeitGames;
              assert(forfeitGames);
            }
          }

          if (match.gameWasPlayed)
          {
            ++playedGames;
            assert(playedGames);

            if (match.matchScore == MATCH_SCORE_DRAW)
            {
              ++drawnGames;
              assert(drawnGames);
            }
          }

          if (!match.participatedInPairing)
          {
            if (match.matchScore == MATCH_SCORE_DRAW)
            {
              halfPointBye = true;
            }
            else if (match.matchScore == MATCH_SCORE_LOSS)
            {
              zeroPointBye = true;
            }
          }
          ++matchIndex;
        }

        if (zeroPointBye)
        {
          ++retiredPlayers;
          assert(retiredPlayers);
        }
        if (halfPointBye)
        {
          ++halfPointByePlayers;
          assert(halfPointByePlayers);
        }

        player.matches.clear();
        player.scoreWithoutAcceleration = 0;
      }

      constexpr float maxFloat =
        std::numeric_limits<float>::has_infinity
          ? std::numeric_limits<float>::infinity()
          : std::numeric_limits<float>::max();
      retiredRate =
        retiredPlayers
          ? utility::uintfloatconversion::divide<float>(
              tournament.playersByRank.size(),
              retiredPlayers)
          : maxFloat;
      halfPointByeRate =
        halfPointByePlayers
          ? utility::uintfloatconversion::divide<float>(
              tournament.playersByRank.size(),
              halfPointByePlayers)
          : maxFloat;
      forfeitRate =
        forfeitGames
          ? utility::uintfloatconversion
              ::divide<float>(scheduledGames, forfeitGames)
          : maxFloat;
      drawPercentage =
        (decltype(drawPercentage))
          (playedGames ? drawnGames * 100u / playedGames : playedGames);

      tournament.playedRounds = 0;
    }

    namespace
    {
      /**
       * Divide playerCount by rate, using as much precision as is offered by
       * float.
       */
      player_index applyRate(
        const player_index playerCount,
        const float rate)
      {
        const float result =
          utility::uintfloatconversion::multiply(playerCount, 1.f / rate);
        if (
          utility::uintfloatconversion
              ::bigger_float_or_uint<float, player_index>(result)
            >= playerCount)
        {
          return playerCount;
        }
        else
        {
          return result;
        }
      }
    }

    /**
     * Generate a tournament using the provided players and configuration
     * options.
     */
    template <class RandomEngine>
    void generateTournament(
      Tournament &result,
      MatchesConfiguration &&configuration,
      const swisssystems::SwissSystem swissSystem,
      RandomEngine &randomEngine,
      std::ostream *checklistStream)
    {
      result = std::move(configuration.tournament);

      result.initialColor =
        std::uniform_int_distribution<unsigned char>(false, true)(randomEngine)
          ? COLOR_BLACK
          : COLOR_WHITE;

      /**
       * A vector indicating how many games for each player will be zero-point
       * byes.
       */
      std::vector<round_index> zeroPointByeCounts(
        result.playersByRank.size());
      /**
       * A vector indicating how many games for each player will be half-point
       * byes.
       */
      std::vector<round_index> halfPointByeCounts(
        result.playersByRank.size());
      if (zeroPointByeCounts.size() < result.playersByRank.size())
      {
        throw std::length_error("");
      }

      /**
       * The maximum number of rounds a player can be given a zero-point bye.
       */
      const round_index initialRemainingCount =
        configuration.roundsNumber < 2u ? 0u
          : configuration.roundsNumber < 3u ? 1u
          : configuration.roundsNumber - 2u;
      /**
       * A vector indicating how many more byes of the current kind a player may
       * receive.
       */
      std::vector<round_index> remainingCounts(
        result.playersByRank.size(),
        initialRemainingCount);

      /**
       * The number of players without byes of the current kind who need to
       * receive one.
       */
      player_index remainingPlayers =
        applyRate(
          result.playersByRank.size(),
          configuration.retiredRate);
      /**
       * The number of games that can be used as byes of the current kind.
       */
      game_index eligibleGames =
        game_index{ initialRemainingCount }
          * result.playersByRank.size();
      if (
        initialRemainingCount
          && eligibleGames / initialRemainingCount < result.playersByRank.size()
      )
      {
        assert(
          initialRemainingCount > maxRounds
            || result.players.size() > maxPlayers);
        throw BuildLimitExceededException(
          "This build supports at most "
            + (initialRemainingCount > maxRounds
                ? utility::uintstringconversion::toString(maxRounds)
                    + " rounds."
                : utility::uintstringconversion::toString(maxPlayers)
                    + " players."));
      }
      /**
       * Until enough people have received a zero-point bye,
       */
      while (initialRemainingCount && remainingPlayers)
      {
        assert(eligibleGames);
        /**
         * Pick a random game from the remaining eligible games.
         */
        game_index gameIndex =
          utility::random::uniformUint(randomEngine, 0u, eligibleGames - 1u);
        // Determine which player the game belongs to.
        player_index playerIndex{ };
        for (
          const round_index playerEligibleGames
            : remainingCounts)
        {
          if (gameIndex < playerEligibleGames)
          {
            break;
          }
          gameIndex -= playerEligibleGames;
          ++playerIndex;
        }
        --eligibleGames;
        --remainingCounts[playerIndex];
        if (!zeroPointByeCounts[playerIndex]++)
        {
          --remainingPlayers;
        }
      }

      if (configuration.roundsNumber > 2u)
      {
        for (round_index &playerEligibleGames : remainingCounts)
        {
          ++playerEligibleGames;
        }
        eligibleGames += result.playersByRank.size();
        if (eligibleGames < result.playersByRank.size())
        {
          assert(
            initialRemainingCount >= maxRounds
              || result.players.size() > maxPlayers);
          throw BuildLimitExceededException(
            "This build supports at most "
              + (initialRemainingCount >= maxRounds
                  ? utility::uintstringconversion::toString(maxRounds)
                      + " rounds."
                  : utility::uintstringconversion::toString(maxPlayers)
                      + " players."));
        }
      }
      remainingPlayers =
        applyRate(
          result.playersByRank.size(),
          configuration.halfPointByeRate);

      // Until enough people have received a half-point bye or no more can
      // receive one,
      while (remainingPlayers && eligibleGames)
      {
        // Choose a random game from the eligible remaining games.
        game_index gameIndex =
          utility::random::uniformUint(randomEngine, 0, eligibleGames - 1u);

        // Determine which player the game belongs to.
        player_index playerIndex{ };
        for (
          const round_index playerEligibleGames
            : remainingCounts)
        {
          if (gameIndex < playerEligibleGames)
          {
            break;
          }
          gameIndex -= playerEligibleGames;
          ++playerIndex;
        }
        --eligibleGames;
        assert(playerIndex < result.playersByRank.size());
        --remainingCounts[playerIndex];
        if (!halfPointByeCounts[playerIndex]++)
        {
          --remainingPlayers;
        }
      }

      // Compute the matchings for each round of the tournament, and generate
      // the results.
      for (
        result.playedRounds = 0;
        result.playedRounds < configuration.roundsNumber;
        ++result.playedRounds)
      {
        player_index rankIndex{ };
        for (const player_index playerIndex : result.playersByRank)
        {
          Player &player = result.players[playerIndex];
          // Choose whether the player should receive a bye this round.
          if (
            result.playedRounds
                < configuration.roundsNumber - 1u
              && utility::random::uniformUint(
                    randomEngine,
                    0u,
                    configuration.roundsNumber
                      - result.playedRounds
                      - 2u
                  ) < halfPointByeCounts[rankIndex])
          {
            player.matches.emplace_back(
              player.id,
              COLOR_NONE,
              MATCH_SCORE_DRAW,
              false,
              false);
            --halfPointByeCounts[rankIndex];
          }
          else if (
            utility::random::uniformUint(
              randomEngine,
              0u,
              configuration.roundsNumber
                - result.playedRounds
                - halfPointByeCounts[rankIndex]
                - 1u
            ) < zeroPointByeCounts[rankIndex]
          )
          {
            player.matches.emplace_back(
              player.id,
              COLOR_NONE,
              MATCH_SCORE_LOSS,
              false,
              false);
            --zeroPointByeCounts[rankIndex];
          }
          ++rankIndex;
        }

        if (checklistStream)
        {
          *checklistStream << "Round #"
            << utility::uintstringconversion
                 ::toString(result.playedRounds + 1u)
            << '.'
            << std::endl;
        }

        // Compute the matching.

        result.updateRanks();
        result.computePlayerData();
        if (result.defaultAcceleration)
        {
          swisssystems::getInfo(swissSystem)
            .updateAccelerations(result, result.playedRounds);
        }

        std::list<swisssystems::Pairing> matching;
        try
        {
          matching =
            swisssystems::getInfo(swissSystem).computeMatching(
              Tournament(result),
              checklistStream);
        }
        catch (const swisssystems::NoValidPairingException &exception)
        {
          throw
            swisssystems::NoValidPairingException(
              "No valid pairing exists for round "
                + utility::uintstringconversion::toString(
                    result.playedRounds + 1u)
                + " of the generated tournament: "
                + exception.what());
        }

        // Generate the game results.
        for (const swisssystems::Pairing &pair : matching)
        {
          assert(
            result.players[pair.white].isValid
              && result.players[pair.white].matches.size()
                  <= result.playedRounds);
          assert(
            result.players[pair.black].isValid
              && result.players[pair.black].matches.size()
                  <= result.playedRounds);
          if (pair.white == pair.black)
          {
            result.players[pair.white].matches.emplace_back(
              pair.white,
              COLOR_NONE,
              MATCH_SCORE_WIN,
              false,
              true);
          }
          else
          {
            const float nonForfeitProbability =
              std::sqrt(1.f - 1.f / configuration.forfeitRate);
            MatchScore resultForWhite =
              std::uniform_real_distribution<float>(0, 1)(randomEngine)
                  >= nonForfeitProbability
                ? MATCH_SCORE_LOSS
                : MATCH_SCORE_WIN;
            MatchScore resultForBlack =
              std::uniform_real_distribution<float>(0, 1)(randomEngine)
                  >= nonForfeitProbability
                ? MATCH_SCORE_LOSS
                : MATCH_SCORE_WIN;
            bool forfeit =
              resultForWhite == MATCH_SCORE_LOSS
                || resultForBlack == MATCH_SCORE_LOSS;
            if (!forfeit)
            {
              const Color strongerPlayer =
                result.players[pair.black].rating
                    > result.players[pair.white].rating
                  ? COLOR_BLACK
                  : COLOR_WHITE;
              const rating ratingDifference =
                strongerPlayer == COLOR_BLACK
                  ? result.players[pair.black].rating
                      - result.players[pair.white].rating
                  : result.players[pair.white].rating
                      - result.players[pair.black].rating;
              const float expectedValueOfResult =
                std::erfc(
                  utility::uintfloatconversion
                      ::safeCastToFloat<float>(ratingDifference)
                    * (-7.f / std::sqrt(2.f) / 2000.f)
                ) / 2.f;
              float drawProbability =
                std::min(
                  configuration.drawPercentage / 100.f,
                  2.f - expectedValueOfResult * 2.f);
              float randomValue =
                std::uniform_real_distribution<float>(0, 1)(randomEngine);
              if (randomValue < drawProbability)
              {
                resultForWhite = MATCH_SCORE_DRAW;
              }
              else
              {
                resultForWhite =
                  (randomValue < expectedValueOfResult + drawProbability / 2.f)
                      ^ (strongerPlayer == COLOR_BLACK)
                    ? MATCH_SCORE_WIN
                    : MATCH_SCORE_LOSS;
              }
              resultForBlack = invert(resultForWhite);
            }
            result.players[pair.white].matches.emplace_back(
              pair.black,
              COLOR_WHITE,
              resultForWhite,
              !forfeit,
              true);
            result.players[pair.black].matches.emplace_back(
              pair.white,
              COLOR_BLACK,
              resultForBlack,
              !forfeit,
              true);
          }
        }

        // Update players' scores.
        for (const player_index playerIndex : result.playersByRank)
        {
          Player &player = result.players[playerIndex];

          const points newPoints =
            result.getPoints(player, player.matches.back());
          player.scoreWithoutAcceleration += newPoints;
          if (player.scoreWithoutAcceleration < newPoints)
          {
            throw BuildLimitExceededException(
              "This build only supports scores up to "
                + utility::uintstringconversion
                    ::toString(tournament::maxPoints, 1)
                + '.');
          }
        }
      }
    }

    template
    void generateTournament<std::minstd_rand>(
      Tournament &,
      MatchesConfiguration &&,
      swisssystems::SwissSystem,
      std::minstd_rand &,
      std::ostream *);
  }
}
#endif
