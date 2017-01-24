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


#ifndef GRAPHSIG_H
#define GRAPHSIG_H

#include <type_traits>
#include <vector>

#include <utility/memory.h>

#include "types.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class ParentBlossom;
    template <typename>
    class RootBlossom;
    template <typename>
    struct Vertex;

    /**
     * The container for the vertices of the graph.
     */
    template <typename edge_weight>
    class Graph : public std::vector<Vertex<edge_weight>>
    {
    public:
      utility::memory::IterablePool<RootBlossom<edge_weight>> rootBlossomPool;
      utility::memory::IterablePool<ParentBlossom<edge_weight>>
        parentBlossomPool;

      /**
       * Dual variables of the Vertexes, indexed by vertexIndex.
       */
      typename edge_weight_traits<edge_weight>::vector vertexDualVariables;
      /**
       * RootBlossom minOuterEdgeResistances, indexed by
       * utility::memory::IterablePool<RootBlossom<edge_weight>>::getIndex().
       */
      typename edge_weight_traits<edge_weight>::vector
        rootBlossomMinOuterEdgeResistances;

      /**
       * A number that is strictly greater than twice the maximum edge weight
       * stored internally, that is, strictly greater than four times the
       * maximum edge weight passed in by the user.
       */
      edge_weight aboveMaxEdgeWeight;

      Graph(typename Graph<edge_weight>::size_type, const edge_weight &);
      Graph(Graph &) = delete;

      ~Graph();

      void computeMatching() &;

      void updateInnerOuterEdges(const RootBlossom<edge_weight> &) &;

    private:
      void initializeLabeling() const &;
      void initializeInnerOuterEdges() &;
      void initializeOuterOuterEdges(RootBlossom<edge_weight> &) const &;
      void initializeOuterOuterEdges(
        RootBlossom<edge_weight> &,
        edge_weight &,
        edge_weight &) const &;
      void initializeOuterOuterEdges() const &;
      void initializeMinOuterOuterEdgeResistance(
        RootBlossom<edge_weight> *&,
        edge_weight &
      ) const;
      void initializeMinInnerDualVariable(
        ParentBlossom<edge_weight> *&,
        edge_weight &
      ) const;

      bool augmentMatching() &;
    };

    template <typename edge_weight>
    void updateOuterOuterEdges(
      const RootBlossom<edge_weight> &,
      const RootBlossom<edge_weight> &,
      edge_weight &,
      typename
        std::conditional<
          std::is_trivially_copyable<edge_weight>::value,
          edge_weight,
          edge_weight &
        >::type);
  }
}

#endif
