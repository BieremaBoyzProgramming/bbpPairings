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


#ifndef ROOTBLOSSOMITERATORSIG_H
#define ROOTBLOSSOMITERATORSIG_H

#include <iterator>

#include "graphsig.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class RootBlossom;

    /**
     * An iterator over the RootBlossoms in a Graph. The iterator is invalidated
     * if RootBlossoms are created/destroyed or the baseVertex of a RootBlossom
     * changes.
     *
     * The end iterator is constructed in time O(1), and the begin iterator is
     * constructed in time O(n). O(n) time is required to iterate through all of
     * the RootBlossoms.
     */
    template <typename edge_weight>
    class RootBlossomIterator
    {
    public:
      typedef void difference_type;
      typedef RootBlossom<edge_weight> value_type;
      typedef RootBlossom<edge_weight> *pointer;
      typedef RootBlossom<edge_weight> &reference;
      typedef std::forward_iterator_tag iterator_category;

      constexpr RootBlossomIterator() noexcept;
      RootBlossomIterator(
        typename Graph<edge_weight>::const_iterator,
        typename Graph<edge_weight>::const_iterator);
      RootBlossomIterator(typename Graph<edge_weight>::const_iterator);

      bool operator==(const RootBlossomIterator<edge_weight>) const;
      bool operator!=(const RootBlossomIterator<edge_weight>) const;

      reference operator*() const;
      pointer operator->() const;

      RootBlossomIterator<edge_weight> &operator++() &;
      RootBlossomIterator<edge_weight> operator++() &&;
      RootBlossomIterator<edge_weight> operator++(int) &;

    private:
      typename Graph<edge_weight>::const_iterator currentIterator;
      const typename Graph<edge_weight>::const_iterator endIterator;

      void advanceToFirstPosition() &;
    };
  }
}

#endif
