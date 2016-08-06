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


#ifndef ROOTBLOSSOMITERATORIMPL_H
#define ROOTBLOSSOMITERATORIMPL_H

#include <memory>

#include "graphsig.h"
#include "rootblossomiteratorsig.h"
#include "rootblossomsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    template <typename edge_weight>
    inline constexpr RootBlossomIterator<edge_weight>::RootBlossomIterator()
        noexcept
      = default;
    template <typename edge_weight>
    inline RootBlossomIterator<edge_weight>::RootBlossomIterator(
        const typename Graph<edge_weight>::const_iterator beginIterator,
        const typename Graph<edge_weight>::const_iterator endIterator_)
      : currentIterator(beginIterator), endIterator(endIterator_)
    {
      advanceToFirstPosition();
    }
    template <typename edge_weight>
    inline RootBlossomIterator<edge_weight>::RootBlossomIterator(
        const typename Graph<edge_weight>::const_iterator endIterator_)
      : RootBlossomIterator<edge_weight>(endIterator_, endIterator_) { }

    template <typename edge_weight>
    inline bool RootBlossomIterator<edge_weight>::operator==(
      const RootBlossomIterator<edge_weight> that
    ) const
    {
      return currentIterator == that.currentIterator;
    }
    template <typename edge_weight>
    inline bool RootBlossomIterator<edge_weight>::operator!=(
      const RootBlossomIterator<edge_weight> that
    ) const
    {
      return currentIterator != that.currentIterator;
    }

    template <typename edge_weight>
    inline RootBlossom<edge_weight>
      &RootBlossomIterator<edge_weight>::operator*() const
    {
      return *(*currentIterator)->rootBlossom;
    }
    template <typename edge_weight>
    inline RootBlossom<edge_weight>
      *RootBlossomIterator<edge_weight>::operator->() const
    {
      return (*currentIterator)->rootBlossom.get();
    }

    template <typename edge_weight>
    inline RootBlossomIterator<edge_weight>
      &RootBlossomIterator<edge_weight>::operator++() &
    {
      ++currentIterator;
      advanceToFirstPosition();
      return *this;
    }
    template <typename edge_weight>
    inline RootBlossomIterator<edge_weight>
      RootBlossomIterator<edge_weight>::operator++() &&
    {
      ++*this;
      return std::move(*this);
    }
    template <typename edge_weight>
    inline RootBlossomIterator<edge_weight>
      RootBlossomIterator<edge_weight>::operator++(int) &
    {
      RootBlossomIterator<edge_weight> result = *this;
      ++*this;
      return result;
    }

    template <typename edge_weight>
    inline void RootBlossomIterator<edge_weight>::advanceToFirstPosition() &
    {
      while (
        currentIterator != endIterator
          && (*currentIterator)->rootBlossom->baseVertex
              != currentIterator->get())
      {
        ++currentIterator;
      }
    }
  }
}

#include "graphimpl.h"
#include "verteximpl.h"

#endif
