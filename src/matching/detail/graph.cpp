#include <algorithm>
#include <cassert>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <vector>

#include "../templateinstantiation.h"

#include "blossomsig.h"
#include "graphsig.h"
#include "parentblossomsig.h"
#include "rootblossomimpl.h"
#include "types.h"
#include "verteximpl.h"

namespace matching
{
  namespace detail
  {
    namespace
    {
      /**
       * If the provided outer Vertex has a lower dual variable than the current
       * minimum, update the minimum.
       */
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
       * If the provided outer RootBlossom has a lower minOuterEdgeResistance
       * than the current minimum, update the minimum.
       */
      template <typename edge_weight>
      void updateMinOuterOuterEdgeResistance(
        RootBlossom<edge_weight> &newRootBlossom,
        RootBlossom<edge_weight> *&minOuterOuterEdgeResistanceRootBlossom,
        edge_weight &minOuterOuterEdgeResistance)
      {
        if (newRootBlossom.minOuterEdgeResistance < minOuterOuterEdgeResistance)
        {
          minOuterOuterEdgeResistance =
            newRootBlossom.minOuterEdgeResistance;
          minOuterOuterEdgeResistanceRootBlossom = &newRootBlossom;
        }
        assert(
          !minOuterOuterEdgeResistanceRootBlossom
            || !(minOuterOuterEdgeResistance & 1u));
      }

      /**
       * If the provided inner RootBlossom has a lower blossom dual variable
       * than the minimum, update the minimum.
       */
      template <typename edge_weight>
      void updateMinInnerDualVariable(
        RootBlossom<edge_weight> &newRootBlossom,
        ParentBlossom<edge_weight> *&minInnerDualVariableBlossom,
        edge_weight &minInnerDualVariable)
      {
        if (!newRootBlossom.rootChild.isVertex)
        {
          ParentBlossom<edge_weight> &parentBlossom =
            static_cast<ParentBlossom<edge_weight> &>(newRootBlossom.rootChild);
          if (parentBlossom.dualVariable < minInnerDualVariable)
          {
            minInnerDualVariable = parentBlossom.dualVariable;
            minInnerDualVariableBlossom = &parentBlossom;
          }
        }
        assert(!minInnerDualVariableBlossom || !(minInnerDualVariable & 1u));
      }

      /**
       * If the resistance between the two Vertexes is smaller than the min
       * stored in innerVertex, update the min.
       */
      template <typename edge_weight>
      void updateInnerOuterEdge(
        Vertex<edge_weight> &innerVertex,
        Vertex<edge_weight> &outerVertex,
        edge_weight &resistanceStorage)
      {
        outerVertex.resistance(resistanceStorage, innerVertex);
        if (resistanceStorage < innerVertex.minOuterEdgeResistance)
        {
          innerVertex.minOuterEdgeResistance = resistanceStorage;
          innerVertex.minOuterEdge = &outerVertex;
        }
      }
    }

    /**
     * Find the minimum resistance between a Vertex in blossom0 and a Vertex in
     * blossom1, and save these minimum Vertexes. Update the RootBlossoms'
     * minOuterEdgeResistance field if appropriate.
     */
    template <typename edge_weight>
    void updateOuterOuterEdges(
      const RootBlossom<edge_weight> &blossom0,
      const RootBlossom<edge_weight> &blossom1,
      edge_weight &minResistance,
      typename
        std::conditional<
          std::is_trivially_copyable<edge_weight>::value,
          edge_weight,
          edge_weight &
        >::type
        resistanceStorage)
    {
      RootBlossom<edge_weight> &actualBlossom0 =
        *blossom0.rootChild.rootBlossom;
      RootBlossom<edge_weight> &actualBlossom1 =
        *blossom1.rootChild.rootBlossom;

      decltype(resistanceStorage) minResistanceTemp = minResistance;

      for (
        Vertex<edge_weight> *vertexIterator0 =
          blossom0.rootChild.vertexListHead;
        vertexIterator0;
        vertexIterator0 = vertexIterator0->nextVertex)
      {
        for (
          Vertex<edge_weight> *vertexIterator1 =
            blossom1.rootChild.vertexListHead;
          vertexIterator1;
          vertexIterator1 = vertexIterator1->nextVertex)
        {
          vertexIterator0->resistance(resistanceStorage, *vertexIterator1);

          assert(!(resistanceStorage & 1u));

          if (resistanceStorage < minResistanceTemp)
          {
            minResistanceTemp = resistanceStorage;

            actualBlossom0.minOuterEdges
                [actualBlossom1.baseVertex->vertexIndex] =
              &*vertexIterator0;
            actualBlossom1.minOuterEdges
                [actualBlossom0.baseVertex->vertexIndex] =
              &*vertexIterator1;

            if (minResistanceTemp < actualBlossom0.minOuterEdgeResistance)
            {
              actualBlossom0.minOuterEdgeResistance = minResistanceTemp;
            }
            if (minResistanceTemp < actualBlossom1.minOuterEdgeResistance)
            {
              actualBlossom1.minOuterEdgeResistance = minResistanceTemp;
            }
          }
        }
      }

      if (std::is_trivially_copyable<edge_weight>::value)
      {
        minResistance = minResistanceTemp;
      }
    }

#define UPDATE_OUTER_OUTER_EDGES_INSTANTIATION(a) \
template void updateOuterOuterEdges( \
const RootBlossom<a> &, \
const RootBlossom<a> &, \
a &, \
typename std::conditional<std::is_trivially_copyable<a>::value, a, a &>::type);
    INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(
      UPDATE_OUTER_OUTER_EDGES_INSTANTIATION)

