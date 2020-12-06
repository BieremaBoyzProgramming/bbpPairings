#include <algorithm>
#include <codecvt>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <tournament/tournament.h>
#include <utility/tokenizer.h>
#include <utility/uintstringconversion.h>

#include "trf.h"
#include "types.h"

namespace fileformats
{
  namespace trf
  {
    namespace
    {
      /**
       * Retrieve the single white-space-delimited token in the string.
       */
      template <typename T>
      T getSingleValue(const T &string)
      {
        utility::Tokenizer<T> tokenizer(string, T(1, ' '));
        if (tokenizer == utility::Tokenizer<T>())
        {
          throw std::invalid_argument("");
        }
        const T result = *tokenizer;
        if (++tokenizer != utility::Tokenizer<T>())
        {
          throw std::invalid_argument("");
        }
        return result;
      }

      /**
       * Trim whitespace and read a player ID.
       */
      template <typename T>
      tournament::player_index readPlayerId(const T &string)
      {
        tournament::player_index playerId;
        try
        {
          playerId =
            utility::uintstringconversion
                ::parse<tournament::player_index>(getSingleValue(string))
              - 1u;
        }
        catch (const std::invalid_argument &)
        {
          throw InvalidLineException();
        }
        catch (const std::out_of_range &)
        {
          throw tournament::BuildLimitExceededException(
            "This build only supports player IDs up to "
              + utility::uintstringconversion::toString(tournament::maxPlayers)
              + '.');
        }
        return playerId;
      }

      /**
       * Trim whitespace and read a score.
       */
      template <typename T>
      tournament::points readScore(const T &string)
      {
        tournament::points score;
        try
        {
          score =
            utility::uintstringconversion
              ::parse<tournament::points>(getSingleValue(string), 1);
        }
        catch (const std::invalid_argument &)
        {
          throw InvalidLineException();
        }
        catch (const std::out_of_range &)
        {
          throw tournament::BuildLimitExceededException(
            "This build only supports scores up to "
              + utility::uintstringconversion
                  ::toString(tournament::maxPoints, 1)
              + '.');
        }
        return score;
      }

