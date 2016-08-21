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

#include "blossomsig.h"
#include "graphsig.h"
#include "rootblossomsig.h"
#include "types.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    /**
     * Construct a new Vertex and its parent RootBlossom.
     */
    template <typename edge_weight>
    inline Vertex<edge_weight>::Vertex(
        const vertex_index vertexIndex_,
        Graph<edge_weight> &graph)
      : Blossom<edge_weight>(
          graph.rootBlossomPool.construct(*this, vertexIndex_, graph),
          *this,
          *this,
          true),
        dualVariable(graph.vertexDualVariables[vertexIndex_]),
        minOuterEdgeResistance(graph.aboveMaxEdgeWeight),
        vertexIndex(vertexIndex_) { }

    /**
     * Determine the resistance between two Vertexes in different RootBlossoms.
     */
    template <typename edge_weight>
    inline void Vertex<edge_weight>::resistance(
      edge_weight &result,
      const Vertex<edge_weight> &that
    ) const
    {
      assert(this->rootBlossom != that.rootBlossom);
      result = dualVariable;
      edge_weight_traits<edge_weight>::addSubtract(
        result,
        that.dualVariable,
        edgeWeights[that.vertexIndex]);
    }
    template <typename edge_weight>
    inline edge_weight Vertex<edge_weight>::resistance(
      const Vertex<edge_weight> &that
    ) const
    {
      edge_weight result{ dualVariable };
      edge_weight_traits<edge_weight>::addSubtract(
        result,
        that.dualVariable,
        edgeWeights[that.vertexIndex]);
      return result;
    }
  }
}

#include "blossomimpl.h"
#include "rootblossomimpl.h"

#endif
