#ifndef UINT_H
#define UINT_H
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>

#include "typesizes.h"

namespace utility
{
  namespace uinttypes
  {
    template <std::size_t>
    class uint;

    namespace detail
    {
      namespace
      {
        struct larger_uint_tag { };
        struct equal_uint_tag { };
        struct smaller_uint_tag { };
        template <std::size_t pieces0, std::size_t pieces1>
        using pieces_compare =
          typename
            std::conditional<
              pieces1 < pieces0,
              larger_uint_tag,
              typename
                std::conditional<
                  pieces0 < pieces1,
                  smaller_uint_tag,
                  equal_uint_tag
                >::type
            >::type;

        template <std::size_t pieces, typename T>
        using preferred_type =
          typename
            std::conditional<std::is_floating_point<T>::value, T, uint<pieces>>
              ::type;

        struct floating_tag { };
        struct integer_tag { };
      }
    }

    /**
     * A type for fixed-size unsigned integers larger than std::uintmax_t. The
     * data is stored in std::uintmax_t fields, with the template parameter
     * indicating the number of such fields.
     */
    template <std::size_t pieces>
    class uint
    {
      static_assert(
        (unsigned int)pieces
              * (unsigned int)std::numeric_limits<std::uintmax_t>::digits
              / pieces
            >= std::numeric_limits<std::uintmax_t>::digits
          && (unsigned int)pieces
                  * (unsigned int)std::numeric_limits<std::uintmax_t>::digits
                <= std::numeric_limits<int>::max(),
        "Overflow");

    public:
      constexpr uint() { }
      template <std::size_t pieces_>
      constexpr uint(const uint<pieces_> value)
        : uint<pieces>(value, detail::pieces_compare<pieces, pieces_>{ }) { }
      template <
        typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
      constexpr uint(const T value)
        : uint<pieces>(
            value,
            typename
              std::conditional<
                std::is_floating_point<T>::value,
                detail::floating_tag,
                detail::integer_tag
              >::type{ }
          )
      { }

      constexpr bool operator==(const uint<pieces> that) const
      {
        return highPiece == that.highPiece && lowPieces == that.lowPieces;
      }
      constexpr bool operator<(const uint<pieces> that) const
      {
        return
          std::tie(highPiece, lowPieces)
            < std::tie(that.highPiece, that.lowPieces);
      }
      constexpr bool operator>(const uint<pieces> that) const
      {
        return
          std::tie(highPiece, lowPieces)
            > std::tie(that.highPiece, that.lowPieces);
      }
      constexpr bool operator!=(const uint<pieces> that) const
      {
        return highPiece != that.highPiece || lowPieces != that.lowPieces;
      }
      constexpr bool operator<=(const uint<pieces> that) const
      {
        return
          std::tie(highPiece, lowPieces)
            <= std::tie(that.highPiece, that.lowPieces);
      }
      constexpr bool operator>=(const uint<pieces> that) const
      {
        return
          std::tie(highPiece, lowPieces)
            >= std::tie(that.highPiece, that.lowPieces);
      }

      constexpr uint<pieces> operator~() const
      {
        return uint<pieces>(~lowPieces, ~highPiece);
      }
      constexpr uint<pieces> operator-() const
      {
        return
          uint<pieces>(-lowPieces, lowPieces ? ~highPiece : -highPiece);
      }

      constexpr uint<pieces> operator<<(const unsigned int shift) const
      {
        uint<pieces> result = *this;
        result <<= shift;
        return result;
      }
      constexpr uint<pieces> operator>>(const unsigned int shift) const
      {
        uint<pieces> result = *this;
        result >>= shift;
        return result;
      }
      constexpr uint<pieces> operator&(const uint<pieces> that) const
      {
        uint<pieces> result = *this;
        result &= that;
        return result;
      }
      constexpr uint<pieces> operator|(const uint<pieces> that) const
      {
        uint<pieces> result = *this;
        result |= that;
        return result;
      }
      constexpr uint<pieces> operator+(uint<pieces> that) const
      {
        that += *this;
        return that;
      }
      constexpr uint<pieces> operator-(const uint<pieces> that) const
      {
        uint<pieces> result = *this;
        result -= that;
        return result;
      }
      constexpr uint<pieces> operator*(const std::uintmax_t that) const
      {
        static_assert(
          !(std::numeric_limits<std::uintmax_t>::digits % 2u),
          "Odd-length types are not supported.");
        constexpr unsigned int shift =
          std::numeric_limits<std::uintmax_t>::digits / 2u;
        return
          (highHalves().multiplyHalves(that >> shift) << shift << shift)
            + (highHalves().multiplyHalves(that << shift >> shift) << shift)
            + (lowHalves().multiplyHalves(that >> shift) << shift)
            + lowHalves().multiplyHalves(that << shift >> shift);
      }
      constexpr uint<pieces> operator/(const uint<pieces> that) const
      {
        return uint<pieces>(*this).modGetQuotient(that);
      }
      constexpr uint<pieces> operator%(const uint<pieces> that) const
      {
        uint<pieces> result = *this;
        result %= that;
        return result;
      }

