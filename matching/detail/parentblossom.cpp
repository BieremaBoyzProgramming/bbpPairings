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
#include <memory>

#include "../templateinstantiation.h"

#include "blossomsig.h"
#include "parentblossomimpl.h"
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
      const std::shared_ptr<ParentBlossom<edge_weight>> pointer(this);

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
        subblossom->parentBlossom = pointer;
        subblossom->previousBlossom = previousChild;
        previousChild = subblossom;
      }
    }

    MATCHINGEDGEWEIGHTPARAMETERS1(template struct ParentBlossom<, >;)
  }
}
