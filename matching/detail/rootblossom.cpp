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


#include <cassert>
#include <memory>

#include "../templateinstantiation.h"

#include "blossomsig.h"
#include "blossomiteratorimpl.h"
#include "blossomvertexiteratorimpl.h"
#include "graphimpl.h"
#include "parentblossomimpl.h"
#include "rootblossomimpl.h"
#include "types.h"
#include "verteximpl.h"

namespace matching
{
  namespace detail
  {
    /**
     * Initialize the subblossom and iteratorStartsWithSubblossom fields of the
     * ParentBlossoms so that BlossomVertexIterator returns vertices in an order
     * in which matched vertices are consecutive.
     *
     * This invalidates all extant iterators over the RootBlossom.
     */
    template <typename edge_weight>
    void RootBlossom<edge_weight>::useMatchingIterators() const &
    {
      const Blossom<edge_weight> *currentBlossom = &rootChild;
      Vertex<edge_weight> *currentVertex = baseVertex;
      bool startsWithBase = true;

      do
      {
        setPointersFromAncestor<edge_weight>(
          *currentVertex,
          *currentBlossom,
          startsWithBase);
        currentBlossom = currentVertex;
        while (currentBlossom != &rootChild)
        {
          assert(
            currentBlossom->nextBlossom->parentBlossom
              == currentBlossom->parentBlossom);
          startsWithBase =
            !startsWithBase
              && currentBlossom->parentBlossom->subblossom != currentBlossom;
          currentBlossom = currentBlossom->nextBlossom;
          if (currentBlossom == currentBlossom->parentBlossom->subblossom)
          {
            ParentBlossom<edge_weight> &parentBlossom =
              *currentBlossom->parentBlossom;
            currentBlossom = &parentBlossom;
            startsWithBase = parentBlossom.iterationStartsWithSubblossom;
            continue;
          }
          else
          {
            currentVertex =
              startsWithBase
                ? currentBlossom->vertexToPreviousSiblingBlossom
                : currentBlossom->vertexToNextSiblingBlossom;
            break;
          }
        }
      } while (currentBlossom != &rootChild);
    }

    /**
     * Initialize rootBlossom pointers and minimum resistance values for a newly
     * formed OUTER ParentBlossom.
     */
    template <typename edge_weight>
    void RootBlossom<edge_weight>::initializeFromChildren(
      const std::deque<std::shared_ptr<const RootBlossom<edge_weight>>>
        &originalBlossoms,
      const Graph<edge_weight> &graph
    ) &
    {
      minOuterEdgeResistance = graph.maxEdgeWeight * 2u + 1u;
      for (
        BlossomIterator<edge_weight> blossomIterator = blossomBegin();
        blossomIterator != blossomEnd();
        ++blossomIterator)
      {
        blossomIterator->rootBlossom = rootChild.rootBlossom;
      }

      for (
        RootBlossomIterator<edge_weight> iterator = graph.rootBlossomBegin();
        iterator != graph.rootBlossomEnd();
        ++iterator)
      {
        if (iterator->label == LABEL_OUTER && &*iterator != this)
        {
          edge_weight minResistance = graph.maxEdgeWeight * 2u + 1u;

          for (
            const std::shared_ptr<const RootBlossom<edge_weight>> &blossom
              : originalBlossoms)
          {
            if (blossom->label == LABEL_INNER)
            {
              minResistance =
                updateOuterOuterEdges(*blossom, *iterator, minResistance);
            }
            else
            {
              assert(blossom->label == LABEL_OUTER);

              if (
                blossom->minOuterEdges[iterator->baseVertex->vertexIndex]
                  && iterator->minOuterEdges[blossom->baseVertex->vertexIndex]
              )
              {
                const edge_weight resistance =
                  blossom->minOuterEdges
                    [iterator->baseVertex->vertexIndex]
                    ->resistance(
                        *iterator->minOuterEdges
                          [blossom->baseVertex->vertexIndex]
                      );

                assert(!(resistance & 1u));

                if (resistance < minResistance)
                {
                  minResistance = resistance;

                  minOuterEdges[iterator->baseVertex->vertexIndex] =
                    blossom->minOuterEdges[iterator->baseVertex->vertexIndex];
                  iterator->minOuterEdges[baseVertex->vertexIndex] =
                    iterator->minOuterEdges[blossom->baseVertex->vertexIndex];

                  minOuterEdgeResistance =
                    std::min(minOuterEdgeResistance, minResistance);
                  iterator->minOuterEdgeResistance =
                    std::min(iterator->minOuterEdgeResistance, minResistance);
                }
              }
            }
          }
        }
      }

      for (
        const std::shared_ptr<const RootBlossom<edge_weight>> &rootBlossom
          : originalBlossoms
      )
      {
        if (rootBlossom->label != LABEL_OUTER)
        {
          graph.updateInnerOuterEdges(*rootBlossom);
        }
      }
    }

