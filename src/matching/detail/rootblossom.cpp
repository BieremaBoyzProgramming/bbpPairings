#include <cassert>
#include <vector>

#include "../templateinstantiation.h"

#include "blossomimpl.h"
#include "graphsig.h"
#include "parentblossomimpl.h"
#include "rootblossomimpl.h"
#include "types.h"
#include "verteximpl.h"

namespace matching
{
  namespace detail
  {
    /**
     * Reorder the RootBlossom's linked list of vertices so that they are
     * returned in an order in which matched vertices are consecutive.
     */
    template <typename edge_weight>
    void RootBlossom<edge_weight>::putVerticesInMatchingOrder() const &
    {
      const Blossom<edge_weight> *currentBlossom = &rootChild;
      Vertex<edge_weight> *currentVertex = baseVertex;
      bool startsWithBase = true;

      // Each iteration of this loop begins with a new currentVertex.
      do
      {
        setPointersFromAncestor<edge_weight>(
          *currentVertex,
          *currentBlossom,
          startsWithBase);
        currentBlossom = currentVertex;

        // This loop follows the path up from a Vertex to the last ParentBlossom
        // we have finished.
        while (currentBlossom != &rootChild)
        {
          assert(
            currentBlossom->nextBlossom->parentBlossom
              == currentBlossom->parentBlossom);
          startsWithBase =
            !startsWithBase
              && currentBlossom->parentBlossom->subblossom != currentBlossom;
          currentBlossom->previousBlossom->vertexListTail->nextVertex =
            currentBlossom->vertexListHead;
          currentBlossom = currentBlossom->nextBlossom;
          if (currentBlossom == currentBlossom->parentBlossom->subblossom)
          {
            ParentBlossom<edge_weight> &parentBlossom =
              *currentBlossom->parentBlossom;
            startsWithBase = parentBlossom.iterationStartsWithSubblossom;
            currentBlossom->previousBlossom->vertexListTail->nextVertex =
              currentBlossom->vertexListHead;
            parentBlossom.vertexListHead =
              (startsWithBase ? currentBlossom : currentBlossom->nextBlossom)
                ->vertexListHead;
            parentBlossom.vertexListTail =
              (startsWithBase
                ? currentBlossom->previousBlossom
                : currentBlossom
              )->vertexListTail;
            currentBlossom = &parentBlossom;
            // Repeat the inner loop, iterating up to the parent blossom.
          }
          else
          {
            currentVertex =
              startsWithBase
                ? currentBlossom->vertexToPreviousSiblingBlossom
                : currentBlossom->vertexToNextSiblingBlossom;
            break;
            // Repeat the outer loop with a new Vertex.
          }
        }
      } while (currentBlossom != &rootChild);
      rootChild.vertexListTail->nextVertex = nullptr;
    }

    /**
     * Initialize rootBlossom pointers and minimum resistance values for a newly
     * formed OUTER ParentBlossom. Destory the old RootBlossoms.
     */
    template <typename edge_weight>
    void RootBlossom<edge_weight>::initializeFromChildren(
      const std::vector<RootBlossom<edge_weight> *> &originalBlossoms,
      Graph<edge_weight> &graph
    ) &
    {
      minOuterEdgeResistance = graph.aboveMaxEdgeWeight;
      for (RootBlossom<edge_weight> *const rootBlossom : originalBlossoms)
      {
        graph.rootBlossomPool.hide(*rootBlossom);
        rootBlossom->updateRootBlossomInDescendants(*this);
      }

      edge_weight resistanceStorage = graph.aboveMaxEdgeWeight;
      edge_weight minResistance = graph.aboveMaxEdgeWeight;

      for (
        auto iterator = graph.rootBlossomPool.begin();
        iterator != graph.rootBlossomPool.end();
        ++iterator)
      {
        assert(&*iterator != this);
        assert(iterator->rootChild.rootBlossom != this);
        if (iterator->label == LABEL_OUTER)
        {
          minResistance = graph.aboveMaxEdgeWeight;

          for (RootBlossom<edge_weight> *const blossom : originalBlossoms)
          {
            if (blossom->label == LABEL_INNER)
            {
              updateOuterOuterEdges(
                *blossom,
                *iterator,
                minResistance,
                resistanceStorage);
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

                  if (minResistance < minOuterEdgeResistance)
                  {
                    minOuterEdgeResistance = minResistance;
                  }
                  if (minResistance < iterator->minOuterEdgeResistance)
                  {
                    iterator->minOuterEdgeResistance = minResistance;
                  }
                }
              }
            }
          }
        }
      }