      /**
       * Process a 001 line.
       */
      void readPlayer(
        const std::u32string &line,
        tournament::Tournament &tournament,
        FileData *data)
      {
        if (line.size() < 84)
        {
          throw InvalidLineException();
        }

        const tournament::player_index id =
          readPlayerId(std::u32string(&line[4], &line[8]));

        tournament::rating rating{ };

        const std::u32string ratingString(&line[48], &line[52]);
        utility::Tokenizer<std::u32string> tokenizer(ratingString, U" ");
        if (tokenizer != utility::Tokenizer<std::u32string>())
        {
          try
          {
            rating =
              utility::uintstringconversion
                ::parse<tournament::rating>(getSingleValue(ratingString));
          }
          catch (const std::invalid_argument &) {
            throw InvalidLineException();
          }
          catch (const std::out_of_range &)
          {
            throw tournament::BuildLimitExceededException(
              "This build only supports ratings up to "
                + utility::uintstringconversion
                    ::toString(~tournament::rating{ })
                + '.');
          }
        }

        const tournament::points score = readScore(line.substr(80, 4));

        tournament::round_index skippedRounds{ };
        std::vector<tournament::Match> matches;
        std::u32string::size_type startIndex = 91u;
        for (
          ;
          startIndex <= line.size() - 8u;
          startIndex += 10u)
        {
          /**
           * true if this entry should be treated as trailing space unless later
           * games have been played.
           */
          bool skip = true;
          bool gameWasPlayed = true;
          const std::u32string opponentString = line.substr(startIndex, 4);
          tournament::player_index opponent = id;
          if (opponentString != U"    ")
          {
            if (opponentString != U"0000")
            {
              opponent = readPlayerId(opponentString);
              if (opponent == id)
              {
                throw InvalidLineException();
              }
            }
            skip = false;
          }
          if (opponent == id)
          {
            gameWasPlayed = false;
          }
          const char32_t colorChar = line[startIndex + 5];
          tournament::Color color =
            colorChar == U'w' ? tournament::COLOR_WHITE
              : colorChar == U'b' ? tournament::COLOR_BLACK
              : tournament::COLOR_NONE;
          if (colorChar == U'w' || colorChar == U'b')
          {
            skip = false;
          }
          else if (colorChar == U'-')
          {
            skip = false;
            gameWasPlayed = false;
          }
          else if (colorChar == U' ')
          {
            gameWasPlayed = false;
          }
          else
          {
            throw InvalidLineException();
          }

          if (opponent == id && color != tournament::COLOR_NONE)
          {
            throw InvalidLineException();
          }

          if (std::numeric_limits<char>::max() < line[startIndex + 7])
          {
            throw InvalidLineException();
          }
          const char resultChar =
            std::toupper<char>(line[startIndex + 7], std::locale::classic());
          tournament::MatchScore matchScore;
          if (resultChar == 'D' || resultChar == '=' || resultChar == 'H')
          {
            matchScore = tournament::MATCH_SCORE_DRAW;
          }
          else if (
            resultChar == '+'
              || resultChar == 'W'
              || resultChar == '1'
              || resultChar == 'F'
              || resultChar == 'U')
          {
            matchScore = tournament::MATCH_SCORE_WIN;
          }
          else if (
            resultChar == '-'
              || resultChar == 'L'
              || resultChar == '0'
              || resultChar == 'Z'
              || resultChar == ' ')
          {
            matchScore = tournament::MATCH_SCORE_LOSS;
          }
          else
          {
            throw InvalidLineException();
          }
          if (
            resultChar == '+'
              || resultChar == '-'
              || resultChar == 'H'
              || resultChar == 'F'
              || resultChar == 'U'
              || resultChar == 'Z'
              || resultChar == ' ')
          {
            gameWasPlayed = false;
            if (resultChar != '+' && resultChar != '-' && opponent != id)
            {
              throw InvalidLineException();
            }
          }
          else if (
            color == tournament::COLOR_NONE
              && (resultChar != '=' || opponent != id))
          {
            throw InvalidLineException();
          }
          if (resultChar != ' ')
          {
            skip = false;
          }
          const bool participatedInPairing =
            opponent != id
              || resultChar == 'U'
              || resultChar == '+';
          if (skip)
          {
            ++skippedRounds;
            if (!skippedRounds)
            {
              --skippedRounds;
            }
          }
          else
          {
            if (
              skippedRounds
                >= std::numeric_limits<tournament::round_index>::max())
            {
              throw tournament::BuildLimitExceededException(
                "This build supports at most "
                  + utility::uintstringconversion::toString(
                      tournament::maxRounds)
                  + " rounds.");
            }
            matches.insert(matches.end(), skippedRounds, tournament::Match(id));
            skippedRounds = 0;
            if (matches.size() > tournament.playedRounds)
            {
              tournament.playedRounds = matches.size();
            }
            matches.emplace_back(
              opponent,
              color,
              matchScore,
              gameWasPlayed,
              participatedInPairing);
            if (
              participatedInPairing && matches.size() > tournament.playedRounds)
            {
              tournament.playedRounds = matches.size();
            }
            if (matches.size() - 1u > tournament.playedRounds)
            {
              throw tournament::BuildLimitExceededException(
                "This build supports at most "
                  + utility::uintstringconversion::toString(
                      tournament::maxRounds)
                  + " rounds.");
            }
          }
        }
        if (line.find_first_not_of(U" ", startIndex) < line.npos)
        {
          throw InvalidLineException();
        }
        tournament::Player player(id, score, rating, std::move(matches));
        if (id >= tournament.players.size())
        {
          tournament.players.insert(
            tournament.players.end(),
            id - tournament.players.size(),
            tournament::Player());
          tournament.players.push_back(std::move(player));
        }
        else if (tournament.players[id].isValid)
        {
          throw FileFormatException("A pairing number is repeated.");
        }
        else
        {
          player.accelerations =
            std::move(tournament.players[id].accelerations);
          player.forbiddenPairs =
            std::move(tournament.players[id].forbiddenPairs);
          tournament.players[id] = std::move(player);
        }
        tournament.playersByRank.push_back(id);
        if (data)
        {
          if (data->playerLines.size() <= id)
          {
            data->playerLines.insert(
              data->playerLines.end(),
              id - data->playerLines.size() + 1u,
              0);
          }
          data->playerLines[id] = data->lines.size() - 1u;
        }
      }

