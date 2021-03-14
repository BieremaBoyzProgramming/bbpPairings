#ifndef UNSIGNEDTYPES_H
#define UNSIGNEDTYPES_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "typesizes.h"
#include "uint.h"

namespace utility
{
  namespace uinttypes
  {
    namespace detail
    {
      template <template<typename...> class T>
      using append_known_uint_types =
        T<
          std::uintmax_t,
          std::make_unsigned<std::ptrdiff_t>::type,
          std::size_t,
          std::uint_least8_t,
          std::uint_least16_t,
          std::uint_least32_t,
          std::uint_least64_t,
          std::uint_fast8_t,
          std::uint_fast16_t,
          std::uint_fast32_t,
          std::uint_fast64_t,
          unsigned char,
          unsigned short int,
          unsigned int,
          unsigned long int,
          unsigned long long int
        >;

      namespace
      {
        template <
          std::uintmax_t bits,
          typename MinSoFar,
          typename T = void,
          typename... Targs>
        struct smallest_supporting
        {
          typedef
            typename
              smallest_supporting<
                bits,
                typename
                  std::conditional<
                    (std::numeric_limits<typesizes::make_unsigned_if_signed<T>>
                          ::max()
                        < std::numeric_limits<MinSoFar>::max()
                      && typesizes::bitSupport<
                            uint<
                              typesizes::bitsToRepresent<unsigned int>(
                                std::numeric_limits<T>::radix - 1u)
                            >
                          >(
                            std::numeric_limits<
                              typesizes::make_unsigned_if_signed<T>
                            >::max()
                          ) >= bits
                    ) || typesizes::bitSupport<
                            uint<
                              typesizes::bitsToRepresent<unsigned int>(
                                std::numeric_limits<MinSoFar>::radix - 1u)
                            >
                          >(std::numeric_limits<MinSoFar>::max()) < bits,
                    typesizes::make_unsigned_if_signed<T>,
                    MinSoFar
                  >::type,
                Targs...
              >::type
            type;
        };

        template <std::uintmax_t bits, typename MinSoFar>
        struct smallest_supporting<bits, MinSoFar>
        {
          typedef MinSoFar type;
        };

        template <std::uintmax_t bits, typename... MoreTypes>
        struct smallest_supporting_bind
        {
          template <typename... KnownTypes>
          using type =
            typename
              smallest_supporting<bits, MoreTypes..., KnownTypes...>::type;
        };

        template <
          std::uintmax_t value,
          typename MinSoFar,
          typename T = void,
          typename... Targs>
        struct smallest_supporting_value
        {
          typedef
            typename
              smallest_supporting_value<
                value,
                typename
                  std::conditional<
                    (std::numeric_limits<typesizes::make_unsigned_if_signed<T>>
                            ::max()
                          < std::numeric_limits<MinSoFar>::max()
                        && std::numeric_limits<
                                typesizes::make_unsigned_if_signed<T>
                              >::max()
                            >= value
                    ) || std::numeric_limits<MinSoFar>::max() < value,
                    typesizes::make_unsigned_if_signed<T>,
                    MinSoFar
                  >::type,
                Targs...
              >::type
            type;
        };

        template <std::uintmax_t value, typename MinSoFar>
        struct smallest_supporting_value<value, MinSoFar>
        {
          typedef MinSoFar type;
        };

        template <std::uintmax_t value, typename... MoreTypes>
        struct smallest_supporting_value_bind
        {
          template <typename... KnownTypes>
          using type =
            typename
              smallest_supporting_value<value, MoreTypes..., KnownTypes...>
                ::type;
        };

        template <std::uintmax_t bits, typename T>
        struct check_fit
        {
          typedef
            typename
              std::conditional<
                bits
                  <= typesizes::bitSupport<
                        uint<
                          typesizes::bitsToRepresent<unsigned int>(
                            std::numeric_limits<T>::radix - 1u)
                        >
                      >(std::numeric_limits<T>::max()),
                T,
                uint<
                  (bits - !!bits) / std::numeric_limits<std::uintmax_t>::digits
                    + 1u>
              >::type
            type;
        };
      }
    }

    /**
     * The smallest integral type, either from the above list of known unsigned
     * types or from those passed as a template parameter (which are transformed
     * to unsigned types if possible), that supports all unsigned integers with
     * the specified number of bits.
     */
    template <std::uintmax_t bits, typename... Targs>
    using uint_least =
      typename
        detail::check_fit<
          bits,
          detail::append_known_uint_types<
            detail::smallest_supporting_bind<bits, Targs...>::template type
          >
        >::type;

    /**
     * The smallest integral type, either from the above list of known unsigned
     * types or from those passed as a template parameter (which are transformed
     * to unsigned types if possible), that can represent the specified value.
     */
    template <std::uintmax_t maxValue, typename... Targs>
    using uint_least_for_value =
      detail::append_known_uint_types<
        detail::smallest_supporting_value_bind<maxValue, Targs...>
          ::template type
      >;
  }
}

#endif