      Vertex<edge_weight> *previousHead{ };
      for (RootBlossom<edge_weight> *const rootBlossom : originalBlossoms)
      {
        if (rootBlossom->label != LABEL_OUTER)
        {
          graph.updateInnerOuterEdges(*rootBlossom);
        }
        rootBlossom->rootChild.vertexListTail->nextVertex = previousHead;
        previousHead = rootBlossom->rootChild.vertexListHead;
        graph.rootBlossomPool.destroy(*rootBlossom);
      }
    }

    /**
     * Assuming ancestor contains the baseVertex of the RootBlossom, disassemble
     * the Blossoms above it, so that ancestor is its own RootBlossom. Do this
     * while maintaining the invariant that edge resistances cannot be negative.
     */
    template <typename edge_weight>
    void RootBlossom<edge_weight>::freeAncestorOfBase(
      Blossom<edge_weight> &ancestor,
      Graph<edge_weight> &graph
    ) &
    {
      if (&ancestor == &rootChild)
      {
        return;
      }

      // Calculate the total dualVariable adjustment.
      edge_weight dualVariableAdjustment = graph.aboveMaxEdgeWeight & 0u;
      ParentBlossom<edge_weight> *blossom = ancestor.parentBlossom;
      while (blossom)
      {
        assert(!(blossom->dualVariable & 1u));
        dualVariableAdjustment += blossom->dualVariable >> 1;
        blossom = blossom->parentBlossom;
      }

      blossom = ancestor.parentBlossom;
      Blossom<edge_weight> *nextBlossom = ancestor.nextBlossom;

      // Create a RootBlossom for ancestor.
      graph.rootBlossomPool.construct(
        ancestor,
        *ancestor.rootBlossom->baseVertex,
        ancestor.rootBlossom->baseVertexMatch,
        graph);
      for (
        auto iterator = ancestor.vertexListHead;
        iterator;
        iterator = iterator->nextVertex)
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
          graph.rootBlossomPool.construct(
            *currentBlossom,
            linksForward
              ? *currentBlossom->vertexToNextSiblingBlossom
              : *currentBlossom->vertexToPreviousSiblingBlossom,
            linksForward
              ? nextBlossom->vertexToPreviousSiblingBlossom
              : previousBlossom->vertexToNextSiblingBlossom,
            graph);

          for (
            auto iterator = currentBlossom->vertexListHead;
            iterator;
            iterator = iterator->nextVertex)
          {
            iterator->dualVariable += dualVariableAdjustment;
          }

          linksForward = !linksForward;
          previousBlossom = currentBlossom;
        }

        dualVariableAdjustment -= blossom->dualVariable >> 1;
        assert(!(blossom->dualVariable & 1u));

        if (childToFree != &ancestor)
        {
          // Destroy unused ParentBlossoms.
          graph.parentBlossomPool.destroy(
            *static_cast<ParentBlossom<edge_weight> *>(childToFree));
        }
        childToFree = blossom;
        nextBlossom = blossom->nextBlossom;
        blossom = blossom->parentBlossom;
      }

      // Destroy the old RootBlossom and its rootChild.
      graph.parentBlossomPool.destroy(
        static_cast<ParentBlossom<edge_weight> &>(rootChild));
      graph.rootBlossomPool.destroy(*this);
    }

#define ROOT_BLOSSOM_INSTANTIATION(a) template class RootBlossom<a>;
    INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(ROOT_BLOSSOM_INSTANTIATION)

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

#define AUGMENT_TO_SOURCE_INSTANTIATION(a) \
template void augmentToSource(Vertex<a> *, Vertex<a> *);
    INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(AUGMENT_TO_SOURCE_INSTANTIATION)
  }
}
