#include <deque>

#include "../templateinstantiation.h"

#include "blossomimpl.h"
#include "parentblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    /**
     * Initialize sibling links, parent links, and child links.
     */
    template <typename edge_weight>
    void ParentBlossom<edge_weight>::connectChildren(
      typename std::deque<Vertex<edge_weight> *>::const_iterator pathIterator,
      const typename std::deque<Vertex<edge_weight> *>::const_iterator pathEnd
    ) &
    {
      Blossom<edge_weight> *previousChild =
        &getAncestorOfVertex<edge_weight>(**pathIterator, nullptr);
      for (
        ;
        pathIterator != pathEnd;
        ++pathIterator
      )
      {
        previousChild->vertexToNextSiblingBlossom = *pathIterator;
        ++pathIterator;
        subblossom = &getAncestorOfVertex<edge_weight>(**pathIterator, nullptr);
        previousChild->nextBlossom = subblossom;
        subblossom->vertexToPreviousSiblingBlossom =
          *pathIterator;
        subblossom->parentBlossom = this;
        subblossom->previousBlossom = previousChild;
        previousChild = subblossom;
      }
    }
#define PARENT_BLOSSOM_INSTANTIATION(a) template class ParentBlossom<a>;
    INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(PARENT_BLOSSOM_INSTANTIATION)
  }
}
