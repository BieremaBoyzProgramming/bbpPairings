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
