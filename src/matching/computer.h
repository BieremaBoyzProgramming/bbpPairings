#ifndef COMPUTER_H
#define COMPUTER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <utility/uinttypes.h>

#include "detail/graphsig.h"
#include "detail/types.h"

namespace matching
{
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
    typedef typename detail::Graph<edge_weight_>::size_type size_type;

    Computer(size_type, const edge_weight_ &);
    ~Computer() noexcept;
    Computer(Computer &) = delete;

    typedef detail::vertex_index vertex_index;
    /**
     * Edges can have weight at most
     * std::numeric_limits<edge_weight>::max() >> 2.
     */
    typedef edge_weight_ edge_weight;

    size_type size() const;

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