      /**
       * Process an XXA line.
       */
      void readAccelerations(
        const std::u32string &line,
        tournament::Tournament &tournament)
      {
        tournament.defaultAcceleration = false;
        const tournament::player_index playerId =
          readPlayerId(std::u32string(&line[4], &line[8]));
        if (playerId >= tournament.players.size())
        {
          tournament.players.insert(
            tournament.players.end(),
            playerId - tournament.players.size() + 1,
            tournament::Player());
        }
        std::u32string::size_type startIndex = 9;
        for (
          ;
          startIndex + 4 <= line.size();
          startIndex += 5)
        {
          const tournament::points points =
            line.substr(startIndex, startIndex + 4) == U"    "
              ? 0
              : readScore(
                  std::u32string(&line[startIndex], &line[startIndex + 4u]));
          tournament.players[playerId].accelerations.push_back(points);
        }
        if (line.find_first_not_of(U" ", startIndex) < line.npos)
        {
          throw InvalidLineException();
        }
      }

      /**
       * Process an XXP line.
       */
      std::deque<tournament::player_index> readForbiddenPairs(
        const std::u32string &line)
      {
        std::deque<tournament::player_index> result;
        const std::u32string string = line.substr(3);
        utility::Tokenizer<std::u32string> tokenizer(string, U" \t");
        while (tokenizer != utility::Tokenizer<std::u32string>())
        {
          result.push_back(readPlayerId(*tokenizer));
          ++tokenizer;
        }
        return result;
      }

      /**
       * Read a line containing a point value, throwing an InvalidLineException
       * if it is improperly formatted.
       */
      tournament::points readPoints(const std::u32string &line)
      {
        if (line.length() < 8)
        {
          throw InvalidLineException();
        }
        return readScore(line.substr(4, 8));
      }

      /**
       * Finalize the number of played rounds in the tournament, and add empty
       * games to the end of the list of matches for each player who doesn't
       * have enough.
       *
       * If includesUnpairedRound is true, then the records in the last column
       * will be considered byes for the next round, if that makes sense. In all
       * other cases, all notated matches are considered past matches.
       */
      void evenUpMatchHistories(
        tournament::Tournament &tournament,
        const bool includesUnpairedRound)
      {
        bool forwardRoundIsComplete = includesUnpairedRound;
        for (const tournament::Player &player : tournament.players)
        {
          if (player.isValid)
          {
            if (
              includesUnpairedRound
                ^ (player.matches.size() > tournament.playedRounds))
            {
              forwardRoundIsComplete = !includesUnpairedRound;
            }
          }
        }
        if (tournament.playersByRank.size() && forwardRoundIsComplete)
        {
          ++tournament.playedRounds;
        }
        for (tournament::Player &player : tournament.players)
        {
          if (player.isValid && player.matches.size() < tournament.playedRounds)
          {
            player.matches.insert(
              player.matches.end(),
              tournament.playedRounds - player.matches.size(),
              tournament::Match(player.id));
          }
        }
      }

      /**
       * Compute the ranks to be used as effective pairing numbers when
       * assigning colors and breaking ties.
       */
      void computePlayerIndexes(
        tournament::Tournament &tournament,
        const bool useRank)
      {
        tournament::player_index rankIndex{ };
        for (tournament::player_index &playerIndex : tournament.playersByRank)
        {
          if (!useRank)
          {
            playerIndex = rankIndex;
          }
          tournament::Player &player = tournament.players[playerIndex];

          player.rankIndex = rankIndex++;
        }
      }

      /**
       * If possible, infer the randomly chosen color for the top player who was
       * present in the first round (or more precisely, the first round where
       * some two players were assigned colors). This assumes the color was not
       * specified by the user.
       */
      tournament::Color inferFirstColor(const tournament::Tournament &tournament
      )
      {
        tournament::round_index minColorRound = ~tournament::round_index{ };
        for (const tournament::Player &player : tournament.players)
        {
          if (player.isValid)
          {
            tournament::round_index roundIndex{ };
            for (const tournament::Match &match : player.matches)
            {
              if (match.color != tournament::COLOR_NONE)
              {
                minColorRound = std::min(minColorRound, roundIndex);
              }
              ++roundIndex;
            }
          }
        }

        tournament::Color result = tournament::COLOR_NONE;

        tournament::player_index effectivePairingNumberMinColorRound{ };
        for (
          const tournament::player_index playerIndex : tournament.playersByRank)
        {
          const tournament::Player &player = tournament.players[playerIndex];

          bool playerHadPairingNumberMinColorRound{ };
          tournament::round_index matchIndex{ };
          for (const tournament::Match &match : player.matches)
          {
            if (matchIndex > minColorRound)
            {
              break;
            }
            if (match.participatedInPairing)
            {
              playerHadPairingNumberMinColorRound = true;
            }
            ++matchIndex;
          }

          if (playerHadPairingNumberMinColorRound)
          {
            if (result == tournament::COLOR_NONE)
            {
              result =
                effectivePairingNumberMinColorRound & 1u
                  ? invert(player.matches[minColorRound].color)
                  : player.matches[minColorRound].color;
            }
            ++effectivePairingNumberMinColorRound;
          }
        }

        return result;
      }

