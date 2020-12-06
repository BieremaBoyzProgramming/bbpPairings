#include <chrono>
#include <exception>
#ifdef EXPERIMENTAL_FILESYSTEM
  #include <experimental/filesystem>
  #define FILESYSTEM_NS std::experimental::filesystem
#else
  #ifdef FILESYSTEM
    #include <filesystem>
    #define FILESYSTEM_NS std::filesystem
  #endif
#endif
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

#include "fileformats/generatorconfiguration.h"
#include "fileformats/trf.h"
#include "fileformats/types.h"
#include "swisssystems/common.h"
#include "tournament/checker.h"
#include "tournament/generator.h"
#include "tournament/tournament.h"
#include "utility/uintstringconversion.h"

#define NO_VALID_PAIRING 1
#define UNEXPECTED_ERROR 2
#define INVALID_REQUEST 3
#define LIMIT_EXCEEDED 4
#define FILE_ERROR 5

#define STRINGIFY(x) #x
#define STRINGIFY_MACRO(x) STRINGIFY(x)

namespace
{
  /**
   * Print the build info to the ostream.
   */
  void printProgramInfo(std::ostream &ostream, bool checkComponents = true)
  {
#ifdef OMIT_BURSTEIN
#define CUSTOM_BUILD
#endif
#ifdef OMIT_DUTCH
#define CUSTOM_BUILD
#endif
#ifdef OMIT_GENERATOR
#define CUSTOM_BUILD
#endif
#ifdef OMIT_CHECKER
#define CUSTOM_BUILD
#endif
#ifdef MAX_PLAYERS
#define CUSTOM_BUILD
#endif
#ifdef MAX_POINTS
#define CUSTOM_BUILD
#endif
#ifdef MAX_RATING
#define CUSTOM_BUILD
#endif
#ifdef MAX_ROUNDS
#define CUSTOM_BUILD
#endif
    ostream
      << "BBP Pairings (https://github.com/BieremaBoyzProgramming/bbpPairings) "
          "- "
          STRINGIFY_MACRO(VERSION_INFO)
          " (Built "
      << (checkComponents
            ? ""
#ifdef CUSTOM_BUILD
              "with customized settings"
#endif
#ifndef FILESYSTEM_NS
#ifdef CUSTOM_BUILD
              " and "
#endif
              "without file path manipulation, "
#else
#ifdef CUSTOM_BUILD
              ", "
#endif
#endif
            : "")
      << __DATE__
          " "
          __TIME__
          ")";
  }

  void relativizePath(std::string &filename, const std::string &pathBase)
  {
#ifdef FILESYSTEM_NS
    if (!FILESYSTEM_NS::path(filename).has_parent_path())
    {
      filename =
        FILESYSTEM_NS::path(pathBase).replace_filename(filename).string();
    }
#endif
  }

  void getChecklistFilename(
    std::string &checklistFilename,
    const bool userSpecifiedFilename,
    const std::string &baseFilename)
  {
    if (userSpecifiedFilename)
    {
      relativizePath(checklistFilename, baseFilename);
    }
    else
    {
#ifdef FILESYSTEM_NS
      checklistFilename =
        FILESYSTEM_NS::path(baseFilename).replace_extension("list").string();
#else
      assert(false);
#endif
    }
  }

  std::unique_ptr<std::ofstream> openChecklist(
     const std::string &filename,
     const bool userSpecifiedFilename,
     const std::string &baseFilename)
  {
    std::unique_ptr<std::ofstream> result;
    std::string relativizedFilename = filename;
#ifdef FILESYSTEM_NS
    try
    {
#endif
      getChecklistFilename(
        relativizedFilename,
        userSpecifiedFilename,
        baseFilename);
      result.reset(new std::ofstream(relativizedFilename));
      if (!*result)
      {
        std::cerr << "The checklist file ("
          << filename
          << ") could not be opened."
          << std::endl;
        result.reset();
      }
#ifdef FILESYSTEM_NS
    }
    catch (const FILESYSTEM_NS::filesystem_error &)
    {
      std::cerr
        << "Error inferring the path to the checklist."
        << std::endl;
    }
#endif
    return result;
  }

  void closeChecklist(std::ofstream *const stream, const std::string &filename)
  {
    if (stream)
    {
      // Append the build information, and check for errors.
      printProgramInfo(*stream, false);
      stream->close();
      if (!*stream)
      {
        std::cerr << "Error while writing to checklist file "
          << filename
          << '.'
          << std::endl;
      }
    }
  }
}

