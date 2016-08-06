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


#ifndef COMPUTER_H
#define COMPUTER_H

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <utility/typesizes.h>
#include <utility/uinttypes.h>

#include "detail/graphsig.h"
#include "detail/rootblossomsig.h"
#include "detail/types.h"
#include "detail/vertexsig.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class Graph;
  }

  /**
   * A class used for constructing instances of the weighted matching problem
   * and solving them. It is implemented in time O(n^3) using the basic
   * algorithm presented in "An O(EV log V) Algorithm for Finding a Maximal
   * Weighted Matching in General Graphs," by Zvi Galil, Silvio Micali, and
   * Harold Gabow, 1986, with slight modification to allow for updates. An
   * update adding j nodes and changing weights spanned by k nodes takes time
   * O((j+k)n^2).
   *
   * The graph is considered to be complete. Edges that are not present have
   * weight zero, and the algorithm never includes such edges in the matching.
   */
  template <typename edge_weight_>
  class Computer
  {
  public:
    Computer();
    ~Computer() noexcept;
    Computer(Computer &) = delete;

    typedef detail::vertex_index vertex_index;
    /**
     * Edges can have weight at most
     * std::numeric_limits<edge_weight>::max() >> 2.
     */
    typedef edge_weight_ edge_weight;

    constexpr static vertex_index maxVertexIndex =
      utility::typesizes::minUint(
        std::numeric_limits<vertex_index>::max(),
        std::numeric_limits<typename detail::Graph<edge_weight>::size_type>
            ::max()
          - 1u,
        std::numeric_limits<
          typename
            decltype(std::declval<detail::Vertex<edge_weight>>().edgeWeights)
              ::size_type
        >::max() - 1u,
        std::numeric_limits<
          typename
            decltype(
              std::declval<detail::RootBlossom<edge_weight>>().minOuterEdges
            )::size_type
        >::max() - 1u
      );

    void addVertex() &;
    void setEdgeWeight(vertex_index, vertex_index, edge_weight) &;

    void computeMatching() const &;

    std::vector<vertex_index> getMatching() const;

  private:
    const std::unique_ptr<detail::Graph<edge_weight>> graph;
  };

  namespace
  {
    /**
     * A helper struct template used to determine the edge_weight type for a
     * Computer supporting edge weights of a given size.
     */
    template <std::uintmax_t MaxEdgeWeight>
    struct computer_supporting_value
    {
      static_assert(MaxEdgeWeight << 2 >> 2 >= MaxEdgeWeight, "Overflow");

      typedef
        Computer<
          utility::uinttypes::uint_least_for_value<MaxEdgeWeight << 2>
        >
        type;
    };
  }
}

#endif