      /**
       * Check that opponent colors are different and that opponents are listed
       * as playing against each other.
       */
      void validatePairConsistency(const tournament::Tournament &tournament)
      {
        for (const tournament::Player &player : tournament.players)
        {
          if (player.isValid)
          {
            tournament::round_index matchIndex{ };
            for (const tournament::Match &match : player.matches)
            {
              if (match.gameWasPlayed)
              {
                const tournament::Player &opponent =
                  tournament.players[match.opponent];
                if (
                  !opponent.isValid
                    || !opponent.matches[matchIndex].gameWasPlayed
                    || opponent.matches[matchIndex].color == match.color
                    || opponent.matches[matchIndex].opponent != player.id)
                {
                  throw FileFormatException(
                    "Match "
                      + utility::uintstringconversion::toString(matchIndex + 1u)
                      + " for player "
                      + utility::uintstringconversion::toString(player.id + 1u)
                      + " contradicts the entry for the opponent.");
                }
              }
              ++matchIndex;
            }
          }
        }
      }

      /**
       * Check that the score in the TRF matches the score computed by counting
       * the number of wins and draws for that player and (optionally) adding
       * the acceleration. Also check that there are not more accelerated rounds
       * than tournament rounds.
       */
      void validateScores(tournament::Tournament &tournament)
      {
        for (tournament::Player &player : tournament.players)
        {
          if (player.isValid)
          {
            if (player.accelerations.size() > tournament.expectedRounds)
            {
              throw FileFormatException(
                "Player "
                  + utility::uintstringconversion::toString(player.id + 1u)
                  + " has more acceleration entries than the total number of "
                    "rounds in the tournament.");
            }
            tournament::points points{ };
            tournament::round_index matchIndex{ };
            for (const tournament::Match &match : player.matches)
            {
              if (matchIndex >= tournament.playedRounds)
              {
                break;
              }
              points += tournament.getPoints(player, match);
              if (points < tournament.getPoints(player, match))
              {
                throw tournament::BuildLimitExceededException(
                  "This build only supports scores up to "
                    + utility::uintstringconversion
                        ::toString(tournament::maxPoints, 1)
                    + '.');
              }
              ++matchIndex;
            }

            if (player.scoreWithoutAcceleration != points)
            {
              if (
                player.scoreWithoutAcceleration
                  >= player.acceleration(tournament))
              {
                player.scoreWithoutAcceleration
                  -= player.acceleration(tournament);
              }
              if (player.scoreWithoutAcceleration != points)
              {
                player.scoreWithoutAcceleration
                  += player.acceleration(tournament);
              }
            }
            if (player.scoreWithoutAcceleration != points)
            {
              if (player.matches.size() > tournament.playedRounds)
              {
                tournament::points nextRoundPoints =
                  tournament
                    .getPoints(player, player.matches[tournament.playedRounds]);
                if (player.scoreWithoutAcceleration >= nextRoundPoints)
                {
                  player.scoreWithoutAcceleration -= nextRoundPoints;
                }
              }
            }
            if (player.scoreWithoutAcceleration != points)
            {
              if (
                player.scoreWithoutAcceleration
                  >= player.acceleration(tournament))
              {
                player.scoreWithoutAcceleration
                  -= player.acceleration(tournament);
              }
            }
            if (player.scoreWithoutAcceleration != points)
            {
              throw FileFormatException(
                "The score for player "
                  + utility::uintstringconversion::toString(player.id + 1u)
                  + " does not match the game results.");
            }
          }
        }
      }