int main(const int argc, char**const argv)
{
  try
  {
    // Check which optional arguments are present and whether the args conform
    // to one of the accepted patterns.
    const bool printInfo = argc <= 1 || argv[1] == std::string("-r");
    const std::string swissSystemString(
      argc <= 1 + printInfo ? "" : argv[1u + printInfo]);
    const swisssystems::SwissSystem swissSystem =
      argc <= 1 + printInfo ? swisssystems::NONE
#ifndef OMIT_DUTCH
        : swissSystemString == "--dutch" ? swisssystems::DUTCH
#endif
#ifndef OMIT_BURSTEIN
        : swissSystemString == "--burstein" ? swisssystems::BURSTEIN
#endif
        : swisssystems::NONE;
    int processedArgCount = 2 + printInfo;

    const char *inputFilename;
#ifndef OMIT_CHECKER
    const bool checkPairings =
      argc >= 2 + processedArgCount
        && argv[1u + processedArgCount] == std::string("-c");
    if (checkPairings)
    {
      inputFilename = argv[processedArgCount];
      processedArgCount += 2;
    }
#endif

    std::string outputFilename;
    const bool pairingsOutputFile =
      argc >= 3 + processedArgCount
        && argv[2u + processedArgCount] != std::string("-l");
    const bool doPairings =
      argc >= 2 + processedArgCount + pairingsOutputFile
        && argv[1u + processedArgCount] == std::string("-p");
    if (doPairings)
    {
      inputFilename = argv[processedArgCount];
      processedArgCount += 2;
      if (pairingsOutputFile)
      {
        outputFilename = argv[processedArgCount];
        ++processedArgCount;
      }
    }

#ifndef OMIT_GENERATOR
    const char *seedString;
    const bool modelFile =
      argc >= 1 + processedArgCount
        && argv[processedArgCount] != std::string("-g");
    const bool configurationFile =
      argc >= 2 + processedArgCount + modelFile
        && argv[1u + processedArgCount + modelFile] != std::string("-o");
    const bool seed =
      argc >= 5 + processedArgCount + modelFile + configurationFile
        && argv[3u + processedArgCount + modelFile + configurationFile]
            == std::string("-s");
    const bool generateTournament =
      (!modelFile || !configurationFile)
        && argc
            >= 3 + processedArgCount + modelFile + configurationFile + 2 * seed
        && argv[1u + processedArgCount + modelFile + configurationFile]
            == std::string("-o");
    if (generateTournament)
    {
      if (modelFile)
      {
        inputFilename = argv[processedArgCount];
        ++processedArgCount;
      }
      ++processedArgCount;
      if (configurationFile)
      {
        inputFilename = argv[processedArgCount];
        ++processedArgCount;
      }
      ++processedArgCount;
      outputFilename = argv[processedArgCount];
      ++processedArgCount;
      if (seed)
      {
        ++processedArgCount;
        seedString = argv[processedArgCount];
        ++processedArgCount;
      }
    }
#endif

    const bool checklist =
#ifdef FILESYSTEM_NS
      argc >= 1 + processedArgCount
#else
      argc >= 2 + processedArgCount
#endif
        && argv[processedArgCount] == std::string("-l");
    std::string checklistFilename;
    bool checklistCustomFilename{ };
    if (checklist)
    {
      ++processedArgCount;
      checklistCustomFilename = processedArgCount < argc;
      if (checklistCustomFilename)
      {
        checklistFilename = argv[processedArgCount];
        ++processedArgCount;
      }
    }
    if (
      argc > 1 + printInfo
        && (swissSystem == swisssystems::NONE
              || (unsigned int)doPairings
#ifndef OMIT_CHECKER
                    + checkPairings
#endif
#ifndef OMIT_GENERATOR
                    + generateTournament
#endif
                  != 1
              || processedArgCount != argc))
    {
      // Invalid command.

      printProgramInfo(std::cerr);
      const char*const swissSystemSyntax =
#ifndef OMIT_DUTCH
#ifndef OMIT_BURSTEIN
        "("
#endif
        "--dutch"
#endif
#ifndef OMIT_BURSTEIN
#ifndef OMIT_DUTCH
        " | "
#endif
        "--burstein"
#ifndef OMIT_DUTCH
        ")"
#endif
#endif
        ;
      const char*const checklistString =
#ifdef FILESYSTEM_NS
        "[-l [check-list-file]]";
#else
        "[-l check-list-file]";
#endif
      std::cerr << std::endl
        << std::endl
        << "Command line argument syntax:"
        << std::endl
        << argv[0]
        << " [-r]"
        << std::endl
#ifndef OMIT_CHECKER
        << argv[0]
        << " [-r] "
        << swissSystemSyntax
        << " input-file -c "
        << checklistString
        << std::endl
#endif
        << argv[0]
        << " [-r] "
        << swissSystemSyntax
        << " input-file -p [output-file] "
        << checklistString
        << std::endl
#ifndef OMIT_GENERATOR
        << argv[0]
        << " [-r] "
        << swissSystemSyntax
        << " (model-file -g | -g [config-file]) -o trf_file [-s random_seed] "
        << checklistString
        << std::endl
#endif
        ;
      return INVALID_REQUEST;
    }
    if (printInfo)
    {
      // Print build info.
      printProgramInfo(std::cout);
      std::cout << std::endl;
    }
#ifndef OMIT_CHECKER
    if (checkPairings)
    {
      // Input a tournament and check that the pairings are correct.
      std::ifstream inputStream(inputFilename);

      try
      {
        // Read the tournament.
        tournament::Tournament tournament;
        try
        {
          tournament = fileformats::trf::readFile(inputStream, false);
        }
        catch (const fileformats::FileFormatException &exception)
        {
          std::cerr << "Error parsing file "
            << inputFilename
            << ": "
            << exception.what()
            << std::endl;
          return INVALID_REQUEST;
        }
        catch (const fileformats::FileReaderException &exception)
        {
          std::cerr << "Error reading file "
            << inputFilename
            << ": "
            << exception.what()
            << std::endl;
          return FILE_ERROR;
        }

        std::unique_ptr<std::ofstream> checklistStream;
        if (checklist)
        {
          checklistStream =
            openChecklist(
              checklistFilename,
              checklistCustomFilename,
              inputFilename);
        }

        // Check that the pairings are correct.
        try
        {
          tournament::checker::check(
            tournament,
            swissSystem,
            checklistStream.get(),
  #ifdef FILESYSTEM_NS
            FILESYSTEM_NS::path(inputFilename).stem().string());
  #else
            inputFilename);
  #endif
        }
        catch (const swisssystems::UnapplicableFeatureException &exception)
        {
          std::cerr << "Error checking file "
            << inputFilename
            << ": "
            << exception.what()
            << std::endl;
          return INVALID_REQUEST;
        }

        std::cout << std::endl;

        closeChecklist(checklistStream.get(), checklistFilename);
      }
      catch (const tournament::BuildLimitExceededException &exception)
      {
        std::cerr << "Error processing file "
          << inputFilename
          << ": "
          << exception.what()
          << std::endl;
        return LIMIT_EXCEEDED;
      }
      catch (const std::length_error &)
      {
        std::cerr
          << "Error processing file "
          << inputFilename
          << ": The build does not support tournaments this large."
          << std::endl;
        return LIMIT_EXCEEDED;
      }
      catch (const std::bad_alloc &)
      {
        std::cerr
          << "Error processing file "
          << inputFilename
          << ": The program ran out of memory."
          << std::endl;
        return LIMIT_EXCEEDED;
      }
    }
#endif
    if (doPairings)
    {
      // Input a tournament file, and compute the pairings of the next round.
      try
      {
        std::ifstream inputStream(inputFilename);

        // Read the tournament.
        tournament::Tournament tournament;
        try
        {
          tournament = fileformats::trf::readFile(inputStream, true);
        }
        catch (const fileformats::FileFormatException &exception)
        {
          std::cerr << "Error parsing file "
            << inputFilename
            << ": "
            << exception.what()
            << std::endl;
          return INVALID_REQUEST;
        }
        catch (const fileformats::FileReaderException &exception)
        {
          std::cerr << "Error reading file "
            << inputFilename
            << ": "
            << exception.what()
            << std::endl;
          return FILE_ERROR;
        }
        if (tournament.initialColor == tournament::COLOR_NONE)
        {
          std::cerr << "Error while parsing "
            << inputFilename
            << ": Please configure the initial piece colors."
            << std::endl;
          return INVALID_REQUEST;
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

        // Open the output file, if specified.
        std::ofstream outputFileStream;
        std::ostream *outputStream = &std::cout;
        if (pairingsOutputFile)
        {
#ifdef FILESYSTEM_NS
          try
          {
#endif
            relativizePath(outputFilename, inputFilename);
#ifdef FILESYSTEM_NS
          }
          catch (const FILESYSTEM_NS::filesystem_error &)
          {
            std::cerr
              << "Error extracting the directory of the input file."
              << std::endl;
            return FILE_ERROR;
          }
#endif

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
      }
      catch (const tournament::BuildLimitExceededException &exception)
      {
        std::cerr << "Error processing file "
          << inputFilename
          << ": "
          << exception.what()
          << std::endl;
        return LIMIT_EXCEEDED;
      }
      catch (const std::length_error &)
      {
        std::cerr
          << "Error processing file "
          << inputFilename
          << ": The build does not support tournaments this large."
          << std::endl;
        return LIMIT_EXCEEDED;
      }
      catch (const std::bad_alloc &)
      {
        std::cerr
          << "Error processing file "
          << inputFilename
          << ": The program ran out of memory."
          << std::endl;
        return LIMIT_EXCEEDED;
      }
    }
#ifndef OMIT_GENERATOR
    else if (generateTournament)
    {
      // Generate a random tournament from configuration options or a model
      // tournament.

      // Choose a random seed, using the (high-precision) time and, if possible,
      // a nondeterministic source of randomness.
      try
      {
        std::minstd_rand::result_type seedValue;
        if (seed)
        {
          try
          {
            seedValue =
              utility::uintstringconversion
                ::parse<decltype(seedValue)>(seedString);
          }
          catch (const std::out_of_range &)
          {
            std::cerr << "The seed must be between 0 and "
              << utility::uintstringconversion
                   ::toString(~decltype(seedValue){ })
              << '.'
              << std::endl;
            return LIMIT_EXCEEDED;
          }
        }
        else
        {
          const std::chrono::high_resolution_clock::duration::rep now =
            std::chrono::high_resolution_clock::now().time_since_epoch()
              .count();
          try
          {
            std::random_device random;
            if (random.entropy())
            {
              const decltype(random() + now) initializer[]{ random(), now };
              std::seed_seq(
                initializer,
                initializer + sizeof(initializer) / sizeof(*initializer)
              ).generate(&seedValue, 1 + &seedValue);
            }
            else
            {
              std::seed_seq({ now }).generate(&seedValue, 1 + &seedValue);
            }
          }
          catch (const std::exception &exception)
          {
            std::seed_seq({ now }).generate(&seedValue, 1 + &seedValue);
          }
        }
        std::minstd_rand engine(seedValue);

        tournament::generator::MatchesConfiguration matchesConfiguration;
        fileformats::trf::FileData fileData;

        if (modelFile)
        {
          try
          {
            std::ifstream inputStream(inputFilename);

            // Read the model tournament.
            tournament::Tournament tournament;
            try
            {
              tournament =
                fileformats::trf::readFile(inputStream, false, &fileData);
            }
            catch (const fileformats::FileFormatException &exception)
            {
              std::cerr << "Error parsing file "
                << inputFilename
                << ": "
                << exception.what()
                << std::endl;
              return INVALID_REQUEST;
            }
            catch (fileformats::FileReaderException &exception)
            {
              std::cerr << "Error reading file "
                << inputFilename
                << ": "
                << exception.what()
                << std::endl;
              return FILE_ERROR;
            }

            // Verify that ratings are nonzero.
            for (const tournament::Player &player : tournament.players)
            {
              if (player.isValid && !player.rating)
              {
                std::cerr << "Error processing file "
                  << inputFilename
                  << ": All players must have meaningful (nonzero) ratings."
                  << std::endl;
                return INVALID_REQUEST;
              }
            }

            // Compute the configuration options for generating the matches.
            matchesConfiguration =
              tournament::generator
                ::MatchesConfiguration(std::move(tournament));
          }
          catch (const tournament::BuildLimitExceededException &exception)
          {
            std::cerr << "Error processing file "
              << inputFilename
              << ": "
              << exception.what()
              << std::endl;
            return LIMIT_EXCEEDED;
          }
          catch (const std::length_error &)
          {
            std::cerr
              << "Error processing file "
              << inputFilename
              << ": The build does not support tournaments this large."
              << std::endl;
            return LIMIT_EXCEEDED;
          }
          catch (const std::bad_alloc &)
          {
            std::cerr
              << "Error processing file "
              << inputFilename
              << ": The program ran out of memory."
              << std::endl;
            return LIMIT_EXCEEDED;
          }
        }
        else
        {
          // Compute random configuration options.
          tournament::generator::Configuration configuration(engine);

          if (configurationFile)
          {
            try
            {
              std::ifstream inputStream(inputFilename);

              // Override configuration options using a file.
              fileformats::generatorconfiguration::readFile(
                configuration,
                inputStream);
            }
            catch (const fileformats::FileFormatException &exception)
            {
              std::cerr << "Error parsing configuration file "
                << inputFilename
                << ": "
                << exception.what()
                << std::endl;
              return INVALID_REQUEST;
            }
            catch (const fileformats::FileReaderException &exception)
            {
              std::cerr << "Error while reading configuration file "
                << inputFilename
                << ": "
                << exception.what()
                << std::endl;
              return FILE_ERROR;
            }
            catch (const tournament::BuildLimitExceededException &exception)
            {
              std::cerr << "Error processing configuration file "
                << inputFilename
                << ": "
                << exception.what()
                << std::endl;
              return LIMIT_EXCEEDED;
            }
          }

          // Randomly generate the player ratings.
          try
          {
            matchesConfiguration =
              tournament::generator::MatchesConfiguration(
                std::move(configuration),
                engine);
          }
          catch (
            const tournament::generator::BadConfigurationException &exception)
          {
            std::cerr << "Error while processing configuration file "
              << inputFilename
              << ": "
              << exception.what()
              << std::endl;
            return INVALID_REQUEST;
          }
        }

        // Open the output file.
        if (modelFile || configurationFile)
        {
#ifdef FILESYSTEM_NS
          try
          {
#endif
            relativizePath(outputFilename, inputFilename);
#ifdef FILESYSTEM_NS
          }
          catch (const FILESYSTEM_NS::filesystem_error &)
          {
            std::cerr
              << "Error extracting the directory of the input file."
              << std::endl;
            return FILE_ERROR;
          }
#endif
        }

        std::ofstream outputStream(outputFilename);
        if (!outputStream.good())
        {
          std::cerr << "The output file ("
            << outputFilename
            << ") could not be opened."
            << std::endl;
          return FILE_ERROR;
        }

        std::unique_ptr<std::ofstream> checklistStream;
        if (checklist)
        {
          checklistStream =
            openChecklist(
              checklistFilename,
              checklistCustomFilename,
              outputFilename);
        }

        // Write the seed to the output file in advance so we can reproduce
        // discovered bugs.
        fileformats::trf::writeSeed(outputStream, seedValue);

        int result = 0;

        // Pair the tournament, randomly generating each match.
        tournament::Tournament tournament;
        try
        {
          tournament::generator
            ::generateTournament(
              tournament,
              std::move(matchesConfiguration),
              swissSystem,
              engine,
              checklistStream.get());
        }
        catch (const swisssystems::NoValidPairingException &exception)
        {
          std::cerr << "Error generating "
            << outputFilename
            << ": "
            << exception.what()
            << std::endl;
          result = NO_VALID_PAIRING;
        }
        catch (const swisssystems::UnapplicableFeatureException &exception)
        {
          std::cerr << "Error generating "
            << outputFilename
            << ": "
            << exception.what()
            << std::endl;
          return INVALID_REQUEST;
        }

        closeChecklist(checklistStream.get(), checklistFilename);

        // Output the generated tournament.
        try
        {
          if (modelFile)
          {
            fileformats::trf
              ::writeFile(outputStream, tournament, std::move(fileData));
          }
          else
          {
            fileformats::trf::writeFile(outputStream, tournament);
          }
        }
        catch (const fileformats::LimitExceededException &exception)
        {
          std::cerr << "Error writing tournament to "
            << outputFilename
            << ": "
            << exception.what()
            << std::endl;
          return LIMIT_EXCEEDED;
        }

        // Check for errors.
        outputStream.close();
        if (!outputStream)
        {
          std::cerr << "Error while writing to "
            << outputFilename
            << '.'
            << std::endl;
          return FILE_ERROR;
        }

        return result;
      }
      catch (const tournament::BuildLimitExceededException &exception)
      {
        std::cerr << "Error processing file "
          << outputFilename
          << ": "
          << exception.what()
          << std::endl;
        return LIMIT_EXCEEDED;
      }
      catch (const std::length_error &)
      {
        std::cerr
          << "Error processing file "
          << outputFilename
          << ": The build does not support tournaments this large."
          << std::endl;
        return LIMIT_EXCEEDED;
      }
      catch (const std::bad_alloc &)
      {
        std::cerr
          << "Error processing file "
          << outputFilename
          << ": The program ran out of memory."
          << std::endl;
        return LIMIT_EXCEEDED;
      }
    }
#endif

    return 0;
  }
  catch (const std::exception &exception)
  {
    std::cerr << "Unexpected error (please report): "
      << exception.what()
      << std::endl;
    return UNEXPECTED_ERROR;
  }
}
