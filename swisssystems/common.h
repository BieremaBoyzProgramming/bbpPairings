#ifndef SWISSSYSTEMSCOMMON_H
#define SWISSSYSTEMSCOMMON_H

#include <deque>
#include <functional>
#include <list>
#include <ostream>
#include <stdexcept>
#include <string>

#include <tournament/tournament.h>

namespace swisssystems
{
  enum SwissSystem
  {
#ifndef OMIT_DUTCH
    DUTCH,
#endif
#ifndef OMIT_BURSTEIN
    BURSTEIN,
#endif
    NONE
  };

  /**
   * An exception indicating that no pairing satisfies the requirements imposed
   * by the system.
   */
  struct NoValidPairingException : public std::runtime_error
  {
    NoValidPairingException() : std::runtime_error("") { }
    NoValidPairingException(const std::string &message)
      : std::runtime_error(message) { }
  };

  /**
   * An exception indicating that the chosen Swiss system does not support all
   * of the selected options, for example, nonstandard point systems.
   */
  struct UnapplicableFeatureException : public std::runtime_error
  {
    UnapplicableFeatureException(const std::string &message)
      : std::runtime_error(message) { }
  };

  /**
   * An object representing the assignment of two people to player each other,
   * along with the assignment of colors.
   */
  struct Pairing
  {
    tournament::player_index white;
    tournament::player_index black;

    Pairing(
        const tournament::player_index white_,
        const tournament::player_index black_)
      : white(white_), black(black_) { }
    Pairing(
        const tournament::player_index player0,
        const tournament::player_index player1,
        const tournament::Color player0color)
      : white(player0color == tournament::COLOR_WHITE ? player0 : player1),
        black(player0color == tournament::COLOR_WHITE ? player1 : player0) { }
  };

  /**
   * An object representing info about a Swiss system, including a matching
   * computer and information about acceleration rules.
   */
  struct Info
  {
    virtual std::list<Pairing> computeMatching(
      tournament::Tournament &&,
      std::ostream *
    ) const = 0;
    /**
     * Assign accelerations for the next round, assuming a default acceleration
     * system is specified for this Swiss system. Otherwise, throw an
     * UnsupportedFeatureException.
     */
    virtual void updateAccelerations(
      tournament::Tournament &,
      tournament::round_index
    ) const { }

  protected:
    constexpr Info() noexcept { }
  };

  /**
   * Retrieve the Info object associated with the specified SwissSystem.
   */
  const Info &getInfo(SwissSystem);

  /**
   * Check whether two players can play each other under the normal (pre-last
   * round) restrictions imposed on all Swiss systems.
   */
  inline bool colorPreferencesAreCompatible(
    tournament::Color preference0,
    tournament::Color preference1)
  {
    return
      preference0 != preference1
        || preference0 == tournament::COLOR_NONE
        || preference1 == tournament::COLOR_NONE;
  }

  /**
   * Check whether the player is eligible for the bye under the normal
   * restrictions imposed on all Swiss systems.
   */
  inline bool eligibleForBye(
    const tournament::Player &player,
    const tournament::Tournament &tournament)
  {
    for (const tournament::Match &match : player.matches)
    {
      if (
        !match.gameWasPlayed
          && match.participatedInPairing
          && match.matchScore == tournament::MATCH_SCORE_WIN)
      {
        return false;
      }
    }
    return true;
  }

  void findFirstColorDifference(
    const tournament::Player &,
    const tournament::Player &,
    tournament::Color &,
    tournament::Color &);
  tournament::Color choosePlayerNeutralColor(
    const tournament::Player &,
    const tournament::Player &);

  void sortResults(std::list<Pairing> &, const tournament::Tournament &);

  void printChecklist(
    std::ostream &,
    const std::deque<std::string> &,
    const std::function<std::deque<std::string>(const tournament::Player &)> &,
    const tournament::Tournament &,
    const std::list<const tournament::Player *> &);

  /**
   * Set the weight of the edge between the two vertices to defaultEdgeWeight,
   * and set edge weights of all other edges incident on these vertices to 0.
   */
  template <class MatchingComputer>
  void finalizePair(
    const typename MatchingComputer::vertex_index vertexIndex0,
    const typename MatchingComputer::vertex_index vertexIndex1,
    MatchingComputer &matchingComputer,
    typename MatchingComputer::edge_weight defaultEdgeWeight = 1)
  {
    for (
      decltype(matchingComputer.size()) unpairedVertexIndex{ };
      unpairedVertexIndex < matchingComputer.size();
      ++unpairedVertexIndex)
    {
      if (vertexIndex0 != unpairedVertexIndex)
      {
        matchingComputer.setEdgeWeight(
          vertexIndex0,
          unpairedVertexIndex,
          unpairedVertexIndex == vertexIndex1
            ? defaultEdgeWeight
            : defaultEdgeWeight & 0u
        );
      }
      if (vertexIndex1 != unpairedVertexIndex)
      {
        matchingComputer.setEdgeWeight(
          vertexIndex1,
          unpairedVertexIndex,
          unpairedVertexIndex == vertexIndex0
            ? defaultEdgeWeight
            : defaultEdgeWeight & 0u
        );
      }
    }
  }
}

#endif
