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


#ifndef BLOSSOMVERTEXITERATORSIG_H
#define BLOSSOMVERTEXITERATORSIG_H

#include <iterator>

#include "blossomiteratorsig.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    class RootBlossom;
    template <typename>
    struct Vertex;

    /**
     * An iterator over the Vertexes in a RootBlossom (non-strict descendants of
     * the rootChild). The iterator uses the subBlossom member of ParentBlossom,
     * so if this is changed (as by RootBlossom::useMatchingIterators()), all
     * extant iterators are invalidated.
     *
     * The end iterator is constructed in O(1) time, and the begin iterator in
     * O(n) time. Iterating through the entire blossom takes O(n) time total.
     */
    template <typename edge_weight>
    class BlossomVertexIterator
    {
    public:
      typedef void difference_type;
      typedef Vertex<edge_weight> value_type;
      typedef Vertex<edge_weight> *pointer;
      typedef Vertex<edge_weight> &reference;
      typedef std::forward_iterator_tag iterator_category;

      constexpr BlossomVertexIterator() noexcept;
      explicit BlossomVertexIterator(const RootBlossom<edge_weight> &);

      bool operator==(BlossomVertexIterator<edge_weight>) const;
      bool operator!=(BlossomVertexIterator<edge_weight>) const;

      reference operator*() const;
      pointer operator->() const;

      BlossomVertexIterator<edge_weight> &operator++() &;
      BlossomVertexIterator<edge_weight> operator++() &&;
      BlossomVertexIterator<edge_weight> operator++(int) &;

    private:
      BlossomIterator<edge_weight> currentIterator;

      void advance() &;
    };
  }
}

#endif
