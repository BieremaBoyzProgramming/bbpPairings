/*
 * This file is part of BBP Pairings, a Swiss-system chess tournament engine
 * Copyright (C) 2016  Jeremy Bierema
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3.0, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <tournament/generator.h>
#include <tournament/tournament.h>
#include <utility/uintstringconversion.h>

#include "generatorconfiguration.h"
#include "types.h"

namespace fileformats
{
  namespace generatorconfiguration
  {
    /**
     * Read a configuration file and extract the parameters.
     *
     * @throws FileReaderException if the file could not be read.
     * @throws FileFormatException on malformed input.
     * @throws tournament::BuildLimitExceededException if parameters are not
     * supported by the constants in tournament.h.
     */
    void readFile(
      tournament::generator::Configuration &configuration,
      std::istream &inputStream)
    {
      while (inputStream.good())
      {
        std::string buffer;
        std::getline(inputStream, buffer);
        if (buffer.length() && buffer[0] != '#' && buffer[0] != '\r')
        {
          std::string propertyName;
          std::istringstream stringStream(buffer);
          std::getline(stringStream, propertyName, '=');
          std::string propertyValue;
          std::getline(stringStream, propertyValue, '\r');
          if (stringStream)
          {
            try
            {
              if (propertyName == "PlayersNumber")
              {
                configuration.playersNumber =
                  utility::uintstringconversion
                    ::parse<tournament::player_index>(propertyValue);
              }
              else if (propertyName == "RoundsNumber")
              {
                configuration.roundsNumber =
                  utility::uintstringconversion
                    ::parse<tournament::round_index>(propertyValue);
                configuration.tournament.expectedRounds =
                  configuration.roundsNumber;
              }
              else if (propertyName == "DrawPercentage")
              {
                configuration.drawPercentage =
                  utility::uintstringconversion::parse<
                    decltype(configuration.drawPercentage)
                  >(propertyValue);
                if (configuration.drawPercentage > 100)
                {
                  throw std::invalid_argument("");
                }
              }
              else if (propertyName == "ForfeitRate")
              {
                configuration.forfeitRate = std::stof(propertyValue);
                if (configuration.forfeitRate < 1)
                {
                  throw std::invalid_argument("");
                }
              }
              else if (propertyName == "RetiredRate")
              {
                configuration.retiredRate = std::stof(propertyValue);
                if (configuration.retiredRate < 2)
                {
                  throw std::invalid_argument("");
                }
              }
              else if (
                propertyName == "HalfPointByeRate"
                  || propertyName == "HalfPointByteRate")
              {
                configuration.halfPointByeRate = std::stof(propertyValue);
                if (configuration.halfPointByeRate < 1)
                {
                  throw std::invalid_argument("");
                }
              }
              else if (propertyName == "HighestRating")
              {
                configuration.highestRating =
                  utility::uintstringconversion
                    ::parse<tournament::rating>(propertyValue);
              }
              else if (propertyName == "LowestRating")
              {
                configuration.lowestRating =
                  utility::uintstringconversion
                    ::parse<tournament::rating>(propertyValue);
              }
              else if (propertyName == "PointsForWin")
              {
                configuration.tournament.pointsForWin =
                  utility::uintstringconversion
                    ::parse<tournament::points>(propertyValue, 1);
              }
              else if (propertyName == "PointsForDraw")
              {
                configuration.tournament.pointsForDraw =
                  utility::uintstringconversion
                    ::parse<tournament::points>(propertyValue, 1);
              }
              else if (propertyName == "Accelerated")
              {
                if (propertyValue == "1")
                {
                  configuration.automaticAcceleration = true;
                }
                else if (propertyValue == "0")
                {
                  configuration.automaticAcceleration = false;
                }
                else
                {
                  throw std::invalid_argument("");
                }
              }
              else
              {
                throw FileFormatException(
                  "Unexpected parameter \""
                    + propertyName
                    + "\" in configuration file.");
              }

              if (!stringStream)
              {
                throw std::invalid_argument("");
              }
            }
            catch (std::invalid_argument &)
            {
              throw FileFormatException(
                "The value for parameter \""
                  + propertyName
                  + "\" in the configuration file is invalid.");
            }
            catch (std::out_of_range &)
            {
              throw tournament::BuildLimitExceededException(
                "The value for parameter \""
                  + propertyName
                  + "\" in the configuration file is not supported by this "
                    "build.");
            }
          }
          else
          {
            throw FileFormatException(
              "Error parsing configuration file line: " + buffer);
          }
        }
      }
      if (!inputStream.eof())
      {
        throw FileReaderException("Error loading configuration file.");
      }
    }
  }
}
