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


#include <algorithm>
#include <cassert>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <vector>

#include "../templateinstantiation.h"

#include "blossomsig.h"
#include "blossomvertexiteratorimpl.h"
#include "graphimpl.h"
#include "parentblossomsig.h"
#include "rootblossomimpl.h"
#include "rootblossomiteratorimpl.h"
#include "types.h"
#include "verteximpl.h"

namespace matching
{
  namespace detail
  {
    namespace
    {
      template <typename edge_weight>
      void updateMinOuterDualVariable(
        Vertex<edge_weight> &newOuterVertex,
        Vertex<edge_weight> *& minOuterDualVariableVertex,
        edge_weight &minOuterDualVariable)
      {
        assert(newOuterVertex.rootBlossom->label == LABEL_OUTER);
        if (newOuterVertex.dualVariable < minOuterDualVariable)
        {
          minOuterDualVariableVertex = &newOuterVertex;
          minOuterDualVariable = newOuterVertex.dualVariable;
        }
      }

      /**
       * If the resistance between the two Vertexes is smaller than the min
       * stored in innerVertex, update the min.
       */
      template <typename edge_weight>
      void updateInnerOuterEdge(
        Vertex<edge_weight> &innerVertex,
        Vertex<edge_weight> &outerVertex)
      {
        const edge_weight resistance = innerVertex.resistance(outerVertex);
        if (resistance < innerVertex.minOuterEdgeResistance)
        {
          innerVertex.minOuterEdgeResistance = resistance;
          innerVertex.minOuterEdge = &outerVertex;
        }
      }
    }

    /**
     * If the resistance between blossom0 and blossom1 is smaller than
     * minResistance, update the min.
     */
    template <typename edge_weight>
    edge_weight updateOuterOuterEdges(
      const RootBlossom<edge_weight> &blossom0,
      const RootBlossom<edge_weight> &blossom1,
      edge_weight minResistance)
    {
      for (
        BlossomVertexIterator<edge_weight> vertexIterator0 =
          blossom0.blossomVertexBegin();
        vertexIterator0 != blossom0.blossomVertexEnd();
        ++vertexIterator0)
      {
        for (
          BlossomVertexIterator<edge_weight> vertexIterator1 =
            blossom1.blossomVertexBegin();
          vertexIterator1 != blossom1.blossomVertexEnd();
          ++vertexIterator1)
        {
          const edge_weight resistance =
            vertexIterator0->resistance(*vertexIterator1);

          assert(!(resistance & 1u));

          if (resistance < minResistance)
          {
            minResistance = resistance;

            vertexIterator0->rootBlossom
                ->minOuterEdges
                [vertexIterator1->rootBlossom->baseVertex->vertexIndex] =
              &*vertexIterator0;
            vertexIterator1->rootBlossom
                ->minOuterEdges
                [vertexIterator0->rootBlossom->baseVertex->vertexIndex] =
              &*vertexIterator1;

            vertexIterator0->rootBlossom->minOuterEdgeResistance =
              std::min(
                vertexIterator0->rootBlossom->minOuterEdgeResistance,
                minResistance);
            vertexIterator1->rootBlossom->minOuterEdgeResistance =
              std::min(
                vertexIterator1->rootBlossom->minOuterEdgeResistance,
                minResistance);
          }
        }
      }
      return minResistance;
    }

