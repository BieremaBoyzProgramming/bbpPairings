#ifndef RANDOM_H
#define RANDOM_H

#include <cassert>
#include <cstdint>
#include <limits>
#include <random>

#include "typesizes.h"
#include "uint.h"

namespace utility
{
  namespace random
  {
    /**
     * Generate a uniformly random integer between two unsigned numbers
     * (inclusive).
     */
    template <typename T, typename U = T, typename RandomEngine>
    U uniformUint(
      RandomEngine &randomEngine,
      const T min = 0,
      const U max = std::numeric_limits<U>::max())
    {
      if (std::numeric_limits<U>::max() <= ~0ull)
      {
        return
          std::uniform_int_distribution<unsigned long long>
            ((unsigned long long) min, (unsigned long long) max)
            (randomEngine);
      }
      const U scale = U(~0ull) + 1u;
      const U difference = max - min;
      U scaledDifference = difference;
      U result;
      do
      {
        result = 0;
        U currentScale = 1u;
        while (scaledDifference)
        {
          // Add more bits from the standard generator, but no more than are
          // required.
          result |=
            std::uniform_int_distribution<unsigned long long>(
              0,
              typesizes::minUint(scaledDifference, ~0ull)
            )(randomEngine) * currentScale;
          scaledDifference /= scale;
          if (scaledDifference)
          {
            currentScale *= scale;
          }
        }
        // If the number we generated was too big, try again.
      } while (result > difference);
      result += min;
      return result;
    }
  }
}

#endif
