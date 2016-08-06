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


#ifndef GRAPHIMPL_H
#define GRAPHIMPL_H

#include <memory>
#include <vector>

#include "graphsig.h"
#include "rootblossomiteratorsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    template <typename edge_weight>
    inline Graph<edge_weight>::Graph() = default;

    template <typename edge_weight>
    inline Graph<edge_weight>::~Graph() = default;

    template <typename edge_weight>
    inline RootBlossomIterator<edge_weight>
      Graph<edge_weight>::rootBlossomBegin() const &
    {
      return RootBlossomIterator<edge_weight>(this->begin(), this->end());
    }
    template <typename edge_weight>
    inline RootBlossomIterator<edge_weight> Graph<edge_weight>::rootBlossomEnd()
      const &
    {
      return RootBlossomIterator<edge_weight>(this->end());
    }
  }
}

#include "rootblossomiteratorimpl.h"
#include "verteximpl.h"

#endif