      /**
       * Produce the string for a 001 line, but only starting from the points
       * section.
       */
      std::string stringifyGames(
        const tournament::Player &player,
        const tournament::player_index rank)
      {
        if (player.scoreWithoutAcceleration > 999u)
        {
          throw LimitExceededException(
            "The output file format does not support scores above 99.9.");
        }
        std::ostringstream outputStream;
        outputStream << std::setfill(' ')
          << std::setw(4)
          << utility::uintstringconversion
              ::toString(player.scoreWithoutAcceleration, 1)
          << std::setw(5)
          << utility::uintstringconversion::toString(rank + 1u);

        for (const tournament::Match &match : player.matches)
        {
          outputStream << "  ";

          if (!match.participatedInPairing)
          {
            outputStream << "0000 - ";
            outputStream <<
              (match.matchScore == tournament::MATCH_SCORE_WIN ? 'F'
                : match.matchScore == tournament::MATCH_SCORE_DRAW ? 'H'
                : 'Z');
          }
          else if (match.opponent == player.id)
          {
            outputStream << "0000 - U";
          }
          else
          {
            outputStream << std::setw(4)
              << utility::uintstringconversion::toString(match.opponent + 1u)
              << ' '
              << (match.color == tournament::COLOR_WHITE ? 'w' : 'b')
              << ' '
              << (match.gameWasPlayed
                    ? match.matchScore == tournament::MATCH_SCORE_WIN ? '1'
                        : match.matchScore == tournament::MATCH_SCORE_DRAW ? '='
                        : '0'
                    : match.matchScore == tournament::MATCH_SCORE_WIN
                      ? '+'
                      : '-'
                  );
          }
        }

        return outputStream.str();
      }

      /**
       * Compute the tournament ranks for the players.
       */
      std::vector<tournament::player_index> computeRanks(
        const tournament::Tournament &tournament)
      {
        std::list<const tournament::Player *> rankedPlayers;
        for (const tournament::Player &player : tournament.players)
        {
          rankedPlayers.push_back(&player);
        }
        rankedPlayers.sort(tournament::unacceleratedScoreRankCompare);

        tournament::player_index rankIndex = tournament.players.size();
        std::vector<tournament::player_index> result(rankIndex);

        for (const tournament::Player *const player : rankedPlayers)
        {
          result[player->id] = --rankIndex;
        }

        return result;
      }
    }

