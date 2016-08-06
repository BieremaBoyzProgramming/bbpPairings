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


#ifndef BLOSSOMMATCHINGITERATORIMPL_H
#define BLOSSOMMATCHINGITERATORIMPL_H

#include <cassert>
#include <memory>

#include "blossommatchingiteratorsig.h"
#include "blossomsig.h"
#include "parentblossomsig.h"
#include "rootblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    /**
     * Construct an end iterator.
     */
    template <typename edge_weight>
    constexpr BlossomMatchingIterator<edge_weight>::BlossomMatchingIterator()
        noexcept =
      default;
    /**
     * Construct a begin iterator.
     */
    template <typename edge_weight>
    inline BlossomMatchingIterator<edge_weight>::BlossomMatchingIterator(
        const RootBlossom<edge_weight> &rootBlossom)
      : currentVertex(rootBlossom.baseVertex),
        endBlossom(&rootBlossom.rootChild)
    {
      ++*this;
    }

    template <typename edge_weight>
    inline bool BlossomMatchingIterator<edge_weight>::operator==(
      const BlossomMatchingIterator<edge_weight> that
    ) const
    {
      return currentVertex == that.currentVertex;
    }
    template <typename edge_weight>
    inline bool BlossomMatchingIterator<edge_weight>::operator!=(
      const BlossomMatchingIterator<edge_weight> that
    ) const
    {
      return currentVertex != that.currentVertex;
    }

    template <typename edge_weight>
    inline Vertex<edge_weight>
      &BlossomMatchingIterator<edge_weight>::operator*() const
    {
      return *currentVertex;
    }
    template <typename edge_weight>
    inline Vertex<edge_weight>
      *BlossomMatchingIterator<edge_weight>::operator->() const
    {
      return currentVertex;
    }

    template <typename edge_weight>
    inline BlossomMatchingIterator<edge_weight>
      &BlossomMatchingIterator<edge_weight>::operator++() &
    {
      Blossom<edge_weight> *currentBlossom = currentVertex;
      // This loop takes us back up toward the root once we have finished the
      // subblossoms.
      while (currentBlossom != endBlossom)
      {
        // If subBlossom is the last subblossom to consider, head back toward
        // the root.
        if (
          currentBlossom == currentBlossom->parentBlossom->subblossom
            && !currentBlossom->parentBlossom->iterationStartsWithSubblossom)
        {
          currentBlossom = currentBlossom->parentBlossom.get();
          continue;
        }

        assert(
          currentBlossom->nextBlossom->parentBlossom
            == currentBlossom->parentBlossom);

        currentBlossom = currentBlossom->nextBlossom;

        if (
          currentBlossom != currentBlossom->parentBlossom->subblossom
            || !currentBlossom->parentBlossom->iterationStartsWithSubblossom)
        {
          break;
        }

        // We considered subblossom first and we are back, so head toward the
        // root.
        currentBlossom = currentBlossom->parentBlossom.get();
      }
      if (currentBlossom == endBlossom)
      {
        currentVertex = nullptr;
      }
      else
      {
        while (!currentBlossom->isVertex)
        {
          const ParentBlossom<edge_weight> &parentBlossom =
            *static_cast<const ParentBlossom<edge_weight> *const>(currentBlossom
              );
          currentBlossom = parentBlossom.subblossom;
          if (!parentBlossom.iterationStartsWithSubblossom)
          {
            currentBlossom = currentBlossom->nextBlossom;
          }
        }
        currentVertex = static_cast<Vertex<edge_weight> *const>(currentBlossom);
      }
      return *this;
    }
    template <typename edge_weight>
    inline BlossomMatchingIterator<edge_weight>
      BlossomMatchingIterator<edge_weight>::operator++() &&
    {
      ++*this;
      return *this;
    }
    template <typename edge_weight>
    inline BlossomMatchingIterator<edge_weight>
      BlossomMatchingIterator<edge_weight>::operator++(int) &
    {
      BlossomMatchingIterator<edge_weight> result = *this;
      ++*this;
      return result;
    }
  }
}

#endif
