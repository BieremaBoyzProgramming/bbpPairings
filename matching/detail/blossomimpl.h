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


#ifndef BLOSSOMIMPL_H
#define BLOSSOMIMPL_H

#include <cassert>
#include <memory>

#include "blossomsig.h"
#include "parentblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class RootBlossom;
    template <typename>
    struct Vertex;

    template <typename edge_weight>
    inline Blossom<edge_weight>::Blossom(
        RootBlossom<edge_weight> &rootBlossom_,
        Vertex<edge_weight> &vertexListHead_,
        Vertex<edge_weight> &vertexListTail_,
        const bool isVertex_)
      : rootBlossom(&rootBlossom_),
        vertexListHead(&vertexListHead_),
        vertexListTail(&vertexListTail_),
        isVertex(isVertex_)
    { }
    /**
     * This constructor should never be called.
     */
    template <typename edge_weight>
    Blossom<edge_weight>::Blossom(Blossom<edge_weight> &&that) noexcept
      : vertexListHead(that.vertexListHead),
        vertexListTail(that.vertexListTail),
        isVertex(false)
    {
      assert(false);
    }
    template <typename edge_weight>
    inline Blossom<edge_weight>::~Blossom() = default;

    /**
     * Traverse the parent links up from vertex until reaching ancestor. Return
     * the child of ancestor through which we arrived.
     *
     * Takes time O(k), where k is the number of links traversed.
     */
    template <typename edge_weight>
    inline Blossom<edge_weight> &getAncestorOfVertex(
      Vertex<edge_weight> &vertex,
      const ParentBlossom<edge_weight> *const ancestor)
    {
      Blossom<edge_weight> *blossom = &vertex;
      while (blossom->parentBlossom != ancestor)
      {
        blossom = blossom->parentBlossom;
      }
      return *blossom;
    }

    /**
     * Set the subblossom pointers on the path between vertex and ancestor so
     * that they point to vertex. Also set all the iterationStartsWithSubblossom
     * flags to the provided flag.
     */
    template <typename edge_weight>
    inline void setPointersFromAncestor(
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
        blossom = blossom->parentBlossom;
      }
    }
  }
}

#endif