    /**
     * Read a TRF(x) file, and return a Tournament containing the parsed data.
     * If data is not 0, store the file contents there.
     * If includesUnpairedRound is true, then we are being asked to pair the
     * next round, so we should look for future-round byes and require that the
     * total number of rounds in the tournament be specified.
     *
     * @throws FileFormatException if we detect that the file is not formatted
     * properly or contains inconsistent data.
     * @throws tournament::BuildLimitExceededException if a user value exceeds
     * that supported by the type sizes.
     * @throws FileReaderException on other exceptional conditions, such as an
     * unreadable file.
     */
    tournament::Tournament readFile(
      std::istream &stream,
      const bool includesUnpairedRound,
      FileData *const data)
    {
      tournament::Tournament result;
      bool useRank{ };
      bool usePairingAllocatedByeScore{ };
      std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
      std::string buffer;
      while (stream.good())
      {
        getline(stream, buffer, '\r');
        decltype(buffer)::size_type start = 0, end;
        while (start < buffer.size())
        {
          end = buffer.find_first_of('\n', start);
          if (end == buffer.npos)
          {
            end = buffer.size();
          }
          std::u32string line =
            convert.from_bytes(&buffer[start], &buffer[end]);
          if (convert.converted() < end - start)
          {
            throw FileFormatException("The file is not legal UTF-8.");
          }
          if (line.size() >= 3)
          {
            if (data)
            {
              data->lines.push_back(line);
            }
            const std::u32string prefix = line.substr(0, 3);
            try
            {
              if (prefix == U"001")
              {
                readPlayer(line, result, data);
              }
              else if (prefix == U"XXA")
              {
                readAccelerations(line, result);
              }
              else if (prefix == U"XXP")
              {
                result.forbidPairs(readForbiddenPairs(line));
              }
              else if (prefix == U"XXR")
              {
                try
                {
                  result.expectedRounds =
                    utility::uintstringconversion
                        ::parse<tournament::round_index>(
                      getSingleValue(line.substr(3)));
                }
                catch (const std::invalid_argument &)
                {
                  throw InvalidLineException();
                }
                catch (const std::out_of_range &)
                {
                  throw tournament::BuildLimitExceededException(
                    "This build only supports up to "
                      + utility::uintstringconversion
                          ::toString(tournament::maxRounds)
                      + " rounds");
                }
                if (result.expectedRounds <= 0)
                {
                  throw InvalidLineException();
                }
              }
              else if (prefix == U"XXC")
              {
                std::u32string newLine;
                std::u32string originalLine = line.substr(3);
                for (
                  utility::Tokenizer<std::u32string>
                    tokenizer(originalLine, U" \t");
                  tokenizer != utility::Tokenizer<std::u32string>();
                  ++tokenizer)
                {
                  if (*tokenizer == U"rank")
                  {
                    useRank = true;
                    newLine += U" rank";
                  }
                  else if (*tokenizer == U"white1")
                  {
                    result.initialColor = tournament::COLOR_WHITE;
                  }
                  else if (*tokenizer == U"black1")
                  {
                    result.initialColor = tournament::COLOR_BLACK;
                  }
                }
                if (data)
                {
                  if (newLine.empty())
                  {
                    data->lines.pop_back();
                  }
                  else
                  {
                    data->lines.back() = U"XXC" + newLine;
                  }
                }
              }
              else if (prefix == U"BBW")
              {
                result.pointsForWin = readPoints(line);
                if (!usePairingAllocatedByeScore)
                {
                  result.pointsForPairingAllocatedBye = result.pointsForWin;
                }
              }
              else if (prefix == U"BBD")
              {
                result.pointsForDraw = readPoints(line);
              }
              else if (prefix == U"BBL")
              {
                result.pointsForLoss = readPoints(line);
              }
              else if (prefix == U"BBZ")
              {
                result.pointsForZeroPointBye = readPoints(line);
              }
              else if (prefix == U"BBF")
              {
                result.pointsForForfeitLoss = readPoints(line);
              }
              else if (prefix == U"BBU")
              {
                result.pointsForPairingAllocatedBye = readPoints(line);
                usePairingAllocatedByeScore = true;
              }
            }
            catch (const InvalidLineException &exception)
            {
              throw FileFormatException(
                "Invalid line \"" + convert.to_bytes(line) + "\"");
            }
          }

          start = end + 1;
        }
      }
      if (!stream.eof())
      {
        throw FileReaderException("The file could not be loaded.");
      }
      if (!useRank && result.playersByRank.size() != result.players.size())
      {
        throw FileFormatException("A pairing number is missing.");
      }

      evenUpMatchHistories(result, includesUnpairedRound);
      if (
        result.expectedRounds
          && result.playedRounds > result.expectedRounds - includesUnpairedRound
      )
      {
        throw FileFormatException(
          "The number of rounds is larger than the reported number of rounds.");
      }
      else if (includesUnpairedRound && !result.expectedRounds)
      {
        throw FileFormatException(
          "The total number of rounds in the tournament must be specified.");
      }
      else if (!result.expectedRounds)
      {
        result.expectedRounds = result.playedRounds;
      }
      computePlayerIndexes(result, useRank);
      if (result.initialColor == tournament::COLOR_NONE)
      {
        result.initialColor = inferFirstColor(result);
      }
      validatePairConsistency(result);
      validateScores(result);

      return result;
    }