      constexpr uint<pieces> &operator<<=(const unsigned int shift) &
      {
        constexpr unsigned int unsafeShift =
          std::numeric_limits<std::uintmax_t>::digits;
        if (shift >= unsafeShift)
        {
          highPiece = lowPieces.highPiece;
          lowPieces <<= unsafeShift;
          *this <<= shift - unsafeShift;
        }
        else if (shift)
        {
          highPiece =
            (highPiece << shift)
              | (lowPieces.highPiece >> (unsafeShift - shift));
          lowPieces <<= shift;
        }
        return *this;
      }
      constexpr uint<pieces> &operator>>=(const unsigned int shift) &
      {
        constexpr unsigned int unsafeShift =
          std::numeric_limits<std::uintmax_t>::digits;
        if (shift >= unsafeShift)
        {
          lowPieces >>= unsafeShift;
          lowPieces.highPiece = highPiece;
          highPiece = 0;
          *this >>= shift - unsafeShift;
        }
        else if (shift)
        {
          lowPieces >>= shift;
          lowPieces.highPiece |= highPiece << (unsafeShift - shift);
          highPiece >>= shift;
        }
        return *this;
      }
      constexpr uint<pieces> &operator&=(const uint<pieces> that) &
      {
        highPiece &= that.highPiece;
        lowPieces &= that.lowPieces;
        return *this;
      }
      constexpr uint<pieces> &operator|=(const uint<pieces> that) &
      {
        highPiece |= that.highPiece;
        lowPieces |= that.lowPieces;
        return *this;
      }
      constexpr uint<pieces> &operator+=(const uint<pieces> that) &
      {
        addCarry(that);
        return *this;
      }
      constexpr uint<pieces> &operator-=(const uint<pieces> that) &
      {
        *this += -that;
        return *this;
      }
      constexpr uint<pieces> &operator*=(const uint<pieces> that) &
      {
        *this = *this * that;
        return *this;
      }
      constexpr uint<pieces> &operator/=(const uint<pieces> that) &
      {
        *this = modGetQuotient(that);
        return *this;
      }
      constexpr uint<pieces> &operator%=(const uint<pieces> that) &
      {
        modGetQuotient(that);
        return *this;
      }

      constexpr uint<pieces> &operator++() &
      {
        *this += 1u;
        return *this;
      }
      constexpr uint<pieces> operator++(int) &
      {
        const uint<pieces> result = *this;
        ++*this;
        return result;
      }

      constexpr uint<pieces> &operator--() &
      {
        *this -= 1u;
        return *this;
      }
      constexpr uint<pieces> operator--(int) &
      {
        const uint<pieces> result = *this;
        --*this;
        return result;
      }

      template <
        typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
      explicit constexpr operator T() const
      {
        if (
          std::is_floating_point<T>::value
            && *this > uint<pieces>{ ~std::uintmax_t{ } })
        {
          const unsigned int radix = std::numeric_limits<T>::radix;
          const unsigned int exponent =
            typesizes::bitsToRepresent<unsigned int, radix>(~std::uintmax_t{ })
              - 1u;
          const std::uintmax_t divisor =
            typesizes::exponential<std::uintmax_t, radix>(exponent);

          uint<pieces> mod = *this;
          const uint<pieces> quotient = mod.modGetQuotient(divisor);
          assert(quotient < *this);
          return (T) (T{ quotient } * divisor + mod);
        }
        else if (std::is_same<T, bool>::value)
        {
          return highPiece || lowPieces;
        }
        else
        {
          return (T) lowPieces;
        }
      }