    /**
     * Set the label member of all the RootBlossoms, and set labeledVertex and
     * labelingVertex to 0.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeLabeling() const &
    {
      for (
        auto rootBlossomIterator = rootBlossomPool.begin();
        rootBlossomIterator != rootBlossomPool.end();
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
    ) &
    {
      edge_weight resistance = aboveMaxEdgeWeight;
      for (Vertex<edge_weight> &innerVertex : *this)
      {
        if (innerVertex.rootBlossom->label != LABEL_OUTER)
        {
          for (
            auto outerVertexIterator = outerBlossom.rootChild.vertexListHead;
            outerVertexIterator;
            outerVertexIterator = outerVertexIterator->nextVertex)
          {
            updateInnerOuterEdge(
              innerVertex,
              *outerVertexIterator,
              resistance);
          }
        }
      }
    }

    /**
     * Reset the minimum outer edges for all non-OUTER Vertexes, and re-compute
     * their values.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeInnerOuterEdges() &
    {
      std::vector<Vertex<edge_weight> *> outerVertices;
      outerVertices.reserve(this->size());
      for (Vertex<edge_weight> &outerVertex : *this)
      {
        if (outerVertex.rootBlossom->label == LABEL_OUTER)
        {
          outerVertices.push_back(&outerVertex);
        }
      }
      edge_weight resistance = aboveMaxEdgeWeight;
      for (Vertex<edge_weight> &innerVertex : *this)
      {
        if (innerVertex.rootBlossom->label != LABEL_OUTER)
        {
          innerVertex.minOuterEdgeResistance = aboveMaxEdgeWeight;

          for (Vertex<edge_weight> *outerVertex : outerVertices)
          {
            updateInnerOuterEdge(innerVertex, *outerVertex, resistance);
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
      RootBlossom<edge_weight> &blossom0,
      edge_weight &resistanceStorage0,
      edge_weight &resistanceStorage1
    ) const &
    {
      blossom0.minOuterEdgeResistance = aboveMaxEdgeWeight;
      for (
        auto iterator = rootBlossomPool.begin();
        iterator != rootBlossomPool.end();
        ++iterator)
      {
        resistanceStorage0 = aboveMaxEdgeWeight;
        if (iterator->label == LABEL_OUTER && &blossom0 != &*iterator)
        {
          blossom0.minOuterEdges[iterator->baseVertex->vertexIndex] =
            nullptr;
          if (std::is_trivially_copyable<edge_weight>::value)
          {
            edge_weight storage;
            updateOuterOuterEdges(
              blossom0,
              *iterator,
              resistanceStorage0,
              storage);
          }
          else
          {
            updateOuterOuterEdges(
              blossom0,
              *iterator,
              resistanceStorage0,
              resistanceStorage1);
          }
        }
      }
    }
    template <typename edge_weight>
    void Graph<edge_weight>::initializeOuterOuterEdges(
      RootBlossom<edge_weight> &blossom0
    ) const &
    {
      edge_weight resistance0 = aboveMaxEdgeWeight;
      edge_weight resistance1 = aboveMaxEdgeWeight;
      initializeOuterOuterEdges(blossom0, resistance0, resistance1);
    }

    /**
     * Reset the minimum outer edges for all OUTER RootBlossoms, and re-compute
     * their values.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeOuterOuterEdges() const &
    {
      edge_weight resistance0 = aboveMaxEdgeWeight;
      edge_weight resistance1 = aboveMaxEdgeWeight;
      for (
        auto iterator0 = rootBlossomPool.begin();
        iterator0 != rootBlossomPool.end();
        ++iterator0)
      {
        if (iterator0->label == LABEL_OUTER)
        {
          initializeOuterOuterEdges(*iterator0, resistance0, resistance1);
        }
      }
    }

    /**
     * Compute the minimum resistance between outer vertices in different
     * RootBlossoms.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeMinOuterOuterEdgeResistance(
      RootBlossom<edge_weight> *&minOuterOuterEdgeResistanceRootBlossom,
      edge_weight &minOuterOuterEdgeResistance
    ) const
    {
      minOuterOuterEdgeResistance = aboveMaxEdgeWeight;
      minOuterOuterEdgeResistanceRootBlossom = nullptr;
      for (
        auto rootBlossomIterator = rootBlossomPool.begin();
        rootBlossomIterator != rootBlossomPool.end();
        ++rootBlossomIterator)
      {
        if (rootBlossomIterator->label == LABEL_OUTER)
        {
          updateMinOuterOuterEdgeResistance(
            *rootBlossomIterator,
            minOuterOuterEdgeResistanceRootBlossom,
            minOuterOuterEdgeResistance);
        }
      }
    }

    /**
     * Compute the minimum blossom dual variable among inner blossoms.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::initializeMinInnerDualVariable(
      ParentBlossom<edge_weight> *&minInnerDualVariableBlossom,
      edge_weight &minInnerDualVariable
    ) const
    {
      minInnerDualVariable = aboveMaxEdgeWeight;
      minInnerDualVariableBlossom = nullptr;
      for (
        auto rootBlossomIterator = rootBlossomPool.begin();
        rootBlossomIterator != rootBlossomPool.end();
        ++rootBlossomIterator)
      {
        if (rootBlossomIterator->label == LABEL_INNER)
        {
          updateMinInnerDualVariable(
            *rootBlossomIterator,
            minInnerDualVariableBlossom,
            minInnerDualVariable);
        }
      }
    }

    /**
     * Augment the matching, leaving one or two fewer non-free, non-zero
     * vertices.
     */
    template <typename edge_weight>
    bool Graph<edge_weight>::augmentMatching() &
    {
      initializeLabeling();

      edge_weight minOuterDualVariable = aboveMaxEdgeWeight;
      Vertex<edge_weight> *minOuterDualVariableVertex{ };
      for (Vertex<edge_weight> &vertex : *this)
      {
        assert(vertex.dualVariable <= aboveMaxEdgeWeight >> 1);

        if (vertex.rootBlossom->label == LABEL_OUTER)
        {
          updateMinOuterDualVariable(
            vertex,
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

      // Set up variables needed to choose the dual variable adjustment, which
      // will be update incrementally.
      edge_weight minOuterOuterEdgeResistance = aboveMaxEdgeWeight;
      RootBlossom<edge_weight> *minOuterOuterEdgeResistanceRootBlossom;
      initializeMinOuterOuterEdgeResistance(
        minOuterOuterEdgeResistanceRootBlossom,
        minOuterOuterEdgeResistance);

      edge_weight minInnerDualVariable = aboveMaxEdgeWeight;
      ParentBlossom<edge_weight> *minInnerDualVariableBlossom{ };

      // The loop for manipulating blossoms and adjusting the dual variables.
      while (true)
      {
        // Compute variables needed to choose the dual variable adjustment.
        edge_weight minInnerOuterEdgeResistance = aboveMaxEdgeWeight;
        Vertex<edge_weight> *minInnerOuterEdgeResistanceVertex{ };
        for (Vertex<edge_weight> &vertex : *this)
        {
          if (
            (vertex.rootBlossom->label == LABEL_FREE
                || vertex.rootBlossom->label == LABEL_ZERO
            ) && vertex.minOuterEdgeResistance < minInnerOuterEdgeResistance
          ) {
            minInnerOuterEdgeResistance = vertex.minOuterEdgeResistance;
            minInnerOuterEdgeResistanceVertex = &vertex;
          }
        }

        // Adjust the dual variables.
        const edge_weight dualAdjustment =
          std::min(
            {
              minOuterDualVariable,
              minInnerOuterEdgeResistance,
              edge_weight(minOuterOuterEdgeResistance >> 1),
              edge_weight(minInnerDualVariable >> 1)
            }
          );
        if (dualAdjustment)
        {
          edge_weight twiceAdjustment = dualAdjustment << 1;
          minOuterDualVariable -= dualAdjustment;
          minInnerOuterEdgeResistance -= dualAdjustment;
          minOuterOuterEdgeResistance -= twiceAdjustment;
          minInnerDualVariable -= twiceAdjustment;

          for (
            typename Graph<edge_weight>::iterator vertexIterator =
              this->begin();
            vertexIterator != this->end();
            ++vertexIterator)
          {
            RootBlossom<edge_weight> &rootBlossom =
              *vertexIterator->rootBlossom;
            Label label = rootBlossom.label;
            if (label == LABEL_OUTER)
            {
              vertexIterator->dualVariable -= dualAdjustment;
            }
            else if (label == LABEL_INNER)
            {
              vertexIterator->dualVariable += dualAdjustment;
            }
            else if (
              vertexIterator->minOuterEdgeResistance < aboveMaxEdgeWeight)
            {
              vertexIterator->minOuterEdgeResistance -= dualAdjustment;
            }
            assert(vertexIterator->dualVariable <= aboveMaxEdgeWeight >> 1);
            if (rootBlossom.baseVertex == &*vertexIterator)
            {
              if (label == LABEL_OUTER)
              {
                if (rootBlossom.minOuterEdgeResistance < aboveMaxEdgeWeight)
                {
                  rootBlossom.minOuterEdgeResistance -= twiceAdjustment;
                }
                if (!rootBlossom.rootChild.isVertex)
                {
                  static_cast<ParentBlossom<edge_weight> &>(
                    rootBlossom.rootChild
                  ).dualVariable += twiceAdjustment;
                }
              }
              else if (label == LABEL_INNER && !rootBlossom.rootChild.isVertex)
              {
                static_cast<ParentBlossom<edge_weight> &>(
                    rootBlossom.rootChild
                  ).dualVariable -= twiceAdjustment;
              }
            }
          }
        }

        // Find the condition that halted the dual variable adjustment, and
        // apply the relevant change.
        if (!minOuterDualVariable) {
          // An OUTER blossom has a Vertex with dualVariable 0.
          augmentToSource<edge_weight>(minOuterDualVariableVertex, nullptr);
          return true;
        }
        if (
          !minInnerOuterEdgeResistance
            && minInnerOuterEdgeResistanceVertex->rootBlossom->label
                == LABEL_ZERO)
        {
          // The resistance between a ZERO Vertex and an OUTER Vertex is 0.
          // Augment from the ZERO Vertex to the OUTER Vertex.

          augmentToSource(
            minInnerOuterEdgeResistanceVertex->minOuterEdge,
            minInnerOuterEdgeResistanceVertex);
          augmentToSource(
            minInnerOuterEdgeResistanceVertex,
            minInnerOuterEdgeResistanceVertex->minOuterEdge);

          return true;
        }
        else if (!minOuterOuterEdgeResistance)
        {
          // The resistance between two OUTER Vertexes is zero.
          Vertex<edge_weight> *vertex0, *vertex1;
          for (
            auto rootBlossomIterator = rootBlossomPool.begin();
            ;
            ++rootBlossomIterator)
          {
            assert(rootBlossomIterator != rootBlossomPool.end());

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
              if (vertex0 && vertex1 && !vertex0->resistance(*vertex1))
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
            RootBlossom<edge_weight> &newBlossom =
              rootBlossomPool.construct(path.cbegin(), path.cend(), *this);

            assert(newBlossom.label == LABEL_OUTER);
            for (
              auto iterator = newBlossom.rootChild.vertexListHead;
              iterator;
              iterator = iterator->nextVertex)
            {
              updateMinOuterDualVariable(
                *iterator,
                minOuterDualVariableVertex,
                minOuterDualVariable);
            }
            initializeMinOuterOuterEdgeResistance(
              minOuterOuterEdgeResistanceRootBlossom,
              minOuterOuterEdgeResistance);
            initializeMinInnerDualVariable(
              minInnerDualVariableBlossom,
              minInnerDualVariable);
          }
          else
          {
            // Augment from vertex0 to vertex1.
            augmentToSource(vertex0, vertex1);
            augmentToSource(vertex1, vertex0);

            return true;
          }
        }
        else if (!minInnerOuterEdgeResistance)
        {
          // The resistance between a FREE Vertex and an OUTER Vertex is 0.
          // Label the FREE Vertex and its match.
          assert(
            minInnerOuterEdgeResistanceVertex->rootBlossom->label
              == LABEL_FREE);

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
            auto iterator = matchedRootBlossom.rootChild.vertexListHead;
            iterator;
            iterator = iterator->nextVertex)
          {
            updateMinOuterDualVariable(
              *iterator,
              minOuterDualVariableVertex,
              minOuterDualVariable);
          }
          updateMinOuterOuterEdgeResistance(
            matchedRootBlossom,
            minOuterOuterEdgeResistanceRootBlossom,
            minOuterOuterEdgeResistance);
          updateMinInnerDualVariable(
            *minInnerOuterEdgeResistanceVertex->rootBlossom,
            minInnerDualVariableBlossom,
            minInnerDualVariable);

          continue;
        }
        else if (!minInnerDualVariable)
        {
          rootBlossomPool.hide(*minInnerDualVariableBlossom->rootBlossom);

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

            rootBlossomPool.construct(
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
                    : nullptr,
              *this);
            if (label == LABEL_OUTER)
            {
              updateInnerOuterEdges(*currentChild->rootBlossom);
              initializeOuterOuterEdges(*currentChild->rootBlossom);
              for (
                auto iterator = currentChild->vertexListHead;
                iterator;
                iterator = iterator->nextVertex)
              {
                updateMinOuterDualVariable(
                  *iterator,
                  minOuterDualVariableVertex,
                  minOuterDualVariable);
              }
              updateMinOuterOuterEdgeResistance(
                *currentChild->rootBlossom,
                minOuterOuterEdgeResistanceRootBlossom,
                minOuterOuterEdgeResistance);
            }

            if (currentChild == &(connectForward ? connectChild : rootChild))
            {
              isFree = true;
            }
            linksToNext = !linksToNext;
            previousChild = currentChild;
          }

          rootBlossomPool.destroy(*minInnerDualVariableBlossom->rootBlossom);
          parentBlossomPool.destroy(*minInnerDualVariableBlossom);

          initializeMinInnerDualVariable(
            minInnerDualVariableBlossom,
            minInnerDualVariable);
        }
      }
    }

    /**
     * Find the maximum matching for the graph.
     */
    template <typename edge_weight>
    void Graph<edge_weight>::computeMatching() &
    {
      // Make sure all exposed Vertex dualVariables have the same parity.
      for (
        auto rootBlossomIterator = rootBlossomPool.begin();
        rootBlossomIterator != rootBlossomPool.end();
      )
      {
        RootBlossom<edge_weight> &rootBlossom = *rootBlossomIterator;
        ++rootBlossomIterator;
        if (
          !rootBlossom.baseVertexMatch
            && rootBlossom.baseVertex->dualVariable & 1u)
        {
          Blossom<edge_weight> *adjustableBlossom =
            &rootBlossom.rootChild;
          setPointersFromAncestor(
            *rootBlossom.baseVertex,
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
          rootBlossom.freeAncestorOfBase(*adjustableBlossom, *this);

          if (!adjustableBlossom->isVertex)
          {
            assert(
              static_cast<const ParentBlossom<edge_weight> *const>(
                adjustableBlossom
              )->dualVariable >= 2u);

            static_cast<ParentBlossom<edge_weight> *const>(
              adjustableBlossom
            )->dualVariable -= 2u;
          }
          for (
            auto vertexIterator = adjustableBlossom->vertexListHead;
            vertexIterator;
            vertexIterator = vertexIterator->nextVertex)
          {
            ++vertexIterator->dualVariable;
            assert(!(vertexIterator->dualVariable & 1u));
          }
        }
      }

      // Continue augmenting the matching until it is maximum.
      while (augmentMatching()) { }
    }

#define GRAPH_INSTANTIATION(a) template class Graph<a>;
    INSTANTIATE_MATCHING_EDGE_WEIGHT_TEMPLATES(GRAPH_INSTANTIATION)
  }
}
