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
  // Input a tournament file, and compute the pairings of the next round.
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

    return "NOT YET IMPLEMENTED";    

  /*
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

    // Open the output file, if specified.
    std::ofstream outputFileStream;
    std::ostream *outputStream = &std::cout;
    if (pairingsOutputFile)
    {
      try
      {
        relativizePath(outputFilename, inputFilename);
      }
      catch (const std::filesystem::filesystem_error &)
      {
        std::cerr
          << "Error extracting the directory of the input file."
          << std::endl;
        return FILE_ERROR;
      }

      outputFileStream = std::ofstream(outputFilename);
      if (!outputFileStream)
      {
        std::cerr << "The output file ("
          << outputFilename
          << ") could not be opened."
          << std::endl;
        return FILE_ERROR;
      }
      outputStream = &outputFileStream;
    }

    // Open the checklist output file, if requested.
    std::unique_ptr<std::ofstream> checklistStream;
    if (checklist)
    {
      checklistStream =
        openChecklist(
          checklistFilename,
          checklistCustomFilename,
          inputFilename);
    }

    // Compute the matching.
    std::list<swisssystems::Pairing> pairs;
    try
    {
      pairs =
        info.computeMatching(std::move(tournament), checklistStream.get());
    }
    catch (const swisssystems::NoValidPairingException &exception)
    {
      std::cerr << "Error while pairing "
        << inputFilename
        << ": No valid pairing exists: "
        << exception.what()
        << std::endl;
      return NO_VALID_PAIRING;
    }
    catch (const swisssystems::UnapplicableFeatureException &exception)
    {
      std::cerr << "Error while pairing "
        << inputFilename
        << ": "
        << exception.what()
        << std::endl;
      return INVALID_REQUEST;
    }

    closeChecklist(checklistStream.get(), checklistFilename);

    swisssystems::sortResults(pairs, tournament);

    // Output the pairs.
    *outputStream << pairs.size() << std::endl;
    for (const swisssystems::Pairing &pair : pairs)
    {
      *outputStream << pair.white + 1u
        << ' '
        << (pair.white == pair.black
              ? "0"
              : utility::uintstringconversion::toString(pair.black + 1u))
        << std::endl;
    }

    if (pairingsOutputFile)
    {
      // Check for errors.
      outputFileStream.close();
      if (!outputFileStream)
      {
        std::cerr << "Error while writing to output file "
          << outputFilename
          << '.'
          << std::endl;
      }
    }
*/
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