    private:
      uint<pieces - 1> lowPieces{ };
      std::uintmax_t highPiece{ };

      constexpr uint(
          const uint<pieces - 1> lowPieces_,
          const std::uintmax_t highPiece_)
        : lowPieces{ lowPieces_ }, highPiece{ highPiece_ } { }
      template <std::size_t pieces_>
      constexpr uint(const uint<pieces_> value, detail::larger_uint_tag)
        : lowPieces{ value } { }
      template <std::size_t pieces_>
      constexpr uint(const uint<pieces_> value, detail::smaller_uint_tag)
        : uint<pieces>(value.lowPieces) { }
      constexpr uint(const std::uintmax_t value, detail::integer_tag)
        : lowPieces{ value } { }
      template <typename T>
      constexpr uint(const T &value, detail::floating_tag)
      {
        assert(std::isfinite(value));
        assert(value > -1.f);
        constexpr unsigned int radix = std::numeric_limits<T>::radix;
        constexpr unsigned int exponent =
          typesizes::minUint(
            std::numeric_limits<T>::max_exponent,
            typesizes::bitsToRepresent<unsigned int, radix>(~std::uintmax_t{ })
          ) - 1u;
        constexpr std::uintmax_t divisor =
          typesizes::exponential<std::uintmax_t, radix>(exponent);
        if (value < divisor)
        {
          lowPieces = value;
        }
        else
        {
          *this = value / divisor;
          *this *= divisor;
          *this += value - *this;
        }
      }

      constexpr bool addCarry(const uint<pieces> that) &
      {
        const bool lowCarry = lowPieces.addCarry(that.lowPieces);
        highPiece += that.highPiece;
        const bool highCarry = highPiece < that.highPiece;
        highPiece += lowCarry;
        return highPiece < lowCarry || highCarry;
      }
      constexpr uint<pieces> lowHalves() const
      {
        constexpr std::size_t shift =
          std::numeric_limits<std::uintmax_t>::digits / 2u;
        return uint<pieces>(lowPieces.lowHalves(), highPiece << shift >> shift);
      }
      constexpr uint<pieces> highHalves() const
      {
        return
          uint<pieces>(
            lowPieces.highHalves(),
            highPiece >> std::numeric_limits<std::uintmax_t>::digits / 2u);
      }
      constexpr uint<pieces> multiplyHalves(const std::uintmax_t halfMultiplier)
      {
        return
          uint<pieces>(
            lowPieces.multiplyHalves(halfMultiplier),
            highPiece * halfMultiplier);
      }
      constexpr uint<pieces> modGetQuotient(const uint<pieces> divisor)
      {
        uint<pieces> quotient;

        for (
          unsigned int bit =
            std::numeric_limits<std::uintmax_t>::digits * pieces;
          bit-- > 0; )
        {
          const uint<pieces> bitDivisor = divisor << bit;
          if (bitDivisor >> bit == divisor && *this >= bitDivisor)
          {
            quotient |= uint<pieces>(1) << bit;
            *this -= bitDivisor;
          }
        }
        return quotient;
      }

      template <std::size_t>
      friend class uint;

      template <std::size_t pieces0, std::size_t pieces1>
      friend constexpr uint<std::max(pieces0, pieces1)>
        operator*(uint<pieces0>, uint<pieces1>);
    };

    template <>
    class uint<1>
    {
    public:
      constexpr uint() { }
      template <typename T>
      constexpr uint(const T value) : highPiece(value) {
        assert(
          !std::is_floating_point<T>::value || std::isfinite((long double)value)
        );
        assert(!std::is_floating_point<T>::value || value > -1.f);
      }

      constexpr bool operator==(const uint<1> that) const
      {
        return highPiece == that.highPiece;
      }
      constexpr bool operator<(const uint<1> that) const
      {
        return highPiece < that.highPiece;
      }
      constexpr bool operator>(const uint<1> that) const
      {
        return highPiece > that.highPiece;
      }
      constexpr bool operator!=(const uint<1> that) const
      {
        return highPiece != that.highPiece;
      }
      constexpr bool operator<=(const uint<1> that) const
      {
        return highPiece <= that.highPiece;
      }
      constexpr bool operator>=(const uint<1> that) const
      {
        return highPiece >= that.highPiece;
      }

