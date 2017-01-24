/*
 * This file is part of BBP Pairings, a Swiss-system chess tournament engine
 * Copyright (C) 2016  Jeremy Bierema
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3.0, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


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
