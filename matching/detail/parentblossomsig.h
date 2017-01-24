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
