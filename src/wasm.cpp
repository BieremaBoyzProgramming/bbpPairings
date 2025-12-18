// TODO: Remove unused includes.
#include <chrono>
#include <exception>
#include <emscripten/bind.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "fileformats/trf.h"
#include "fileformats/types.h"
#include "swisssystems/common.h"
#include "tournament/checker.h"
#include "tournament/tournament.h"
#include "utility/uintstringconversion.h"

using namespace emscripten;

std::string error(const std::string& code, const std::string& message) {
    return "{ error: { code: \"" + code + "\", message: \"" + message + "\" }}";
}

std::string pairing(const std::string& input) {
  const swisssystems::SwissSystem swissSystem = swisssystems::DUTCH;

  try
  {
    std::istringstream inputStream(input);

    // Read the tournament.
    tournament::Tournament tournament;
    try
    {
      tournament = fileformats::trf::readFile(inputStream, true);
    }
    catch (const fileformats::FileFormatException &exception)
    {
      return error("INVALID_REQUEST", "Error parsing file: " + std::string(exception.what()));
    }
    catch (const fileformats::FileReaderException &exception)
    {
      return error("FILE_ERROR", "Error reading file: " + std::string(exception.what()));
    }
    if (tournament.initialColor == tournament::COLOR_NONE)
    {
      return error("INVALID_REQUEST", "Please configure the initial piece colors.");
    }
    tournament.updateRanks();
    tournament.computePlayerData();

    // Add default accelerations.
    const swisssystems::Info &info = swisssystems::getInfo(swissSystem);
    if (tournament.defaultAcceleration)
    {
      for (
        tournament::round_index round_index{ };
        round_index <= tournament.playedRounds;
        ++round_index)
      {
        info.updateAccelerations(tournament, round_index);
      }
    }

    // Compute the matching.
    std::list<swisssystems::Pairing> pairs;
    try
    {
      pairs =
        info.computeMatching(std::move(tournament), 0);
    }
    catch (const swisssystems::NoValidPairingException &exception)
    {
      return error("NO_VALID_PAIRING", "No valid pairing exists: " + std::string(exception.what()));
    }
    catch (const swisssystems::UnapplicableFeatureException &exception)
    {
      return error("INVALID_REQUEST", "Error while pairing: " + std::string(exception.what()));
    }

    swisssystems::sortResults(pairs, tournament);

    // Output the pairs.
    if (pairs.empty()) {
      return "[]";
    }
    std::ostringstream outputStream;
    std::string prefix = "\n  ";  // Prefix if there is another element.
    outputStream << "[\n";
    for (const swisssystems::Pairing &pair : pairs)
    {
      outputStream << prefix
        << '['
        << pair.white + 1u
        << ','
        << (pair.white == pair.black
              ? "0"
              : utility::uintstringconversion::toString(pair.black + 1u))
        << "]";
      prefix = ",\n  ";
    }
    outputStream << "\n]\n";
    return outputStream.str();
  }
  catch (const tournament::BuildLimitExceededException &exception)
  {
    return error("LIMIT_EXCEEDED", "Error processing file: " + std::string(exception.what()));
  }
  catch (const std::length_error &)
  {
    return error("LIMIT_EXCEEDED", "The build does not support tournaments this large.");
  }
  catch (const std::bad_alloc &)
  {
    return error("LIMIT_EXCEEDED", "The program ran out of memory.");
  }
  catch (const std::exception &exception)
  {
    return error("UNEXPECTED_ERROR", "Unexpected error: " + std::string(exception.what()));
  }
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("pairing", &pairing);
}
