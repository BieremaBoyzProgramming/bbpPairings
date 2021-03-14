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
