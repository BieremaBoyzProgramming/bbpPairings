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


#ifndef FILEFORMATSTYPES_H
#define FILEFORMATSTYPES_H

#include <stdexcept>
#include <string>

namespace fileformats
{
  /**
   * An exception indicating that a file could not be read.
   */
  struct FileReaderException : public std::runtime_error
  {
    explicit FileReaderException(const std::string &explanation)
      : std::runtime_error(explanation) { }
  };

  /**
   * A FileReaderException indicating that the input file was malformed or had
   * inconsistent data.
   */
  struct FileFormatException : public FileReaderException
  {
    explicit FileFormatException(const std::string &explanation)
      : FileReaderException(explanation) { }
  };

  /**
   * An internal exception indicating that the line of the input file currently
   * being processed is malformed.
   */
  struct InvalidLineException : public FileReaderException
  {
    InvalidLineException() : FileReaderException("") { }
  };

  /**
   * An exception indicating that the tournament cannot be represented in the
   * output filetype because of format limitations.
   */
  struct LimitExceededException : std::runtime_error
  {
    explicit LimitExceededException(const std::string &explanation)
      : std::runtime_error(explanation) { }
  };
}

#endif