    /**
     * Set the label member of all the RootBlossoms, and set labeledVertex and
     * labelingVertex to 0.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeLabeling() const &
    {
      for (
        RootBlossomIterator<edge_weight> rootBlossomIterator =
          rootBlossomBegin();
        rootBlossomIterator != rootBlossomEnd();
        ++rootBlossomIterator)
      {
        rootBlossomIterator->label =
          rootBlossomIterator->baseVertexMatch ? LABEL_FREE
            : rootBlossomIterator->baseVertex->dualVariable ? LABEL_OUTER
            : LABEL_ZERO;
        rootBlossomIterator->labeledVertex = nullptr;
        rootBlossomIterator->labelingVertex = nullptr;
      }
    }

    /**
     * Update the minimum outer edges of all non-OUTER Vertexes if the edge to
     * one of the Vertexes in outerBlossom is less.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::updateInnerOuterEdges(
      const RootBlossom<edge_weight> &outerBlossom
    ) const &
    {
      for (const std::unique_ptr<Vertex<edge_weight>> &innerVertex : *this)
      {
        if (innerVertex->rootBlossom->label != LABEL_OUTER)
        {
          for (
            BlossomVertexIterator<edge_weight> vertexIterator =
              outerBlossom.blossomVertexBegin();
            vertexIterator != outerBlossom.blossomVertexEnd();
            ++vertexIterator)
          {
            updateInnerOuterEdge(*innerVertex, *vertexIterator);
          }
        }
      }
    }

    /**
     * Reset the minimum outer edges for all non-OUTER Vertexes, and re-compute
     * their values.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeInnerOuterEdges() const &
    {
      std::deque<Vertex<edge_weight> *> outerVertices;
      for (const std::unique_ptr<Vertex<edge_weight>> &outerVertex : *this)
      {
        if (outerVertex->rootBlossom->label == LABEL_OUTER)
        {
          outerVertices.push_back(outerVertex.get());
        }
      }
      for (const std::unique_ptr<Vertex<edge_weight>> &innerVertex : *this)
      {
        if (innerVertex->rootBlossom->label != LABEL_OUTER)
        {
          innerVertex->minOuterEdgeResistance = maxEdgeWeight * 2u + 1u;

          for (Vertex<edge_weight> *outerVertex : outerVertices)
          {
            updateInnerOuterEdge(*innerVertex, *outerVertex);
          }
        }
      }
    }

    /**
     * Reset the minimum outer edges for the given RootBlossom, and re-compute
     * its values.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeOuterOuterEdges(
      RootBlossom<edge_weight> &blossom0
    ) const &
    {
      blossom0.minOuterEdgeResistance = maxEdgeWeight * 2u + 1u;
      for (
        RootBlossomIterator<edge_weight> iterator = rootBlossomBegin();
        iterator != rootBlossomEnd();
        ++iterator)
      {
        if (iterator->label == LABEL_OUTER && &blossom0 != &*iterator)
        {
          blossom0.minOuterEdges[iterator->baseVertex->vertexIndex] =
            nullptr;
          updateOuterOuterEdges(
            blossom0,
            *iterator,
            maxEdgeWeight * 2u + 1u);
        }
      }
    }

    /**
     * Reset the minimum outer edges for all OUTER RootBlossoms, and re-compute
     * their values.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeOuterOuterEdges() const &
    {
      for (
        RootBlossomIterator<edge_weight> iterator0 = rootBlossomBegin();
        iterator0 != rootBlossomEnd();
        ++iterator0)
      {
        if (iterator0->label == LABEL_OUTER)
        {
          iterator0->minOuterEdgeResistance = maxEdgeWeight * 2u + 1u;
        }
      }
      for (
        RootBlossomIterator<edge_weight> iterator0 = rootBlossomBegin();
        iterator0 != rootBlossomEnd();
        ++iterator0)
      {
        if (iterator0->label == LABEL_OUTER)
        {
          initializeOuterOuterEdges(*iterator0);
        }
      }
    }

    /**
     * Augment the matching, leaving one or two fewer non-free, non-zero
     * vertices.
     */
    template <typename edge_weight>
    bool Graph<edge_weight>::augmentMatching() const &
    {
      initializeLabeling();

      edge_weight minOuterDualVariable = maxEdgeWeight * 2u + 1u;
      Vertex<edge_weight> *minOuterDualVariableVertex{ };
      for (const std::unique_ptr<Vertex<edge_weight>> &vertex : *this)
      {
        assert(vertex->dualVariable <= maxEdgeWeight);

        if (vertex->rootBlossom->label == LABEL_OUTER)
        {
          updateMinOuterDualVariable(
            *vertex,
            minOuterDualVariableVertex,
            minOuterDualVariable);
        }
      }
      if (!minOuterDualVariableVertex)
      {
        return false;
      }

      initializeInnerOuterEdges();
      initializeOuterOuterEdges();

      // The loop for manipulating blossoms and adjusting the dual variables.
      while (true)
      {
        // Compute the variables needed to choose the dual variable adjustment.
        edge_weight minInnerOuterEdgeResistance = maxEdgeWeight * 2u + 1u;
        Vertex<edge_weight> *minInnerOuterEdgeResistanceVertex{ };
        for (const std::unique_ptr<Vertex<edge_weight>> &vertex : *this)
        {
          if (
            (vertex->rootBlossom->label == LABEL_FREE
                || vertex->rootBlossom->label == LABEL_ZERO
            ) && vertex->minOuterEdgeResistance < minInnerOuterEdgeResistance
          ) {
            minInnerOuterEdgeResistance = vertex->minOuterEdgeResistance;
            minInnerOuterEdgeResistanceVertex = vertex.get();
          }
        }

        edge_weight minOuterOuterEdgeResistance = maxEdgeWeight * 2u + 1u;
        RootBlossom<edge_weight> *minOuterOuterEdgeResistanceRootBlossom{ };
        for (
          RootBlossomIterator<edge_weight> rootBlossomIterator =
            rootBlossomBegin();
          rootBlossomIterator != rootBlossomEnd();
          ++rootBlossomIterator)
        {
          if (
            rootBlossomIterator->label == LABEL_OUTER
              && rootBlossomIterator->minOuterEdgeResistance
                  < minOuterOuterEdgeResistance)
          {
            minOuterOuterEdgeResistance =
              rootBlossomIterator->minOuterEdgeResistance;
            minOuterOuterEdgeResistanceRootBlossom = &*rootBlossomIterator;
          }
        }
        assert(
          !minOuterOuterEdgeResistanceRootBlossom
            || !(minOuterOuterEdgeResistance & 1));
        minOuterOuterEdgeResistance >>= 1;

        edge_weight minInnerDualVariable = maxEdgeWeight * 2u + 1u;
        ParentBlossom<edge_weight> *minInnerDualVariableBlossom{ };
        for (
          RootBlossomIterator<edge_weight> rootBlossomIterator =
            rootBlossomBegin();
          rootBlossomIterator != rootBlossomEnd();
          ++rootBlossomIterator)
        {
          if (
            rootBlossomIterator->label == LABEL_INNER
              && !rootBlossomIterator->rootChild.isVertex)
          {
            ParentBlossom<edge_weight> &parentBlossom =
              static_cast<ParentBlossom<edge_weight> &>(
                rootBlossomIterator->rootChild);
            if (parentBlossom.dualVariable < minInnerDualVariable)
            {
              minInnerDualVariable = parentBlossom.dualVariable;
              minInnerDualVariableBlossom = &parentBlossom;
            }
          }
        }
        assert(!minInnerDualVariableBlossom || !(minInnerDualVariable & 1u));
        minInnerDualVariable >>= 1;

        // Adjust the dual variables.
        const edge_weight dualAdjustment =
          std::min(
            {
              minOuterDualVariable,
              minInnerOuterEdgeResistance,
              minOuterOuterEdgeResistance,
              minInnerDualVariable
            }
          );
        minOuterDualVariable -= dualAdjustment;
        minInnerOuterEdgeResistance -= dualAdjustment;
        minOuterOuterEdgeResistance -= dualAdjustment;
        minInnerDualVariable -= dualAdjustment;
        const edge_weight twiceAdjustment = dualAdjustment << 1;

        for (
          RootBlossomIterator<edge_weight> rootBlossomIterator =
            rootBlossomBegin();
          rootBlossomIterator != rootBlossomEnd();
          ++rootBlossomIterator)
        {
          if (rootBlossomIterator->label == LABEL_OUTER)
          {
            if (
              rootBlossomIterator->minOuterEdgeResistance <= maxEdgeWeight * 2u)
            {
              rootBlossomIterator->minOuterEdgeResistance -= twiceAdjustment;
            }
            if (!rootBlossomIterator->rootChild.isVertex)
            {
              static_cast<ParentBlossom<edge_weight> &>(
                rootBlossomIterator->rootChild
              ).dualVariable += twiceAdjustment;
            }
          }
          else if (
            rootBlossomIterator->label == LABEL_INNER
              && !rootBlossomIterator->rootChild.isVertex)
          {
            static_cast<ParentBlossom<edge_weight> &>(
                rootBlossomIterator->rootChild
              ).dualVariable -= twiceAdjustment;
          }
          for (
            BlossomVertexIterator<edge_weight> vertexIterator =
              rootBlossomIterator->blossomVertexBegin();
            vertexIterator != rootBlossomIterator->blossomVertexEnd();
            ++vertexIterator)
          {
            if (rootBlossomIterator->label == LABEL_OUTER)
            {
              vertexIterator->dualVariable -= dualAdjustment;
            }
            else if (rootBlossomIterator->label == LABEL_INNER)
            {
              vertexIterator->dualVariable += dualAdjustment;
            }
            else if (
              vertexIterator->minOuterEdgeResistance <= maxEdgeWeight * 2u)
            {
              vertexIterator->minOuterEdgeResistance -= dualAdjustment;
            }
            assert(vertexIterator->dualVariable <= maxEdgeWeight);
          }
        }

        // Find the condition that halted the dual variable adjustment, and
        // apply the relevant change.
        if (!minOuterDualVariable) {
          // An OUTER blossom has a Vertex with dualVariable 0.
          augmentToSource<edge_weight>(minOuterDualVariableVertex, nullptr);
          return true;
        }
        else if (!minInnerOuterEdgeResistance)
        {
          if (
            minInnerOuterEdgeResistanceVertex->rootBlossom->label
              == LABEL_FREE)
          {
            // The resistance between a FREE Vertex and an OUTER Vertex is 0.
            // Label the FREE Vertex and its match.
            RootBlossom<edge_weight> &matchedRootBlossom =
              *minInnerOuterEdgeResistanceVertex->rootBlossom
                ->baseVertexMatch
                ->rootBlossom;
            minInnerOuterEdgeResistanceVertex->rootBlossom->label =
              LABEL_INNER;
            matchedRootBlossom.label = LABEL_OUTER;
            minInnerOuterEdgeResistanceVertex->rootBlossom->labelingVertex =
              minInnerOuterEdgeResistanceVertex->minOuterEdge;
            minInnerOuterEdgeResistanceVertex->rootBlossom->labeledVertex =
              minInnerOuterEdgeResistanceVertex;
            updateInnerOuterEdges(matchedRootBlossom);
            initializeOuterOuterEdges(matchedRootBlossom);
            for (
              BlossomVertexIterator<edge_weight> iterator =
                matchedRootBlossom.blossomVertexBegin();
              iterator != matchedRootBlossom.blossomVertexEnd();
              ++iterator)
            {
              updateMinOuterDualVariable(
                *iterator,
                minOuterDualVariableVertex,
                minOuterDualVariable);
            }

            continue;
          }
          else
          {
            // The resistance between a ZERO Vertex and an OUTER Vertex is 0.
            // Augment from the ZERO Vertex to the OUTER Vertex.
            assert(
              minInnerOuterEdgeResistanceVertex->rootBlossom->label
                == LABEL_ZERO);

            augmentToSource(
              minInnerOuterEdgeResistanceVertex->minOuterEdge,
              minInnerOuterEdgeResistanceVertex);
            augmentToSource(
              minInnerOuterEdgeResistanceVertex,
              minInnerOuterEdgeResistanceVertex->minOuterEdge);

            return true;
          }
        }
        else if (!minOuterOuterEdgeResistance)
        {
          // The resistance between two OUTER Vertexes is zero.
          Vertex<edge_weight> *vertex0, *vertex1;
          for (
            RootBlossomIterator<edge_weight> rootBlossomIterator =
              rootBlossomBegin();
            ;
            ++rootBlossomIterator)
          {
            assert(rootBlossomIterator != rootBlossomEnd());

            if (
              rootBlossomIterator->label == LABEL_OUTER
                && &*rootBlossomIterator
                    != minOuterOuterEdgeResistanceRootBlossom)
            {
              vertex0 =
                minOuterOuterEdgeResistanceRootBlossom
                  ->minOuterEdges
                  [rootBlossomIterator->baseVertex->vertexIndex];
              vertex1 =
                rootBlossomIterator
                  ->minOuterEdges
                  [minOuterOuterEdgeResistanceRootBlossom
                    ->baseVertex
                    ->vertexIndex];
              if (vertex0 && vertex1 && vertex0->resistance(*vertex1) <= 0)
              {
                break;
              }
            }
          }

          std::deque<Vertex<edge_weight> *> path;
          path.push_front(vertex0);
          path.push_back(vertex1);
          while (path.front()->rootBlossom->baseVertexMatch)
          {
            path.push_front(path.front()->rootBlossom->baseVertex);
            path.push_front(path.front()->rootBlossom->baseVertexMatch);
            path.push_front(path.front()->rootBlossom->labeledVertex);
            path.push_front(path.front()->rootBlossom->labelingVertex);
          }
          while (path.back()->rootBlossom->baseVertexMatch)
          {
            path.push_back(path.back()->rootBlossom->baseVertex);
            path.push_back(path.back()->rootBlossom->baseVertexMatch);
            path.push_back(path.back()->rootBlossom->labeledVertex);
            path.push_back(path.back()->rootBlossom->labelingVertex);
          }

          if (path.front()->rootBlossom == path.back()->rootBlossom)
          {
            // Form a new OUTER blossom.
            while (
              (*std::next(path.begin(), 1))->rootBlossom
                == (*std::next(path.rbegin(), 1))->rootBlossom)
            {
              path.pop_front();
              path.pop_front();
              path.pop_front();
              path.pop_front();
              path.pop_back();
              path.pop_back();
              path.pop_back();
              path.pop_back();
            }
            assert(path.front()->rootBlossom->label == LABEL_OUTER);
            RootBlossom<edge_weight> *const newBlossom =
              new RootBlossom<edge_weight>(path.cbegin(), path.cend(), *this);

            assert(newBlossom->label == LABEL_OUTER);
            for (
              BlossomVertexIterator<edge_weight> iterator =
                newBlossom->blossomVertexBegin();
              iterator != newBlossom->blossomVertexEnd();
              ++iterator)
            {
              updateMinOuterDualVariable(
                *iterator,
                minOuterDualVariableVertex,
                minOuterDualVariable);
            }
          }
          else
          {
            // Augment from vertex0 to vertex1.
            augmentToSource(vertex0, vertex1);
            augmentToSource(vertex1, vertex0);

            return true;
          }
        }
        else if (!minInnerDualVariable)
        {
          // An INNER RootBlossom has dualVariable zero. Dissolve it.
          Vertex<edge_weight> &rootVertex =
            *minInnerDualVariableBlossom->rootBlossom->baseVertex;
          Blossom<edge_weight> &rootChild =
            getAncestorOfVertex(rootVertex, minInnerDualVariableBlossom);

          Blossom<edge_weight> &connectChild =
            getAncestorOfVertex(
              *minInnerDualVariableBlossom->rootBlossom->labeledVertex,
              minInnerDualVariableBlossom);
          bool connectForward = true;
          for (
            Blossom<edge_weight> *currentChild = &rootChild;
            currentChild != &connectChild;
            currentChild = currentChild->nextBlossom)
          {
            connectForward = !connectForward;
          }

          bool linksToNext{ };
          bool isFree{ };
          Blossom<edge_weight> *previousChild = rootChild.previousBlossom;
          for (
            Blossom<edge_weight> *currentChild = &rootChild, *nextChild{ };
            nextChild != &rootChild;
            currentChild = nextChild)
          {
            nextChild = currentChild->nextBlossom;
            if (currentChild == &connectChild && !connectForward)
            {
              isFree = false;
            }

            const Label label =
              isFree ? LABEL_FREE
                : linksToNext ^ connectForward || &rootChild == currentChild
                    ? LABEL_INNER
                    : LABEL_OUTER;

            new RootBlossom<edge_weight>(
              *currentChild,
              &rootChild == currentChild ? rootVertex
                : linksToNext ? *currentChild->vertexToNextSiblingBlossom
                : *currentChild->vertexToPreviousSiblingBlossom,
              &rootChild == currentChild
                ? minInnerDualVariableBlossom->rootBlossom->baseVertexMatch
                : linksToNext
                    ? nextChild->vertexToPreviousSiblingBlossom
                    : previousChild->vertexToNextSiblingBlossom,
              label,
              &connectChild == currentChild
                ? minInnerDualVariableBlossom->rootBlossom->labelingVertex
                : label == LABEL_INNER
                    ? connectForward
                        ? nextChild->vertexToPreviousSiblingBlossom
                        : previousChild->vertexToNextSiblingBlossom
                    : nullptr,
              &connectChild == currentChild
                ? minInnerDualVariableBlossom->rootBlossom->labeledVertex
                : label == LABEL_INNER
                    ? connectForward
                        ? currentChild->vertexToNextSiblingBlossom
                        : currentChild->vertexToPreviousSiblingBlossom
                    : nullptr);
            if (label == LABEL_OUTER)
            {
              updateInnerOuterEdges(*currentChild->rootBlossom);
              initializeOuterOuterEdges(*currentChild->rootBlossom);
              for (
                BlossomVertexIterator<edge_weight> iterator =
                  currentChild->rootBlossom->blossomVertexBegin();
                iterator != currentChild->rootBlossom->blossomVertexEnd();
                ++iterator)
              {
                updateMinOuterDualVariable(
                  *iterator,
                  minOuterDualVariableVertex,
                  minOuterDualVariable);
              }
            }

            if (currentChild == &(connectForward ? connectChild : rootChild))
            {
              isFree = true;
            }
            linksToNext = !linksToNext;
            previousChild = currentChild;
          }
        }
      }
    }

