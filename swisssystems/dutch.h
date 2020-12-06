#ifndef DUTCH_H
#define DUTCH_H

#include <list>
#include <utility>

#include <matching/computer.h>
#include <utility/dynamicuint.h>

#include "common.h"

#ifndef OMIT_DUTCH
namespace tournament
{
  struct Tournament;
}

namespace swisssystems
{
  namespace dutch
  {
    typedef
      matching::computer_supporting_value<1>::type
      validity_matching_computer;
    typedef matching::Computer<utility::uinttypes::DynamicUint>
      optimality_matching_computer;

    std::list<Pairing> computeMatching(
      tournament::Tournament &&,
      std::ostream *const = nullptr);

    struct DutchInfo final : public Info
    {
      std::list<Pairing> computeMatching(
        tournament::Tournament &&tournament,
        std::ostream *const ostream
      ) const override
      {
        return dutch::computeMatching(std::move(tournament), ostream);
      }
    };
  }
}
#endif

#endif
