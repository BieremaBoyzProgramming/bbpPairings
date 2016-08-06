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


#ifndef BLOSSOMITERATORSIG_H
#define BLOSSOMITERATORSIG_H

#include <iterator>

namespace matching
{
  namespace detail
  {
    template <typename>
    struct Blossom;
    template <typename>
    class RootBlossom;

    /**
     * An iterator over the blossoms and subblossoms in a RootBlossom (the
     * rootChild and all descendants). The iterator uses the subBlossom member
     * of ParentBlossom, so if this is changed (as by
     * RootBlossom::useMatchingIterators()), all extant iterators are
     * invalidated.
     *
     * The iterator is constructed in O(1) time, and increment operations
     * require amortized O(1) time each.
     */
    template <typename edge_weight>
    class BlossomIterator
    {
    public:
      typedef void difference_type;
      typedef Blossom<edge_weight> value_type;
      typedef Blossom<edge_weight> *pointer;
      typedef Blossom<edge_weight> &reference;
      typedef std::forward_iterator_tag iterator_category;

      constexpr BlossomIterator() noexcept;
      explicit BlossomIterator(const RootBlossom<edge_weight> &);

      bool operator==(BlossomIterator<edge_weight>) const;
      bool operator!=(BlossomIterator<edge_weight>) const;

      reference operator*() const;
      pointer operator->() const;

      BlossomIterator<edge_weight> &operator++() &;
      BlossomIterator<edge_weight> operator++() &&;
      BlossomIterator<edge_weight> operator++(int) &;

    private:
      Blossom<edge_weight> *currentBlossom{ };
      const Blossom<edge_weight> *endBlossom;
    };
  }
}

#endif
