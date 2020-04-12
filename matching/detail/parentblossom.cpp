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
