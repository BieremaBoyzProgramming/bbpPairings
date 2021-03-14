#include <cassert>
#include <limits>
#include <vector>

#include "computer.h"
#include "templateinstantiation.h"

#include "detail/graphimpl.h"
#include "detail/rootblossomimpl.h"
#include "detail/types.h"
#include "detail/verteximpl.h"

namespace matching
{
  namespace
  {
    using namespace detail;
  }

  /**
   * Construct a new maximum matching computer that supports at most the
   * specified number of nodes and at most the specified edge weight.
   */
  template <typename edge_weight>
  Computer<edge_weight>::Computer(
      const Computer<edge_weight>::size_type capacity,
      const edge_weight &maxEdgeWeight)
    : graph(new Graph<edge_weight>(capacity, maxEdgeWeight)) { }
  template <typename edge_weight>
  Computer<edge_weight>::~Computer() noexcept = default;

  /**
   * Return the number of nodes currently in the matching graph.
   */
  template <typename edge_weight>
  auto Computer<edge_weight>::size() const -> Computer<edge_weight>::size_type
  {
    return graph->size();
  }

  /**
   * Add a Vertex with the lowest unused vertexIndex.
   */
  template <typename edge_weight>
  void Computer<edge_weight>::addVertex() &
  {
    assert(graph->size() <= std::numeric_limits<vertex_index>::max());
    assert(graph->size() < graph->capacity());

    for (
      auto iterator = graph->rootBlossomPool.begin();
      iterator != graph->rootBlossomPool.end();
      ++iterator)
    {
      iterator->minOuterEdges.emplace_back();
    }
    graph->emplace_back(graph->size(), *graph);
    for (Vertex<edge_weight> &vertex : *graph) {
      if (&vertex != &graph->back())
      {
        vertex.edgeWeights.push_back(graph->aboveMaxEdgeWeight & 0u);
      }
      graph->back().edgeWeights.push_back(graph->aboveMaxEdgeWeight & 0u);
    }
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
    edge_weight edgeWeight) &
  {
    assert(modifiedVertex != neighbor);
    assert(modifiedVertex < graph->size());
    assert(neighbor < graph->size());
    assert((graph->aboveMaxEdgeWeight - 1u) >> 2 >= edgeWeight);
    assert(edgeWeight << 2 < graph->aboveMaxEdgeWeight);

    edgeWeight <<= 1;
    (*graph)[modifiedVertex]
      .rootBlossom
      ->prepareVertexForWeightAdjustments((*graph)[modifiedVertex], *graph);
    (*graph)[modifiedVertex].edgeWeights[neighbor] = edgeWeight;
    (*graph)[neighbor].edgeWeights[modifiedVertex] = std::move(edgeWeight);
  }

  template <typename edge_weight>
  void Computer<edge_weight>::computeMatching() const &
  {
    graph->computeMatching();
    for (
      auto rootBlossomIterator = graph->rootBlossomPool.begin();
      rootBlossomIterator != graph->rootBlossomPool.end();
      ++rootBlossomIterator)
    {
      rootBlossomIterator->putVerticesInMatchingOrder();
    }
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
      auto rootBlossomIterator = graph->rootBlossomPool.begin();
      rootBlossomIterator != graph->rootBlossomPool.end();
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

      assert(
        rootBlossomIterator->baseVertex
          == rootBlossomIterator->rootChild.vertexListHead);

      for (
        auto matchingIterator =
          rootBlossomIterator->rootChild.vertexListHead->nextVertex;
        matchingIterator;
        matchingIterator = matchingIterator->nextVertex)
      {
        Vertex<edge_weight> &firstVertex = *matchingIterator;

        matchingIterator = matchingIterator->nextVertex;
        assert(matchingIterator);
        result[firstVertex.vertexIndex] = matchingIterator->vertexIndex;
        result[matchingIterator->vertexIndex] = firstVertex.vertexIndex;

        assert(firstVertex.edgeWeights[matchingIterator->vertexIndex]);
      }
    }
    return result;
  }

#define COMPUTER_INSTANTIATION(a) template class Computer<a>;
    INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(COMPUTER_INSTANTIATION)
}
