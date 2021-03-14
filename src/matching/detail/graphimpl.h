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
