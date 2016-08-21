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

#include <cassert>
#include <stdexcept>

#include "graphsig.h"
#include "parentblossomsig.h"
#include "rootblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    template <typename edge_weight>
    inline Graph<edge_weight>::Graph(
        const typename Graph<edge_weight>::size_type capacity,
        const edge_weight &maxEdgeWeight)
      : rootBlossomPool(
          typename decltype(rootBlossomPool)::size_type{ capacity } + 1u),
        parentBlossomPool(capacity / 2u),
        vertexDualVariables(capacity, maxEdgeWeight & 0u),
        rootBlossomMinOuterEdgeResistances(
          typename
              decltype(rootBlossomMinOuterEdgeResistances)
                ::size_type{ capacity }
            + 1u,
          maxEdgeWeight),
        aboveMaxEdgeWeight((maxEdgeWeight << 2) + 1u)
    {
      assert(aboveMaxEdgeWeight >> 2 == maxEdgeWeight);
      if (!(typename decltype(rootBlossomPool)::size_type{ capacity } + 1u))
      {
        throw std::length_error("");
      }
      if (
        typename decltype(parentBlossomPool)::size_type{ capacity / 2u }
          < capacity / 2u)
      {
        throw std::length_error("");
      }
      if (capacity > ~typename decltype(vertexDualVariables)::size_type{ })
      {
        throw std::length_error("");
      }
      if (
        !(typename
              decltype(rootBlossomMinOuterEdgeResistances)
                ::size_type{ capacity }
            + 1u)
      )
      {
        throw std::length_error("");
      }
      this->reserve(capacity);
    }

    template <typename edge_weight>
    inline Graph<edge_weight>::~Graph() = default;
  }
}

#endif
