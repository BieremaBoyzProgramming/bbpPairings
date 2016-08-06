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


#ifndef BLOSSOMIMPL_H
#define BLOSSOMIMPL_H

#include <memory>

#include "blossomsig.h"
#include "rootblossomsig.h"

namespace matching
{
  namespace detail
  {
    template <typename edge_weight>
    inline Blossom<edge_weight>::Blossom(
        RootBlossom<edge_weight> &rootBlossom_,
        const bool isVertex_)
      : rootBlossom(&rootBlossom_), isVertex(isVertex_) { }

    template <typename edge_weight>
    inline Blossom<edge_weight>::~Blossom() = default;
  }
}

#include "rootblossomimpl.h"

#endif
