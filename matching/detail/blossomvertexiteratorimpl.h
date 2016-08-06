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


#ifndef BLOSSOMVERTEXITERATORIMPL_H
#define BLOSSOMVERTEXITERATORIMPL_H

#include "blossomiteratorsig.h"
#include "blossomsig.h"
#include "blossomvertexiteratorsig.h"
#include "vertexsig.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class RootBlossom;

    template <typename edge_weight>
    constexpr BlossomVertexIterator<edge_weight>::BlossomVertexIterator()
        noexcept =
      default;
    template <typename edge_weight>
    inline BlossomVertexIterator<edge_weight>::BlossomVertexIterator(
        const RootBlossom<edge_weight> &rootBlossom)
      : currentIterator(rootBlossom)
    {
      advance();
    }

    template <typename edge_weight>
    inline bool BlossomVertexIterator<edge_weight>::operator==(
      const BlossomVertexIterator<edge_weight> that
    ) const
    {
      return currentIterator == that.currentIterator;
    }
    template <typename edge_weight>
    inline bool BlossomVertexIterator<edge_weight>::operator!=(
      const BlossomVertexIterator<edge_weight> that
    ) const
    {
      return currentIterator != that.currentIterator;
    }

    template <typename edge_weight>
    inline Vertex<edge_weight> &BlossomVertexIterator<edge_weight>::operator*()
      const
    {
      return static_cast<Vertex<edge_weight> &>(*currentIterator);
    }
    template <typename edge_weight>
    inline Vertex<edge_weight> *BlossomVertexIterator<edge_weight>::operator->()
      const
    {
      return &**this;
    }

    template <typename edge_weight>
    inline BlossomVertexIterator<edge_weight>
      &BlossomVertexIterator<edge_weight>::operator++() &
    {
      ++currentIterator;
      advance();
      return *this;
    }
    template <typename edge_weight>
    inline BlossomVertexIterator<edge_weight>
      BlossomVertexIterator<edge_weight>::operator++() &&
    {
      ++*this;
      return *this;
    }
    template <typename edge_weight>
    inline BlossomVertexIterator<edge_weight>
      BlossomVertexIterator<edge_weight>::operator++(int) &
    {
      BlossomVertexIterator<edge_weight> result = *this;
      ++*this;
      return result;
    }

    template <typename edge_weight>
    inline void BlossomVertexIterator<edge_weight>::advance() &
    {
      while (
        currentIterator != BlossomIterator<edge_weight>()
          && !currentIterator->isVertex)
      {
        ++currentIterator;
      }
    }
  }
}

#include "blossomiteratorimpl.h"

#endif
