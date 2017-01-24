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


#ifndef BLOSSOMSIG_H
#define BLOSSOMSIG_H

namespace matching
{
  namespace detail
  {
    template <typename>
    class ParentBlossom;
    template <typename>
    class RootBlossom;
    template <typename>
    struct Vertex;

    /**
     * An abstract parent class for a blossom or subblossom.
     */
    template <typename edge_weight>
    struct Blossom
    {
      RootBlossom<edge_weight> *rootBlossom;
      ParentBlossom<edge_weight> *parentBlossom{ };
      /**
       * The head of a linked list of the vertices contained in this blossom.
       * The nextVertex field of Vertex forms the links.
       */
      Vertex<edge_weight> *vertexListHead;
      /**
       * The tail of the linked list of the vertices contained in this blossom.
       */
      Vertex<edge_weight> *vertexListTail;
      /**
       * The vertex in this subblossom which was used to link this subblossom to
       * the next child subblossom of their mutual parent (sub)blossom. If this
       * is not a subblossom, the value is unspecified.
       */
      Vertex<edge_weight> *vertexToPreviousSiblingBlossom;
      /**
       * The vertex in this subblossom which was used to link this subblossom to
       * the previous child subblossom of their mutual parent (sub)blossom. If
       * this is not a subblossom, the value is unspecified.
       */
      Vertex<edge_weight> *vertexToNextSiblingBlossom;
      /**
       * The next child of parentBlossom. If this is not a subblossom, the value
       * is unspecified.
       */
      Blossom<edge_weight> *nextBlossom;
      /**
       * The previous child of parentBlossom. If this is not a subblossom, the
       * value is unspecified.
       */
      Blossom<edge_weight> *previousBlossom;
      const bool isVertex;

      Blossom(Blossom<edge_weight> &&) noexcept;
      Blossom(
        RootBlossom<edge_weight> &,
        Vertex<edge_weight> &,
        Vertex<edge_weight> &,
        bool isVertex);

      virtual ~Blossom() = 0;
    };

    template <typename edge_weight>
    Blossom<edge_weight> &getAncestorOfVertex(
      Vertex<edge_weight> &,
      const ParentBlossom<edge_weight> *);
    template <typename edge_weight>
    void setPointersFromAncestor(
      Vertex<edge_weight> &,
      const Blossom<edge_weight> &,
      bool);
  }
}

#endif