    /**
     * Find the maximum matching for the graph.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::computeMatching() const &
    {
      // Make sure all exposed Vertex dualVariables have the same parity.
      for (
        RootBlossomIterator<edge_weight> rootBlossomIterator =
          rootBlossomBegin();
        rootBlossomIterator != rootBlossomEnd();
        ++rootBlossomIterator)
      {
        if (
          !rootBlossomIterator->baseVertexMatch
            && rootBlossomIterator->baseVertex->dualVariable & 1)
        {
          Blossom<edge_weight> *adjustableBlossom =
            &rootBlossomIterator->rootChild;
          setPointersFromAncestor(
            *rootBlossomIterator->baseVertex,
            *adjustableBlossom,
            true);

          while (
            !adjustableBlossom->isVertex
              && !static_cast<ParentBlossom<edge_weight> *const>(
                    adjustableBlossom
                  )->dualVariable)
          {
            adjustableBlossom =
              static_cast<const ParentBlossom<edge_weight> *const>(
                adjustableBlossom
              )->subblossom;
          }
          rootBlossomIterator->freeAncestorOfBase(*adjustableBlossom);

          if (!adjustableBlossom->isVertex)
          {
            assert(
              static_cast<const ParentBlossom<edge_weight> *const>(
                adjustableBlossom
              )->dualVariable >= 2u);

            static_cast<ParentBlossom<edge_weight> *const>(
              adjustableBlossom
            )->dualVariable -= 2;
          }
          for (
            BlossomVertexIterator<edge_weight> vertexIterator =
              rootBlossomIterator->blossomVertexBegin();
            vertexIterator != rootBlossomIterator->blossomVertexEnd();
            ++vertexIterator)
          {
            ++vertexIterator->dualVariable;
            assert(!(vertexIterator->dualVariable & 1u));
          }
        }
      }

      // Continue augmenting the matching until it is maximum.
      while (augmentMatching()) { }
    }

    MATCHINGEDGEWEIGHTPARAMETERS1(template class Graph<, >;)
  }
}
