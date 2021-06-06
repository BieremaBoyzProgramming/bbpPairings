#ifndef UINTSTRINGCONVERSIONS_H
#define UINTSTRINGCONVERSIONS_H

#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>

#include "typesizes.h"
#include "uint.h"
#include "uinttypes.h"

namespace utility
{
  namespace uintstringconversion
  {
    template <
      typename Result = std::string,
      uinttypes::uint_least_for_value<10> radix = 10,
      typename T>
    inline Result toString(const T &value)
    {
      Result result;
      if (!value)
      {
        result += '0';
        return result;
      }
      T divisor =
        typesizes::exponential<T, radix>(
          typesizes::bitsToRepresent<uinttypes::uint<2>, radix>(value) - 1u);
      T currentValue = value;
      while (divisor)
      {
        result += '0' + char(currentValue / divisor);
        currentValue %= divisor;
        divisor /= radix;
      }
      return result;
    }

    /**
     * Return a string of the form 0.10, where the passed-in number is
     * considered 10^precision times the represented value.
     */
    template <
      typename Result = std::string,
      uinttypes::uint_least_for_value<10> radix = 10,
      typename T>
    inline Result
      toString(const T &value, const typename Result::size_type precision)
    {
      Result result = toString<Result, radix>(value);
      if (result.size() <= precision)
      {
        result.insert(0, precision - result.size() + 1u, '0');
      }
      result.insert(result.size() - precision, 1, '.');
      return result;
    }

    /**
     * Parse an unsigned integer from the beginning of the string, and advance
     * iterator to the next character.
     */
    template <
      typename Result,
      uinttypes::uint_least_for_value<10> radix = 10,
      typename T>
    inline Result parse(T &iterator, const T endIterator)
    {
      if (iterator == endIterator)
      {
        throw std::invalid_argument("");
      }
      Result result{ };
      while (
        iterator != endIterator
          && *iterator >= '0'
          && *iterator < char{ '0' + radix })
      {
        Result newValue = result * radix;
        if (newValue / radix < result)
        {
          throw std::out_of_range("");
        }
        result = *iterator - '0' + newValue;
        if (result < newValue)
        {
          throw std::out_of_range("");
        }
        ++iterator;
      }
      return result;
    }

    /**
     * Given a string consisting of an unsigned integer, parse it.
     */
    template <
      typename Result,
      uinttypes::uint_least_for_value<10> radix = 10,
      typename T>
    inline Result parse(const T &string)
    {
      auto iterator = std::cbegin(string);
      Result result = parse<Result, radix>(iterator, std::cend(string));
      if (iterator != std::cend(string))
      {
        throw std::invalid_argument("");
      }
      return result;
    }

    /**
     * Given a string consisting of an unsigned integer, parse it.
     */
    template <
      typename Result,
      uinttypes::uint_least_for_value<10> radix = 10,
      typename T>
    inline Result parse(const T *string)
    {
      const T *const stringEnd = string + std::strlen(string);
      Result result = parse<Result, radix>(string, stringEnd);
      if (string != stringEnd)
      {
        throw std::invalid_argument("");
      }
      return result;
    }

    /**
     * Parse a decimal with up to precision digits after the decimal point, and
     * returns it as the number times 10^precision. iterator is advanced to the
     * end of the parsed number.
     */
    template <
      typename Result,
      uinttypes::uint_least_for_value<10> radix = 10,
      typename T>
    inline Result parse(
      T &iterator,
      const T endIterator,
      typename std::iterator_traits<T>::difference_type precision)
    {
      Result value = parse<Result, radix>(iterator, endIterator);
      if (iterator != endIterator && *iterator == '.')
      {
        if (
          value
            && precision
                > typesizes::bitsToRepresent<uinttypes::uint<2>, radix>(
                    std::numeric_limits<Result>::max()))
        {
          throw std::out_of_range("");
        }
        ++iterator;
        const T startIterator = iterator;
        Result addend = parse<Result, radix>(iterator, endIterator);
        if (iterator - startIterator > precision)
        {
          throw std::invalid_argument("");
        }
        Result multiplier =
          typesizes::exponential<Result, radix>(iterator - startIterator);
        Result newValue = value * multiplier;
        if (newValue / multiplier < value)
        {
          throw std::out_of_range("");
        }
        value = newValue + addend;
        if (value < addend)
        {
          throw std::out_of_range("");
        }
        precision -= iterator - startIterator;
      }
      if (
        value
          && precision
              > typesizes::bitsToRepresent<uinttypes::uint<2>, radix>(
                  std::numeric_limits<Result>::max()))
      {
        throw std::out_of_range("");
      }
      Result multiplier = typesizes::exponential<Result, radix>(precision);
      Result result = value * multiplier;
      if (result / multiplier < value)
      {
        throw std::out_of_range("");
      }
      return result;
    }

    /**
     * Parse a string consisting of a decimal with up to precision digits after
     * the decimal point, and returns it as the number times 10^precision.
     */
    template <
      typename Result,
      uinttypes::uint_least_for_value<10> radix = 10,
      typename T>
    inline Result
      parse(const T &string, const typename T::size_type precision)
    {
      typename T::const_iterator iterator = string.begin();
      Result result = parse<Result, radix>(iterator, string.end(), precision);
      if (iterator != string.end())
      {
        throw std::invalid_argument("");
      }
      return result;
    }
  }
}

#endif
