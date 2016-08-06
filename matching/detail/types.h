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


#ifndef MATCHINGDETAILTYPES_H
#define MATCHINGDETAILTYPES_H

#include <tournament/tournament.h>
#include <utility/uinttypes.h>

namespace matching
{
  namespace detail
  {
    typedef tournament::player_index vertex_index;

    /**
     * A RootBlossom has label LABEL_ZERO iff it is exposed and its base has
     * dual variable zero.
     */
    enum Label : utility::uinttypes::uint_least_for_value<4>
    {
      LABEL_OUTER, LABEL_ZERO, LABEL_INNER, LABEL_FREE
    };
  }
}

#endif