    /**
     * Assuming ancestor contains the baseVertex of the RootBlossom, disassemble
     * the Blossoms above it, so that ancestor is its own RootBlossom. Do this
     * while maintaining the invariant that edge resistances cannot be negative.
     */
    template <typename edge_weight>
    void RootBlossom<edge_weight>::freeAncestorOfBase(
      Blossom<edge_weight> &ancestor
    ) const &
    {
      if (&ancestor == &rootChild)
      {
        return;
      }

      edge_weight dualVariableAdjustment{ };
      std::shared_ptr<ParentBlossom<edge_weight>> blossom =
        ancestor.parentBlossom;
      while (blossom)
      {
        assert(!(blossom->dualVariable & 1u));
        dualVariableAdjustment += blossom->dualVariable >> 1;
        blossom = blossom->parentBlossom;
      }

      blossom = ancestor.parentBlossom;
      Blossom<edge_weight> *nextBlossom = ancestor.nextBlossom;

      // Create a RootBlossom for ancestor.
      new RootBlossom<edge_weight>(
        ancestor,
        *ancestor.rootBlossom->baseVertex,
        ancestor.rootBlossom->baseVertexMatch);
      for (
        BlossomVertexIterator<edge_weight> iterator =
          ancestor.rootBlossom->blossomVertexBegin();
        iterator != ancestor.rootBlossom->blossomVertexEnd();
        ++iterator)
      {
        iterator->dualVariable += dualVariableAdjustment;
      }

      // Create all the other RootBlossoms.
      Blossom<edge_weight> *childToFree = &ancestor;
      while (blossom)
      {
        bool linksForward = true;
        Blossom<edge_weight> *previousBlossom;
        for (
          Blossom<edge_weight> *currentBlossom = nextBlossom;
          currentBlossom != childToFree;
          currentBlossom = nextBlossom)
        {
          nextBlossom = currentBlossom->nextBlossom;
          new RootBlossom<edge_weight>(
            *currentBlossom,
            linksForward
              ? *currentBlossom->vertexToNextSiblingBlossom
              : *currentBlossom->vertexToPreviousSiblingBlossom,
            linksForward
              ? nextBlossom->vertexToPreviousSiblingBlossom
              : previousBlossom->vertexToNextSiblingBlossom);

          for (
            BlossomVertexIterator<edge_weight> iterator =
              currentBlossom->rootBlossom->blossomVertexBegin();
            iterator != currentBlossom->rootBlossom->blossomVertexEnd();
            ++iterator)
          {
            iterator->dualVariable += dualVariableAdjustment;
          }

          linksForward = !linksForward;
          previousBlossom = currentBlossom;
        }

        dualVariableAdjustment -= blossom->dualVariable >> 1;
        assert(!(blossom->dualVariable & 1u));

        childToFree = blossom.get();
        nextBlossom = blossom->nextBlossom;
        blossom = blossom->parentBlossom;
      }
    }

    MATCHINGEDGEWEIGHTPARAMETERS1(template class RootBlossom<, >;)

    /**
     * Perform one direction of the augmentation, augmenting between vertex and
     * the exposed OUTER RootBlossom that led to its labeling. vertex is set as
     * matched to newMatch.
     */
    template <typename edge_weight>
    void augmentToSource(
      Vertex<edge_weight> *vertex,
      Vertex<edge_weight> *newMatch)
    {
      while (vertex->rootBlossom->baseVertexMatch)
      {
        vertex->rootBlossom->baseVertex = vertex;
        RootBlossom<edge_weight> &originalMatch =
          *vertex->rootBlossom->baseVertexMatch->rootBlossom;
        vertex->rootBlossom->baseVertexMatch = newMatch;
        originalMatch.baseVertex = originalMatch.labeledVertex;
        originalMatch.baseVertexMatch = originalMatch.labelingVertex;
        vertex = originalMatch.labelingVertex;
        newMatch = originalMatch.labeledVertex;
      }

      vertex->rootBlossom->baseVertex = vertex;
      vertex->rootBlossom->baseVertexMatch = newMatch;
    }

#define COMMA ,
#define LPAREN (
#define RPAREN )

    MATCHINGEDGEWEIGHTPARAMETERS2(
      template void augmentToSource LPAREN Vertex<,
      > * COMMA Vertex<,
      > * RPAREN
    );
  }
}
