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


#include "../templateinstantiation.h"

#include "blossomsig.h"
#include "parentblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    /**
     * Traverse the parent links up from vertex until reaching ancestor. Return
     * the child of ancestor through which we arrived.
     *
     * Takes time O(k), where k is the number of links traversed.
     */
    template <typename edge_weight>
    Blossom<edge_weight> &getAncestorOfVertex(
      Vertex<edge_weight> &vertex,
      const ParentBlossom<edge_weight> *const ancestor)
    {
      Blossom<edge_weight> *blossom = &vertex;
      while (blossom->parentBlossom.get() != ancestor)
      {
        blossom = blossom->parentBlossom.get();
      }
      return *blossom;
    }

#define COMMA ,
#define LPAREN (
#define RPAREN )

    MATCHINGEDGEWEIGHTPARAMETERS3(
      template Blossom<,
      > &getAncestorOfVertex LPAREN Vertex<,
      > & COMMA const ParentBlossom<,
      > * RPAREN ;
    )

    /**
     * Set the subblossom pointers on the path between vertex and ancestor so
     * that they point to vertex. Also set all the iterationStartsWithSubblossom
     * flags to the provided flag.
     */
    template <typename edge_weight>
    void setPointersFromAncestor(
      Vertex<edge_weight> &vertex,
      const Blossom<edge_weight> &ancestor,
      const bool startWithSubblossom)
    {
      Blossom<edge_weight> *blossom = &vertex;
      while (blossom != &ancestor)
      {
        blossom->parentBlossom->subblossom = blossom;
        blossom->parentBlossom->iterationStartsWithSubblossom =
          startWithSubblossom;
        blossom = blossom->parentBlossom.get();
      }
    }

    MATCHINGEDGEWEIGHTPARAMETERS2(
      template void setPointersFromAncestor LPAREN Vertex<,
      > & COMMA const Blossom<,
      > & COMMA bool RPAREN ;
    )
  }
}
