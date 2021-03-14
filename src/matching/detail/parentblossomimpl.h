#ifndef PARENTBLOSSOMIMPL_H
#define PARENTBLOSSOMIMPL_H

#include <deque>
#include <iterator>

#include "blossomsig.h"
#include "parentblossomsig.h"
#include "rootblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    /**
     * Construct a blossom from the sequence of vertices referenced by begin
     * and end. Each subblossom is represented by two vertices, and
     * consecutive vertices in different subblossoms are the endpoints for the
     * edge connecting the two subblossoms.
     */
    template <typename edge_weight>
    template <class PathIterator>
    ParentBlossom<edge_weight>::ParentBlossom(
        RootBlossom<edge_weight> &rootBlossom_,
        const PathIterator begin,
        const PathIterator end)
      : Blossom<edge_weight>(
          rootBlossom_,
          *(*std::prev(end, 2))->rootBlossom->rootChild.vertexListHead,
          *(*begin)->rootBlossom->rootChild.vertexListTail,
          false),
          dualVariable((*begin)->dualVariable & 0u)
    {
      connectChildren(begin, end);
    }

    template <typename edge_weight>
    template <class PathIterator>
    void ParentBlossom<edge_weight>::connectChildren(
      const PathIterator begin,
      const PathIterator end) &
    {
      const std::deque<Vertex<edge_weight> *> path(begin, end);
      connectChildren(path.begin(), path.end());
    }
  }
}

#include "blossomimpl.h"

#endif
