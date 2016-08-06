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

#include <memory>
#include <vector>

namespace matching
{
  namespace detail
  {
    template <typename>
    class RootBlossom;
    template <typename>
    class RootBlossomIterator;
    template <typename>
    struct Vertex;

    /**
     * The container for the vertices of the graph.
     */
    template <typename edge_weight>
    class Graph : public std::vector<std::unique_ptr<Vertex<edge_weight>>>
    {
    public:
      edge_weight maxEdgeWeight{ };

      Graph();
      Graph(Graph &) = delete;

      ~Graph();

      RootBlossomIterator<edge_weight> rootBlossomBegin() const &;
      RootBlossomIterator<edge_weight> rootBlossomEnd() const &;

      void computeMatching() const &;

      void updateInnerOuterEdges(const RootBlossom<edge_weight> &) const &;

    private:
      void initializeLabeling() const &;
      void initializeInnerOuterEdges() const &;
      void initializeOuterOuterEdges(RootBlossom<edge_weight> &) const &;
      void initializeOuterOuterEdges() const &;

      bool augmentMatching() const &;
    };

    template <typename edge_weight>
    edge_weight updateOuterOuterEdges(
      const RootBlossom<edge_weight> &,
      const RootBlossom<edge_weight> &,
      edge_weight);
  }
}

#endif
