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


#ifndef BLOSSOMMATCHINGITERATORSIG_H
#define BLOSSOMMATCHINGITERATORSIG_H

#include <iterator>

namespace matching
{
  namespace detail
  {
    template <typename>
    struct Blossom;
    template <typename>
    class RootBlossom;
    template <typename>
    struct Vertex;

    /**
     * An iterator over the non-base Vertexes in a RootBlossom (descendants of
     * the rootChild). The subBlossom and iterationStartsWithSubBlossom members
     * of ParentBlossom must first be set by calling
     * RootBlossom::useMatchingIterator().
     *
     * If RootBlossom::useMatchingIterators() has been called, the iterator
     * returns blossoms in an order such that vertices that are matched will be
     * returned consecutively.
     *
     * The iterator is constructed in O(n) time, and iterating through the
     * entire RootBlossom takes time O(n).
     */
    template <typename edge_weight>
    class BlossomMatchingIterator
    {
    public:
      typedef void difference_type;
      typedef Vertex<edge_weight> value_type;
      typedef Vertex<edge_weight> *pointer;
      typedef Vertex<edge_weight> &reference;
      typedef std::forward_iterator_tag iterator_category;

      constexpr BlossomMatchingIterator() noexcept;
      explicit BlossomMatchingIterator(const RootBlossom<edge_weight> &);

      bool operator==(BlossomMatchingIterator<edge_weight>) const;
      bool operator!=(BlossomMatchingIterator<edge_weight>) const;

      reference operator*() const;
      pointer operator->() const;

      BlossomMatchingIterator<edge_weight> &operator++() &;
      BlossomMatchingIterator<edge_weight> operator++() &&;
      BlossomMatchingIterator<edge_weight> operator++(int) &;

    private:
      Vertex<edge_weight> *currentVertex{ };
      const Blossom<edge_weight> *endBlossom;
    };
  }
}

#endif
