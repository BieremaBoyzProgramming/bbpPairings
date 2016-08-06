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


#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

#include "computer.h"
#include "templateinstantiation.h"

#include "detail/blossommatchingiteratorimpl.h"
#include "detail/graphimpl.h"
#include "detail/rootblossomimpl.h"
#include "detail/rootblossomiteratorimpl.h"
#include "detail/types.h"
#include "detail/verteximpl.h"

namespace matching
{
  namespace
  {
    using namespace detail;
  }

  template <typename edge_weight>
  Computer<edge_weight>::Computer() : graph(new Graph<edge_weight>) { }
  template <typename edge_weight>
  Computer<edge_weight>::~Computer() noexcept = default;

  /**
   * Add a Vertex with the lowest unused vertexIndex.
   */
  template <typename edge_weight>
  void Computer<edge_weight>::addVertex() &
  {
    assert(graph->size() <= maxVertexIndex);

    for (const std::unique_ptr<Vertex<edge_weight>> &vertex : *graph) {
      vertex->edgeWeights.emplace_back();
    }
    for (
      RootBlossomIterator<edge_weight> iterator = graph->rootBlossomBegin();
      iterator != graph->rootBlossomEnd();
      ++iterator)
    {
      iterator->minOuterEdges.emplace_back();
    }
    graph->emplace_back(new Vertex<edge_weight>(graph->size()));
  }

  /**
   * Set the weight of the edge between modifiedVertex and neighbor. A weight of
   * zero is equivalent to a non-edge. After calls to setEdgeWeight using at
   * most k different values for modifiedVertex, the subsequent
   * computeMatching() operation will take O(k n^2) time.
   *
   * edgeWeight must be no greater than
   * std::numeric_limits<edge_weight>::max() >> 2.
   */
  template <typename edge_weight>
  void Computer<edge_weight>::setEdgeWeight(
    const vertex_index modifiedVertex,
    const vertex_index neighbor,
    const edge_weight edgeWeight) &
  {
    assert(modifiedVertex != neighbor);
    assert(modifiedVertex < graph->size());
    assert(neighbor < graph->size());
    assert(edgeWeight << 2 >> 2 == edgeWeight);

    graph->maxEdgeWeight = std::max(graph->maxEdgeWeight, edgeWeight * 2u);
    (*graph)[modifiedVertex]
      ->rootBlossom
      ->prepareVertexForWeightAdjustments(*(*graph)[modifiedVertex], *graph);
    (*graph)[modifiedVertex]->edgeWeights[neighbor] = edgeWeight << 1;
    (*graph)[neighbor]->edgeWeights[modifiedVertex] = edgeWeight << 1;
  }

  template <typename edge_weight>
  void Computer<edge_weight>::computeMatching() const &
  {
    graph->computeMatching();
  }

  /**
   * Return a vector, where each entry contains the index of the vertex matched
   * to the current index. An unmatched vertex is reported as being matched to
   * itself.
   *
   * computeMatching() must be called before this if any update operations have
   * been performed.
   */
  template <typename edge_weight>
  std::vector<vertex_index> Computer<edge_weight>::getMatching() const
  {
    std::vector<vertex_index> result(graph->size());
    for (
      RootBlossomIterator<edge_weight> rootBlossomIterator =
        graph->rootBlossomBegin();
      rootBlossomIterator != graph->rootBlossomEnd();
      ++rootBlossomIterator)
    {
      if (rootBlossomIterator->baseVertexMatch)
      {
        assert(
          rootBlossomIterator->baseVertex->edgeWeights[
            rootBlossomIterator->baseVertexMatch->vertexIndex]);

        result[rootBlossomIterator->baseVertex->vertexIndex] =
          rootBlossomIterator->baseVertexMatch->vertexIndex;
      }
      else
      {
        assert(!rootBlossomIterator->baseVertex->dualVariable);

        result[rootBlossomIterator->baseVertex->vertexIndex] =
          rootBlossomIterator->baseVertex->vertexIndex;
      }

      rootBlossomIterator->useMatchingIterators();
      for (
        BlossomMatchingIterator<edge_weight> matchingIterator =
          rootBlossomIterator->blossomMatchingBegin();
        matchingIterator != rootBlossomIterator->blossomMatchingEnd();
        ++matchingIterator)
      {
        Vertex<edge_weight> &firstVertex = *matchingIterator;

        ++matchingIterator;
        assert(matchingIterator != rootBlossomIterator->blossomMatchingEnd());
        result[firstVertex.vertexIndex] = matchingIterator->vertexIndex;
        result[matchingIterator->vertexIndex] = firstVertex.vertexIndex;

        assert(firstVertex.edgeWeights[matchingIterator->vertexIndex]);
      }
    }
    return result;
  }

  MATCHINGEDGEWEIGHTPARAMETERS1(template class Computer<, >;)
}
