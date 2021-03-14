#ifndef ROOTBLOSSOMIMPL_H
#define ROOTBLOSSOMIMPL_H

#include <cassert>
#include <iterator>
#include <stdexcept>
#include <vector>

#include "blossomsig.h"
#include "parentblossomsig.h"
#include "rootblossomsig.h"
#include "vertexsig.h"
#include "types.h"
#include "graphsig.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class Graph;

    /**
     * Return a vector of the RootBlossoms of every other Vertex in the path.
     */
    template <typename edge_weight, class PathIterator>
    inline std::vector<RootBlossom<edge_weight> *>
      getRootBlossomsFromPath(
        PathIterator pathIterator,
        const PathIterator endIterator)
    {
      std::vector<RootBlossom<edge_weight> *> result;
      for (
        ;
        pathIterator != endIterator;
        std::advance(pathIterator, 2))
      {
        result.push_back((*pathIterator)->rootBlossom);
      }
      return result;
    }

    /**
     * Construct a new RootBlossom containing only a Vertex.
     */
    template <typename edge_weight>
    inline RootBlossom<edge_weight>::RootBlossom(
        Vertex<edge_weight> &child,
        const vertex_index vertexIndex,
        Graph<edge_weight> &graph)
      : minOuterEdges(
          typename decltype(minOuterEdges)::size_type{ vertexIndex } + 1u),
        minOuterEdgeResistance(
          graph.rootBlossomMinOuterEdgeResistances
            [graph.rootBlossomPool.getIndex(*this)]),
        rootChild(child),
        baseVertex(&child)
    {
      if (minOuterEdges.empty())
      {
        throw std::length_error("");
      }
    }
    /**
     * Construct a new RootBlossom, using the blossoms of the path of Vertexes
     * as the subblossoms.
     */
    template <typename edge_weight>
    template <class PathIterator>
    inline RootBlossom<edge_weight>::RootBlossom(
        const PathIterator pathBegin,
        const PathIterator pathEnd,
        Graph<edge_weight>& graph)
      : RootBlossom<edge_weight>(
          graph.parentBlossomPool.construct(*this, pathBegin, pathEnd),
          getRootBlossomsFromPath<edge_weight, PathIterator>(pathBegin, pathEnd
          ),
          graph,
          *(*pathBegin)->rootBlossom)
    { }
    /**
     * Construct a RootBlossom for the child of a current RootBlossom. This
     * should only be called when deconstructing the former RootBlossom and
     * when exiting the augmentation phase (since the augmentation data fields
     * are not set to meaningful values).
     */
    template <typename edge_weight>
    inline RootBlossom<edge_weight>::RootBlossom(
        Blossom<edge_weight> &rootChild_,
        Vertex<edge_weight> &baseVertex_,
        Vertex<edge_weight> *const baseVertexMatch_,
        Graph<edge_weight> &graph)
      : RootBlossom<edge_weight>(
          rootChild_,
          baseVertex_,
          baseVertexMatch_,
          LABEL_ZERO,
          nullptr,
          nullptr,
          graph)
    { }

    /**
     * Construct a RootBlossom for the child of a current RootBlossom. This
     * should only be called when deconstructing the former RootBlossom.
     */
    template <typename edge_weight>
    inline RootBlossom<edge_weight>::RootBlossom(
        Blossom<edge_weight> &rootChild_,
        Vertex<edge_weight> &baseVertex_,
        Vertex<edge_weight> *const baseVertexMatch_,
        const Label label_,
        Vertex<edge_weight> *const labelingVertex_,
        Vertex<edge_weight> *const labeledVertex_,
        Graph<edge_weight> &graph)
      : minOuterEdges(rootChild_.rootBlossom->minOuterEdges.size()),
        minOuterEdgeResistance(
          graph.rootBlossomMinOuterEdgeResistances
            [graph.rootBlossomPool.getIndex(*this)]),
        rootChild(rootChild_),
        baseVertex(&baseVertex_),
        baseVertexMatch(baseVertexMatch_),
        label(label_),
        labelingVertex(labelingVertex_),
        labeledVertex(labeledVertex_)
    {
      rootChild.parentBlossom = nullptr;
      rootChild.vertexListTail->nextVertex = nullptr;
      updateRootBlossomInDescendants(*this);
    }

    /**
     * Disconnect the vertex from its RootBlossom and its matched vertex, while
     * maintaining the invariant that resistances are nonnegative.
     */
    template <typename edge_weight>
    inline void RootBlossom<edge_weight>::prepareVertexForWeightAdjustments(
      Vertex<edge_weight> &vertex,
      Graph<edge_weight> &graph
    ) &
    {
      if (baseVertexMatch)
      {
        baseVertexMatch->rootBlossom->baseVertexMatch = nullptr;
        baseVertexMatch = nullptr;
      }
      baseVertex = &vertex;

      freeAncestorOfBase(vertex, graph);

      vertex.dualVariable = graph.aboveMaxEdgeWeight;
      vertex.dualVariable >>= 1;
    }

    template <typename edge_weight>
    inline RootBlossom<edge_weight>::RootBlossom(
        ParentBlossom<edge_weight> &rootChild_,
        const std::vector<RootBlossom<edge_weight> *> &rootBlossoms,
        Graph<edge_weight> &graph,
        const RootBlossom<edge_weight> &baseRoot)
      : minOuterEdges(baseRoot.minOuterEdges),
        minOuterEdgeResistance(
          graph.rootBlossomMinOuterEdgeResistances
            [graph.rootBlossomPool.getIndex(*this)]),
        rootChild(rootChild_),
        baseVertex(baseRoot.baseVertex),
        baseVertexMatch(baseRoot.baseVertexMatch),
        label(baseRoot.label),
        labelingVertex(baseRoot.labelingVertex),
        labeledVertex(baseRoot.labeledVertex)
    {
      initializeFromChildren(rootBlossoms, graph);
    }

    template <typename edge_weight>
    inline void RootBlossom<edge_weight>::updateRootBlossomInDescendants(
      RootBlossom<edge_weight> &newRootBlossom
    ) &
    {
      for (
        auto vertexIterator = rootChild.vertexListHead;
        vertexIterator;
        vertexIterator = vertexIterator->nextVertex)
      {
        vertexIterator->rootBlossom = &newRootBlossom;
        ParentBlossom<edge_weight> *parentBlossom =
          vertexIterator->parentBlossom;
        while (
          parentBlossom && parentBlossom->vertexListTail == vertexIterator)
        {
          parentBlossom->rootBlossom = &newRootBlossom;
          parentBlossom = parentBlossom->parentBlossom;
        }
      }
    }
  }
}

#include "parentblossomimpl.h"

#endif
