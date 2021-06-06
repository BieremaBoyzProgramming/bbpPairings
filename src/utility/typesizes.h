#ifndef TYPESIZES_H
#define TYPESIZES_H

#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace utility
{
  namespace typesizes
  {
    namespace detail
    {
      namespace
      {
        template <typename, bool>
        struct make_unsigned_helper;

        template <typename T>
        struct make_unsigned_helper<T, true>
        {
          typedef std::make_unsigned_t<T> type;
        };
        template <typename T>
        struct make_unsigned_helper<T, false>
        {
          typedef T type;
        };
      }
    }

    /**
     * Refers to the unsigned version of T if T is signed, or to T otherwise.
     */
    template <typename T>
    using make_unsigned_if_signed =
      typename detail::make_unsigned_helper<T, std::is_signed<T>::value>::type;

    /**
     * Return the type with smallest max value.
     */
    template <
      typename MinSoFar,
      typename T = void,
      typename... Targs>
    struct smallest
    {
      typedef
        typename
          smallest<
            typename
              std::conditional<
                std::numeric_limits<T>::max()
                  < std::numeric_limits<MinSoFar>::max(),
                T,
                MinSoFar
              >::type,
            Targs...
          >::type
        type;
    };

    template <typename MinSoFar>
    struct smallest<MinSoFar>
    {
      typedef MinSoFar type;
    };

    /**
     * Return the minimum of two unsigned integers, returned in the smaller of
     * the two types (as determined by smallest<T, U>).
     */
    template <typename T, typename U>
    constexpr typename smallest<T, U>::type
      minUint(const T t, const U u)
    {
      if (make_unsigned_if_signed<T>(t) < make_unsigned_if_signed<U>(u))
      {
        return (typename smallest<T, U>::type) t;
      }
      else
      {
        return u;
      }
    }

    /**
     * Return the minimum of the unsigned integers, returned in the smallest of
     * the types (as determined by smallest<T, U, Targs...>).
     */
    template <typename T, typename U, typename ...Targs>
    constexpr typename smallest<T, U, Targs...>::type
      minUint(const T t, const U u, Targs... targs)
    {
      return minUint(minUint(t, u), targs...);
    }

    /**
     * Return the base-radix exponent to the given integer power. If inverse is
     * true, the exponent is first negated.
     *
     * The function operates recursively in time and depth logarithmic in the
     * magnitude of the exponent.
     */
    template <
      typename Result,
      std::uintmax_t radix = 2,
      typename Exponent>
    constexpr Result
      exponential(const Exponent exponent, const bool inverse = false)
    {
      if (exponent >= Exponent{ 2 })
      {
        const Result multiplier0 =
          exponential<Result, radix>(exponent / 2u, inverse);
        const Result multiplier1 =
          exponent % 2u
            ? multiplier0 * (inverse ? Result{ 1 } / radix : Result{ radix })
            : multiplier0;
        assert(
          inverse || !(exponent & 1u) || multiplier1 / radix == multiplier0);
        const Result result = multiplier0 * multiplier1;
        assert(inverse || result / multiplier0 == multiplier1);
        return result;
      }
      else if (exponent == 1u)
      {
        return inverse ? Result{ 1 } / radix : Result{ radix };
      }
      else if (exponent == 0u)
      {
        return 1;
      }
      else
      {
        return
          exponential<Result, radix>(Exponent{ 1 }, !inverse)
            * exponential<Result, radix>(-(exponent + 1), !inverse);
      }
    }

    /**
     * Calculate the number of base-radix digits needed to represent t
     * (truncated if floating) and all smaller nonnegative numbers.
     */
    template <typename Result, std::uintmax_t radix = 2, typename T>
    constexpr Result bitsToRepresent(const T &t)
    {
      assert(t >= 0);
      T currentValue = t;
      Result result{ };
      while (currentValue >= 1u)
      {
        ++result;
        assert(result);
        currentValue /= radix;
      }
      return result;
    }

    /**
     * Calculate the maximum number of base-radix digits such that all unsigned
     * integers using that many digits have value t or less.
     */
    template <typename Result, std::uintmax_t radix = 2, typename T>
    constexpr Result bitSupport(const T &t)
    {
      assert(t >= 0u);
      Result result{ };
      T divisor = 1u;
      while (t / divisor >= radix)
      {
        ++result;
        assert(result);
        divisor *= radix;
      }
      if (t - divisor + 1u >= (radix - 1u) * divisor)
      {
        ++result;
        assert(result);
      }
      return result;
    }
  }
}

#endif