      constexpr uint<1> operator~() const
      {
        return ~highPiece;
      }
      constexpr uint<1> operator-() const
      {
        return -highPiece;
      }

      constexpr uint<1> operator<<(const unsigned int shift) const
      {
        uint<1> result = *this;
        result <<= shift;
        return result;
      }
      constexpr uint<1> operator>>(const unsigned int shift) const
      {
        uint<1> result = *this;
        result >>= shift;
        return result;
      }
      constexpr uint<1> operator&(const uint<1> that) const
      {
        return highPiece & that.highPiece;
      }
      constexpr uint<1> operator|(const uint<1> that) const
      {
        return highPiece | that.highPiece;
      }
      constexpr uint<1> operator+(const uint<1> that) const
      {
        return highPiece + that.highPiece;
      }
      constexpr uint<1> operator-(const uint<1> that) const
      {
        return highPiece - that.highPiece;
      }
      constexpr uint<1> operator*(const uint<1> that) const
      {
        return highPiece * that.highPiece;
      }
      constexpr uint<1> operator/(const uint<1> that) const
      {
        return highPiece / that.highPiece;
      }
      constexpr uint<1> operator%(const uint<1> that) const
      {
        return highPiece % that.highPiece;
      }

      constexpr uint<1> &operator<<=(const unsigned int shift) &
      {
        if (shift >= std::numeric_limits<std::uintmax_t>::digits)
        {
          highPiece = 0;
        }
        else
        {
          highPiece <<= shift;
        }
        return *this;
      }
      constexpr uint<1> &operator>>=(const unsigned int that) &
      {
        if (that >= std::numeric_limits<std::uintmax_t>::digits)
        {
          highPiece = 0;
        }
        else
        {
          highPiece >>= that;
        }
        return *this;
      }
      constexpr uint<1> &operator&=(const uint<1> that) &
      {
        highPiece &= that.highPiece;
        return *this;
      }
      constexpr uint<1> &operator|=(const uint<1> that) &
      {
        highPiece |= that.highPiece;
        return *this;
      }
      constexpr uint<1> &operator+=(const uint<1> that) &
      {
        highPiece += that.highPiece;
        return *this;
      }
      constexpr uint<1> &operator-=(const uint<1> that) &
      {
        highPiece -= that.highPiece;
        return *this;
      }
      constexpr uint<1> &operator*=(const uint<1> that) &
      {
        highPiece *= that.highPiece;
        return *this;
      }
      constexpr uint<1> &operator/=(const uint<1> that) &
      {
        highPiece /= that.highPiece;
        return *this;
      }
      constexpr uint<1> &operator%=(const uint<1> that) &
      {
        highPiece %= that.highPiece;
        return *this;
      }

      constexpr uint<1> &operator++() &
      {
        ++highPiece;
        return *this;
      }
      constexpr uint<1> operator++(int) &
      {
        return highPiece++;
      }

      constexpr uint<1> &operator--() &
      {
        --highPiece;
        return *this;
      }
      constexpr uint<1> operator--(int) &
      {
        return highPiece--;
      }

      template <typename T>
      explicit constexpr operator T() const
      {
        return highPiece;
      }

    private:
      std::uintmax_t highPiece{ };

      constexpr bool addCarry(const uint<1> that) &
      {
        highPiece += that.highPiece;
        return highPiece < that.highPiece;
      }
      constexpr uint<1> lowHalves() const
      {
        constexpr std::size_t shift =
          std::numeric_limits<std::uintmax_t>::digits / 2u;
        return highPiece << shift >> shift;
      }
      constexpr uint<1> highHalves() const
      {
        return highPiece >> std::numeric_limits<std::uintmax_t>::digits / 2u;
      }
      constexpr uint<1> multiplyHalves(const std::uintmax_t halfMultiplier)
      {
        return highPiece * halfMultiplier;
      }

      template <std::size_t pieces>
      friend class uint;

      template <std::size_t pieces>
      friend constexpr uint<pieces> operator*(uint<pieces>, uint<1>);
    };

