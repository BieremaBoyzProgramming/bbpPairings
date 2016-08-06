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


#ifndef BLOSSOMITERATORIMPL_H
#define BLOSSOMITERATORIMPL_H

#include <cassert>
#include <memory>

#include "blossomiteratorsig.h"
#include "blossomsig.h"
#include "parentblossomsig.h"
#include "rootblossomsig.h"

namespace matching
{
  namespace detail
  {
    /**
     * Construct an end iterator.
     */
    template <typename edge_weight>
    constexpr BlossomIterator<edge_weight>::BlossomIterator() noexcept =
      default;
    /**
     * Construct a begin iterator.
     */
    template <typename edge_weight>
    inline BlossomIterator<edge_weight>::BlossomIterator(
        const RootBlossom<edge_weight> &rootBlossom)
      : currentBlossom(&rootBlossom.rootChild),
        endBlossom(&rootBlossom.rootChild)
    { }

    template <typename edge_weight>
    inline bool BlossomIterator<edge_weight>::operator==(
      const BlossomIterator<edge_weight> that
    ) const
    {
      return currentBlossom == that.currentBlossom;
    }
    template <typename edge_weight>
    inline bool BlossomIterator<edge_weight>::operator!=(
      const BlossomIterator<edge_weight> that
    ) const
    {
      return currentBlossom != that.currentBlossom;
    }

    template <typename edge_weight>
    inline Blossom<edge_weight> &BlossomIterator<edge_weight>::operator*() const
    {
      return *currentBlossom;
    }
    template <typename edge_weight>
    inline Blossom<edge_weight> *BlossomIterator<edge_weight>::operator->()
      const
    {
      return currentBlossom;
    }

    template <typename edge_weight>
    inline BlossomIterator<edge_weight>
      &BlossomIterator<edge_weight>::operator++() &
    {
      if (currentBlossom->isVertex)
      {
        // This loop takes us back up toward the root once we have finished the
        // subblossoms.
        while (currentBlossom != endBlossom)
        {
          assert(
            currentBlossom->nextBlossom->parentBlossom
              == currentBlossom->parentBlossom);

          currentBlossom = currentBlossom->nextBlossom;

          if (currentBlossom != currentBlossom->parentBlossom->subblossom)
          {
            break;
          }

          // We considered subBlossom first and we are back, so head toward the
          // root.
          currentBlossom = currentBlossom->parentBlossom.get();
        }
        if (currentBlossom == endBlossom)
        {
          currentBlossom = nullptr;
        }
      }
      else
      {
        const ParentBlossom<edge_weight> &parentBlossom =
          *static_cast<const ParentBlossom<edge_weight> *const>(currentBlossom);
        currentBlossom = parentBlossom.subblossom;
      }
      return *this;
    }
    template <typename edge_weight>
    inline BlossomIterator<edge_weight>
      BlossomIterator<edge_weight>::operator++() &&
    {
      ++*this;
      return std::move(*this);
    }
    template <typename edge_weight>
    inline BlossomIterator<edge_weight>
      BlossomIterator<edge_weight>::operator++(int) &
    {
      BlossomIterator<edge_weight> result = *this;
      ++*this;
      return result;
    }
  }
}

#endif
