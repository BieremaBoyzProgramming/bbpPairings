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


#ifndef VERTEXIMPL_H
#define VERTEXIMPL_H

#include <cassert>
#include <memory>

#include "blossomsig.h"
#include "rootblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    /**
     * Construct a new Vertex and its parent RootBlossom.
     */
    template <typename edge_weight>
    inline Vertex<edge_weight>::Vertex(const vertex_index vertexIndex_)
      : Blossom<edge_weight>(
          *new RootBlossom<edge_weight>(*this, vertexIndex_),
          true),
        edgeWeights(
          typename decltype(edgeWeights)::size_type{ vertexIndex_ }
            + 1u),
        vertexIndex(vertexIndex_)
    {
      assert(!edgeWeights.empty());
    }

    /**
     * Determine the resistance between two Vertexes in different RootBlossoms.
     */
    template <typename edge_weight>
    inline edge_weight
      Vertex<edge_weight>::resistance(const Vertex<edge_weight>& that) const
    {
      assert(this->rootBlossom != that.rootBlossom);
      return dualVariable + that.dualVariable - edgeWeights[that.vertexIndex];
    }
  }
}

#include "blossomimpl.h"
#include "rootblossomimpl.h"

#endif