    template <std::size_t pieces0, std::size_t pieces1>
    constexpr bool operator==(
      const uint<pieces0> uint0,
      const uint<pieces1> uint1)
    {
      return
        pieces0 < pieces1
          ? uint<pieces1>{ uint0 } == uint1
          : uint0 == uint<pieces0>{ uint1 };
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator==(const T value0, const uint<pieces> value1)
    {
      return
        std::is_floating_point<T>::value
          ? value0 < T{ value1 }
          : uint<pieces>{ value0 } < value1;
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator==(const uint<pieces> value0, const T value1)
    {
      return
        std::is_floating_point<T>::value
          ? T{ value0 } == value1
          : value0 == uint<pieces>{ value1 };
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr bool operator<(
      const uint<pieces0> uint0,
      const uint<pieces1> uint1)
    {
      return
        pieces0 < pieces1
          ? uint<pieces1>{ uint0 } < uint1
          : uint0 < uint<pieces0>{ uint1 };
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator<(const T value0, const uint<pieces> value1)
    {
      return
        std::is_floating_point<T>::value
          ? value0 < T{ value1 }
          : uint<pieces>{ value0 } < value1;
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator<(const uint<pieces> value0, const T value1)
    {
      return
        std::is_floating_point<T>::value
          ? T{ value0 } < value1
          : value0 < uint<pieces>{ value1 };
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr bool operator>(
      const uint<pieces0> uint0,
      const uint<pieces1> uint1)
    {
      return
        pieces0 < pieces1
          ? uint<pieces1>{ uint0 } > uint1
          : uint0 > uint<pieces0>{ uint1 };
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator>(const T value0, const uint<pieces> value1)
    {
      return
        std::is_floating_point<T>::value
          ? value0 > T{ value1 }
          : uint<pieces>{ value0 } > value1;
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator>(const uint<pieces> value0, const T value1)
    {
      return
        std::is_floating_point<T>::value
          ? T{ value0 } > value1
          : value0 > uint<pieces>{ value1 };
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr bool operator!=(
      const uint<pieces0> uint0,
      const uint<pieces1> uint1)
    {
      return
        pieces0 < pieces1
          ? uint<pieces1>{ uint0 } != uint1
          : uint0 != uint<pieces0>{ uint1 };
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator!=(const T value0, const uint<pieces> value1)
    {
      return
        std::is_floating_point<T>::value
          ? value0 != T{ value1 }
          : uint<pieces>{ value0 } != value1;
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator!=(const uint<pieces> value0, const T value1)
    {
      return
        std::is_floating_point<T>::value
          ? T{ value0 } != value1
          : value0 != uint<pieces>{ value1 };
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr bool operator<=(
      const uint<pieces0> uint0,
      const uint<pieces1> uint1)
    {
      return
        pieces0 < pieces1
          ? uint<pieces1>{ uint0 } <= uint1
          : uint0 <= uint<pieces0>{ uint1 };
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator<=(const T value0, const uint<pieces> value1)
    {
      return
        std::is_floating_point<T>::value
          ? value0 <= T{ value1 }
          : uint<pieces>{ value0 } <= value1;
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator<=(const uint<pieces> value0, const T value1)
    {
      return
        std::is_floating_point<T>::value
          ? T{ value0 } <= value1
          : value0 <= uint<pieces>{ value1 };
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr bool operator>=(
      const uint<pieces0> uint0,
      const uint<pieces1> uint1)
    {
      return
        pieces0 < pieces1
          ? uint<pieces1>{ uint0 } >= uint1
          : uint0 >= uint<pieces0>{ uint1 };
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator>=(const T value0, const uint<pieces> value1)
    {
      return
        std::is_floating_point<T>::value
          ? value0 >= T{ value1 }
          : uint<pieces>{ value0 } >= value1;
    }
    template <std::size_t pieces, typename T>
    constexpr bool operator>=(const uint<pieces> value0, const T value1)
    {
      return
        std::is_floating_point<T>::value
          ? T{ value0 } >= value1
          : value0 >= uint<pieces>{ value1 };
    }

    template <std::size_t pieces0, std::size_t pieces1>
    constexpr uint<std::max(pieces0, pieces1)>
      operator&(const uint<pieces0> value0, const uint<pieces1> value1)
    {
      if (pieces0 < pieces1) {
        return uint<pieces1>{ value0 } & value1;
      }
      else
      {
        return value0 & uint<pieces0>{ value1 };
      }
    }
    template <std::size_t pieces, typename T>
    constexpr uint<pieces> operator&(const T value0, const uint<pieces> value1)
    {
      static_assert(!std::is_floating_point<T>::value, "Undefined.");
      return uint<pieces>{ value0 } & value1;
    }
    template <std::size_t pieces, typename T>
    constexpr uint<pieces> operator&(const uint<pieces> value0, const T value1)
    {
      static_assert(!std::is_floating_point<T>::value, "Undefined.");
      return value0 & uint<pieces>{ value1 };
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr uint<std::max(pieces0, pieces1)>
      operator|(const uint<pieces0> value0, const uint<pieces1> value1)
    {
      if (pieces0 < pieces1) {
        return uint<pieces1>{ value0 } | value1;
      }
      else
      {
        return value0 | uint<pieces0>{ value1 };
      }
    }
    template <std::size_t pieces, typename T>
    constexpr uint<pieces>
      operator|(const T value0, const uint<pieces> value1)
    {
      static_assert(!std::is_floating_point<T>::value, "Undefined.");
      return uint<pieces>{ value0 } | value1;
    }
    template <std::size_t pieces, typename T>
    constexpr uint<pieces>
      operator|(const uint<pieces> value0, const T value1)
    {
      static_assert(!std::is_floating_point<T>::value, "Undefined.");
      return value0 | uint<pieces>{ value1 };
    }

    template <std::size_t pieces0, std::size_t pieces1>
    constexpr uint<std::max(pieces0, pieces1)>
      operator+(const uint<pieces0> value0, const uint<pieces1> value1)
    {
      if (pieces0 < pieces1) {
        return uint<pieces1>{ value0 } + value1;
      }
      else
      {
        return value0 + uint<pieces0>{ value1 };
      }
    }
    template <std::size_t pieces, typename T>
    constexpr detail::preferred_type<pieces, T>
      operator+(const T value0, const uint<pieces> value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return value0 + T{ value1 };
      }
      else
      {
        return
          (detail::preferred_type<pieces, T>) uint<pieces>{ value0 } + value1;
      }
    }
    template <std::size_t pieces, typename T>
    constexpr detail::preferred_type<pieces, T>
      operator+(const uint<pieces> value0, const T value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return T{ value0 } + value1;
      }
      else
      {
        return value0 + uint<pieces>{ value1 };
      }
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr uint<std::max(pieces0, pieces1)>
      operator-(const uint<pieces0> value0, const uint<pieces1> value1)
    {
      if (pieces0 < pieces1)
      {
        return uint<pieces1>{ value0 } - value1;
      }
      else
      {
        return value0 - uint<pieces0>{ value1 };
      }
    }
    template <std::size_t pieces, typename T>
    constexpr detail::preferred_type<pieces, T>
      operator-(const T value0, const uint<pieces> value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return value0 - T{ value1 };
      }
      else
      {
        return
          (detail::preferred_type<pieces, T>) uint<pieces>{ value0 } - value1;
      }
    }
    template <std::size_t pieces, typename T>
    constexpr detail::preferred_type<pieces, T>
      operator-(const uint<pieces> value0, const T value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return T{ value0 } - value1;
      }
      else
      {
        return value0 - uint<pieces>{ value1 };
      }
    }
    template <std::size_t pieces>
    constexpr uint<pieces>
      operator*(const uint<pieces> value0, const uint<1> value1)
    {
      return value0 * value1.highPiece;
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr uint<std::max(pieces0, pieces1)>
      operator*(const uint<pieces0> value0, const uint<pieces1> value1)
    {
      if (pieces0 < pieces1)
      {
        return value1 * value0;
      }
      else
      {
        return
          value0 * value1.lowPieces
            + (value0 * value1.highPiece
                << std::numeric_limits<std::uintmax_t>::digits
                    * (pieces1 - 1u));
      }
    }
    template <std::size_t pieces, typename T>
    constexpr detail::preferred_type<pieces, T>
      operator*(const T value0, const uint<pieces> value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return value0 * T{ value1 };
      }
      else
      {
        return uint<pieces>{ value0 } * value1;
      }
    }
    template <
      std::size_t pieces,
      typename T,
      typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    constexpr detail::preferred_type<pieces, T>
      operator*(const uint<pieces> value0, const T value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return T{ value0 } * value1;
      }
      else
      {
        return value0 * std::uintmax_t{ value1 };
      }
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr uint<std::max(pieces0, pieces1)>
      operator/(const uint<pieces0> value0, const uint<pieces1> value1)
    {
      if (pieces0 < pieces1)
      {
        return uint<pieces1>{ value0 } / value1;
      }
      else
      {
        return value0 / uint<pieces0>{ value1 };
      }
    }
    template <std::size_t pieces, typename T>
    constexpr detail::preferred_type<pieces, T>
      operator/(const T value0, const uint<pieces> value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return value0 / T{ value1 };
      }
      else
      {
        return uint<pieces>{ value0 } / value1;
      }
    }
    template <std::size_t pieces, typename T>
    constexpr detail::preferred_type<pieces, T>
      operator/(const uint<pieces> value0, const T value1)
    {
      if (std::is_floating_point<T>::value)
      {
        return T{ value0 } / value1;
      }
      else
      {
        return value0 / uint<pieces>{ value1 };
      }
    }
    template <std::size_t pieces0, std::size_t pieces1>
    constexpr uint<std::max(pieces0, pieces1)>
      operator%(const uint<pieces0> value0, const uint<pieces1> value1)
    {
      if (pieces0 < pieces1)
      {
        return uint<pieces1>{ value0 } % value1;
      }
      else
      {
        return value0 + uint<pieces0>{ value1 };
      }
    }
    template <std::size_t pieces, typename T>
    constexpr uint<pieces> operator%(const T value0, const uint<pieces> value1)
    {
      return uint<pieces>{ value0 } % value1;
    }
    template <std::size_t pieces, typename T>
    constexpr uint<pieces> operator%(const uint<pieces> value0, const T value1)
    {
      return value0 % uint<pieces>{ value1 };
    }
  }
}

namespace std
{
  template <std::size_t pieces>
  struct numeric_limits<utility::uinttypes::uint<pieces>>
  {
    static constexpr bool is_specialized = true;
    static constexpr utility::uinttypes::uint<pieces> min() noexcept
    {
      return 0;
    }
    static constexpr utility::uinttypes::uint<pieces> max() noexcept
    {
      return ~utility::uinttypes::uint<pieces>{ };
    }
    static constexpr utility::uinttypes::uint<pieces> lowest() noexcept
    {
      return min();
    }
    static constexpr int digits =
      std::numeric_limits<std::uintmax_t>::digits * pieces;
    static constexpr int digits10 =
      utility::typesizes::bitSupport<unsigned int, 10>(max());
    static constexpr int max_digits10 =
      utility::typesizes::bitsToRepresent<unsigned int, 10>(max());
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr int radix =
      std::numeric_limits<std::uintmax_t>::radix;
    static constexpr utility::uinttypes::uint<pieces> epsilon() noexcept
    {
      return 0;
    }
    static constexpr utility::uinttypes::uint<pieces> round_error() noexcept
    {
      return 0;
    }
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr bool has_denorm = false;
    static constexpr bool has_denorm_loss = false;
    static constexpr utility::uinttypes::uint<pieces> infinity() noexcept
    {
      return 0;
    }
    static constexpr utility::uinttypes::uint<pieces> quiet_NaN() noexcept
    {
      return 0;
    }
    static constexpr utility::uinttypes::uint<pieces> signaling_NaN() noexcept
    {
      return 0;
    }
    static constexpr utility::uinttypes::uint<pieces> denorm_min() noexcept
    {
      return 0;
    }
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr bool traps = false;
    static constexpr bool tinyness_before = false;
    static constexpr std::float_round_style round_style =
      std::round_toward_zero;
  };
  template <std::size_t pieces>
  struct numeric_limits<const utility::uinttypes::uint<pieces>>
    : public numeric_limits<utility::uinttypes::uint<pieces>>
  { };
  template <std::size_t pieces>
  struct numeric_limits<volatile utility::uinttypes::uint<pieces>>
    : public numeric_limits<utility::uinttypes::uint<pieces>>
  { };
  template <std::size_t pieces>
  struct numeric_limits<const volatile utility::uinttypes::uint<pieces>>
    : public numeric_limits<utility::uinttypes::uint<pieces>>
  { };
}

#endif
