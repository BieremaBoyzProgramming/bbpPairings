#ifndef UINTFLOATCONVERSION_H
#define UINTFLOATCONVERSION_H

#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "typesizes.h"
#include "uint.h"
#include "uinttypes.h"

namespace utility
{
  namespace uintfloatconversion
  {
    /**
     * The value member indicates whether the maximum finite value of Float is
     * at least as big as the maximum value of Uint.
     */
    template <typename Float, typename Uint>
    struct float_is_bigger_than_uint
    {
    private:
      static constexpr bool biggerByBitAnalysis =
        (std::numeric_limits<Float>::max_exponent - 1u)
            / typesizes::bitsToRepresent<unsigned int>(
                std::numeric_limits<Float>::radix - 1u)
          > std::numeric_limits<Uint>::digits
              / typesizes::bitSupport<unsigned int>(
                  std::numeric_limits<Uint>::radix - 1u);

      static_assert(
        biggerByBitAnalysis
          || std::numeric_limits<Float>::max_exponent
                * typesizes::bitsToRepresent<unsigned int>(
                    std::numeric_limits<Float>::radix - 1u)
                / std::numeric_limits<Float>::max_exponent
              >= typesizes::bitsToRepresent<unsigned int>(
                    std::numeric_limits<Float>::radix - 1u),
        "Overflow");

    public:
      static constexpr bool value =
        biggerByBitAnalysis
          || uinttypes::uint_least<
                std::numeric_limits<Float>::max_exponent
                  * typesizes::bitsToRepresent<unsigned int>(
                      std::numeric_limits<Float>::radix - 1u)
              >(std::numeric_limits<Float>::max())
                >= std::numeric_limits<Uint>::max();
    };

    /**
     * A type alias for the type whose maximum finite value is at least as large
     * as the other's.
     */
    template <typename Float, typename Uint>
    using bigger_float_or_uint =
      typename
        std::conditional<
          float_is_bigger_than_uint<Float, Uint>::value,
          Float,
          Uint
        >::type;

    /**
     * Casts value to Float, but returns std::numeric_limits<Float>::max()
     * instead if value is not in the range of values representable by Float.
     */
    template <typename Float, typename Uint>
    constexpr Float safeCastToFloat(const Uint value)
    {
      return
        std::numeric_limits<Float>::has_infinity ? value
          : bigger_float_or_uint<Float, Uint>(value)
              > bigger_float_or_uint<Float, Uint>(
                  std::numeric_limits<Float>::max())
            ? std::numeric_limits<Float>::max()
          : value;
    }

    /**
     * Divide two Uints and store the result in type Float, even if the two
     * Uints cannot individually be represented in Float. If the quotient is not
     * in the range of Float (or near the edge),
     * std::numeric_limits<Float>::max() is returned instead.
     */
    template <typename Float, typename Uint0, typename Uint1>
    constexpr Float divide(const Uint0 uint0, const Uint1 uint1)
    {
      assert(uint1);
      typedef decltype(uint0 + uint1) Uint;
      if (float_is_bigger_than_uint<Float, Uint>::value)
      {
        return Float(uint0) / Float(uint1);
      }
      constexpr Float max =
        std::numeric_limits<Float>::has_infinity
          ? std::numeric_limits<Float>::infinity()
          : std::numeric_limits<Float>::max();
      constexpr int radix = std::numeric_limits<Float>::radix;
      Uint uint0current = uint0;
      Uint uint1current = uint1;
      uinttypes::uint<2> positiveExponent{ };
      while (uint0current > Uint(std::numeric_limits<Float>::max()))
      {
        uint0current /= radix;
        ++positiveExponent;
      }
      decltype(positiveExponent) negativeExponent{ };
      while (uint1current > Uint(std::numeric_limits<Float>::max()))
      {
        uint1current /= radix;
        ++negativeExponent;
      }
      const bool inverse = positiveExponent < negativeExponent;
      Float scalingFactor{ };
      if (
        !inverse
          && positiveExponent - negativeExponent
              >= std::numeric_limits<Float>::max_exponent)
      {
        assert(uint0current);
        if (std::numeric_limits<Float>::has_infinity)
        {
          scalingFactor = std::numeric_limits<Float>::infinity();
        }
        else
        {
          return max;
        }
      }
      else
      {
        scalingFactor =
          typesizes::exponential<Float, radix>(
            inverse
              ? negativeExponent - positiveExponent
              : positiveExponent - negativeExponent,
            inverse);
      }
      const Float quotient = Float(uint0current) / Float(uint1current);
      if (
        !std::numeric_limits<Float>::has_infinity
          && !inverse
          && std::numeric_limits<Float>::max() / scalingFactor <= quotient)
      {
        return max;
      }
      return quotient * scalingFactor;
    }

    /**
     * Multiply a Uint by a Float, even if the Uint is too large to fit in type
     * Float. The result is calculated with at most the precision offered by
     * Float.
     */
    template <typename Uint, typename Float>
    constexpr Uint multiply(const Uint uint, const Float floatValue)
    {
      if (float_is_bigger_than_uint<Float, Uint>::value)
      {
        return Float(uint) * floatValue;
      }
      constexpr int radix = std::numeric_limits<Float>::radix;
      Uint uintScale = 1;
      Uint uintCurrent = uint;
      while (uintCurrent > Uint(std::numeric_limits<Float>::max()))
      {
        uintCurrent /= radix;
        uintScale *= radix;
      }
      const Float convertedUint = uint;
      const int exponent =
        std::max(
          0,
          utility::typesizes::bitsToRepresent<int, radix>(floatValue)
            - (std::numeric_limits<Float>::max_exponent - 1)
            + utility::typesizes::bitsToRepresent<int, radix>(convertedUint));
      return
        uintScale
          * utility::typesizes::exponential<Uint, radix>(exponent)
          * Uint(
              convertedUint
                / utility::typesizes::exponential<Float, radix>(exponent)
                * floatValue);
    }
  }
}

#endif
