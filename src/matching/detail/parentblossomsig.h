#ifndef PARENTBLOSSOMSIG_H
#define PARENTBLOSSOMSIG_H

#include <deque>

#include "blossomsig.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class RootBlossom;
    template <typename>
    struct Vertex;

    /**
     * A class representing a blossom or subblossom that has children.
     */
    template <typename edge_weight>
    class ParentBlossom final : public Blossom<edge_weight>
    {
    public:
      edge_weight dualVariable;
      /**
       * Any of the subblossoms (although if
       * RootBlossom::putVerticesInMatchingOrder() is called, subblossom is the
       * subblossom containing the subblossom's base.
       */
      Blossom<edge_weight> *subblossom;
      /**
       * Used by RootBlossom::putVerticesInMatchingOrder() to remember whether
       * matching order iteration begins with the subblossom pointed to by
       * subblossom.
       */
      bool iterationStartsWithSubblossom{ };

      template <class PathIterator>
      ParentBlossom(RootBlossom<edge_weight> &, PathIterator, PathIterator);

    private:
      void connectChildren(
        typename std::deque<Vertex<edge_weight> *>::const_iterator,
        typename std::deque<Vertex<edge_weight> *>::const_iterator
      ) &;
      template <class PathIterator>
      void connectChildren(PathIterator, PathIterator) &;
    };
  }
}

#endif
