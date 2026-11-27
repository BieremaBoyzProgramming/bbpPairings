#ifndef DUBOV_H
#define DUBOV_H

#include <list>
#include <ostream>
#include <utility>

#include <matching/computer.h>
#include <utility/dynamicuint.h>

#include "common.h"

#ifndef OMIT_DUBOV
namespace tournament
{
  struct Tournament;
}

namespace swisssystems
{
  /**
   * An implementation of the FIDE Dubov System, C.04.4.1 of the FIDE Handbook
   * (approved by the Council on 28/10/2025, applied from 1 February 2026).
   */
  namespace dubov
  {
    typedef matching::Computer<utility::uinttypes::DynamicUint>
      matching_computer;

    std::list<Pairing> computeMatching(
      tournament::Tournament &&,
      std::ostream *const = nullptr);

    struct DubovInfo final : public Info
    {
      std::list<Pairing> computeMatching(
        tournament::Tournament &&tournament,
        std::ostream *const ostream
      ) const override
      {
        return dubov::computeMatching(std::move(tournament), ostream);
      }
    };
  }
}
#endif

#endif
