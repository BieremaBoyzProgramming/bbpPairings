#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include <deque>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <utility/typesizes.h>
#include <utility/uintstringconversion.h>
#include <utility/uinttypes.h>

#ifdef MAX_PLAYERS
#define TOURNAMENT_MAX_PLAYERS MAX_PLAYERS
#else
#define TOURNAMENT_MAX_PLAYERS 9999
#endif

#ifdef MAX_POINTS
#define TOURNAMENT_MAX_POINTS MAX_POINTS
#else
#define TOURNAMENT_MAX_POINTS 1998
#endif

#ifdef MAX_RATING
#define TOURNAMENT_MAX_RATING MAX_RATING
#else
#define TOURNAMENT_MAX_RATING 9999
#endif

namespace tournament
{
  struct Tournament;

  /**
   * An exception indicating that the operation could not be completed because
   * the constants set in this file are too small.
   */
  struct BuildLimitExceededException : public std::logic_error
  {
    explicit BuildLimitExceededException(const std::string &explanation)
      : std::logic_error(explanation) { }
  };

  typedef
    utility::uinttypes::uint_least_for_value<TOURNAMENT_MAX_PLAYERS>
    player_index;
  /**
   * A type representing a person's score, stored as ten times the actual score.
   */
  typedef
    utility::uinttypes::uint_least_for_value<TOURNAMENT_MAX_POINTS>
    points;
  typedef
    utility::uinttypes::uint_least_for_value<TOURNAMENT_MAX_RATING>
    rating;

  constexpr player_index maxPlayers = TOURNAMENT_MAX_PLAYERS;
  constexpr points maxPoints = TOURNAMENT_MAX_POINTS;

  enum Color : utility::uinttypes::uint_least_for_value<2>
  {
    COLOR_WHITE, COLOR_BLACK, COLOR_NONE
  };

  constexpr Color invert(const Color color)
  {
    return
      color == COLOR_WHITE ? COLOR_BLACK
        : color == COLOR_BLACK ? COLOR_WHITE
        : COLOR_NONE;
  }

  enum MatchScore : utility::uinttypes::uint_least_for_value<2>
  {
    MATCH_SCORE_LOSS, MATCH_SCORE_DRAW, MATCH_SCORE_WIN
  };

  constexpr MatchScore invert(const MatchScore matchScore)
  {
    return MatchScore(MATCH_SCORE_WIN - matchScore);
  }

  /**
   * A type representing the history of a single player on a single round.
   */
  struct Match
  {
    /**
     * The ID of the opponent. The lack of an opponent is indicated by using the
     * player's own ID.
     */
    player_index opponent;
    Color color;
    MatchScore matchScore;
    bool gameWasPlayed;
    /**
     * The player was either paired or given the pairing-allocated bye.
     */
    bool participatedInPairing;

    Match(const player_index playerIndex)
      : opponent(playerIndex),
        color(COLOR_NONE),
        matchScore(MATCH_SCORE_LOSS),
        gameWasPlayed(false),
        participatedInPairing(false)
    { }
    Match(
        const player_index opponent_,
        const Color color_,
        const MatchScore matchScore_,
        const bool gameWasPlayed_,
        const bool participatedInPairing_)
      : opponent(opponent_),
        color(color_),
        matchScore(matchScore_),
        gameWasPlayed(gameWasPlayed_),
        participatedInPairing(participatedInPairing_)
    { }
  };

  struct Player
  {
    std::vector<Match> matches;
    /**
     * Round-indexed accelerations. If the vector is shorter than the number of
     * rounds, zeroes are implied.
     */
    std::vector<points> accelerations;
    /**
     * The player may not be paired against these opponents.
     */
    std::unordered_set<player_index> forbiddenPairs;

    decltype(matches)::size_type colorImbalance{ };

    /**
     * The zero-indexed pairing ID used for input/output.
     */
    player_index id;
    /**
     * The effective pairing number for the current round, that is, the pairing
     * number used for choosing colors and for breaking ties.
     */
    player_index rankIndex;

    /**
     * Missing ratings are indicated by zeroes.
     */
    tournament::rating rating;

    points scoreWithoutAcceleration;

    Color colorPreference = COLOR_NONE;
    Color repeatedColor = COLOR_NONE;
    bool strongColorPreference{ };

    /**
     * The record corresponds to a player in the tournament, rather than a hole
     * in the player IDs.
     */
    bool isValid{ };

    Player() = default;
    Player(
        const player_index id_,
        const points points_,
        const tournament::rating rating_,
        std::vector<Match> &&matches_ = std::vector<Match>(),
        std::unordered_set<player_index> &&forbiddenPairs_ =
          std::unordered_set<player_index>())
      : matches(std::move(matches_)),
        forbiddenPairs(std::move(forbiddenPairs_)),
        id(id_),
        rankIndex(id_),
        rating(rating_),
        scoreWithoutAcceleration(points_),
        isValid(true)
    { }

