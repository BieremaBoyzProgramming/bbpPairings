#ifndef LIM_H
#define LIM_H

#include <list>
#include <ostream>
#include <utility>

#include <matching/computer.h>
#include <utility/dynamicuint.h>

#include "common.h"

#ifndef OMIT_LIM
namespace tournament
{
  struct Tournament;
}

namespace swisssystems
{
  /**
   * An implementation of the FIDE Lim System, C.04.4.3 of the FIDE Handbook
   * (approved by the Council on 28/10/2025, applied from 1 February 2026).
   */
  namespace lim
  {
    typedef matching::Computer<utility::uinttypes::DynamicUint>
      matching_computer;

    std::list<Pairing> computeMatching(
      tournament::Tournament &&,
      std::ostream *const = nullptr);

    struct LimInfo final : public Info
    {
      std::list<Pairing> computeMatching(
        tournament::Tournament &&tournament,
        std::ostream *const ostream
      ) const override
      {
        return lim::computeMatching(std::move(tournament), ostream);
      }
    };
  }
}
#endif

#endif
