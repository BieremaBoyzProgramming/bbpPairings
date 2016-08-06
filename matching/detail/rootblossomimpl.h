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


#ifndef ROOTBLOSSOMIMPL_H
#define ROOTBLOSSOMIMPL_H

#include <cassert>
#include <deque>
#include <memory>
#include <vector>

#include "blossomsig.h"
#include "blossomiteratorsig.h"
#include "blossommatchingiteratorsig.h"
#include "blossomvertexiteratorsig.h"
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
     * Return a deque of the RootBlossoms of every other Vertex in the path.
     */
    template <typename edge_weight, class PathIterator>
    inline std::deque<std::shared_ptr<const RootBlossom<edge_weight>>>
      getRootBlossomsFromPath(
        PathIterator pathIterator,
        const PathIterator endIterator)
    {
      std::deque<std::shared_ptr<const RootBlossom<edge_weight>>> result;
      for (
        ;
        pathIterator != endIterator;
        ++++pathIterator)
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
        const vertex_index vertexIndex)
      : minOuterEdges(
          typename decltype(minOuterEdges)::size_type{ vertexIndex } + 1u),
        rootChild(child),
        baseVertex(&child)
    {
      assert(!minOuterEdges.empty());
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
        const Graph<edge_weight>& graph)
      : RootBlossom<edge_weight>(
          *new ParentBlossom<edge_weight>(*this, pathBegin, pathEnd),
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
        Vertex<edge_weight> *const baseVertexMatch_)
      : RootBlossom<edge_weight>(
          rootChild_,
          baseVertex_,
          baseVertexMatch_,
          LABEL_ZERO,
          nullptr,
          nullptr)
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
        Vertex<edge_weight> *const labeledVertex_)
      : minOuterEdges(rootChild_.rootBlossom->minOuterEdges.size()),
        rootChild(rootChild_),
        baseVertex(&baseVertex_),
        baseVertexMatch(baseVertexMatch_),
        label(label_),
        labelingVertex(labelingVertex_),
        labeledVertex(labeledVertex_)
    {
      rootChild.parentBlossom.reset();
      const std::shared_ptr<RootBlossom<edge_weight>> sharedPtr(this);
      for (
        BlossomIterator<edge_weight> iterator = blossomBegin();
        iterator != blossomEnd();
        ++iterator)
      {
        iterator->rootBlossom = sharedPtr;
      }
    }

    template <typename edge_weight>
    inline BlossomIterator<edge_weight> RootBlossom<edge_weight>::blossomBegin()
      const &
    {
      return BlossomIterator<edge_weight>(*this);
    }
    template <typename edge_weight>
    inline BlossomIterator<edge_weight> RootBlossom<edge_weight>::blossomEnd()
      const &
    {
      return BlossomIterator<edge_weight>();
    }

    template <typename edge_weight>
    inline BlossomVertexIterator<edge_weight>
      RootBlossom<edge_weight>::blossomVertexBegin() const &
    {
      return BlossomVertexIterator<edge_weight>(*this);
    }
    template <typename edge_weight>
    inline BlossomVertexIterator<edge_weight>
      RootBlossom<edge_weight>::blossomVertexEnd() const &
    {
      return BlossomVertexIterator<edge_weight>();
    }

    template <typename edge_weight>
    inline BlossomMatchingIterator<edge_weight>
      RootBlossom<edge_weight>::blossomMatchingBegin() const &
    {
      return BlossomMatchingIterator<edge_weight>(*this);
    }
    template <typename edge_weight>
    inline BlossomMatchingIterator<edge_weight>
      RootBlossom<edge_weight>::blossomMatchingEnd() const &
    {
      return BlossomMatchingIterator<edge_weight>();
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

      freeAncestorOfBase(vertex);

      vertex.dualVariable = graph.maxEdgeWeight;
    }

    template <typename edge_weight>
    inline RootBlossom<edge_weight>::RootBlossom(
        ParentBlossom<edge_weight> &rootChild_,
        const std::deque<std::shared_ptr<const RootBlossom<edge_weight>>>
          &rootBlossoms,
        const Graph<edge_weight> &graph,
        const RootBlossom<edge_weight> &baseRoot)
      : minOuterEdges(baseRoot.minOuterEdges),
        rootChild(rootChild_),
        baseVertex(baseRoot.baseVertex),
        baseVertexMatch(baseRoot.baseVertexMatch),
        label(baseRoot.label),
        labelingVertex(baseRoot.labelingVertex),
        labeledVertex(baseRoot.labeledVertex)
    {
      initializeFromChildren(rootBlossoms, graph);
    }
  }
}

#include "blossomiteratorimpl.h"
#include "blossommatchingiteratorimpl.h"
#include "blossomvertexiteratorimpl.h"
#include "parentblossomimpl.h"

#endif