    /**
     * Return whether the difference between the number of games played as white
     * and the number played as black leads to an absolute color preference.
     */
    bool absoluteColorImbalance() const
    {
      return colorImbalance > 1u;
    }
    bool absoluteColorPreference() const
    {
      return absoluteColorImbalance() || repeatedColor != COLOR_NONE;
    }
    points scoreWithAcceleration(
      const Tournament &tournament,
      decltype(matches)::size_type roundsBack = 0
    ) const;
    points acceleration(const Tournament &) const;
  };

  /**
   * Compare the players based on current score, breaking ties using the
   * rankIndex.
   */
  inline bool unacceleratedScoreRankCompare(
    const Player *const player0,
    const Player *const player1)
  {
    return
      std::tie(player0->scoreWithoutAcceleration, player1->rankIndex)
        < std::tie(player1->scoreWithoutAcceleration, player0->rankIndex);
  }
  /**
   * Compare the players based on current accelerated score, breaking ties using
   * the rankIndex.
   */
  inline bool acceleratedScoreRankCompare(
    const Player *const player0,
    const Player *const player1,
    const Tournament &tournament)
  {
    return
      std::make_tuple(
          player0->scoreWithAcceleration(tournament),
          player1->rankIndex)
        < std::make_tuple(
            player1->scoreWithAcceleration(tournament),
            player0->rankIndex);
  }

  typedef
    utility::typesizes
      ::smallest<
#ifdef MAX_ROUNDS
        utility::uinttypes::uint_least_for_value<MAX_ROUNDS>,
#endif
        decltype(std::declval<Player>().matches)::size_type,
        std::string::size_type
      >::type
    round_index;
  constexpr round_index maxRounds =
#ifdef MAX_ROUNDS
    utility::typesizes::minUint(
      MAX_ROUNDS,
#endif
      std::numeric_limits<round_index>::max()
#ifdef MAX_ROUNDS
    )
#endif
    ;

  /**
   * A struct representing the details and history of a tournament.
   */
  struct Tournament
  {
    /**
     * Players indexed by ID.
     */
    std::vector<Player> players;
    /**
     * Players indexed by their effective pairing numbers, that is, the pairing
     * number used for choosing colors and breaking ties.
     */
    std::deque<player_index> playersByRank;
    round_index playedRounds{ };
    round_index expectedRounds{ };
    points pointsForWin{ 10u };
    points pointsForDraw{ 5u };
    points pointsForLoss{ 0u };
    points pointsForZeroPointBye{ 0u };
    points pointsForForfeitLoss{ 0u };
    points pointsForPairingAllocatedBye{ 10u };
    Color initialColor = COLOR_NONE;
    bool defaultAcceleration = true;

    points getPoints(const Player &player, const Match &match) const &
    {
      return
        match.matchScore == MATCH_SCORE_LOSS
            ? match.participatedInPairing
                ? match.gameWasPlayed ? pointsForLoss : pointsForForfeitLoss
                : pointsForZeroPointBye
          : match.matchScore == MATCH_SCORE_WIN
              ? match.opponent == player.id && match.participatedInPairing
                  ? pointsForPairingAllocatedBye
                  : pointsForWin
          : pointsForDraw;
    }

    void forbidPairs(const std::deque<player_index> &) &;

    void updateRanks() &;
    void computePlayerData() &;
  };

  /**
   * The score of the specified player including acceleration, on the round that
   * is roundsBack before the current round.
   */
  inline points Player::scoreWithAcceleration(
    const Tournament &tournament,
    decltype(matches)::size_type roundsBack
  ) const
  {
    points score = this->scoreWithoutAcceleration;
    round_index roundIndex = tournament.playedRounds;
    while (roundsBack > 0u)
    {
      --roundIndex;
      score -= tournament.getPoints(*this, matches[roundIndex]);
      --roundsBack;
    }

    const points result =
      score
        + (roundIndex >= accelerations.size() ? 0u : accelerations[roundIndex]);
    if (result < score)
    {
      throw BuildLimitExceededException(
        "This build does not support accelerated scores above "
          + utility::uintstringconversion::toString(maxPoints, 1)
          + '.');
    }
    return result;
  }

  inline points Player::acceleration(const Tournament &tournament) const
  {
    return
      tournament.playedRounds >= accelerations.size()
        ? 0u
        : accelerations[tournament.playedRounds];
  }


  static_assert(
    maxPlayers
      <= std::numeric_limits<
            decltype(std::declval<Tournament>().players)::size_type
          >::max(),
    "Overfill");
}

#endif