    /**
     * Write the rest of the tournament (excluding the seed line) to
     * outputStream.
     */
    void writeFile(
      std::ostream &outputStream,
      const tournament::Tournament &tournament)
    {
      const std::vector<tournament::player_index> ranks =
        computeRanks(tournament);
      outputStream << std::setfill(' ') << std::right;
      if (tournament.playedRounds < tournament.expectedRounds)
      {
        outputStream
          << "XXR "
          << utility::uintstringconversion::toString(tournament.expectedRounds)
          << '\r';
      }
      for (const tournament::Player &player : tournament.players)
      {
        if (player.id > 9999u)
        {
          throw LimitExceededException(
            "The output file format only supports player IDs up to 9999.");
        }
        else if (player.rating > 9999u)
        {
          throw LimitExceededException(
            "The output file format only supports ratings up to 9999.");
        }
        outputStream << "001 "
          << std::setw(4)
          << utility::uintstringconversion::toString(player.id + 1u)
          << std::setw(10)
          << "Test"
          << std::setw(4)
          << std::setfill('0')
          << utility::uintstringconversion::toString(player.id + 1u)
          << " Player"
          << std::setw(4)
          << utility::uintstringconversion::toString(player.id + 1u)
          << std::setfill(' ')
          << std::setw(19)
          << utility::uintstringconversion::toString(player.rating)
          << std::setw(28)
          << ""
          << stringifyGames(player, ranks[player.id]);

        outputStream << '\r';
      }
      outputStream << '\r';

      if (
        tournament.pointsForWin != 10u
          || tournament.pointsForDraw != 5u
          || tournament.pointsForLoss != 0u
          || tournament.pointsForZeroPointBye != 0u
          || tournament.pointsForForfeitLoss != 0u
          || tournament.pointsForPairingAllocatedBye != 10u)
      {
        if (
          tournament.pointsForWin > 999u
            || tournament.pointsForDraw > 999u
            || tournament.pointsForLoss > 999u
            || tournament.pointsForZeroPointBye > 999u
            || tournament.pointsForForfeitLoss > 999u
            || tournament.pointsForPairingAllocatedBye > 999u)
        {
          throw LimitExceededException(
            "The output file format does not support scores above 99.9.");
        }
        if (
          tournament.pointsForWin != 10u
            || tournament.pointsForDraw != 5u
            || tournament.pointsForLoss != 0u
            || tournament.pointsForZeroPointBye != 0u
            || tournament.pointsForForfeitLoss != 0u)
        {
          outputStream << "BBW "
            << std::setw(4)
            << utility::uintstringconversion
                ::toString(tournament.pointsForWin, 1)
            << "\rBBD "
            << std::setw(4)
            << utility::uintstringconversion
                ::toString(tournament.pointsForDraw, 1)
            << "\r";
        }
        if (
          tournament.pointsForLoss != 0u
            || tournament.pointsForZeroPointBye != 0u
            || tournament.pointsForForfeitLoss != 0u)
        {
          outputStream
            << "BBL "
            << std::setw(4)
            << utility::uintstringconversion
                ::toString(tournament.pointsForLoss, 1)
            << "\rBBZ "
            << std::setw(4)
            << utility::uintstringconversion
                ::toString(tournament.pointsForZeroPointBye, 1)
            << "\rBBF "
            << std::setw(4)
            << utility::uintstringconversion
                ::toString(tournament.pointsForForfeitLoss, 1)
            << "\r";
        }
        if (tournament.pointsForWin != tournament.pointsForPairingAllocatedBye)
        {
          outputStream
            << "BBU "
            << std::setw(4)
            << utility::uintstringconversion
                ::toString(tournament.pointsForPairingAllocatedBye, 1)
            << "\r";
        }
        outputStream << "\r";
      }

      if (!tournament.defaultAcceleration)
      {
        for (const tournament::Player &player : tournament.players)
        {
          if (!player.accelerations.empty())
          {
            outputStream << "XXA "
              << std::setfill(' ')
              << std::right
              << std::setw(4)
              << utility::uintstringconversion::toString(player.id + 1u);
            for (
              const tournament::points playerAcceleration : player.accelerations
            )
            {
              if (playerAcceleration > 999u)
              {
                throw LimitExceededException(
                  "The output file format does not support scores above 99.9.");
              }
              outputStream << std::setw(5)
                << utility::uintstringconversion
                    ::toString(playerAcceleration, 1);
            }

            outputStream << '\r';
          }
        }
      }
    }

    /**
     * Replace pieces of a model tournament with a generated tournament, and
     * write the result to outputStream.
     */
    void writeFile(
      std::ostream &outputStream,
      const tournament::Tournament &tournament,
      FileData &&modelFileData)
    {
      std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;

      const std::vector<tournament::player_index> ranks =
        computeRanks(tournament);

      for (
        const tournament::player_index playerIndex : tournament.playersByRank)
      {
        modelFileData.lines[modelFileData.playerLines[playerIndex]].replace(
          80,
          std::u32string::npos,
          convert.from_bytes(
            stringifyGames(tournament.players[playerIndex], ranks[playerIndex]))
        );
      }

      bool roundsLine;
      for (const std::u32string &line : modelFileData.lines)
      {
        if (line.length() >= 3 && line.substr(0, 3) == U"XXR")
        {
          roundsLine = true;
        }
        if (line.length() < 3 || line.substr(0, 3) != U"012")
        {
          outputStream << convert.to_bytes(line) << '\r';
        }
      }
      if (!roundsLine)
      {
        outputStream << "XXR "
          << utility::uintstringconversion::toString(tournament.expectedRounds)
          << '\r';
      }
    }
  }
}
